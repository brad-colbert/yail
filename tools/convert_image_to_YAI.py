import os
import argparse
from typing import List, Union, Callable
import numpy as np
from PIL import Image

GRAPHICS_8 = 2
GRAPHICS_9 = 4
YAIL_W = 320
YAIL_H = 220
DL_TOKEN = 0x01
DLI_TOKEN = 0x02
MEM_TOKEN = 0x03

gfx_mode = GRAPHICS_9

def process_files(input_path: Union[str, List[str]], 
                  extensions: List[str], 
                  F: Callable[[str], None]) -> None:
    extensions = [ext.lower() if ext.startswith('.') else f'.{ext.lower()}' for ext in extensions]

    def process_file(file_path: str):
        _, ext = os.path.splitext(file_path)
        if ext.lower() in extensions:
            F(file_path)

    if isinstance(input_path, list):
        for file_path in input_path:
            process_file(file_path)
    elif os.path.isdir(input_path):
        for root, _, files in os.walk(input_path):
            for file in files:
                process_file(os.path.join(root, file))
    else:
        raise ValueError("input_path must be a directory path or a list of file paths.")

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

def convert_to_Yai(image_data):
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

def F(file_path):
    print(f"Processing file: {file_path}", end='')
    try:
        from PIL import Image
        with Image.open(file_path) as im:
            gray = im.convert(mode='L')
            gray = fix_aspect(gray)
            gray = gray.resize((YAIL_W,YAIL_H), Image.LANCZOS)
            
            if gfx_mode == GRAPHICS_8:
                gray_dithered = dither_image(gray)
                image_data = pack_bits(gray_dithered)
            elif gfx_mode == GRAPHICS_9:
                image_data = pack_shades(gray)

            image_yai = convert_to_Yai(image_data)

        # Save file
        import ntpath

        # We need a 8.3 filename
        filename = ntpath.basename(file_path).upper()
        filename_yai = filename[:4] + hash_string(filename).upper() + '.YAI'
        print(f' -> {filename_yai}')

        with open(filename_yai, "wb") as f:
            f.write(image_yai)

    except Exception as e:
        print('')
        print(f"Error processing file: {file_path}")
        print(e)

def main():
    import sys
    global gfx_mode

    # Check if any arguments were provided (other than the script name)
    if len(sys.argv) > 1:

        parser = argparse.ArgumentParser(description="Process some files.")
        parser.add_argument('paths', nargs='+', help='Directory path or list of file paths')
        parser.add_argument('--extensions', nargs='+', default=['.jpg', '.jpeg', '.gif', '.png'], help='List of file extensions to process')
        parser.add_argument('--mode', nargs='+', default='9', help='8 or 9')
        
        args = parser.parse_args()

        if args.mode == '8':
            gfx_mode = GRAPHICS_8
        elif args.mode == '9':
            gfx_mode = GRAPHICS_9
        
        if len(args.paths) == 1 and os.path.isdir(args.paths[0]):
            # If a single argument is passed and it's a directory
            directory_path = args.paths[0]
            print("Processing files in directory:")
            process_files(directory_path, args.extensions, F)
        else:
            # If multiple file paths are passed
            file_list = args.paths
            print("\nProcessing specific files in list:")
            process_files(file_list, args.extensions, F)
    else:
        print("No arguments provided. Exiting...")

if __name__ == "__main__":
    main()
