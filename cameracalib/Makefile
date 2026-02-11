CXX=g++
CXXFLAGS=-Wall -Wextra -std=c++23
CXXFLAGS+=-O3
LINKFLAGS=
CXXFLAGS+=$(shell pkg-config --cflags spdlog)
LINKFLAGS+=$(shell pkg-config --libs spdlog)
LINKFLAGS+=-lcserialport
CXXFLAGS+=-I/usr/local/include
CXXFLAGS+=$(shell pkg-config --cflags opencv4)
LINKFLAGS+=$(shell pkg-config --libs opencv4)
CXXFLAGS+=$(shell pkg-config --cflags wpilib)
CXXFLAGS+=$(shell pkg-config --libs wpilib)
CXXFLAGS+=-I../2025/2025AprilTag/apriltags_cuda/build/wpilib/src/wpilib/wpiutil/src/main/native/thirdparty/llvm/include/
CXXFLAGS+=-I$(shell pwd)
LINKFLAGS+=-L$(shell pwd)

all: libloggingutils.a  libvisionlog.a cameracalib setpos

loggingutils.o: LoggingUtils.cpp
	$(CXX) $(CXXFLAGS) -c LoggingUtils.cpp -o loggingutils.o $(LINKFLAGS)

libloggingutils.a: loggingutils.o
	ar rcs libloggingutils.a loggingutils.o

visionlog.o: VisionLog.cpp VisionLog.h LogFrameType.h
	$(CXX) $(CXXFLAGS) -c VisionLog.cpp -o visionlog.o $(LINKFLAGS)

libvisionlog.a: visionlog.o
	ar rcs libvisionlog.a visionlog.o

cameracalib.o: cameracalib.cpp
	$(CXX) $(CXXFLAGS) -c cameracalib.cpp -o cameracalib.o $(LINKFLAGS) -lloggingutils -lvisionlog

setpos.o: setpos.cpp
	$(CXX) $(CXXFLAGS) -c setpos.cpp -o setpos.o $(LINKFLAGS) -lloggingutils

setpos: setpos.o
	$(CXX) $(CXXFLAGS) -o setpos setpos.o $(LINKFLAGS) -lloggingutils

cameracalib: cameracalib.o
	$(CXX) $(CXXFLAGS) -o cameracalib cameracalib.o $(LINKFLAGS) -lloggingutils -lvisionlog

clean:
	rm -rf *.o cameracalib setpos libloggingutils.a libvisionlog.a