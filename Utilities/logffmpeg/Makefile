CXXFLAGS+=-I../../lib
CXXFLAGS+=-Ofast
CXXFLAGS+=$(shell pkg-config --cflags opencv4 )
CXXFLAGS+=$(shell pkg-config --cflags libcamera )
CXXFLAGS+=$(shell pkg-config --cflags ../../pkgconfig/ubuntu/wpilib.pc)
LDFLAGS+=-L../../lib
LDFLAGS+=-L/usr/local/lib
LIBS+=$(shell pkg-config -libs libcamera)
LIBS+=$(shell pkg-config -libs opencv4)
LIBS+=-lapriltag -lwpilibNewCommands -lwpilibc -lcameraserver -lcscore -lntcore -lwpinet -lwpimath -lwpiutil
OBJS=logffmpeg.o

all: logffmpeg

clean:
	rm -f *.o
	rm -f logffmpeg

logffmpeg: $(OBJS)
	$(CXXFLAGS) -o logffmpeg $(LDFLAGS) $(OBJS) $(LIBS)
