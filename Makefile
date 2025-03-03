# 编译器配置
CXX := g++
CXXFLAGS := -std=c++11 -Wall -I./include
LDFLAGS := -L./lib -Wl,-rpath=./lib

# 目录配置
SRC_DIR := src
LIB_DIR := lib
INC_DIR := include

# 目标配置
LIB_NAME := mymalloc
LIB_TARGET := $(LIB_DIR)/lib$(LIB_NAME).so
EXEC_TARGET := memory_leak

# 源文件配置
LIB_SRC := $(SRC_DIR)/my_malloc.cpp
EXEC_SRC := $(SRC_DIR)/memory_leak.cpp

# 默认目标：编译动态库和可执行文件
all: $(LIB_TARGET) $(EXEC_TARGET)

# 创建必要的目录
$(LIB_DIR):
	mkdir -p $(LIB_DIR)

# 编译动态库
$(LIB_TARGET): $(LIB_SRC) | $(LIB_DIR)
	$(CXX) -shared -fPIC $< -o $@ $(CXXFLAGS)

# 编译可执行文件
$(EXEC_TARGET): $(EXEC_SRC) $(LIB_TARGET)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS) -l$(LIB_NAME) -ldl

# 清理生成文件
clean:
	rm -rf $(LIB_TARGET) $(EXEC_TARGET)

# 运行程序
run: $(EXEC_TARGET)
	./$(EXEC_TARGET)

.PHONY: all clean run