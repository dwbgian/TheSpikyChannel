ifeq ($(strip $(DEVKITPPC)),)
$(error DEVKITPPC is not set. Install devkitPro/devkitPPC and source the environment first.)
endif

include $(DEVKITPPC)/wii_rules

TARGET		:= boot
BUILD		:= build
SOURCES		:= source
DATA		:= data
INCLUDES	:= include
APPDIR		:= apps/the_spiky_channel

CFLAGS		:= -g -O2 -Wall -Wextra -mrvl -mcpu=750 -meabi -mhard-float
CFLAGS		+= $(MACHDEP) $(INCLUDE)
CXXFLAGS	:= $(CFLAGS)
LDFLAGS		:= -g $(MACHDEP) -Wl,-Map,$(notdir $@).map

LIBS		:= -lfat -lwiiuse -lbte -logc -lm
LIBDIRS		:= $(PORTLIBS)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:= $(CURDIR)/$(TARGET)
export VPATH	:= $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
		   $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR	:= $(CURDIR)/$(BUILD)

CFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:= $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export OFILES	:= $(addsuffix .o,$(BINFILES)) \
		   $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:= $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
		   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
		   -I$(CURDIR)/$(BUILD)
export LIBPATHS	:= $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean run hbc

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

hbc: all
	@mkdir -p $(APPDIR)
	@cp $(TARGET).dol $(APPDIR)/boot.dol
	@echo "Homebrew Channel files are ready in $(APPDIR)"

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).dol $(TARGET).map $(APPDIR)/boot.dol

run: all
	wiiload $(TARGET).dol

else

DEPENDS	:= $(OFILES:.o=.d)

$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

%.bin.o: %.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

endif

