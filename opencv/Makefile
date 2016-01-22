UNAME_P := $(shell uname -p)

OPENBLASPATH=/usr/local/lib

CFLAGS = -O3 -Wall -c -fopenmp -fPIC -I../thnets -I/usr/include/freetype2
LIBS = -L$(OPENBLASPATH) -lopenblas -lm -lpthread -lfreetype -lopencv_highgui -lopencv_core -lopencv_imgproc
CXX = g++
CC = gcc

ifneq ($(filter aarc% arm%,$(UNAME_P)),)
	CFLAGS += -mfpu=neon -DUSEGLES2
	LIBS += -lEGL -lGLESv2
else
	LIBS += -lGL
endif

.PHONY : all
all : thnetsopencvdemo

.cpp.o:
	$(CXX) $(CFLAGS) $<

.c.o:
	$(CC) $(CFLAGS) $<

thnetsopencvdemo: $(LIBOBJS) thnetsopencvdemo.o 8cc.o
	$(CXX) -o $@ thnetsopencvdemo.o 8cc.o $(LIBS) -L/usr/local/lib -lthnets

.PHONY : clean
clean :
	rm -f *.o thnetsopencvdemo
