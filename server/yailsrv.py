#!/usr/bin/env python

# Copyright (C) 2021 Brad Colbert

import requests
import re
#import json
import time
import logging
#import urllib
import os
from tqdm import tqdm
import socket
import threading
import random
from duckduckgo_search import DDGS
from fastcore.all import *
from pprint import pprint
from PIL import Image
import numpy as np
#import asyncio

GRAPHICS_8 = 2
GRAPHICS_9 = 4

bind_ip = '0.0.0.0'
bind_port = 5556

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((bind_ip, bind_port))
server.listen(5)  # max backlog of connections

print('Listening on {}:{}'.format(bind_ip, bind_port))

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

def printJson(objs):
    for obj in objs:
        print("Width {0}, Height {1}".format(obj["width"], obj["height"]))
        print("Thumbnail {0}".format(obj["thumbnail"]))
        print("Url {0}".format(obj["url"]))
        print("Title {0}".format(obj["title"].encode('utf-8')))
        print("Image {0}".format(obj["image"]))
        print("__________")

# This uses the DuckDuckGo search engine to find images.  This is handled by the duckduckgo_search package.
def search_images(term, max_images=1000):
    print(f"Searching for '{term}'")
    with DDGS() as ddgs:
        results = L([r for r in ddgs.images(term, max_results=max_images)])

        for result in results:
            pprint(result)

        urls = []
        for result in results:
            urls.append(result['image'])

        pprint(urls)

        return urls

# This funcion is used to create a 8.3 filename for the YAI file.  It uses the hash of the file to create a unique name.
def hash_string(s):
    import hashlib
    import binascii
    hsh = bytearray(hashlib.shake_256(s.encode(encoding="ascii")).digest(2))
    assert len(hsh) == 2
    output = \
        int.from_bytes(hsh[0:4], "big") # ^ \
        #int.from_bytes(hsh[4:8], "big") ^ \
        #int.from_bytes(hsh[8:12], "big") ^ \
        #int.from_bytes(hsh[12:16], "big")

    return binascii.hexlify(output.to_bytes(2, byteorder='big')).decode("ascii")

def convert_BW_image_to_bits(image):
    bits = np.array(image)
    packed_bits = np.packbits(bits, axis=1)

    return packed_bits

def convertToYai(image, gfx_mode):
    import struct
    show_image = False

    gray = image.convert(mode='L')
    aspect = gray.size[0]/gray.size[1]
    print("Original size:", gray.size, f'4/{4*(1/aspect)}')

    if aspect > 4/3:  # wider than 4:3
        new_height = int(gray.size[0] * (3/4))
        background = Image.new("L", (gray.size[0],new_height))
        background.paste(gray, (0, int((new_height-gray.size[1])/2)))
        gray = background
        print(f'Wider than 4:3  :  {gray.size[1]}->{new_height}')
    else:             # taller than 4:3
        new_width = int(gray.size[1] * (4/3))
        background = Image.new("L", (new_width, gray.size[1]))
        background.paste(gray, (int((new_width-gray.size[0])/2), 0))
        gray = background
        print(f'Taller than 4:3  :  {gray.size[0]}->{new_width}')

    if gfx_mode == GRAPHICS_8:
        bw_dither = gray.resize((320,220)).convert('1')
        data = convert_BW_image_to_bits(bw_dither)

    else:
        gray = gray.resize((80,220), Image.LANCZOS)

        gray_dither = gray.convert(dither=Image.FLOYDSTEINBERG, colors=16)

        im_matrix = np.array(gray_dither)

        im_values = im_matrix[:,:]

        evens = im_values[:,::2]

        odds = im_values[:,1::2]

        # Each byte holds 2 pixels.  The upper four bits for the left pixel and the lower four bits for the right pixel.
        evens_scaled = (evens >> 4) << 4 # left pixel
        odds_scaled =  (odds >> 4)       # right pixel

        # Combine the two 4bit values into a single byte
        combined = evens_scaled + odds_scaled
        data = combined.astype('int8')

    ttlbytes = data.shape[0] * data.shape[1]

    image_yai = bytearray()
    image_yai += bytes([1, 1, 0])            # version
    image_yai += bytes([gfx_mode])         # Gfx mode (8,9)
    image_yai += bytes([3])                  # Memory block type
    image_yai += struct.pack("<H", ttlbytes) # num bytes height x width
    image_yai += bytearray(data)     # image

    if show_image:
        try:
            image.show()
            if gfx_mode == GRAPHICS_8:
                bw_dither.show()
            else:
                pil_image_yai = Image.fromarray(data, mode='L')
                pil_image_yai.resize((320,220), resample=None).show()

        except Exception as e:
            print('Exception:', e)

    print('Size: %d x %d = %d (%d)' % (data.shape[0], data.shape[1], ttlbytes, len(image_yai)))
    
    # Print first 10 bytes of combined_int as hex
    res = ' '.join(format(x, '02x') for x in bytearray(data)[0:10])
    print(str(res))

    # Print last 10 bytes of combined_int as hex
    res = ' '.join(format(x, '02x') for x in bytearray(data)[-10:])
    print(str(res))

    return image_yai

