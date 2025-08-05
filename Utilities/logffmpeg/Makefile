CXXFLAGS+=-I../../lib
CXXFLAGS+=-Ofast
CXXFLAGS+=$(shell pkg-config --cflags opencv4)
LDFLAGS+=-L../../lib
LDFLAGS+=$(shell pkg-config --libs-only-L opencv4)
LIBS+=-l2702
LIBS+=$(shell pkg-config --libs-only-l opencv4)
OBJS=logffmpeg.o

all: logffmpeg

clean:
	rm -f *.o
	rm -f logffmpeg

logffmpeg: $(OBJS)
	$(CXX) -o logffmpeg $(LDFLAGS) $(OBJS) $(LIBS)
