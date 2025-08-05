OBJS=log2mjpeg.o
#CXXFLAGS+=-std=c++20 -I/usr/local/frc/include -L/usr/local/frc/lib -I/usr/local/frc/include/opencv4 -Wno-psabi -fopenmp -Wno-deprecated-enum-enum-conversion
#CXXFLAGS+=-std=c++20 -I/usr/local/frc/include -L/usr/local/frc/lib -I/usr/local/frc/include/opencv4 -Wno-psabi -Wno-deprecated-enum-enum-conversion -I../../lib -L../../lib
CXXFLAGS+=-std=c++20 -Wno-psabi -Wno-deprecated-enum-enum-conversion -I../../lib -L../../lib
CXXFLAGS+=-g
CXXFLAGS+=-Ofast
CXXFLAGS+=$(shell pkg-config --cflags opencv4 )
CXXFLAGS+=$(shell pkg-config --cflags ../../pkgconfig/ubuntu/wpilib.pc)
#CXXFLAGS+=-O2
LDFLAGS+=-L../../lib
LIBS+=-l2702
LDFLAGS+=$(shell pkg-config --libs-only-L opencv4)
LIBS+=$(shell pkg-config -libs ../../pkgconfig/ubuntu/wpilib.pc)
all: log2mjpeg

clean:
	rm -f *.o
	rm -f log2mjpeg

log2mjpeg: $(OBJS)
	$(CXX) $(CXXFLAGS) -o log2mjpeg $(OBJS) $(LDFLAGS) -l2702 -lpthread $(LIBS)

#$(CXX) $(CXXFLAGS) -o log2mjpeg $(OBJS) -l2702 -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lpthread -lntcore -lcscore -lcameraserver -lwpiutil

#$(OBJS): v4lcam-mp.h bpicam.h

