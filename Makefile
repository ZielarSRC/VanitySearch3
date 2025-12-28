SRC = Base58.cpp IntGroup.cpp main.cpp Random.cpp \
      Timer.cpp Int.cpp IntMod.cpp Point.cpp SECP256K1.cpp \
      Vanity.cpp GPU/GPUGenerate.cpp hash/ripemd160.cpp \
      hash/sha256.cpp hash/sha512.cpp hash/ripemd160_sse.cpp \
      hash/sha256_sse.cpp Bech32.cpp Wildcard.cpp

OBJDIR = obj

ifdef gpu

OBJET = $(addprefix $(OBJDIR)/, \
        Base58.o IntGroup.o main.o Random.o Timer.o Int.o \
        IntMod.o Point.o SECP256K1.o Vanity.o GPU/GPUGenerate.o \
        hash/ripemd160.o hash/sha256.o hash/sha512.o \
        hash/ripemd160_sse.o hash/sha256_sse.o \
        GPU/GPUEngine.o Bech32.o Wildcard.o)

else

OBJET = $(addprefix $(OBJDIR)/, \
        Base58.o IntGroup.o main.o Random.o Timer.o Int.o \
        IntMod.o Point.o SECP256K1.o Vanity.o GPU/GPUGenerate.o \
        hash/ripemd160.o hash/sha256.o hash/sha512.o \
        hash/ripemd160_sse.o hash/sha256_sse.o Bech32.o Wildcard.o)

endif

CXX        ?= g++
# CUDA path only needed for GPU build (gpu=1)
CUDA       ?= /usr/local/cuda
CXXCUDA    ?= $(CXX)
NVCC       ?= $(CUDA)/bin/nvcc

# GPU arch list for fatbins.
# Default targets a wide range of devices and includes Blackwell for AWS p6-b200.*.
# Override if you want a smaller binary / faster compile, e.g.:
#   make gpu=1 GPU_ARCHS="90 100"
GPU_ARCHS  ?= 100

# Build-time toggles:
#   native=1  -> enable -march=native on the host compiler
#   lto=1     -> enable LTO on the host linker/compiler
native      ?= 1
lto         ?= 0

# Common host flags
HOST_OPT    = -O3 -DNDEBUG -fno-strict-aliasing -fomit-frame-pointer
HOST_WARN   = -Wno-write-strings
HOST_STD    = -std=c++14
HOST_CPU    = -m64 -mssse3

ifeq ($(native),1)
HOST_CPU   += -march=native -mtune=native
endif

ifeq ($(lto),1)
HOST_OPT   += -flto
LTO_LFLAGS  = -flto
endif

ifdef gpu
ifdef debug
CXXFLAGS   = -DWITHGPU $(HOST_CPU) $(HOST_WARN) -g $(HOST_STD) -fno-strict-aliasing -I. -I$(CUDA)/include
else
CXXFLAGS   = -DWITHGPU $(HOST_CPU) $(HOST_WARN) $(HOST_OPT) $(HOST_STD) -I. -I$(CUDA)/include
endif
LFLAGS     = -lpthread -L$(CUDA)/lib64 -lcudart $(LTO_LFLAGS)
else
ifdef debug
CXXFLAGS   = $(HOST_CPU) $(HOST_WARN) -g $(HOST_STD) -fno-strict-aliasing -I.
else
CXXFLAGS   = $(HOST_CPU) $(HOST_WARN) $(HOST_OPT) $(HOST_STD) -I.
endif
LFLAGS     = -lpthread $(LTO_LFLAGS)
endif


#--------------------------------------------------------------------

ifdef gpu
ifdef debug
$(OBJDIR)/GPU/GPUEngine.o: GPU/GPUEngine.cu
	$(NVCC) -DWITHGPU -G -maxrregcount=0 --ptxas-options=-v --compile \
		--compiler-options "-fPIC" -ccbin $(CXXCUDA) -m64 -g -I$(CUDA)/include \
		$(foreach arch,$(GPU_ARCHS),-gencode=arch=compute_$(arch),code=sm_$(arch)) \
		-gencode=arch=compute_$(lastword $(GPU_ARCHS)),code=compute_$(lastword $(GPU_ARCHS)) \
		-o $(OBJDIR)/GPU/GPUEngine.o -c GPU/GPUEngine.cu
else
$(OBJDIR)/GPU/GPUEngine.o: GPU/GPUEngine.cu
	$(NVCC) -DWITHGPU -maxrregcount=0 --use_fast_math \
		-Xptxas -O3,-v --compile \
		--compiler-options "-fPIC" -ccbin $(CXXCUDA) -m64 -O3 -I$(CUDA)/include \
		$(foreach arch,$(GPU_ARCHS),-gencode=arch=compute_$(arch),code=sm_$(arch)) \
		-gencode=arch=compute_$(lastword $(GPU_ARCHS)),code=compute_$(lastword $(GPU_ARCHS)) \
		-o $(OBJDIR)/GPU/GPUEngine.o -c GPU/GPUEngine.cu
endif
endif

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

all: VanitySearch

VanitySearch: $(OBJET)
	@echo Making VanitySearch...
	$(CXX) $(OBJET) $(LFLAGS) -o VanitySearch

$(OBJET): | $(OBJDIR) $(OBJDIR)/GPU $(OBJDIR)/hash

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/GPU: $(OBJDIR)
	cd $(OBJDIR) &&	mkdir -p GPU

$(OBJDIR)/hash: $(OBJDIR)
	cd $(OBJDIR) &&	mkdir -p hash

clean:
	@echo Cleaning...
	@rm -f obj/*.o
	@rm -f obj/GPU/*.o
	@rm -f obj/hash/*.o

