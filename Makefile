.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
	$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

include $(TOPDIR)/Makefile.config

TARGET := $(BINDIR)/$(BINNAME)

LIBS     := $(foreach lib,$(LIBRARIES),-l$(lib))
LIBDIRS  := $(CTRULIB)

export OUTPUT   := $(CURDIR)/$(TARGET)
export TOPDIR   := $(CURDIR)

export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                   $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export BUILD    := build
export DEPSDIR  := $(CURDIR)/$(BUILD)

CFILES          := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES        := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

export LD       := $(CXX)

export OFILES   := $(addprefix $(BUILD)/, $(CPPFILES:.cpp=.o) $(CFILES:.c=.o))

export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   $(foreach dir,$(PORTLIBS),-I$(dir)/include) \
                   -I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
                   $(foreach dir,$(PORTLIBS),-L$(dir)/lib)

export APP_ICON := $(TOPDIR)/$(ICON)

export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh

# Compiler flags

ARCH     := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS   := -g -Wall -Wextra -pedantic -mword-relocations \
            -fomit-frame-pointer -ffunction-sections \
            $(ARCH) $(EXTRACFLAGS)

CFLAGS   += $(INCLUDE) -DARM11 -D_3DS

# Enable optimization outside debug builds
ifndef DEBUG
	CFLAGS += -Ofast
endif

# Get a version number based on Git tags and stuff
GIT_VER := $(shell git describe --dirty --always --tags)
ifneq ($(strip $(GIT_VER)),)
	CFLAGS += -DGIT_VER=\"$(GIT_VER)\"
endif

CXXFLAGS := $(CFLAGS) -fno-rtti -fexceptions -std=gnu++11

LDFLAGS   = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

ZIPNAME   = lumaupdater-$(shell git describe --tags | tr -d 'v').zip

# Shortcuts

all : prereq $(OUTPUT).3dsx $(OUTPUT).cia
3dsx: prereq $(OUTPUT).3dsx
cia : prereq $(OUTPUT).cia
pkg : prereq $(ZIPNAME)

prereq:
	@[ -d $(TOPDIR)/$(BUILD) ] || mkdir -p $(TOPDIR)/$(BUILD)
	@[ -d $(TOPDIR)/$(BINDIR) ] || mkdir -p $(TOPDIR)/$(BINDIR)

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(OUTPUT).cia $(OUTPUT).smdh $(TARGET).elf

# Archive

$(ZIPNAME): $(OUTPUT).cia
	mkdir -p $(TOPDIR)/archive/3DS/lumaupdate
	cp $(TOPDIR)/lumaupdater.cfg $(TOPDIR)/archive
	cp $(OUTPUT).cia $(TOPDIR)/archive
	cp $(OUTPUT).3dsx $(OUTPUT).smdh $(TOPDIR)/archive/3DS/lumaupdate
	@(cd archive; zip -r -9 ../$(ZIPNAME) .)
	rm -rf $(TOPDIR)/archive
	@echo
	@echo "built ... $(ZIPNAME)"

# Output

MAKEROM ?= makerom

$(OUTPUT).elf: $(OFILES)

$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh

$(OUTPUT).cia: $(OUTPUT).elf $(BUILD)/banner.bnr $(BUILD)/icon.icn
	$(MAKEROM) -f cia -o $@ -elf $< -rsf $(TOPDIR)/rominfo.rsf -target t -exefslogo -banner $(BUILD)/banner.bnr -icon $(BUILD)/icon.icn -DAPP_TITLE="$(APP_TITLE)" -DPRODUCT_CODE="$(PRODUCT_CODE)" -DUNIQUE_ID="$(UNIQUE_ID)"
	@echo "built ... $(BINNAME).cia"

# Banner

BANNERTOOL ?= bannertool

ifeq ($(suffix $(BANNER_IMAGE)),.cgfx)
	BANNER_IMAGE_ARG := -ci
else
	BANNER_IMAGE_ARG := -i
endif

ifeq ($(suffix $(BANNER_AUDIO)),.cwav)
	BANNER_AUDIO_ARG := -ca
else
	BANNER_AUDIO_ARG := -a
endif

$(BUILD)/%.bnr: $(BANNER_IMAGE) $(BANNER_AUDIO)
	$(BANNERTOOL) makebanner $(BANNER_IMAGE_ARG) $(BANNER_IMAGE) $(BANNER_AUDIO_ARG) $(BANNER_AUDIO) -o $@

$(BUILD)/%.icn: $(ICON)
	$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_TITLE)" -p "$(APP_TITLE)" -i $(ICON) -f visible,allow3d,extendedbanner,nosavebackups -o $@

# Source

DEPENDS := $(OFILES:.o=.d)

$(BUILD)/%.o: %.cpp
	@echo $(notdir $<)
	$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) -c $< -o $@ $(ERROR_FILTER)

$(BUILD)/%.o: %.c
	@echo $(notdir $<)
	$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) -c $< -o $@ $(ERROR_FILTER)

-include $(DEPENDS)
