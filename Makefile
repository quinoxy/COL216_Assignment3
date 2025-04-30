CXX = g++
CXXFLAGS = -g -std=c++17 -Iinclude

SRCS = src/input.cpp src/cache.cpp src/dll.cpp src/output.cpp src/testing2.cpp
OBJS = $(SRCS:.cpp=.o)

L1simulate: $(OBJS)
    $(CXX) $(CXXFLAGS) -o L1simulate $(OBJS)

src/%.o: src/%.cpp
    $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
    rm -f L1simulate $(OBJS)

.PHONY: clean