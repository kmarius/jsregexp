TARGET = jsregexp.so
SOURCES = jsregexp.c cutils.c libregexp.c libunicode.c
OBJECTS = $(SOURCES:%.c=%.o)
INCLUDE = -I/usr/include/luajit-2.1/
LIBOPTS = -shared -lluajit-5.1
FLAGS = -fpic -flto
CC = gcc

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) $(LIBOPTS) $^ -L./ -o $@

%.o: %.c
	$(CC) $(FLAGS) -c $^ $(INCLUDE) -o $@

check:
	lua5.1 test.lua
	luajit test.lua

clean:
	rm -f *.o *.so
