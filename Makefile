# Compiler
COMPILER = nvcc

# Folders
SRCDIR = source
INCDIR = include
OBJDIR = obj

# Compiler flags
GCCFLAGS = -std=c++20 -fopenmp -x c++
ifeq ($(OS),Windows_NT)
	NVCCFLAGS = -std=c++20 -Xcompiler -openmp -lcufft -rdc=true 
else
	NVCCFLAGS = -std=c++20 -Xcompiler -fopenmp -lcufft -rdc=true -diag-suppress 177
endif
SFMLLIBS = -I'external/SFML/include' -L'external/SFML/build/lib/Release'

OPTIMIZATION = -O3

SFML ?= FALSE
TETM ?= FALSE
FP32 ?= FALSE
CPU ?= FALSE

# Object files
ifeq ($(SFML),FALSE)
CPP_SRCS := $(shell find $(SRCDIR) -not -path "*sfml*" -name "*.cpp")
else
CPP_SRCS = $(shell find $(SRCDIR) -name "*.cpp")
endif
CU_SRCS = $(shell find $(SRCDIR) -name "*.cu")

CPP_OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(CPP_SRCS))
CU_OBJS = $(patsubst $(SRCDIR)/%.cu,$(OBJDIR)/%.o,$(CU_SRCS))


ifeq ($(SFML),TRUE)
	ADD_FLAGS = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-main $(SFMLLIBS) -DSFML_RENDER
endif
ifeq ($(TETM),TRUE)
	ADD_FLAGS += -DTETMSPLITTING
endif
ifeq ($(FP32),TRUE)
	ADD_FLAGS += -DUSEFP32
endif
ifeq ($(CPU),TRUE)
	ADD_FLAGS += -DUSECPU
endif
#ADD_FLAGS += -gencode arch=compute_86,code=sm_86 # A100: 80, 4090: 89

# Targets
ifndef TARGET
	ifeq ($(OS),Windows_NT)
		TARGET = main.exe
	else
		TARGET = main.o
	endif
endif

ifeq ($(COMPILER),nvcc)
	COMPILER_FLAGS = $(NVCCFLAGS) $(OPTIMIZATION)
else
	COMPILER_FLAGS = $(GCCFLAGS) $(OPTIMIZATION)
endif

all: $(OBJDIR) $(CPP_OBJS) $(CU_OBJS)
	$(COMPILER) -o $(TARGET) $(CPP_OBJS) $(CU_OBJS) $(COMPILER_FLAGS) -I$(INCDIR) $(ADD_FLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@ -I$(INCDIR) $(ADD_FLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cu
	@mkdir -p $(dir $@)
	$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@ -I$(INCDIR) $(ADD_FLAGS)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

clean:
	@rm -f $(OBJDIR)/*.o $(TARGET)
	@rm -fr $(OBJDIR)