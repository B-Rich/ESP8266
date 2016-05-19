# ----------------------------------------------------------------------------
# ----------------------------------------------------------------------------
# Makefile for ESP8266 projects
# ----------------------------------------------------------------------------
# ----------------------------------------------------------------------------
BUILD_BASE			= build
FW_BASE				= firmware
TARGET				= yifi
MODULES		  		= user 
MODULES_INCDIR		= include 

# ----------------------------------------------------------------------------
# Compiler and linker flags
# ----------------------------------------------------------------------------
CFLAGS		        = -Os -ggdb  -Wpointer-arith -Wundef -Werror -std=c99 -Wall -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH -Wno-address
LDFLAGS				= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

# ----------------------------------------------------------------------------
# no user configurable options below here (unless you know what you are doing)
# ----------------------------------------------------------------------------
SDK_BASE			:= ../ESP8266_SDK
XTENSA_TOOLS_ROOT 	:= ../xtensa-lx106-elf/bin
LIBS				= c gcc hal pp phy net80211 lwip wpa main
LD_SCRIPT			= eagle.app.v6.ld
SDK_LIBDIR			= lib
SDK_LDDIR			= ld
FW_FILE				= 0x00000
FW_FILE_ARGS		=  -o firmware\ 
ESPTOOL				:= ../utils/esptool.exe
CC					:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR					:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD					:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
SRC_DIR				:= $(MODULES)
SDK_INCDIR			:= $(SDK_BASE)/include 
INCDIR				:= -I$(SDK_INCDIR) -I$(SRC_DIR) -I$(MODULES_INCDIR)
BUILD_DIR			:= $(addprefix $(BUILD_BASE)/,$(MODULES))
SDK_LIBDIR			:= $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SRC					:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
OBJ					:= $(patsubst %.c,$(BUILD_BASE)/%.o,$(SRC))
LIBS				:= $(addprefix -l,$(LIBS))
APP_AR				:= $(addprefix $(BUILD_BASE)/,$(TARGET)_app.a)
TARGET_OUT			:= $(addprefix $(BUILD_BASE)/,$(TARGET).out)
LD_SCRIPT			:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT))
FW_FILE_1			:= $(addprefix $(FW_BASE)/,$(FW_FILE_1).bin)
FW_FILE_2			:= $(addprefix $(FW_BASE)/,$(FW_FILE_2).bin)

# ----------------------------------------------------------------------------
# Do not fiddle with this gobblety gook, it is confusing but works
# ----------------------------------------------------------------------------
V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

vpath %.c $(SRC_DIR)

define compile-objects
$1/%.o: %.c
	$(vecho) "CC $$<"
	$(Q) $(CC) $(INCDIR) $(CFLAGS) -c $$< -o $$@
endef

.PHONY: all checkdirs clean

all: checkdirs $(TARGET_OUT) $(FW_FILE)

$(FW_FILE): $(TARGET_OUT) firmware
	$(vecho) "FW Making Bins"
	$(Q) $(ESPTOOL) elf2image $(TARGET_OUT) $(FW_FILE_ARGS)

$(TARGET_OUT): $(APP_AR)
	$(vecho) "LD $@"
	$(Q) $(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@

$(APP_AR): $(OBJ)
	$(vecho) "AR $@"
	$(Q) $(AR) cru $@ $^

checkdirs: $(BUILD_DIR) $(FW_BASE)

$(BUILD_DIR):
	$(Q) mkdir -p $@

firmware:
	$(Q) mkdir -p $@

clean:
	$(Q) rm -f $(APP_AR)
	$(Q) rm -f $(TARGET_OUT)
	$(Q) find $(BUILD_BASE) -type f | xargs rm -f
	$(Q) rm -f $(FW_FILE_1)
	$(Q) rm -f $(FW_FILE_2)
	$(Q) rm -rf $(FW_BASE)

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))

# ----------------------------------------------------------------------------

