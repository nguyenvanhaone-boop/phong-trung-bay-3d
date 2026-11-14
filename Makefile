# Build: make        Chay: make run        Kiem tra: make check        Don: make clean
# Mac: can Xcode Command Line Tools (xcode-select --install).
# Windows: MinGW + freeglut (https://github.com/TransmissionZero/freeglut-MinGW), chay bang mingw32-make.

CXX      ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CXXFLAGS += -Iinclude -Isrc

ifeq ($(OS),Windows_NT)
  CXXFLAGS += -pthread
  GL_LIBS = -lfreeglut -lopengl32 -lglu32
  EXE     = .exe
else
  UNAME := $(shell uname -s)
  ifeq ($(UNAME),Darwin)
    CXXFLAGS += -DGL_SILENCE_DEPRECATION
    GL_LIBS   = -framework OpenGL -framework GLUT
  else
    CXXFLAGS += -pthread
    GL_LIBS   = -lglut -lGL
  endif
  EXE =
endif

BUILD = build
SRC   = $(wildcard src/*.cpp src/*/*.cpp)
OBJ   = $(patsubst src/%.cpp,$(BUILD)/obj/%.o,$(SRC))
LIB_OBJ = $(filter-out $(BUILD)/obj/main.o,$(OBJ))
HDR   = $(wildcard include/*.hpp src/*/*.hpp)

APP   = $(BUILD)/phong_trung_bay$(EXE)
CHECK = $(BUILD)/checks$(EXE)

.PHONY: all run check clean

all: $(APP)

$(APP): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(GL_LIBS)

$(BUILD)/obj/%.o: src/%.cpp $(HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Kiem tra thuat toan khong can cua so (chay duoc ca khi khong co GLUT).
$(CHECK): tests/checks.cpp $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

run: $(APP)
	./$(APP)

check: $(CHECK)
	./$(CHECK)

clean:
	rm -rf $(BUILD)
