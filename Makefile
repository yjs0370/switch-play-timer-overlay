TARGET      := switch-play-timer
BUILD       := build

CXX         ?= aarch64-none-elf-g++
CC          ?= aarch64-none-elf-gcc
AS          ?= aarch64-none-elf-as
AR          ?= aarch64-none-elf-ar

DEVKITPRO   ?= /opt/devkitpro
LIBNX       ?= $(DEVKITPRO)/libnx
PORTLIBS    ?= $(DEVKITPRO)/portlibs/switch

CFLAGS      := -Wall -O2 -ffunction-sections -fdata-sections \
               -I$(PORTLIBS)/include -I$(LIBNX)/include
CXXFLAGS    := $(CFLAGS) -std=c++17 -fno-exceptions -fno-rtti
LDFLAGS     := -specs=$(LIBNX)/switch.specs -g -Wl,--build-id=nonce \
               -Wl,--gc-sections -Wl,-q,--wrap,srvcRegisterService \
               -L$(PORTLIBS)/lib -L$(LIBNX)/lib
LIBS        := -lnx -lpdmnt -lpminfo -lpctl -ltime -lpthread -ldeprecated -ldriver

SOURCE_DIR  := source
SOURCES     := $(wildcard $(SOURCE_DIR)/*.cpp)
OBJECTS     := $(SOURCES:$(SOURCE_DIR)/%.cpp=$(BUILD)/$(SOURCE_DIR)/%.o)

TARGET_ELF  := $(BUILD)/$(TARGET).elf
TARGET_NRO  := $(BUILD)/$(TARGET).nro

.PHONY: all clean

all: $(TARGET_NRO)

$(TARGET_NRO): $(TARGET_ELF)
	@cp -f $< $(BUILD)/$(TARGET).nsowrapper.elf
	@$(DEVKITPRO)/tools/bin/nacptool --create "Switch Play Timer" "PlayTimer" 1.0.0 $(BUILD)/$(TARGET).npdm
	@$(DEVKITPRO)/tools/bin/elf2nro $(BUILD)/$(TARGET).nsowrapper.elf $@ --nacp=$(BUILD)/$(TARGET).npdm --romfsdir=romfs
	@rm -f $(BUILD)/$(TARGET).nsowrapper.elf $(BUILD)/$(TARGET).npdm
	@echo "Built $@"

$(TARGET_ELF): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)

$(BUILD)/$(SOURCE_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)
