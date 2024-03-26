import pygame.camera
import pygame.image
from PIL import Image
import numpy as np
import sys
from threading import Thread, Lock
import socket
#import threading
#import time
from pprint import pprint
import logging

# Set up logging first thing
logging.basicConfig(level=logging.ERROR)
logger = logging.getLogger(__name__)

# The yail_data will contain the image that is to be sent.  It
# is protected with a Mutex so that when the image is being sent
# it won't be written by the server.
mutex = Lock()
yail_data = None

GRAPHICS_8 = 2
GRAPHICS_9 = 4
GRAPHICS_RANDOM = 42
YAIL_W = 320
YAIL_H = 220

gfx_mode = GRAPHICS_8
connections = 0

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
            #print(f'Wider than 4:3  :  {w}->{new_width}')
        else:                    # taller than YAIL aspect
            new_height = int(w * aspect_i)
            new_height_diff = h - new_height
            new_height_diff_half = int(new_height_diff/2)
            image = image.crop((0, new_height_diff_half, w, h-new_height_diff_half))
            #print(f'Taller than 4:3  :  {h}->{new_height}')
    else:
        if img_aspect > aspect:  # wider than YAIL aspect
            new_height = int(w * aspect_i)
            background = Image.new("L", (w,new_height))
            background.paste(image, (0, int((new_height-h)/2)))
            image = background
            #print(f'Wider than 4:3  :  {h}->{new_height}')
        else:                    # taller than YAIL aspect
            new_width = int(h * aspect)
            background = Image.new("L", (new_width, h))
            background.paste(image, (int((new_width-w)/2), 0))
            image = background
            #print(f'Taller than 4:3  :  {w}->{new_width}')

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
        print('Sent YAIL data')

def handle_client_connection(client_socket):
    # Set up a new event loop for this thread
    global connections
    global gfx_mode

    connections = connections + 1
    print('Starting Connection:', connections)

    try:
        done = False
        url_idx = 0
        while not done:
            request = client_socket.recv(1024)
            pprint(request)
            r_string = request.decode('UTF-8')
            tokens = r_string.rstrip(' \r\n').split(' ')
            print(tokens)

            if tokens[0] == 'next' or tokens[0] == 'search':
                send_yail_data(client_socket)

            elif tokens[0] == 'gfx':
                gfx_mode = int(tokens[1])

            elif tokens[0] == 'quit':
                done = True

            else:
                print('Received {}'.format(r_string.rstrip(' \r\n')))
                client_socket.send(bytes(b'ACK!'))

    except Exception as ex:
        logger.critical('Problem handling client ' + str(ex))

    finally:
        client_socket.close()
        print('Closing Connection:', connections)
        connections = connections - 1

def camera_handler():
    pygame.camera.init()

    cameras = pygame.camera.list_cameras()

    print( "Using camera %s ..." % cameras[0])

    webcam = pygame.camera.Camera(cameras[0])

    webcam.start()

    # grab first frame
    img = webcam.get_image()

    WIDTH = img.get_width()
    HEIGHT = img.get_height()

    screen = pygame.display.set_mode( ( WIDTH, HEIGHT ) )
    pygame.display.set_caption("pyGame Camera View")

    while True :
        for e in pygame.event.get() :
            if e.type == pygame.QUIT :
                sys.exit()

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
        screen.blit(img, (0,0))
        pygame.display.flip()

        # grab next frame    
        img = webcam.get_image()

def main():
    camera_thread = Thread(target=camera_handler)
    camera_thread.daemon = True
    camera_thread.start()

    bind_ip = '0.0.0.0'
    bind_port = 5556
    connections = 0

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((bind_ip, bind_port))
    server.listen(5)  # max backlog of connections

    print('Listening on {}:{}'.format(bind_ip, bind_port))

    while True:
        client_sock, address = server.accept()
        print('Accepted connection from {}:{}'.format(address[0], address[1]))
        client_handler = Thread(
            target=handle_client_connection,
            args=(client_sock,)  # without comma you'd get a... TypeError: handle_client_connection() argument after * must be a sequence, not _socketobject
        )
        client_handler.daemon = True
        client_handler.start()

if __name__ == "__main__":
    main()
