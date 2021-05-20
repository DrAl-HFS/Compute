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
HACKS:= -DOPENCL_LIB_200 # Fix for JetsonNano/Ubuntu library version issue (deprecation warning)
LIBS:= -lOpenCL -lm -lstdc++

all : $(TARGET) $(MAKEFILE)

$(TARGET) : $(SRC) $(HDR) $(MAKEFILE)
	$(CC) $(OPT) $(SRC) $(DEFINES) $(HACKS) $(LIBS) -o $@

run : $(TARGET)
	./$(TARGET)

clean :
	rm $(TARGET)
