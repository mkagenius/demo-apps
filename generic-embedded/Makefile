UNAME_P := $(shell uname -p)

OPENBLASPATH=/usr/local/lib

CFLAGS = -O3 -Wall -c -fopenmp -fPIC -I../thnets -I/usr/include/freetype2
LIBS = -L$(OPENBLASPATH) -lopenblas -lm -lpthread -lX11 -lfreetype -lswscale
CC = gcc

ifneq ($(filter aarc% arm%,$(UNAME_P)),)
	CFLAGS += -mfpu=neon -DUSEGLES2
	LIBS += -lEGL -lGLESv2
else
	LIBS += -lGL
endif

.PHONY : all
all : thnetsdemo

.c.o:
	$(CC) $(CFLAGS) $<

thnetsdemo: $(LIBOBJS) thnetsdemo.o libgl.o videocap.o
	$(CC) -o $@ thnetsdemo.o libgl.o videocap.o $(LIBS) -L/usr/local/lib -lthnets

.PHONY : clean
clean :
	rm -f *.o thnetsdemo

