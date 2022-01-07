CC65_HOME=C:\\Users\\bradc\\AppData\\Roaming\\cc65\\bin
CC65=$(CC65_HOME)\\cc65.exe
CA65=$(CC65_HOME)\\ca65.exe
CL65=$(CC65_HOME)\\cl65.exe

TARGET=yail.xex

SRC_DIR=src

.SUFFIXES:
.SUFFIXES: .c .s .o

all: c_files s_files link_files

c_files: $(SRC_DIR)\*.c
    @echo Converting $(**) to .s...
    @echo xxx $(**:.c=.s)
    @$(MAKE) /f Makefile.mak $(**:.c=.s)

s_files: $(SRC_DIR)\*.s
    @echo Converting $(**) to .o...
    @$(MAKE) /f Makefile.mak $(**:.s=.o)

link_files: $(SRC_DIR)\*.o
    $(CL65) -t atari -o $(TARGET) -C $(SRC_DIR)\imgload.cfg $(**) atari.lib atarixl.lib

.s.o:
  $(CA65) -t atari $<

.c.s:
  $(CC65) -Oi -t atari $<

$(TARGET): all

clean: s_products c_products $(TARGET)

c_products: *.c
    @echo Cleaning $(**:.c=.s)
    rm $(**:.c=.s)

s_products: *.s
    @echo Cleaning $(**:.s=.o)
    rm $(**:.s=.o)
