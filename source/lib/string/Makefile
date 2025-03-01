

# Include common Makefile
include ../../../Makefile.common

# 手动配置项
PROJECT := string
INCLUDE := ../include ../../os_config
SRCDIR := 
LIBDIR := 

# 查找库文件并生成链接选项
LIB := $(shell find $(LIBDIR) -name '*.a')
LIBS := $(patsubst %,-L%,${LIBDIR})

LFLAGS := $(patsubst lib%.a,-l%,${LIB})
LFLAGS += 

# 路径配置
BUILD := ../../../build/lib
IMAGE_DIR := ../../../image
BUILDS := ${BUILD}/${PROJECT}
OUT := lib$(PROJECT).a

INCLUDES := $(patsubst %,-I%,${INCLUDE})

# 查找源文件
C_SRCS := $(shell find ./ -name '*.c')
ASM_SRCS := $(shell find ./ -name '*.S')

# 判断 SRCDIR 是否为空，并相应地添加源文件
ifeq ($(strip $(SRCDIR)),)
else
    C_SRCS += $(shell find $(SRCDIR) -name '*.c')
    ASM_SRCS += $(shell find $(SRCDIR) -name '*.S')
endif

# 将所有源文件合并到 SRCS 中，并生成目标文件列表
SRCS := $(ASM_SRCS) $(C_SRCS)

# 生成目标文件路径（添加到 $(BUILDS)/ 目录中）
#OBJS := $(patsubst ./%, $(BUILDS)/%, $(SRCS))
OBJS := $(patsubst ./%, $(BUILDS)/%, $(SRCS:.c=.o))  # 将 .c 替换为 .o
OBJS := $(OBJS:.S=.o)  # 将 .S 替换为 .o

# 生成依赖文件路径（添加到 $(BUILDS)/ 目录中）
DEPS := $(patsubst ./%, $(BUILDS)/%, $(SRCS:.c=.d))  # 将 .c 替换为 .d
DEPS := $(DEPS:.S=.d)  # 将 .S 替换为 .d

# 创建构建目录的命令
MD := mkdir -p

# 默认目标  注：objcopy -O binary 去掉了elf头，只保留数据段和代码段，由于直接加载，**汇编的时候，数据段要写到代码段后面**。
all: $(BUILDS) $(IMAGE_DIR) $(OUT)
	@echo "Executing 'all' complete!"
	$(OBJDUMP_TOOL) -x -d -S -m i386 $(OUT) > $(BUILDS)/$(PROJECT)_dist.txt
	$(READELF_TOOL) -a $(OUT) > $(BUILDS)/$(PROJECT)_elf.txt
	mv $(OUT) ../

# 创建构建目录
$(BUILDS):
	-${MD} ${BUILDS}
	@echo "Build directory created."

# 创建镜像目录
$(IMAGE_DIR):
	-${MD} ${IMAGE_DIR}
	@echo "Image directory created."

# 链接目标文件生成静态库文件
$(OUT): $(OBJS)
	$(AR) rcs $@ $(OBJS)
# 自动包含依赖文件
-include $(DEPS)

# 编译 C 文件
$(BUILDS)/%.o: %.c
	@mkdir -p $(dir $@)  
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -c $< -o $@

# 编译汇编文件
$(BUILDS)/%.o: %.S
	@mkdir -p $(dir $@) 
	$(CC) $(ASFLAGS) $(INCLUDES) -MMD -c $< -o $@

.PHONY: clean test

# 测试规则
test:
	@echo "OBJS: $(OBJS)"
	@echo "DEPS: $(DEPS)"
	@echo "SRCS: $(SRCS)"
	@echo "BUILDS: $(BUILDS)"


# 清理规则
clean:
	-rm -f $(OBJS) $(DEPS)
	-rm -f $(BUILDS)/*.txt
	-rm -f ../$(OUT)
	@echo "Clean complete."

