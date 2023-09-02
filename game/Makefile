


# Your output and source files
OUTPUT = ./bin/Linux/main
SOURCES = src/main.cpp src/glad.c src/textrendering.cpp src/tiny_obj_loader.cpp src/stb_image.cpp 

# Compiler Flags
CXXFLAGS = -std=c++11 -Wall -Wno-unused-function -g -I ./include/

# Libraries for linking
LIBS = ./lib-linux/libglfw3.a -lrt -lm -ldl -lX11 -lpthread -lXrandr -lXinerama -lXxf86vm -lXcursor

# Default rule
all: $(OUTPUT)

# Rule for building the main output
$(OUTPUT): $(SOURCES)
	mkdir -p bin/Linux
	g++ $(CXXFLAGS) -o $(OUTPUT) $(SOURCES) $(LIBS)

# Additional rules
.PHONY: clean run

clean:
	rm -f $(OUTPUT)

run: $(OUTPUT)
	cd bin/Linux && ./main
