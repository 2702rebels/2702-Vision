MAKEFLAGS := --jobs=$(shell nproc)

#CXXFLAGS=-I/usr/local/frc/include -I/usr/local/include/opencv4
#CXXFLAGS=-I/usr/local/include/opencv4
CXXFLAGS+=$(shell pkg-config --cflags opencv4)
CXXFLAGS+=$(shell pkg-config --cflags libcamera)
CXXFLAGS+=$(shell pkg-config --cflags ../pkgconfig/ubuntu/wpilib.pc)
CXXFLAGS+=-g
LIBS+=$(shell pkg-config -libs ../pkgconfig/ubuntu/wpilib.pc)

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

lib2702.a: $(OBJS)
	ar -r lib2702.a $(OBJS)

cammgr.o: cammgr.cpp cammgr.h v4lcam.h
nvcam.o: nvcam.cpp nvcam.h v4lcam.h
