MAKEFLAGS := --jobs=$(shell nproc)

CXXFLAGS+=-I$(shell pwd)/../vcpkg_installed/$(shell ../findtriplet.sh)/include/opencv4
CXXFLAGS+=-g
CXXFLAGS+=-Ofast
OBJS=VisLog.o Bayer2RGB.o 2702camera.o 2702math.o Bayer2Grey.o cammgr.o fbdisp.o LogPlayer.o

TEGRA=$(shell uname -a | grep -oh tegra)
DESKTOP=$(shell uname -a | grep -oh x86_64 | uniq)
ifeq "$(TEGRA)" "tegra"
	CXXFLAGS+=-I/usr/local/cuda/include
	OBJS+=nvcam.o
else ifeq "$(DESKTOP)" "x86_64"
	
else
	OBJS+=rpicam.o
endif

#OBJS=VisLog.o Bayer2RGB.o 2702camera.o 2702math.o genio.o Bayer2Grey.o cammgr.o

all: $(OBJS) lib2702.a

clean:
	rm -f *.o
	rm -f *.a

lib2702.a: $(OBJS)
	ar -r lib2702.a $(OBJS)

cammgr.o: cammgr.cpp cammgr.h v4lcam.h
nvcam.o: nvcam.cpp nvcam.h v4lcam.h
