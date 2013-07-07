CC=g++
CFLAGS= -x c -Wall 
LDFLAGS=
SOURCES=filer.c arrlist.c arrlist.h lodepng.c lodepng.h
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=filer

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@