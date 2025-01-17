MAKEFLAGS := --jobs=$(shell nproc)

#CXXFLAGS=-I/usr/local/frc/include -I/usr/local/include/opencv4
CXXFLAGS=-I/usr/local/include/opencv4
CXXFLAGS+=-g

OBJS=VisLog.o Bayer2RGB.o 2702camera.o 2702math.o genio.o Bayer2Grey.o cammgr.o

lib2702.a: $(OBJS)
	ar -r lib2702.a $(OBJS)
