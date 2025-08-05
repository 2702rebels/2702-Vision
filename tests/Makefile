MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

OBJS=test.o
CXXFLAGS+=--std=c++20
CXXFLAGS+=-I../lib
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
CXXFLAGS+=-g
CXXFLAGS+=-Ofast
CXXFLAGS+=-I$(shell pwd)/../2025/Object
CXXFLAGS+=-I$(shell pwd)/../lib
CXXFLAGS+=$(shell pkg-config --cflags opencv4 )
CXXFLAGS+=$(shell pkg-config --cflags libcamera )
CXXFLAGS+=$(shell pkg-config --cflags ../../pkgconfig/ubuntu/wpilib.pc)
LDFLAGS+=-L../../lib
LIBS+=$(shell pkg-config -libs libcamera)
LIBS+=$(shell pkg-config -libs ../../pkgconfig/ubuntu/wpilib.pc)
LIBS+=$(shell pkg-config -libs opencv4)
all: test

clean:
	rm -f *.o
	rm -f test

test: $(OBJS) ../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o test $(OBJS) -l2702 $(LIBS)

test.o: test.cpp SerializationTest.h
