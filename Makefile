#---------------------------------------------------------------------
# Makefile for VanitySearch
#
# Author : Jean-Luc PONS

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

## Modernized build defaults (Ubuntu 24.04 / CUDA 12+)
## CPU-only build works without CUDA installed.

CXX        ?= g++

## CUDA (only required when building with gpu=1)
CUDA_HOME  ?= /usr/local/cuda
NVCC       ?= $(CUDA_HOME)/bin/nvcc

## CUDA architectures (fatbin). Override with: CUDA_ARCHS="86 90 100"
CUDA_ARCHS ?= 86 90 100

define GEN_GENCODE
 -gencode=arch=compute_$(1),code=sm_$(1)
endef
NVCC_GENCODE := $(foreach a,$(CUDA_ARCHS),$(call GEN_GENCODE,$(a)))

## Optional native tuning (fastest on the build machine). Disable with NATIVE=0
NATIVE ?= 1
ifeq ($(NATIVE),1)
  NATIVE_FLAGS = -march=native -mtune=native
else
  NATIVE_FLAGS =
endif

COMMON_CXXFLAGS = -m64 -mssse3 -Wno-write-strings -I. $(NATIVE_FLAGS)

ifdef gpu
  COMMON_CXXFLAGS += -DWITHGPU
  CUDA_INCLUDES = -I$(CUDA_HOME)/include
  CUDA_LIBS     = -L$(CUDA_HOME)/lib64 -lcudart
else
  CUDA_INCLUDES =
  CUDA_LIBS     =
endif

ifdef debug
  CXXFLAGS = $(COMMON_CXXFLAGS) -g -O0 -DDEBUG $(CUDA_INCLUDES)
else
  CXXFLAGS = $(COMMON_CXXFLAGS) -O3 $(CUDA_INCLUDES)
endif

LFLAGS = -lpthread $(CUDA_LIBS)


#--------------------------------------------------------------------

ifdef gpu
$(OBJDIR)/GPU/GPUEngine.o: GPU/GPUEngine.cu
	@command -v $(NVCC) >/dev/null 2>&1 || { echo "NVCC not found. Install CUDA or set CUDA_HOME."; exit 1; }
	$(NVCC) $(NVCC_GENCODE) -maxrregcount=0 --ptxas-options=-v --compile \
	  --compiler-options -fPIC -m64 $(if $(debug),-G -g,-O3) -I$(CUDA_HOME)/include \
	  -o $(OBJDIR)/GPU/GPUEngine.o -c GPU/GPUEngine.cu
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

