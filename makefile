TARGET = jsregexp.so
SOURCES = jsregexp.cpp
INCLUDE = -I/usr/include/lua5.1
LIBOPTS = -shared -llua5.1
FLAGS = -fpic
CXX = g++
OBJECTS = $(SOURCES:%.cpp=%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	g++ $(FLAGS) $(LIBOPTS) $^ -L./ -o $@

%.o: %.cpp
	g++ $(FLAGS) -c $^ $(INCLUDE) -o $@

clean:
	rm -f *.o *.so
