CXX ?= g++

SRC_FILES = $(wildcard entrospy/*.cpp)
OBJ_FILES = $(SRC_FILES:.cpp=.o)

LD_FLAGS = -lboost_program_options -lboost_system -lboost_filesystem

MKDIR_P = mkdir -p

.PHONY: directories

all: directories entrospy

directories:
	mkdir -p build

entrospy: $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) -O3 -std=c++11 $(LD_FLAGS) -o build/entrospy

entrospy/%.o: entrospy/%.cpp
	$(CXX) $(CC_FLAGS) -O3 -c -o $@ $< -Ientrospy/include -std=c++11 -Wall -Wextra

clean:
	rm -r build $(OBJ_FILES)