def saveToYai(filepath):
    from PIL import Image
    import numpy as np
    import struct

    img = Image.open(filepath)

    img_yai = convertToYai(img)

    # We need a 8.3 filename
    path_pos = filepath.rfind('/')
    pathname = filepath[:path_pos]
    filename = filepath[path_pos+1:-4]
    filename_yai = filename[:4] + hash_string(filepath) + '.yai'
    print(filename_yai)

    with open(os.path.join(pathname, filename_yai),"wb") as f:
        f.write(img_yai)

def saveImage(url, pathname):
    """
    Downloads a file given an URL and puts it in the folder `pathname`
    """
    # if path doesn't exist, make that path dir
    if not os.path.isdir(pathname):
        os.makedirs(pathname)

    # download the body of response by chunk, not immediately
    try:
        response = requests.get(url, stream=True)
        
        # get the total file size
        file_size = int(response.headers.get("Content-Length", 0))

        # get the file name
        exts = ['.jpg', '.jpeg', '.gif', '.png']
        ext = re.findall('|'.join(exts), url)
        filename = ''
        if len(ext):
            pos_ext = url.find(ext[0])
            if pos_ext >= 0:
                pos_name = url.rfind("/", 0, pos_ext)
                filename =  url[pos_name+1:pos_ext+4]
        else:
            filename = str(time.time()) + '.jpg'

        if len(filename) < 8:
            filename = str(time.time()) + '_' + filename

        filepath = os.path.join(pathname, filename)

        # progress bar, changing the unit to bytes instead of iteration (default by tqdm)
        progress = tqdm(response.iter_content(1024), f"Downloading {filepath}", total=file_size, unit="B", unit_scale=True, unit_divisor=1024)
        with open(filepath, "wb") as f:
            for data in progress:
                # write data read to the file
                f.write(data)
                # update the progress bar manually
                progress.update(len(data))

        saveToYai(filepath)

    except Exception as e:
        print('Exception:', e)

def stream_YAI(url, client, gfx_mode):
    from io import BytesIO
    from PIL import Image

    """
    Downloads a file given a URL, converts it to the YAI format, and streams on a socket
    """
    # download the body of response by chunk, not immediately
    try:
        response = requests.get(url, stream=True)
        
        # get the total file size
        file_size = int(response.headers.get("Content-Length", 0))

        # get the file name
        exts = ['.jpg', '.jpeg', '.gif', '.png']
        ext = re.findall('|'.join(exts), url)
        filename = ''
        if len(ext):
            pos_ext = url.find(ext[0])
            if pos_ext >= 0:
                pos_name = url.rfind("/", 0, pos_ext)
                filename =  url[pos_name+1:pos_ext+4]
        else:
            filename = str(time.time()) + '.jpg'

        if len(filename) < 8:
            filename = str(time.time()) + '_' + filename

        # progress bar, changing the unit to bytes instead of iteration (default by tqdm)
        filepath = ''
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
        #image.show()
        #input("Press Enter to continue...")

        image_yai = convertToYai(image, gfx_mode)

        # Print image as hex
        #res = ' '.join(format(x, '02x') for x in image_yai) 
        #print(str(res))
        client.sendall(image_yai)
        return True

    except Exception as e:
        print('Exception:', e)
        return False

# For now this function is not async.  It may be in the future.
def handle_client_connection(client_socket):
    # Set up a new event loop for this thread
    #loop = asyncio.new_event_loop()
    #asyncio.set_event_loop(loop)

    gfx_mode = GRAPHICS_9
    try:
        done = False
        url_idx = 0
        while not done:
            request = client_socket.recv(1024)
            pprint(request)
            r_string = request.decode('UTF-8')
            tokens = r_string.rstrip(' \r\n').split(' ')
            print(tokens)

            if tokens[0] == 'quit':
                done = True

            elif tokens[0] == 'search':
                for t in tokens:
                    print('*', t)
                urls = search_images(' '.join(tokens[1:]))
                print(urls)
                url_idx = random.randint(0, len(urls)-1)
                url = urls[url_idx]
                while not stream_YAI(url, client_socket, gfx_mode):
                    print('Problem with image trying another...')
                    url_idx = random.randint(0, len(urls)-1)
                    url = urls[url_idx]
                    time.sleep(1)

            elif tokens[0] == 'next':
                url_idx = random.randint(0, len(urls)-1)
                url = urls[url_idx]
                while not stream_YAI(url, client_socket, gfx_mode):
                    print('Problem with image trying another...')
                    url_idx = random.randint(0, len(urls)-1)
                    url = urls[url_idx]
                    time.sleep(1)

            elif tokens[0] == 'gfx':
                gfx_mode = int(tokens[1])

            else:
                print('Received {}'.format(r_string.rstrip(' \r\n')))
                client_socket.send(bytes(b'ACK!'))

    except Exception as ex:
        logger.critical('Problem handling client ' + str(ex))

    finally:
        client_socket.close()
        #loop.close()  # Close the loop when done

def main():
    while True:
        client_sock, address = server.accept()
        print('Accepted connection from {}:{}'.format(address[0], address[1]))
        client_handler = threading.Thread(
            target=handle_client_connection,
            args=(client_sock,)  # without comma you'd get a... TypeError: handle_client_connection() argument after * must be a sequence, not _socketobject
        )
        client_handler.start()

if __name__ == "__main__":
    main()
