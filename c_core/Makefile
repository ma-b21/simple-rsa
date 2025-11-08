# ==============================================
# Windows + MSVC Makefile for c_core
# ==============================================
# 编译器
CC := cl.exe
# 源码目录
SRC_DIR := src
TEST_DIR := test
BUILD_DIR := build
# 系统 MSVC/SDK include（请根据你实际路径修改版本号）
MSVC_INCLUDE := "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\include"
WINSDK_UCRT_INCLUDE := "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt"
WINSDK_SHARED_INCLUDE := "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared"
WINSDK_UM_INCLUDE := "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um"
WINSDK_WINRT_INCLUDE := "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt"

# 库路径（添加这些）
MSVC_LIB := "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\lib\x64"
WINSDK_UCRT_LIB := "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64"
WINSDK_UM_LIB := "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64"

# 编译选项
CFLAGS := /nologo /utf-8 /MD /O2 /GL /W3 /D_CRT_SECURE_NO_WARNINGS \
          -Iinclude \
          -I$(MSVC_INCLUDE) \
          -I$(WINSDK_UCRT_INCLUDE) \
          -I$(WINSDK_SHARED_INCLUDE) \
          -I$(WINSDK_UM_INCLUDE) \
          -I$(WINSDK_WINRT_INCLUDE)

# 链接选项
LDFLAGS := /link /LTCG \
           /LIBPATH:$(MSVC_LIB) \
           /LIBPATH:$(WINSDK_UCRT_LIB) \
           /LIBPATH:$(WINSDK_UM_LIB)

# 源文件与目标文件
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.obj)
# 测试文件
TEST_RSA_SRC := $(TEST_DIR)/test_rsa.c
TEST_RSA_OBJ := $(BUILD_DIR)/test_rsa.obj
TEST_RSA_TARGET := $(BUILD_DIR)/test_rsa.exe
TEST_PRIME_SRC := $(TEST_DIR)/test_prime.c
TEST_PRIME_OBJ := $(BUILD_DIR)/test_prime.obj
TEST_PRIME_TARGET := $(BUILD_DIR)/test_prime.exe
# ==============================================
# 规则
# ==============================================
.PHONY: all clean test test_rsa test_prime
all: test test_prime
test: $(TEST_RSA_TARGET)
	@echo ✅ 构建完成：$(TEST_RSA_TARGET)
	@echo 运行测试：
	@$(TEST_RSA_TARGET)
test_rsa: $(TEST_RSA_TARGET)
	@echo 运行 RSA 测试：
	@$(TEST_RSA_TARGET)
test_prime: $(TEST_PRIME_TARGET)
	@echo ✅ 构建完成：$(TEST_PRIME_TARGET)
	@echo 运行素数测试：
	@$(TEST_PRIME_TARGET)
# ==============================================
# 编译规则
# ==============================================
$(BUILD_DIR)/%.obj: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) /c $< $(CFLAGS) /Fo$@
$(BUILD_DIR)/test_rsa.obj: $(TEST_RSA_SRC) | $(BUILD_DIR)
	$(CC) /c $< $(CFLAGS) /Fo$@
$(BUILD_DIR)/test_prime.obj: $(TEST_PRIME_SRC) | $(BUILD_DIR)
	$(CC) /c $< $(CFLAGS) /Fo$@
# ==============================================
# 链接规则
# ==============================================
$(TEST_RSA_TARGET): $(OBJS) $(TEST_RSA_OBJ)
	$(CC) $(OBJS) $(TEST_RSA_OBJ) /Fe$@ $(LDFLAGS)
$(TEST_PRIME_TARGET): $(OBJS) $(TEST_PRIME_OBJ)
	$(CC) $(OBJS) $(TEST_PRIME_OBJ) /Fe$@ $(LDFLAGS)
# ==============================================
# 目录创建
# ==============================================
$(BUILD_DIR):
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
# ==============================================
# 清理
# ==============================================
clean:
	if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)