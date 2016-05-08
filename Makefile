CXX ?= g++

SRC_FILES = $(wildcard entrospy/*.cpp)
OBJ_FILES = $(SRC_FILES:.cpp=.o)

LD_FLAGS = -lboost_program_options -lboost_system -lboost_filesystem

all: entrospy.out

entrospy.out: $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) -std=c++11 $(LD_FLAGS) -o entrospy.out

entrospy/%.o: entrospy/%.cpp
	$(CXX) $(CC_FLAGS) -c -o $@ $< -Ientrospy/include -std=c++11 -Wall -Wextra

clean:
	rm entrospy.out $(OBJ_FILES)
