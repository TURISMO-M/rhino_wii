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

export CFLAGS	:=	-g -O2 -Wall -mrvl -mcpu=750 -meabi -mhard-float -DGEKKO -I$(LIBOGC_INC) -I$(DEVKITPRO)/portlibs/wii/include -I$(DEVKITPRO)/portlibs/wii/include/freetype2
export CXXFLAGS	:=	$(CFLAGS)
export LDFLAGS	:=	-g -mrvl -mcpu=750 -meabi -mhard-float -L$(LIBOGC_LIB) -L$(DEVKITPRO)/portlibs/wii/lib -lgrrlib -lpngu $(shell powerpc-eabi-pkg-config --libs freetype2 libpng libjpeg zlib) -lwiiuse -lbte -logc -lm

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
	@cp $< $(notdir $<)
	@$(RAW2C) $(notdir $<)
	@$(CC) $(CFLAGS) -I. -c $(basename $(notdir $<)).c -o $@

endif
