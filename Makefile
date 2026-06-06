TARGET      := switch-play-timer-overlay
BUILD       := build

CXX         ?= aarch64-none-elf-c++
CC          ?= aarch64-none-elf-gcc

CFLAGS      := -Wall -O2 -march=armv8-a -mtune=cortex-a57 \
               -I$(PORTLIBS)/include -I$(LIBNX)/include
CXXFLAGS    := $(CFLAGS) -std=c++17 -fno-exceptions -fno-rtti
LIBS        := -ltesla -lnx

SOURCE_DIR  := source
SOURCES     := $(wildcard $(SOURCE_DIR)/*.cpp)
OBJECTS     := $(SOURCES:$(SOURCE_DIR)/%.cpp=$(BUILD)/$(SOURCE_DIR)/%.o)

TARGET_ELF  := $(BUILD)/$(TARGET).elf
TARGET_OVL  := $(BUILD)/$(TARGET).ovl

.PHONY: all clean

all: $(TARGET_OVL)

$(TARGET_OVL): $(TARGET_ELF)
	cp -f $< $@

$(TARGET_ELF): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@

$(BUILD)/$(SOURCE_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)
