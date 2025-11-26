CXXFLAGS+=-I../../lib
CXXFLAGS+=$(shell pkg-config --cflags opencv4)

LDFLAGS+=-L../../lib
LDFLAGS+=$(shell pkg-config --libs-only-L opencv4)
LIBS+=-l2702
LIBS+=$(shell pkg-config --libs-only-l opencv4)

OBJS=logffmpeg.o

logffmpeg: $(OBJS)
	$(CXX) -o logffmpeg $(LDFLAGS) $(OBJS) $(LIBS) -lavcodec -lavutil
