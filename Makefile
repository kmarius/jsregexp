TARGET = jsregexp.so
SOURCES = jsregexp.c cutils.c libregexp.c libunicode.c
OBJECTS = $(SOURCES:%.c=%.o)
INCLUDE = -I/usr/include/lua5.1
LIBOPTS = -shared -llua5.1
FLAGS = -fpic
CXX = gcc

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(FLAGS) $(LIBOPTS) $^ -L./ -o $@

%.o: %.c
	$(CXX) $(FLAGS) -c $^ $(INCLUDE) -o $@

clean:
	rm -f *.o *.so
