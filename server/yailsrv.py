# Copyright (C) 2021 Brad Colbert

import requests
import re
import json
import time
import logging
import urllib
import os
from tqdm import tqdm
import socket
import threading
import random

bind_ip = '0.0.0.0'
bind_port = 9999

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((bind_ip, bind_port))
server.listen(5)  # max backlog of connections

print('Listening on {}:{}'.format(bind_ip, bind_port))

logging.basicConfig(level=logging.CRITICAL)
logger = logging.getLogger(__name__)

def search(keywords, max_results=100): #None):

    url = 'https://duckduckgo.com/'
    params = {
    	'q': keywords
    }

    logger.debug("Hitting DuckDuckGo for Token")

    #   First make a request to above URL, and parse out the 'vqd'
    #   This is a special token, which should be used in the subsequent request
    res = requests.post(url, data=params)
    searchObj = re.search(r'vqd=([\d-]+)\&', res.text, re.M|re.I)

    if not searchObj:
        logger.error("Token Parsing Failed !")
        return -1

    logger.debug("Obtained Token")

    headers = {
        'authority': 'duckduckgo.com',
        'accept': 'application/json, text/javascript, */*; q=0.01',
        'sec-fetch-dest': 'empty',
        'x-requested-with': 'XMLHttpRequest',
        'user-agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.163 Safari/537.36',
        'sec-fetch-site': 'same-origin',
        'sec-fetch-mode': 'cors',
        'referer': 'https://duckduckgo.com/',
        'accept-language': 'en-US,en;q=0.9',
    }

    params = (
        ('l', 'us-en'),
        ('o', 'json'),
        ('q', keywords),
        ('vqd', searchObj.group(1)),
        ('f', ',,,'),
        ('p', '1'),
        ('v7exp', 'a'),
    )

    requestUrl = url + "i.js"

    logger.debug("Hitting Url : %s", requestUrl)

    '''
    while True:
        while True:
            try:
                res = requests.get(requestUrl, headers=headers, params=params)
                data = json.loads(res.text)
                break
            except ValueError as e:
                logger.debug("Hitting Url Failure - Sleep and Retry: %s", requestUrl)
                time.sleep(5)
                continue

        logger.debug("Hitting Url Success : %s", requestUrl)
        printJson(data["results"])

        for entry in data["results"]:
            saveImage(entry["image"], "./images/")

        if "next" not in data:
            logger.debug("No Next Page - Exiting")
            exit(0)

        requestUrl = url + data["next"]
    '''
    results = []
    while len(results) < max_results:
        while True:
            try:
                res = requests.get(requestUrl, headers=headers, params=params)
                data = json.loads(res.text)
                break
            except ValueError as e:
                logger.debug("Hitting Url Failure - Sleep and Retry: %s %s", requestUrl, str(e))
                time.sleep(5)
                continue

        logger.debug("Hitting Url Success : %s", requestUrl)
        printJson(data["results"])

        for entry in data["results"]:
            results.append(entry["image"])

        if "next" not in data:
            logger.debug("No Next Page - Exiting")
            return results

        requestUrl = url + data["next"]

    return results


def printJson(objs):
    for obj in objs:
        print("Width {0}, Height {1}".format(obj["width"], obj["height"]))
        print("Thumbnail {0}".format(obj["thumbnail"]))
        print("Url {0}".format(obj["url"]))
        print("Title {0}".format(obj["title"].encode('utf-8')))
        print("Image {0}".format(obj["image"]))
        print("__________")

def hash_string(s):
    import hashlib
    import binascii
    #hsh = bytearray(hashlib.md5(s.encode(encoding="ascii")).digest())
    hsh = bytearray(hashlib.shake_256(s.encode(encoding="ascii")).digest(2))
    assert len(hsh) == 2
    output = \
        int.from_bytes(hsh[0:4], "big") # ^ \
        #int.from_bytes(hsh[4:8], "big") ^ \
        #int.from_bytes(hsh[8:12], "big") ^ \
        #int.from_bytes(hsh[12:16], "big")

    return binascii.hexlify(output.to_bytes(2, byteorder='big')).decode("ascii")

