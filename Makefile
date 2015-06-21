#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

TOPDIR ?= $(CURDIR)

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES		:=	source src
DATA		:=
INCLUDES	:=	include

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=
PREFIX	?= $(TOPDIR)/build

GIT_BRANCH	:=	$(shell git rev-parse --abbrev-ref HEAD)
GIT_COMMIT	:=	$(shell git rev-parse --short HEAD)
GIT_BUILD_VERSION	:=	$(GIT_BRANCH)-$(GIT_COMMIT)

CFLAGS	:=	 -DBRANCH_COMMIT=$(GIT_BUILD_VERSION) -DPREFIX=$(PREFIX) -g -Wall \
	-Wno-missing-braces -O2 -fomit-frame-pointer -ffast-math $(ARCH)

CFLAGS	+=	 $(INCLUDE)

CXXFLAGS	:= -std=c++11 $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	= 	-g $(ARCH)

LIBS	:=

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=


LIBHTN_DIR	:= libhtn

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	HOST_THREE	:= gnu
	HOST_TWO	:= linux
endif
ifeq ($(UNAME_S),Darwin)
	HOST_TWO	:= apple
	HOST_THREE	:= darwin
endif
UNAME_P := $(shell uname -p)
#ifeq ($(UNAME_P),x86_64)
#	HOST_CPU := amd64
#endif
ifneq ($(filter %86,$(UNAME_P)),)
	HOST_CPU := i386
endif
ifneq ($(filter arm%,$(UNAME_P)),)
	HOST_CPU := arm
endif
HOST_TRIPLE	:= $(HOST_CPU)-$(HOST_TWO)-$(HOST_THREE)
ifeq ($(HOST_TRIPLE),i386-apple-darwin)
	CXXFLAGS	+= -DDARWIN_HOST=1
endif
DEFAULT_TARGET	?= $(HOST_TRIPLE)
CXXFLAGS	+= -DDEFAULT_TARGET=$(DEFAULT_TARGET)
LIBHTN_MAKE_ARGS	:= TOPDIR=$(TOPDIR) HOST_TRIPLE=$(HOST_TRIPLE) PREFIX=$(PREFIX)

HOST_GNU_AS := $(shell $(PREFIX)/bin/$(HOST_TRIPLE)-as --version 2>/dev/null)

ifdef HOST_GNU_AS
    CXXFLAGS	+= -DUSE_GNU_BINUTILS=1
endif


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)


LIBHTN_AVAILABLE_TOOLCHAINS	:=

HAS_AS	:=	$(shell $(PREFIX)/bin/i386-linux-gnu-as --version 2>/dev/null)
ifdef HAS_AS
	LIBHTN_AVAILABLE_TOOLCHAINS	+=	libhtn-i386-linux-gnu
endif

HAS_AS	:=	$(shell $(PREFIX)/bin/i386-apple-darwin-as --version 2>/dev/null)
ifdef HAS_AS
	LIBHTN_AVAILABLE_TOOLCHAINS	+=	libhtn-i386-apple-darwin
endif

HAS_AS	:=	$(shell $(PREFIX)/bin/arm-vita-eabi-as --version 2>/dev/null)
ifdef HAS_AS
	LIBHTN_AVAILABLE_TOOLCHAINS	+=	libhtn-arm-vita-eabi
endif

.PHONY: $(BUILD) clean all run

all: $(BUILD) libhtn-all
htn	:	$(BUILD)

#---------------------------------------------------------------------------------

libhtn-all: $(LIBHTN_AVAILABLE_TOOLCHAINS)

libhtn-i386-apple-darwin:
	cd $(LIBHTN_DIR)/platform/i386-apple-darwin ; \
	make $(LIBHTN_MAKE_ARGS) COMPILE_TARGET=i386-apple-darwin

libhtn-i386-linux-gnu:
	cd $(LIBHTN_DIR)/platform/i386-linux-gnu ; \
	make $(LIBHTN_MAKE_ARGS) COMPILE_TARGET=i386-linux-gnu

libhtn-arm-vita-eabi:
	cd $(LIBHTN_DIR)/platform/arm-vita-eabi ; \
	make $(LIBHTN_MAKE_ARGS) COMPILE_TARGET=arm-vita-eabi

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET) $(TARGET).elf
	cd libhtn/platform/i386-linux-gnu ; make clean
	cd libhtn/platform/i386-apple-darwin ; make clean
	cd libhtn/platform/arm-vita-eabi ; make clean


run:
	./$(TARGET)

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------

%.o	:	%.c
	$(CC) $(CFLAGS) -c -o $@ $^

%.o	:	%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

$(OUTPUT)	:	$(OFILES)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

%.png.o	:	%.png
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
