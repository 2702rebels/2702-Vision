CXX=g++
CXXFLAGS=-Wall -Wextra -std=c++20
CXXFLAGS+=-O3
LINKFLAGS=
CXXFLAGS+=$(shell pkg-config --cflags spdlog)
LINKFLAGS+=$(shell pkg-config --libs spdlog)
LINKFLAGS+=-lcserialport
CXXFLAGS+=-I/usr/local/include
CXXFLAGS+=$(shell pkg-config --cflags opencv4)
LINKFLAGS+=$(shell pkg-config --libs opencv4)
CXXFLAGS+=$(shell pkg-config --cflags wpilib)
LINKFLAGS+=-L/usr/local/lib -lntcore -lcscore -lcameraserver -lwpilibc -lwpimath -lwpinet -lwpiutil
CXXFLAGS+=-I../2025/2025AprilTag/apriltags_cuda/build/wpilib/src/wpilib/wpiutil/src/main/native/thirdparty/llvm/include/
CXXFLAGS+=-I../2025/2025AprilTag/apriltags_cuda/build/wpilib/src/wpilib/wpiutil/src/main/native/thirdparty/json/include/
CXXFLAGS+=-I../2025/2025AprilTag/apriltags_cuda/build/wpilib/src/wpilib/wpiutil/src/main/native/thirdparty/fmtlib/include/
CXXFLAGS+=-I../2025/2025AprilTag/apriltags_cuda/build/wpilib/src/wpilib/wpiutil/src/main/native/thirdparty/expected/include/
CXXFLAGS+=-I../2025/2025AprilTag/apriltags_cuda/build/wpilib/src/wpilib/wpiutil/src/main/native/thirdparty/memory/include/
CXXFLAGS+=-I../2025/2025AprilTag/apriltags_cuda/build/wpilib/src/wpilib/wpiutil/src/main/native/thirdparty/mpack/include/
CXXFLAGS+=-I../2025/2025AprilTag/apriltags_cuda/build/wpilib/src/wpilib/wpiutil/src/main/native/thirdparty/protobuf/include/
CXXFLAGS+=-I../2025/2025AprilTag/apriltags_cuda/build/wpilib/src/wpilib/wpiutil/src/main/native/thirdparty/sigslot/include/
CXXFLAGS+=-I../2025/2025AprilTag/wpilib/include/cameraserver -I../2025/2025AprilTag/wpilib/include/cscore -I../2025/2025AprilTag/wpilib/include/hal -I../2025/2025AprilTag/wpilib/include/ntcore -I../2025/2025AprilTag/wpilib/include/romiVendordep -I../2025/2025AprilTag/wpilib/include/wpilibc -I../2025/2025AprilTag/wpilib/include/wpilibNewCommands -I../2025/2025AprilTag/wpilib/include/wpimath -I../2025/2025AprilTag/wpilib/include/wpinet -I../2025/2025AprilTag/wpilib/include/wpiutil -I../2025/2025AprilTag/wpilib/include/xrpVendordep
LINKFLAGS+=-L../2025/2025AprilTag/wpilib/lib -L../2025/2025AprilTag/wpilib/lib -L../2025/2025AprilTag/lib
LINKFLAGS+=-L/usr/local/cuda/lib64 -lcudart
CXXFLAGS+=-I$(shell pwd)
LINKFLAGS+=-L$(shell pwd)
CXXFLAGS+=-I../lib
LINKFLAGS+=-L../lib
LINKFLAGS+=-l2702 -lncurses

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
	$(CXX) $(CXXFLAGS) -c cameracalib.cpp -o cameracalib.o -lloggingutils $(LINKFLAGS) -lvisionlog

setpos.o: setpos.cpp
	$(CXX) $(CXXFLAGS) -c setpos.cpp -o setpos.o -lloggingutils $(LINKFLAGS)

setpos: setpos.o
	$(CXX) $(CXXFLAGS) -o setpos setpos.o -lloggingutils $(LINKFLAGS)

cameracalib: cameracalib.o
	$(CXX) $(CXXFLAGS) -o cameracalib cameracalib.o -lloggingutils $(LINKFLAGS) -lvisionlog

clean:
	rm -rf *.o cameracalib setpos libloggingutils.a libvisionlog.a