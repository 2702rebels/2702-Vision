CXX=g++
CXXFLAGS=-Wall -Wextra -std=c++23
CXXFLAGS+=-O3
LINKFLAGS=
CXXFLAGS+=$(shell pkg-config --cflags spdlog)
LINKFLAGS+=$(shell pkg-config --libs spdlog)
LINKFLAGS+=-lcserialport
CXXFLAGS+=-I/usr/local/include
CXXFLAGS+=$(shell pkg-config --cflags opencv4)
CXXFLAGS+=-I$(shell pwd)
LINKFLAGS+=$(shell pkg-config --libs opencv4)

all: libloggingutils.a cameracalib setpos

loggingutils.o: LoggingUtils.cpp
	$(CXX) $(CXXFLAGS) -c LoggingUtils.cpp -o loggingutils.o $(LINKFLAGS)

libloggingutils.a: loggingutils.o
	ar rcs libloggingutils.a loggingutils.o

cameracalib.o: cameracalib.cpp
	$(CXX) $(CXXFLAGS) -c cameracalib.cpp -o cameracalib.o $(LINKFLAGS) -lloggingutils

setpos.o: setpos.cpp
	$(CXX) $(CXXFLAGS) -c setpos.cpp -o setpos.o $(LINKFLAGS) -lloggingutils

setpos: setpos.o
	$(CXX) $(CXXFLAGS) -o setpos setpos.o $(LINKFLAGS) -lloggingutils

cameracalib: cameracalib.o
	$(CXX) $(CXXFLAGS) -o cameracalib cameracalib.o $(LINKFLAGS) -lloggingutils

clean:
	rm -rf *.o cameracalib setpos loggingutils.a