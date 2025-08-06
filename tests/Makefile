MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

CXXFLAGS+=--std=c++20
CXXFLAGS+=-I../lib
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
CXXFLAGS+=-g
CXXFLAGS+=-Ofast
CXXFLAGS+=-I$(shell pwd)/../2025/Object
CXXFLAGS+=$(shell pkg-config --cflags opencv4 )
CXXFLAGS+=$(shell pkg-config --cflags libcamera )
CXXFLAGS+=$(shell pkg-config --cflags ../pkgconfig/ubuntu/wpilib.pc)
LDFLAGS+=-L../lib
LDFLAGS+=-L/usr/local/lib
LIBS+=-lgtest -pthread
LIBS+=-lapriltag -lwpilibNewCommands -lwpilibc -lcameraserver -lcscore -lntcore -lwpinet -lwpimath -lwpiutil
LIBS+=$(shell pkg-config -libs libcamera)
LIBS+=$(shell pkg-config -libs opencv4)
all: test

clean:
	rm -f *.o
	rm -f test

test: test.o ../lib/lib2702.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o test test.o -l2702 $(LIBS)

test.o: test.cpp
