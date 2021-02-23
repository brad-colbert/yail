# Copyright (C) 2021 Brad Colbert

import yai

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
from PIL import Image
from PIL import Image

bind_ip = '0.0.0.0'
bind_port = 9999

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

        saveYai(filepath)

    except Exception as e:
        print('Exception:', e)


def retrieveImage(url):
    from io import BytesIO

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

        return image

    except Exception as e:
        print('Exception:', e)
        return None


def streamYai(url, client):
    from io import BytesIO

    """
    Downloads a file given a URL, converts it to the YAI format, and streams on a socket
    """
    # download the body of response by chunk, not immediately
    try:
        '''
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
        '''
        image = retrieveImage(url)

        if image is None:
            return False

        image_yai = yai.convert(image)

        # Print image as hex
        #res = ' '.join(format(x, '02x') for x in image_yai) 
        #print(str(res))
        client.sendall(image_yai)

        return True

    except Exception as e:
        print('Exception:', e)
        return False

def saveYai(filepath):
    import numpy as np
    import struct

    img = Image.open(filepath)

    img_yai = yai.convert(img)

    # We need a 8.3 filename
    path_pos = filepath.rfind('/')
    pathname = filepath[:path_pos]
    filename = filepath[path_pos+1:-4]
    filename_yai = filename[:4] + hash_string(filepath) + '.yai'
    print(filename_yai)

    with open(os.path.join(pathname, filename_yai),"wb") as f:
        f.write(img_yai)

def streamOne(urls, client_socket):
    success = False
    while not success:
        url_idx = random.randint(0, len(urls)-1)
        url = urls[url_idx]
        del(urls[url_idx])

        if streamYai(url, client_socket):
            success = True
        else:
            print('Problem with image trying another...')
            time.sleep(1)

    return urls

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
                urls = streamOne(urls, client_socket)

            elif tokens[0] == 'next':
                urls = streamOne(urls, client_socket)

            else:
                print('Received {}'.format(r_string.rstrip(' \r\n')))
                client_socket.send(bytes(b'ACK!'))

    except Exception as ex:
        logger.critical('Problem handling client ' + str(ex))

    client_socket.close()

def load_and_render(tokens):
    done = False
    while not done:    
        urls = search(' '.join(tokens))

        while len(urls):
            url_idx = random.randint(0, len(urls)-1)
            url = urls[url_idx]
            del urls[url_idx]

            image = retrieveImage(url)

            if image == None:
                done = True
            else:
                ratio_d = 320.0/220.0
                ratio_i = image.size[0]/image.size[1]
                print(ratio_d, ratio_i, image.size)

                if ratio_i > ratio_d:
                    width = int(image.size[0])
                    height = int(image.size[1] * (ratio_i/ratio_d))
                    print('>', width, height, width/height)
                else:
                    width = int(image.size[0] * (ratio_d/ratio_i))
                    height = int(image.size[1])
                    print('<', width, height, width/height)


                background = Image.new(image.mode, (int(width),int(height)))

                pos_x = (width-image.size[0])//2
                pos_y = (height-image.size[1])//2

                background.paste(image, (pos_x,pos_y))

                background.show()
                input("Press key for next")

def main():
    import argparse

    parser = argparse.ArgumentParser(description="creates and serves images based on a search")
    group = parser.add_mutually_exclusive_group()
    #group.add_argument("-v", "--verbose", action="store_true")
    #group.add_argument("-q", "--quiet", action="store_true")
    group.add_argument("-l", "--local", action="store_true")
    parser.add_argument('-s', '--search', type=str, help="comma sepearated search tokens")
    args = parser.parse_args()

    local = False
    if args.local:
        local = True
    
    if args.search:
        if local:
            load_and_render(args.search.split(','))

        else:
            # search("piper comanche p-24")
            server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server.bind((bind_ip, bind_port))
            server.listen(5)  # max backlog of connections

            print('Listening on {}:{}'.format(bind_ip, bind_port))
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