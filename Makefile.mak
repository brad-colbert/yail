CC65_HOME=C:\\Users\\bradc\\AppData\\Roaming\\cc65\\bin
CC65=$(CC65_HOME)\\cc65.exe
CA65=$(CC65_HOME)\\ca65.exe
CL65=$(CC65_HOME)\\cl65.exe

TARGET=YAIL
SRC_DIR=src

.SUFFIXES:
.SUFFIXES: .c .s .o

all: $(TARGET).XEX

c_files: $(SRC_DIR)\*.c
    @echo Building $(**) to .s...
    @$(MAKE) -nologo /f Makefile.mak $(**:.c=.s)

s_files: $(SRC_DIR)\*.s
    @echo Building $(**) to .o...
    @$(MAKE) -nologo /f Makefile.mak $(**:.s=.o)

link_files: $(SRC_DIR)\*.o
    $(CL65) -t atari -g -o $(TARGET).XEX --config $(SRC_DIR)\$(TARGET).cfg --mapfile $(TARGET).map -Ln $(TARGET).lbl $(**) atari.lib atarixl.lib

.s.o:
  $(CA65) -t atari -g $<

.c.s:
  $(CC65) -t atari -g $<

$(TARGET).XEX: c_files s_files link_files

clean: s_products c_products
  del $(TARGET).XEX $(TARGET).map

c_products: $(SRC_DIR)\*.c
    @echo Cleaning $(**:.c=.s)
    del $(**:.c=.s)

s_products: $(SRC_DIR)\*.s
    @echo Cleaning $(**:.s=.o)
    del $(**:.s=.o)

atr: $(TARGET).XEX
    copy $(TARGET).XEX atr
    dir2atr -d -D -b DosXL230 $(TARGET).ATR atr

