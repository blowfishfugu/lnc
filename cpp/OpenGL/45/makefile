CC=g++
OSTYPE := $(shell uname)

ifeq ($(OSTYPE),Linux)
	CFLAGS=-c -Wall -std=c++17 -Wunreachable-code -fopenmp
	LFLAGS=-lglfw -lGLEW -lGL -L../Utils -lutils -fopenmp -lstdc++fs
	LIBS=
	INCLUDES=-I. -I../Utils
else
	CFLAGS=-c -Wall -std=c++17 -Wunreachable-code -Xclang -fopenmp
	LFLAGS=-lglfw -lGLEW -framework OpenGL -L../Utils -lutils
	LIBS=-lomp -L ../../openmp/lib -L /opt/homebrew/lib
	INCLUDES=-I. -I../Utils -I /opt/homebrew/include -I ../../openmp/include
endif

SRC = YAKManager.cpp YAK42.cpp main.cpp
OBJ = $(SRC:.cpp=.o)
RES = helvetica_neue.pos helvetica_neue.bmp
TARGET = YAK-Fractal

all: $(TARGET) $(RES)

release: CFLAGS += -O3 -DNDEBUG
release: $(TARGET) $(RES)

../Utils/libutils.a:
	cd ../Utils && make $(MAKECMDGOALS)

$(RES):
	cp ../Utils/$@ $@

$(TARGET): $(OBJ) ../Utils/libutils.a
	$(CC) $(INCLUDES) $^ $(LFLAGS) $(LIBS) -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

clean:
	-rm -rf $(OBJ) $(TARGET) $(RES) core

mrproper: clean
	cd ../Utils && make clean
