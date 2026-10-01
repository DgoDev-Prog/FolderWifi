#---------------------------------------------------------------------------------
# FolderWifi - Nintendo Switch Homebrew
#---------------------------------------------------------------------------------

.SUFFIXES:

#---------------------------------------------------------------------------------
# devkitPro
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)

include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# Proyecto
#---------------------------------------------------------------------------------

TARGET   := FolderWifi
BUILD    := build
SOURCES  := source
DATA     := data
INCLUDES := include

#---------------------------------------------------------------------------------
# Información mostrada en hbmenu
#---------------------------------------------------------------------------------

APP_TITLE   := FolderWifi
APP_AUTHOR  := Tssr (Diego Ramirez)
APP_VERSION := 0.2.0-alpha

# Nombre del JPG ubicado junto al Makefile
ICON := icon.jpg

#---------------------------------------------------------------------------------
# Arquitectura Nintendo Switch
#---------------------------------------------------------------------------------

ARCH := -march=armv8-a+crc+crypto \
        -mtune=cortex-a57 \
        -mtp=soft \
        -fPIE

#---------------------------------------------------------------------------------
# Compilación
#---------------------------------------------------------------------------------

CFLAGS := -g \
          -Wall \
          -O2 \
          -ffunction-sections \
          -fdata-sections \
          $(ARCH) \
          $(DEFINES)

CFLAGS += $(INCLUDE) -D__SWITCH__

CXXFLAGS := $(CFLAGS) \
            -std=gnu++17 \
            -fno-rtti \
            -fno-exceptions

ASFLAGS := -g $(ARCH)

#---------------------------------------------------------------------------------
# Linker
#---------------------------------------------------------------------------------

LDFLAGS := -specs=$(DEVKITPRO)/libnx/switch.specs \
           -g \
           $(ARCH) \
           -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# Librerías
#---------------------------------------------------------------------------------

LIBS := -lminizip -lz -lnx

LIBDIRS := $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
# Build
#---------------------------------------------------------------------------------

ifneq ($(BUILD),$(notdir $(CURDIR)))

#---------------------------------------------------------------------------------

export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)

export VPATH := \
	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
	$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR := $(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# Archivos fuente
#---------------------------------------------------------------------------------

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# Linker C / C++
#---------------------------------------------------------------------------------

ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif

#---------------------------------------------------------------------------------
# Objetos
#---------------------------------------------------------------------------------

export OFILES_BIN := $(addsuffix .o,$(BINFILES))

export OFILES_SRC := \
	$(CPPFILES:.cpp=.o) \
	$(CFILES:.c=.o) \
	$(SFILES:.s=.o)

export OFILES := $(OFILES_BIN) $(OFILES_SRC)

export HFILES_BIN := \
	$(addsuffix .h,$(subst .,_,$(BINFILES)))

#---------------------------------------------------------------------------------
# Includes
#---------------------------------------------------------------------------------

export INCLUDE := \
	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
	-I$(CURDIR)/$(BUILD)

export LIBPATHS := \
	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

#---------------------------------------------------------------------------------
# Icono
#---------------------------------------------------------------------------------

ifeq ($(strip $(ICON)),)

icons := $(wildcard *.jpg)

ifneq (,$(findstring $(TARGET).jpg,$(icons)))
export APP_ICON := $(TOPDIR)/$(TARGET).jpg
else
ifneq (,$(findstring icon.jpg,$(icons)))
export APP_ICON := $(TOPDIR)/icon.jpg
endif
endif

else

export APP_ICON := $(TOPDIR)/$(ICON)

endif

#---------------------------------------------------------------------------------
# Generación del NRO
#
# Estas dos líneas son las que faltaban principalmente en nuestro Makefile
# anterior.
#---------------------------------------------------------------------------------

export NROFLAGS += --icon=$(APP_ICON)
export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp

#---------------------------------------------------------------------------------

.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
# Clean
#---------------------------------------------------------------------------------

clean:
	@echo Limpiando proyecto...
	@rm -fr \
		$(BUILD) \
		$(TARGET).nro \
		$(TARGET).nacp \
		$(TARGET).elf \
		$(TARGET).map \
		$(TARGET).lst

#---------------------------------------------------------------------------------

else

#---------------------------------------------------------------------------------
# Build interno
#---------------------------------------------------------------------------------

.PHONY: all

DEPENDS := $(OFILES:.o=.d)

all: $(OUTPUT).nro

# MUY IMPORTANTE:
# El NRO depende tanto del ELF como del NACP.
$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp

$(OUTPUT).elf: $(OFILES)

$(OFILES_SRC): $(HFILES_BIN)

#---------------------------------------------------------------------------------
# Datos binarios
#---------------------------------------------------------------------------------

%.bin.o %_bin.h: %.bin
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------

-include $(DEPENDS)

#---------------------------------------------------------------------------------

endif