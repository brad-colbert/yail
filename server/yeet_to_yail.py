import requests
import re
import time
import logging
import os
from tqdm import tqdm
import socket
from threading import Thread, Lock
import random
from duckduckgo_search import DDGS
from fastcore.all import *
from pprint import pformat
from PIL import Image
import numpy as np
import sys

# Set up logging first thing
logging.basicConfig(level=logging.ERROR)
logger = logging.getLogger(__name__)

GRAPHICS_8 = 2
GRAPHICS_9 = 4
GRAPHICS_RANDOM = 42
YAIL_W = 320
YAIL_H = 220

# The yail_data will contain the image that is to be sent.  It
# is protected with a Mutex so that when the image is being sent
# it won't be written by the server.
mutex = Lock()
yail_data = None

gfx_mode = GRAPHICS_8
connections = 0
camera_thread = None
camera_done = False

def fix_aspect(image, crop=False):
    aspect = YAIL_W/YAIL_H   # YAIL aspect ratio
    aspect_i = 1/aspect
    w = image.size[0]
    h = image.size[1]
    img_aspect = w/h

    if crop:
        if img_aspect > aspect:  # wider than YAIL aspect
            new_width = int(h * aspect)
            new_width_diff = w - new_width
            new_width_diff_half = int(new_width_diff/2)
            image = image.crop((new_width_diff_half, 0, w-new_width_diff_half, h))
        else:                    # taller than YAIL aspect
            new_height = int(w * aspect_i)
            new_height_diff = h - new_height
            new_height_diff_half = int(new_height_diff/2)
            image = image.crop((0, new_height_diff_half, w, h-new_height_diff_half))
    else:
        if img_aspect > aspect:  # wider than YAIL aspect
            new_height = int(w * aspect_i)
            background = Image.new("L", (w,new_height))
            background.paste(image, (0, int((new_height-h)/2)))
            image = background
        else:                    # taller than YAIL aspect
            new_width = int(h * aspect)
            background = Image.new("L", (new_width, h))
            background.paste(image, (int((new_width-w)/2), 0))
            image = background

    return image

def dither_image(image):
    return image.convert('1')

def pack_bits(image):
    bits = np.array(image)
    return np.packbits(bits, axis=1)

def pack_shades(image):
    yail = image.resize((int(YAIL_W/4),YAIL_H), Image.LANCZOS)
    yail = yail.convert(dither=Image.FLOYDSTEINBERG, colors=16)

    im_matrix = np.array(yail)
    im_values = im_matrix[:,:]

    evens = im_values[:,::2]
    odds = im_values[:,1::2]

    # Each byte holds 2 pixels.  The upper four bits for the left pixel and the lower four bits for the right pixel.
    evens_scaled = (evens >> 4) << 4 # left pixel
    odds_scaled =  (odds >> 4)       # right pixel

    # Combine the two 4bit values into a single byte
    combined = evens_scaled + odds_scaled
    
    return combined.astype('int8')

def show_dithered(image):
    image.show()

def show_shades(image_data):
    pil_image_yai = Image.fromarray(image_data, mode='L')
    pil_image_yai.resize((320,220), resample=None).show()

def convertToYai(image_data):
    import struct

    global gfx_mode

    ttlbytes = image_data.shape[0] * image_data.shape[1]

    image_yai = bytearray()
    image_yai += bytes([1, 1, 0])            # version
    image_yai += bytes([gfx_mode])           # Gfx mode (8,9)
    image_yai += bytes([3])                  # Memory block type
    image_yai += struct.pack("<H", ttlbytes) # num bytes height x width
    image_yai += bytearray(image_data)       # image

    return image_yai

def update_yail_data(data, thread_safe=True):
    global yail_data
    if thread_safe:
        mutex.acquire()
    try:
        yail_data = convertToYai(data)
    finally:
        if thread_safe:
            mutex.release()

def send_yail_data(client_socket, thread_safe=True):
    global yail_data

    if thread_safe:
        mutex.acquire()
    try:
        data = yail_data   # a local copy
    finally:
        if thread_safe:
            mutex.release()

    if data is not None:
        client_socket.sendall(data)
        logger.info('Sent YAIL data')

def stream_YAI(url, client, gfx_mode):
    from io import BytesIO

    # download the body of response by chunk, not immediately
    try:
        logger.info('Loading', url, url.encode())

        file_size = 0

        response = requests.get(url, stream=True, timeout=30)

        # get the file name
        filepath = ''
        exts = ['.jpg', '.jpeg', '.gif', '.png']
        ext = re.findall('|'.join(exts), url)
        if len(ext):
            pos_ext = url.find(ext[0])
            if pos_ext >= 0:
                pos_name = url.rfind("/", 0, pos_ext)
                filepath =  url[pos_name+1:pos_ext+4]

        # progress bar, changing the unit to bytes instead of iteration (default by tqdm)
        image_data = b''
        progress = tqdm(response.iter_content(1024), f"Downloading {filepath}", total=file_size, unit="B", unit_scale=True, unit_divisor=1024)
        for data in progress:
            # collect all the data
            image_data += data

            # update the progress bar manually
            progress.update(len(data))

        image_bytes_io = BytesIO()
        image_bytes_io.write(image_data)
        image = Image.open(image_bytes_io)

        gray = image.convert(mode='L')
        gray = fix_aspect(gray)
        gray = gray.resize((YAIL_W,YAIL_H), Image.LANCZOS)

        if gfx_mode == GRAPHICS_8:
            gray_dithered = dither_image(gray)
            image_data = pack_bits(gray_dithered)
        elif gfx_mode == GRAPHICS_9:
            image_data = pack_shades(gray)

        image_yai = convertToYai(image_data)

        client.sendall(image_yai)

        return True

    except Exception as e:
        logger.error(f'Exception: {e} **{file_size}')
        return False

