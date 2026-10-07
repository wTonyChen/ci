# =============================================================================
# Chromium-NX :: spike 探针构建 (devkitPro / libnx)
#
#   make PROBE=ipv6     -> build/ipv6/probe_ipv6.nro
#   make PROBE=posix    -> build/posix/probe_posix.nro
#   make PROBE=gl       -> build/gl/probe_gl.nro
#   bash build-all.sh   -> 一次编三个
#
# 结构照搬 devkitPro/libnx 标准模板（与 nx.js 的 Makefile 同构）。
# =============================================================================
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=/opt/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

PROBE       ?= ipv6
TARGET      := probe_$(PROBE)
BUILD       := build/$(PROBE)
SOURCES     := src/$(PROBE)
DATA        :=
INCLUDES    :=
ROMFS       :=

APP_TITLE   := probe-$(PROBE)
APP_AUTHOR  := Chromium-NX
APP_VERSION := 0.1

NO_ICON     := 1

# ---------------------------------------------------------------------------
# 编译选项
# ---------------------------------------------------------------------------
ARCH    := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS  := -g -Wall -O2 -ffunction-sections $(ARCH) $(DEFINES)
CFLAGS  += $(INCLUDE) -D__SWITCH__ -D_GNU_SOURCE

LDFLAGS  = -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

# gl 探针需要 EGL / GLES / glapi
ifeq ($(PROBE),gl)
LIBS    := -lEGL -lGLESv2 -lglapi -ldrm_nouveau -lnx -lm
else
LIBS    := -lnx -lm
endif

LIBDIRS := $(DEVKITPRO)/libnx $(PORTLIBS)

# ===========================================================================
ifeq ($(IN_SUBDIR),)
# ------------------------- 顶层 make =======================================
# ===========================================================================
# 用显式哨兵区分顶层/子目录：BUILD 形如 build/ipv6，而 notdir(CURDIR) 是 ipv6，
# 两者永不相等 -> 子 make 会走回顶层分支导致递归错位。
export IN_SUBDIR := 1
MKFILE          := $(CURDIR)/Makefile
export OUTPUT   := $(CURDIR)/$(TARGET)
export TOPDIR   := $(CURDIR)
export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                   $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR  := $(CURDIR)/$(BUILD)

CFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES     := $(OFILES_SRC)

export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif

# gl 探针要链接 Mesa 的 C++ 代码（glapi），必须用 CXX 链接以带上 libstdc++
ifeq ($(PROBE),gl)
export LD := $(CXX)
endif

# .nacp（hbmenu 需要）。NO_ICON 已置 1，故不传 --icon。
ifeq ($(strip $(NO_NACP)),)
export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

.PHONY: all clean
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $@ -f $(MKFILE)

clean:
	@echo clean $(TARGET) ...
	@rm -fr build $(TARGET).pfs0 $(TARGET).nso $(TARGET).nro \
	       $(TARGET).nacp $(TARGET).elf $(TARGET).npdm

# ===========================================================================
else
# ------------------------- 构建目录内 make ==================================
# ===========================================================================
.PHONY: all
DEPENDS := $(OFILES:.o=.d)

all : $(OUTPUT).nro

$(OUTPUT).nso : $(OUTPUT).elf
$(OUTPUT).nro : $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf : $(OFILES)

# devkitA64 的 base_rules 只定义了 %.o: %.cpp
%.o: %.c
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) -c $< -o $@ $(ERROR_FILTER)

-include $(DEPENDS)
endif
