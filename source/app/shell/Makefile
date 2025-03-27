include ../../../Makefile.common

# 手动配置项
PROJECT := shell
INCLUDE := ../../lib/include ../../os_config ../../include
SRCDIR := 
LIBDIR :=  ../../lib/std ../../lib

# 查找库文件并生成链接选项
LIB := $(shell find $(LIBDIR) -name '*.a')
LIB_PATHS := $(sort $(dir $(LIB)))   # 提取库文件所在的目录
LIB_NAMES := $(notdir $(LIB))       # 提取库文件名

LIBS := $(addprefix -L, $(LIB_PATHS))       # 生成 -L 参数
LFLAGS := $(patsubst lib%.a,-l%, $(LIB_NAMES)) # 生成 -l 参数
LFLAGS += -melf_i386 -Tshell.lds

# 路径配置
BUILD := ../../../build
IMAGE_DIR := ../../../image
BUILDS := ${BUILD}/${PROJECT}
OUT := ${PROJECT}.elf

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
OBJS := $(addsuffix .o, $(basename $(SRCS)))
DEPS := $(addsuffix .d, $(basename $(SRCS)))

# 创建构建目录的命令
MD := mkdir -p

#这里用objcopy -S 仅仅去除所有调试信息，生成的文件并不是纯二进制格式，还是elf格式，只不过去除了调试信息
all: $(BUILDS) $(IMAGE_DIR) $(OUT)
	@echo "Executing 'all' complete!"
	$(OBJCOPY_TOOL) -S $(OUT) $(IMAGE_DIR)/${PROJECT}.x.elf
	$(OBJDUMP_TOOL) -x -d -S -m i8086 $(OUT) > $(BUILDS)/${PROJECT}_dis.txt
	$(OBJDUMP_TOOL) -x -d -S -m i8086 $(IMAGE_DIR)/${PROJECT}.x.elf > $(BUILDS)/${PROJECT}_x_dis.txt
	$(READELF_TOOL) -a $(OUT) > $(BUILDS)/${PROJECT}_elf.txt
	mv $(OUT) $(IMAGE_DIR)

# 创建构建目录
$(BUILDS):
	-${MD} ${BUILDS}
	@echo "Build directory created."

# 创建镜像目录
$(IMAGE_DIR):
	-${MD} ${IMAGE_DIR}
	@echo "Image directory created."

# 链接目标文件生成 elf 文件
$(OUT): $(OBJS)
	$(LD) $(OBJS) $(LIBS) $(LFLAGS) -o $@

# 自动包含依赖文件
-include $(DEPS)

# 编译 C 文件
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -c $< -o $@

# 编译汇编文件
%.o: %.S
	$(CC) $(ASFLAGS) $(INCLUDES) -MMD -c $< -o $@

.PHONY: clean test

# 测试规则
test:
	@echo "OBJS: $(OBJS)"

# 清理规则
clean:
	-rm -f $(OBJS) $(DEPS)
	-rm -f $(BUILDS)/*.txt
	-rm -f $(IMAGE_DIR)/$(PROJECT).*
