CC:= clang
#OPT:= -Os # Min size (often faster on systems with limited cache)
OPT:= -g # Debug

TARGET:= ocl1
MAKEFILE:=Makefile

SRC:= $(TARGET).cpp
HDR:=  $(shell ls *.hpp)
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
