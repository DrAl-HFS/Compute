# Makefile : OpenCL test builder, parameterised by commandline definition of variable <TNUM>
# https://github.com/DrAl-HFS/Compute.git
# Licence: AGPL3
# (c) Project Contributors May 2021

# define default parameter value
TNUM?=1

CC:= clang
#OPT:= -Os # Min size (often faster on systems with limited cache)
OPT:= -g # Debug


TARGET:= ocl$(TNUM)
MAKEFILE:=Makefile

SRC_DIR:= .
COM_DIR:= $(SRC_DIR)/Common

SRC:= $(SRC_DIR)/$(TARGET).cpp
HDR:=  $(shell ls $(COM_DIR)/*.hpp)
OCL_VER:= 120

DEFINES:= -DCL_TARGET_OPENCL_VERSION=$(OCL_VER)
IDENT:= $(shell cat /etc/issue)
ifneq (,$(findstring Ubuntu,$(IDENT)))
   HACKS:= -DOPENCL_LIB_200 # Fix for JetsonNano/Ubuntu library version issue (deprecation warning)
endif
#HACKS?=
LIBS:= -lOpenCL -lm -lstdc++

$(TARGET) : $(SRC) $(HDR) $(MAKEFILE)
	$(CC) $(OPT) $(SRC) $(DEFINES) $(HACKS) $(LIBS) -o $@


.PHONY: all run clean rgb

all : $(TARGET) $(MAKEFILE)

run : $(TARGET)
	./$(TARGET)

clean :
	rm $(TARGET)

rgb :
	convert -size 256x256 -depth 8 RGB:img.raw img.png
	display img.png