# This uses the DuckDuckGo search engine to find images.  This is handled by the duckduckgo_search package.
def search_images(term, max_images=1000):
    logger.info(f"Searching for '{term}'")
    with DDGS() as ddgs:
        results = L([r for r in ddgs.images(term, max_results=max_images)])

        urls = []
        for result in results:
            urls.append(result['image'])

        return urls

def camera_handler():
    import pygame.camera
    import pygame.image

    global camera_done

    pygame.camera.init()

    cameras = pygame.camera.list_cameras()

    logger.info("Using camera %s ..." % cameras[0])

    webcam = pygame.camera.Camera(cameras[0])

    webcam.start()

    # grab first frame
    img = webcam.get_image()

    WIDTH = img.get_width()
    HEIGHT = img.get_height()

    #screen = pygame.display.set_mode( ( WIDTH, HEIGHT ) )
    #pygame.display.set_caption("pyGame Camera View")

    while not camera_done:
        #for e in pygame.event.get() :
        #    if e.type == pygame.QUIT :
        #        sys.exit()

        imgdata = pygame.surfarray.array3d(img)
        imgdata = imgdata.swapaxes(0,1)
        pil_image = Image.fromarray(np.array(imgdata))
        gray = pil_image.convert(mode='L')
        gray = fix_aspect(gray, crop=True)
        gray = gray.resize((YAIL_W,YAIL_H), Image.LANCZOS)

        if gfx_mode == GRAPHICS_8:
            gray = dither_image(gray)
            update_yail_data(pack_bits(gray))
        elif gfx_mode == GRAPHICS_9:
            update_yail_data(pack_shades(gray))

        # draw frame
        #screen.blit(img, (0,0))
        #pygame.display.flip()

        # grab next frame    
        img = webcam.get_image()

    camera_done = False

def handle_client_connection(client_socket):
    # Set up a new event loop for this thread
    global connections
    global gfx_mode
    global camera_thread
    global camera_done
    client_mode = None

    connections = connections + 1
    logger.info('Starting Connection:', connections)

    try:
        done = False
        url_idx = 0
        tokens = []
        while not done:
            if len(tokens) == 0:
                request = client_socket.recv(1024)
                logger.info('Client request ' + pformat(request))
                r_string = request.decode('UTF-8')
                tokens = r_string.rstrip(' \r\n').split(' ')
            logger.info('Tokens ' + pformat(tokens))

            if tokens[0] == 'video':
                client_mode = 'video'
                if camera_thread is None:
                    camera_done = False
                    camera_thread = Thread(target=camera_handler)
                    camera_thread.daemon = True
                    camera_thread.start()
                send_yail_data(client_socket)
                tokens.pop(0)

            elif tokens[0] == 'search':
                client_mode = 'search'
                urls = search_images(' '.join(tokens[1:]))
                url_idx = random.randint(0, len(urls)-1)
                url = urls[url_idx]
                while not stream_YAI(url, client_socket, gfx_mode):
                    logger.warning(f'Problem with %s trying another...', url)
                    url_idx = random.randint(0, len(urls)-1)
                    url = urls[url_idx]
                    time.sleep(1)
                tokens = []

            elif tokens[0] == 'next':
                if client_mode == 'search':
                    url_idx = random.randint(0, len(urls)-1)
                    url = urls[url_idx]
                    # Loop if we have a problem with the image, selecting the next.
                    while not stream_YAI(url, client_socket, gfx_mode):
                        logger.warning('Problem with image trying another...')
                        url_idx = random.randint(0, len(urls)-1)
                        url = urls[url_idx]
                        time.sleep(1)
                    tokens.pop(0)
                elif client_mode == 'video':
                    send_yail_data(client_socket)
                    tokens.pop(0)

            elif tokens[0] == 'gfx':
                tokens.pop(0)
                gfx_mode = int(tokens[0])
                tokens.pop(0)

            elif tokens[0] == 'quit':
                done = True
                tokens.pop(0)

            else:
                logger.info('Received {}'.format(r_string.rstrip(' \r\n')))
                client_socket.send(bytes(b'ACK!'))

    except Exception as ex:
        logger.critical('Problem handling client ' + str(ex))

    finally:
        client_socket.close()
        logger.info(f'Closing Connection: %d' % connections)
        connections = connections - 1
        if connections == 0:   # Maybe should look into killing this thread when there are no video connections.
            camera_done = True
            time.sleep(1)
            camera_thread = None

def main():
    # Initialize the image to send with something
    initial_image = Image.new("L", (YAIL_W,YAIL_H))
    update_yail_data(pack_shades(initial_image))

    bind_ip = '0.0.0.0'
    bind_port = 5556

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((bind_ip, bind_port))
    server.listen(10)  # max backlog of connections

    logger.info('Listening on {}:{}'.format(bind_ip, bind_port))

    while True:
        client_sock, address = server.accept()
        logger.info('Accepted connection from {}:{}'.format(address[0], address[1]))
        client_handler = Thread(
            target=handle_client_connection,
            args=(client_sock,)  # without comma you'd get a... TypeError: handle_client_connection() argument after * must be a sequence, not _socketobject
        )
        client_handler.daemon = True
        client_handler.start()

if __name__ == "__main__":
    main()
