MAKEFLAGS := --jobs=$(shell nproc)
#MAKEFLAGS +=--output-sync=target

#/usr/bin/clang++-17  -DGFLAGS_IS_A_DLL=0 -DGOOGLE_GLOG_DLL_DECL="" -DGOOGLE_GLOG_DLL_DECL_FOR_UNITTESTS="" -I/home/terry/2702-Vision/2025/2025AprilTag/apriltags_cuda/../include/apriltag -I/home/terry/2702-Vision/2025/2025AprilTag/apriltags_cuda/../include/apriltag/common -I/home/terry/2702-Vision/2025/2025AprilTag/apriltags_cuda/build/cccl/src/CCCL/cub -I/home/terry/2702-Vision/2025/2025AprilTag/apriltags_cuda/build/cccl/src/CCCL/libcudacxx/include -I/home/terry/2702-Vision/2025/2025AprilTag/apriltags_cuda/build/cccl/src/CCCL/thrust -I/home/terry/2702-Vision/2025/2025AprilTag/apriltags_cuda/../include/opencv4 -I/home/terry/2702-Vision/2025/2025AprilTag/apriltags_cuda/../wpilib/include/ntcore -I/home/terry/2702-Vision/2025/2025AprilTag/apriltags_cuda/../wpilib/include/wpiutil -I/home/terry/2702-Vision/2025/2025AprilTag/apriltags_cuda/../include --std c++20 -O3 -O3 -DNDEBUG --cuda-gpu-arch=sm_87 --cuda-path=/usr/local/cuda-12.6 -DGTEST_HAS_PTHREAD=1 -std=c++20 -MD -MT CMakeFiles/gpu_detector_test.dir/src/gpu_detector_test.cu.o -MF CMakeFiles/gpu_detector_test.dir/src/gpu_detector_test.cu.o.d -x cuda -c /home/terry/2702-Vision/2025/2025AprilTag/apriltags_cuda/src/gpu_detector_test.cu -o CMakeFiles/gpu_detector_test.dir/src/gpu_detector_test.cu.o

#CXX=g++
OBJS=2025AprilTag.o field.o ApriltagReport.o udpsender.o

CXXFLAGS+=-I. -I../../lib  --std c++20
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
CXXFLAGS+=-Iwpilib/include/cameraserver -Iwpilib/include/cscore -Iwpilib/include/hal -Iwpilib/include/ntcore -Iwpilib/include/romiVendordep -Iwpilib/include/wpilibc -Iwpilib/include/wpilibNewCommands -Iwpilib/include/wpimath -Iwpilib/include/wpinet -Iwpilib/include/wpiutil -Iwpilib/include/xrpVendordep
CXXFLAGS+=-Iinclude/opencv4
CXXFLAGS+=-Iinclude/apriltag
CXXFLAGS+=-Iapriltags_cuda/src
CXXFLAGS+=-I/usr/local/cuda/include
CXXFLAGS+=-Irapidjson/include
CXXFLAGS+=-DDISPLAY=0
CXXFLAGS+=-DCAMSERVER=0
#CXXFLAGS+=-O2
CXXFLAGS+=-g

LDFLAGS=-g -L../../lib
LDFLAGS+=-Lwpilib/lib -Lwpilib/lib -Llib
LDFLAGS+=$(shell pkg-config --libs-only-L cuda-12.6)
LIBS+=$(shell pkg-config --libs-only-l wpilib)
LIBS+=$(shell pkg-config --libs-only-l opencv4)
LIBS+=$(shell pkg-config --libs-only-l cuda-12.6)
LIBS+=$(shell pkg-config --libs-only-l cudart-12.6)


.SUFFIXES: .cu

.cu.o:
	nvcc $(CXXFLAGS) -c $<
#
#	/usr/bin/clang++-17 $(CXXFLAGS) --cuda-gpu-arch=sm_87 --cuda-path=/usr/local/cuda-12.6 -MD -MT $<

	
2025AprilTag: $(OBJS) 
	$(CXX) $(LDFLAGS) -o 2025AprilTag $(OBJS) -l2702 $(LIBS) -Lapriltags_cuda/build -lapriltag_cuda -lapriltag -lglog -lgflags


frc971/orin/lib971.a:
	make -C frc971/orin

thrid_party/apriltag/libapriltag.a:
	make -C third_party/apriltag

$(OBJS): ../../lib/v4lcam.h ../../lib/cammgr.h ../../lib/fbdisp.h #Makefile


TSTOBJS=tst.o raw2rgb.o
tst.o: ../../lib/v4lcam.h
tst: $(TSTOBJS)
	$(CXX) $(CXXFLAGS) -o tst $(TSTOBJS) -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -lpthread -lapril_tagging

v4linfo: v4linfo.o
	$(CXX) $(CXXFLAGS) -o v4linfo v4linfo.o
	
# DO NOT DELETE
