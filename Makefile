.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
	$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

include $(TOPDIR)/Makefile.config

TARGET := $(BINDIR)/$(BINNAME)


# Enable optimization outside debug builds
ifndef DEBUG
	CFLAGS += -Ofast
endif

# Get a version number based on Git tags and stuff
GIT_VER := $(shell git describe --dirty --always --tags)
ifeq ($(strip $(GIT_VER)),)
	CFLAGS += -DGIT_VER=\"$(GIT_VER)\"
endif

LIBS     := $(foreach lib,$(LIBRARIES),-l$(lib))
LIBDIRS  := $(CTRULIB)

export OUTPUT   := $(CURDIR)/$(TARGET)
export TOPDIR   := $(CURDIR)

export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                   $(foreach dir,$(DATA),$(CURDIR)/$(dir))

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
            $(ARCH)

CFLAGS   += $(INCLUDE) -DARM11 -D_3DS

CXXFLAGS := $(CFLAGS) -fno-rtti -fexceptions -std=gnu++11

LDFLAGS   = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

ZIPNAME   = lumaupdater-$(shell git describe --tags | tr -d 'v').zip

# Shortcuts
all : prereq $(OUTPUT).3dsx $(OUTPUT).cia
3dsx: prereq $(OUTPUT).3dsx
cia : prereq $(OUTPUT).cia
pkg : prereq $(ZIPNAME)

prereq:
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TOPDIR)/archive $(TARGET).3dsx $(OUTPUT).cia $(OUTPUT).smdh $(TARGET).elf

DEPENDS    := $(OFILES:.o=.d)
MAKEROM    ?= makerom
BANNERTOOL ?= bannertool

$(ZIPNAME): $(OUTPUT).cia
	mkdir -p $(TOPDIR)/archive/3DS/lumaupdate
	cp $(TOPDIR)/lumaupdater.cfg $(TOPDIR)/archive
	cp $(OUTPUT).cia $(TOPDIR)/archive
	cp $(OUTPUT).3dsx $(OUTPUT).smdh $(TOPDIR)/archive/3DS/lumaupdate
	@(cd archive; zip -r -9 ../$(ZIPNAME) .)
	rm -rf $(TOPDIR)/archive
	@echo
	@echo "built ... $(ZIPNAME)"

$(OUTPUT).elf: $(OFILES)

$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh

$(OUTPUT).cia: $(OUTPUT).elf $(OUTPUT).smdh
	@$(MAKEROM) -f cia -o $@ -elf $< -rsf $(TOPDIR)/rominfo.rsf -target t -exefslogo -icon $(OUTPUT).smdh -DAPP_TITLE="$(APP_TITLE)" -DPRODUCT_CODE="$(PRODUCT_CODE)" -DUNIQUE_ID="$(UNIQUE_ID)"
	@echo "built ... $(BINNAME).cia"

$(BUILD)/%.o: %.cpp
	@echo $(notdir $<)
	$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) -c $< -o $@ $(ERROR_FILTER)

$(BUILD)/%.o: %.c
	@echo $(notdir $<)
	$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) -c $< -o $@ $(ERROR_FILTER)

-include $(DEPENDS)