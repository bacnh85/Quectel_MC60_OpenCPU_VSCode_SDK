#-------------------------------------------------------------------------------
# Configure GCC installation path, and GCC version.
# To execute "arm-none-eabi-gcc -v" in command line can get the current gcc version 
#-------------------------------------------------------------------------------
GCC_INSTALL_PATH=C:\Program Files (x86)\CodeSourcery\Sourcery_CodeBench_Lite_for_ARM_EABI
GCC_VERSION=4.7.2

C_PREDEF=-D __CUSTOMER_CODE__

#-------------------------------------------------------------------------------
# Configure version and out target
#-------------------------------------------------------------------------------
PLATFORM = APPGS3MD
MEMORY   = M32
VERSION  = A01
TARGET   = $(strip $(PLATFORM))$(strip $(MEMORY))$(strip $(VERSION))

#-------------------------------------------------------------------------------
# Configure the include directories
#-------------------------------------------------------------------------------
INCS =  -I $(ENV_INC) 
INCS += -I ./           \
        -I SDK/include      \
        -I SDK/ril/inc      \
        -I src/config   \
        -I src/fota/inc \

#-------------------------------------------------------------------------------
# Configure source code dirctories
#-------------------------------------------------------------------------------
SRC_DIRS=src\     \
		 src\config     \
     src\fota\src   \
		 SDK\ril\src    \

#-------------------------------------------------------------------------------
# Configure source code files to compile in the source code directories
#-------------------------------------------------------------------------------
# custom source
SRC_CUS=$(wildcard src/*.c)
SRC_SYS=$(wildcard src/config/*.c)
SRC_FOTA=$(wildcard src/fota/src/*.c)
SRC_SYS_RIL=$(wildcard SDK/ril/src/*.c)

OBJS=\
	 $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC_SYS))        \
	 $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC_SYS_RIL))    \
	 $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC_CUS))        \
	 $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC_FOTA))      \

#-------------------------------------------------------------------------------
# Configure user reference library
#-------------------------------------------------------------------------------
USERLIB=SDK/libs/gcc/app_start.lib

#-------------------------------------------------------------------------------
# Configure environment path
#-------------------------------------------------------------------------------
BIN_DIR=build
OBJ_DIR=$(BIN_DIR)\obj
BUILDLOG=$(BIN_DIR)/build.log
ENV_PATH=$(strip $(GCC_INSTALL_PATH))/bin
ENV_INC='$(strip $(GCC_INSTALL_PATH))/arm-none-eabi/include'
ENV_LIB_EABI='$(strip $(GCC_INSTALL_PATH))/arm-none-eabi/lib/thumb'
ENV_LIB_GCC='$(strip $(GCC_INSTALL_PATH))/lib/gcc/arm-none-eabi/$(GCC_VERSION)/thumb'

#-------------------------------------------------------------------------------
# Configure compiling utilities
#-------------------------------------------------------------------------------
CC='$(ENV_PATH)/arm-none-eabi-gcc.exe'
LD='$(ENV_PATH)/arm-none-eabi-ld.exe'
OBJCOPY='$(ENV_PATH)/arm-none-eabi-objcopy.exe'
RM='$(ENV_PATH)/cs-rm.exe'
MAKE=make.exe
HEADGEN=SDK/make/GFH_Generator.exe
#-------------------------------------------------------------------------------
# Configure standard reference library
#-------------------------------------------------------------------------------
STDLIB=$(ENV_LIB_EABI)/libm.a $(ENV_LIB_EABI)/libc.a $(ENV_LIB_EABI)/libcs3.a $(ENV_LIB_GCC)/libgcc.a

#-------------------------------------------------------------------------------
# Configure compiling options
#-------------------------------------------------------------------------------
SFLAGS=-c -mlong-calls -march=armv5te -mlittle-endian -mthumb-interwork -mfpu=vfp -mfloat-abi=soft -Wall -Wstrict-prototypes -Os
CFLAGS=-c -mlong-calls -march=armv5te -mlittle-endian -mthumb-interwork -mfpu=vfp -mfloat-abi=soft -Wall -Wstrict-prototypes -std=c99 -Os \
       -ffunction-sections -pipe -ftracer -fivopts

C_DEF=-D MT6252 -D __OCPU_COMPILER_GCC__
LDFLAGS=-Rbuild -X --gc-sections -T SDK/libs/gcc/linkscript.ld -nostartfiles
OBJCOPYFLAGS=

.PHONY: all
all: new

#-------------------------------------------------------------------------------
# Definition for compiling procedure
#-------------------------------------------------------------------------------
new: CreateDir $(BIN_DIR)/$(TARGET).bin
	$(HEADGEN) $(BIN_DIR)/$(TARGET).bin
	@if not exist $(BIN_DIR)/app_image_bin.cfg (copy SDK\libs\app_image_bin.cfg $(BIN_DIR)/app_image_bin.cfg)

$(BIN_DIR)/$(TARGET).bin: $(BIN_DIR)/$(TARGET).elf
	@$(OBJCOPY) $(OBJCOPYFLAGS) -O binary $< $@
	@echo ----------------------------------------------------
	@echo - GCC Compiling Finished Sucessfully.
	@echo - The target image is in the '$(BIN_DIR)' directory.
	@echo ----------------------------------------------------

$(BIN_DIR)/$(TARGET).elf: $(OBJS)
	$(LD) $(LDFLAGS) -Map $(BIN_DIR)/$(TARGET).map -o $@ $(OBJS) $(USERLIB) $(STDLIB)

$(OBJ_DIR)/%.o: %.S
	@echo - Building  $@ 
	$(CC) $(C_DEF) $(SFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: %.c
#	$(warning <-- Start to CC, C_PREDEF=$(C_PREDEF) -->)
	@echo - Building  $@ 
	$(CC) $(C_DEF) $(C_PREDEF) $(CFLAGS) $(INCS) -o $@ $<

CreateDir:
	@$(RM) -f $(BIN_DIR)/$(TARGET).bin
	@if not exist $(BIN_DIR) (md $(BIN_DIR))
	@if not exist $(OBJ_DIR) (md $(OBJ_DIR))
	@for /d %%y in ($(SRC_DIRS)) do \
		@if not exist $(OBJ_DIR)/%%y ( \
			(@echo creating directory $(OBJ_DIR)\%%y) & \
			(md $ $(OBJ_DIR)\%%y))

clean:
	$(RM) -f $(OBJS) $(BUILDLOG) \
	    $(BIN_DIR)/$(TARGET).map \
		$(BIN_DIR)/$(TARGET).bin \
		$(BIN_DIR)/$(TARGET).elf
	rmdir /s /q $(BIN_DIR)

	@echo -------------------
	@echo clean finished.
	@echo -------------------

.PHONY: all clean CreateDir