def convertToYai(image):
    from PIL import Image
    import numpy as np
    import struct

    d_size = (80,220)

    print(d_size, image.size)

    s_ratio = image.size[0] / image.size[1]   
    d_ratio = 1.4545  # 320/220  #  d_size[0] / d_size[1]

    print(s_ratio, d_ratio)

    background = Image.new("LA", d_size)

    if s_ratio >= d_ratio:
        image_scaled = image.resize((d_size[0], int(d_size[1] / s_ratio)), Image.LANCZOS)
        print(">", (d_size[0], int(d_size[1] / s_ratio)))
    else:
        image_scaled = image.resize((int(d_size[0] * s_ratio), d_size[1]), Image.LANCZOS)
        print("<", (int(d_size[0] * s_ratio), d_size[1]))

    background.paste(image_scaled, ((d_size[0]-image_scaled.size[0])//2,
                                    (d_size[1]-image_scaled.size[1])//2))

    gray = background.convert(mode='LA') # image.convert(mode='LA')
    #scaled = gray.resize(d_size, Image.LANCZOS)
    gray_dither = gray.convert(dither=Image.FLOYDSTEINBERG, colors=16) # scaled.convert(dither=Image.FLOYDSTEINBERG, colors=16)

    #gray_dither.show()
    #input("Press Enter to continue...")

    im_matrix = np.array(gray_dither)

    im_values = im_matrix[:,:,0]
    # print(im_values.shape)
    # print(im_values)
    # print("\n")

    evens = im_values[:,::2]
    # print(evens)
    # print("\n")

    odds = im_values[:,1::2]
    # print(odds)
    # print("\n")

    evens_scaled = (evens >> 4) << 4 # np.zeros((220,40))
    # print(evens_scaled)
    # print("\n")

    odds_scaled =  (odds >> 4)
    # print(odds_scaled)
    # print("\n")

    combined = evens_scaled + odds_scaled
    # print(combined)
    # print("\n")

    combined_int = combined.astype('int8')
    # print(combined_int.shape)
    # print(combined_int.shape[0])
    # print(combined_int)
    # print("\n")

    ttlbytes = combined_int.shape[0] * combined_int.shape[1]

    image_yai = bytearray()
    image_yai += bytes([1, 1, 0])  # version
    image_yai += bytes([4])        # Gfx 9
    image_yai += bytes([3])        # MemToken
    image_yai += struct.pack("<H",combined_int.shape[0]*combined_int.shape[1]) #, height x width
    image_yai += bytearray(combined_int)  # image

    print('Size: %d x %d = %d (%d)' % (combined_int.shape[0], combined_int.shape[1], ttlbytes, len(image_yai)))

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


def streamYai(url, client):
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

        image_yai = convertToYai(image)

        # Print image as hex
        #res = ' '.join(format(x, '02x') for x in image_yai) 
        #print(str(res))
        client.sendall(image_yai)
        return True

    except Exception as e:
        print('Exception:', e)
        return False

def handle_client_connection(client_socket):
    try:
        done = False
        url_idx = 0
        while not done:
            request = client_socket.recv(1024)
            r_string = request.decode('UTF-8')
            tokens = r_string.rstrip(' \r\n').split(' ')
            print(tokens)

            if tokens[0] == 'quit':
                done = True

            elif tokens[0] == 'search':
                urls = search(' '.join(tokens[1:]))
                url_idx = random.randint(0, len(urls)-1)
                url = urls[url_idx]
                while not streamYai(url, client_socket):
                    print('Problem with image trying another...')
                    url_idx = random.randint(0, len(urls)-1)
                    url = urls[url_idx]
                    time.sleep(1)

            elif tokens[0] == 'next':
                url_idx = random.randint(0, len(urls)-1)
                url = urls[url_idx]
                while not streamYai(url, client_socket):
                    print('Problem with image trying another...')
                    url_idx = random.randint(0, len(urls)-1)
                    url = urls[url_idx]
                    time.sleep(1)

            else:
                print('Received {}'.format(r_string.rstrip(' \r\n')))
                client_socket.send(bytes(b'ACK!'))

    except Exception as ex:
        logger.critical('Problem handling client ' + str(ex))

    client_socket.close()

def main():
    # search("piper comanche p-24")
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