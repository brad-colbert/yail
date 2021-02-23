from PIL import Image
import numpy as np
import struct

def convert(image):
    '''
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
    '''
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

    background.resize((80,220))

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