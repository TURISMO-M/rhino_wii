TARGET		:=	boot
SOURCES		:=	source
BUILD		:=	build
GRAPHICS	:=	assets

ifneq ($(BUILD),$(notdir $(CURDIR)))

export TOPDIR	:=	$(CURDIR)
export TARGET	:=	$(TARGET)
export LIBOGC_BASE	:=	$(DEVKITPRO)/libogc
export LIBOGC_INC	:=	$(LIBOGC_BASE)/include
export LIBOGC_LIB	:=	$(LIBOGC_BASE)/lib/wii

export PATH	:=	$(DEVKITPPC)/bin:$(DEVKITPRO)/tools/bin:$(PATH)

export CC	:=	powerpc-eabi-gcc
export CXX	:=	powerpc-eabi-g++
export LD	:=	powerpc-eabi-g++
export ELF2DOL	:=	elf2dol
export RAW2C	:=	raw2c

export CFLAGS	:=	-g -O2 -Wall -mrvl -mcpu=750 -meabi -mhard-float -DGEKKO -I$(LIBOGC_INC)
export CXXFLAGS	:=	$(CFLAGS)
export LDFLAGS	:=	-g -mrvl -mcpu=750 -meabi -mhard-float -L$(LIBOGC_LIB) -lwiiuse -lbte -logc -lm

CPPFILES	:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp))
CFILES		:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
PNGFILES	:=	$(foreach dir,$(GRAPHICS),$(wildcard $(dir)/*.png))

export OFILES	:=	$(notdir $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(PNGFILES:.png=.o))

.PHONY: clean all

all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).dol

else

VPATH	:=	$(foreach dir,$(SOURCES),$(TOPDIR)/$(dir)) $(foreach dir,$(GRAPHICS),$(TOPDIR)/$(dir))

all: $(TOPDIR)/$(TARGET).dol

$(TOPDIR)/$(TARGET).dol: $(TOPDIR)/$(TARGET).elf
	@echo "Creating DOL: $@"
	@$(ELF2DOL) $< $@

$(TOPDIR)/$(TARGET).elf: $(OFILES)
	@echo "Linking ELF: $@"
	@$(LD) $^ $(LDFLAGS) -o $@

%.o: %.cpp
	@echo "Compiling C++: $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.png
	@echo "Converting PNG: $<"
	@$(RAW2C) $<
	@$(CC) $(CFLAGS) -c $(basename $(notdir $<))_png.c -o $@
	@rm -f $(basename $(notdir $<))_png.c $(basename $(notdir $<))_png.h

endif
