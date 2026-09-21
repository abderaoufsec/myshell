# Makefile for myshell project (Windows/MSYS2)
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -I include
LDFLAGS = -static -static-libgcc -static-libstdc++

# Source files
SOURCES = src/main.cpp src/shell.cpp src/parser.cpp src/executor.cpp src/builtins.cpp src/jobs.cpp src/posix_compat.cpp
TEST_SOURCES = tests/test_parser_only.cpp src/parser.cpp src/builtins.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)

# Executables
TARGET = myshell.exe
TEST_TARGET = test_parser_only.exe

# Default target
all: $(TARGET)

# Main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Test executable
test: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(TARGET) $(TEST_TARGET)
	rm -f *.o *.exe

# Run tests
run-test: test
	./$(TEST_TARGET)

.PHONY: all test clean run-test