CXX=g++
CXXFLAGS=-Wall -Wextra -std=c++20
CXXFLAGS+=-Og
CXXFLAGS+=-g
LINKFLAGS=
CXXFLAGS+=-I../lib
LINKFLAGS+=-L../lib
LINKFLAGS+=-l2702 -lncurses
CXXFLAGS+=$(shell pkg-config --cflags spdlog)
LINKFLAGS+=$(shell pkg-config --libs spdlog)
LINKFLAGS+=-lcserialport
CXXFLAGS+=-I/usr/local/include
CXXFLAGS+=-I../2025/2025AprilTag/include/opencv4
LINKFLAGS+=-L../2025/2025AprilTag/lib/
LINKFLAGS+=-lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio -lopencv_highgui -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_ml -lopencv_photo -lopencv_video -lopencv_dnn -lopencv_flann -lopencv_gapi -lopencv_stitching
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

LINKFLAGS+=$(shell pkg-config --libs cuda-12.6)
LINKFLAGS+=$(shell pkg-config --libs cudart-12.6)
LINKFLAGS+=$(shell pkg-config --cflags cuda-12.6)
LINKFLAGS+=$(shell pkg-config --cflags cudart-12.6)
CXXFLAGS+=-I$(shell pwd)
LINKFLAGS+=-L$(shell pwd)

all: libloggingutils.a  libvisionlog.a cameracalib setpos

loggingutils.o: LoggingUtils.cpp
	$(CXX) $(CXXFLAGS) -c LoggingUtils.cpp -o loggingutils.o

libloggingutils.a: loggingutils.o
	ar rcs libloggingutils.a loggingutils.o

visionlog.o: VisionLog.cpp VisionLog.h LogFrameType.h
	$(CXX) $(CXXFLAGS) -c VisionLog.cpp -o visionlog.o

libvisionlog.a: visionlog.o
	ar rcs libvisionlog.a visionlog.o

cameracalib.o: cameracalib.cpp libloggingutils.a libvisionlog.a
	$(CXX) $(CXXFLAGS) -c cameracalib.cpp -o cameracalib.o

setpos.o: setpos.cpp libloggingutils.a
	$(CXX) $(CXXFLAGS) -c setpos.cpp -o setpos.o

setpos: setpos.o libloggingutils.a
	$(CXX) $(CXXFLAGS) -o setpos setpos.o -lloggingutils $(LINKFLAGS)

cameracalib: cameracalib.o libvisionlog.a libloggingutils.a
	$(CXX) $(CXXFLAGS) -o cameracalib cameracalib.o -lloggingutils $(LINKFLAGS) -lvisionlog

clean:
	rm -rf *.o cameracalib setpos libloggingutils.a libvisionlog.a