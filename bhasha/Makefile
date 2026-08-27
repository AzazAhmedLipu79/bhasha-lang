# Bhasha Compiler - Makefile
CXX      ?= clang++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wno-unused-parameter -O0 -g

SRC_DIR := src
OBJ_DIR := build

SRCS := \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/lexer/Token.cpp \
	$(SRC_DIR)/lexer/Lexer.cpp \
	$(SRC_DIR)/ast/Type.cpp \
	$(SRC_DIR)/parser/Parser.cpp \
	$(SRC_DIR)/semantic/SymbolTable.cpp \
	$(SRC_DIR)/semantic/SemanticAnalyzer.cpp \
	$(SRC_DIR)/codegen/PythonGenerator.cpp

OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

TARGET := bhashac

.PHONY: all clean test run-demo

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

test: $(TARGET)
	@echo "=== Running tests ==="
	@bash tests/run_tests.sh

run-demo: $(TARGET)
	./$(TARGET) examples/hello.bng
