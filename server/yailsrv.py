from PIL import Image
import numpy as np
import struct

def splitNumber (num):
    lst = []
    while num > 0:
        lst.append(num & 0xFF)
        num >>= 8
    return lst[::-1]

img = Image.open("server/comanche.jpg")
gray = img.convert(mode='LA')
scaled = gray.resize((80,220), Image.LANCZOS)
gray_dither = scaled.convert(dither=Image.FLOYDSTEINBERG, colors=16)

im_matrix = np.array(gray_dither)

im_values = im_matrix[:,:,0]
print(im_values.shape)
print(im_values)
print("\n")

evens = im_values[:,::2]
print(evens)
print("\n")

odds = im_values[:,1::2]
print(odds)
print("\n")

evens_scaled = (evens >> 4) << 4 # np.zeros((220,40))
print(evens_scaled)
print("\n")

odds_scaled =  (odds >> 4)
print(odds_scaled)
print("\n")

combined = evens_scaled + odds_scaled
print(combined)
print("\n")

combined_int = combined.astype('int8')
print(combined_int.shape)
print(combined_int.shape[0])
print(combined_int)
print("\n")

with open("test.yai","wb") as f:
    f.write(bytes([1, 1, 0]))
    f.write(bytes([4]))                 # Gfx 9
    f.write(bytes([3])) # MemToken
    f.write(struct.pack("<H",im_values.shape[0]*im_values.shape[1])) #, height x width
    f.write(bytearray(combined_int))

gray_dither.show()
