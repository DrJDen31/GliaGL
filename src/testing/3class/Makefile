# Makefile for 3-Class Test

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
TARGET = 3class_test

# Source files
ARCH_DIR = ../../arch
SRC_FILES = 3class_test.cpp $(ARCH_DIR)/glia.cpp $(ARCH_DIR)/neuron.cpp
OBJ_FILES = 3class_test.o glia.o neuron.o

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ_FILES)
	@echo "Build complete! Run with: ./$(TARGET)"

# Compile 3class_test.cpp
3class_test.o: 3class_test.cpp $(ARCH_DIR)/glia.h $(ARCH_DIR)/neuron.h $(ARCH_DIR)/output_detection.h
	$(CXX) $(CXXFLAGS) -c 3class_test.cpp

# Compile glia.cpp
glia.o: $(ARCH_DIR)/glia.cpp $(ARCH_DIR)/glia.h $(ARCH_DIR)/neuron.h
	$(CXX) $(CXXFLAGS) -c $(ARCH_DIR)/glia.cpp

# Compile neuron.cpp
neuron.o: $(ARCH_DIR)/neuron.cpp $(ARCH_DIR)/neuron.h
	$(CXX) $(CXXFLAGS) -c $(ARCH_DIR)/neuron.cpp

# Clean build artifacts
clean:
	rm -f $(OBJ_FILES) $(TARGET)
	@echo "Clean complete"

# Run the test
run: $(TARGET)
	./$(TARGET)

# Rebuild from scratch
rebuild: clean all

.PHONY: all clean run rebuild
