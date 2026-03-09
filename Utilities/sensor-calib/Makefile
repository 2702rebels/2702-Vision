OBJS=sensor-calib.o
#CXXFLAGS+=-std=c++20 -I/usr/local/frc/include -L/usr/local/frc/lib -I/usr/local/frc/include/opencv4 -Wno-psabi -fopenmp -Wno-deprecated-enum-enum-conversion
#CXXFLAGS+=-std=c++20 -I/usr/local/frc/include -L/usr/local/frc/lib -I/usr/local/frc/include/opencv4 -Wno-psabi -Wno-deprecated-enum-enum-conversion -I../../lib -L../../lib
CXXFLAGS+=-std=c++20 -Wno-psabi -Wno-deprecated-enum-enum-conversion -I../../lib -L../../lib
CXXFLAGS+=$(shell pkg-config --cflags opencv4 )
CXXFLAGS+=-g
#CXXFLAGS+=-O2
LDFLAGS+=$(shell pkg-config --libs-only-L opencv4)

sensor-calib: $(OBJS)
	$(CXX) $(CXXFLAGS) -o sensor-calib $(OBJS) $(LDFLAGS) -l2702 -lopencv_calib3d -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lpthread

