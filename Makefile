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

# Default compute capability for GPU build.
# AWS p6-b200.* targets Blackwell (sm_100). Override if needed, e.g.: make gpu=1 ccap=90
ccap       ?= 100

ifdef gpu
ifdef debug
CXXFLAGS   = -DWITHGPU -m64 -mssse3 -Wno-write-strings -g -std=c++14 -fno-strict-aliasing -I. -I$(CUDA)/include
else
CXXFLAGS   = -DWITHGPU -m64 -mssse3 -Wno-write-strings -O3 -std=c++14 -fno-strict-aliasing -I. -I$(CUDA)/include
endif
LFLAGS     = -lpthread -L$(CUDA)/lib64 -lcudart
else
ifdef debug
CXXFLAGS   = -m64 -mssse3 -Wno-write-strings -g -std=c++14 -fno-strict-aliasing -I.
else
CXXFLAGS   = -m64 -mssse3 -Wno-write-strings -O3 -std=c++14 -fno-strict-aliasing -I.
endif
LFLAGS     = -lpthread
endif


#--------------------------------------------------------------------

ifdef gpu
ifdef debug
$(OBJDIR)/GPU/GPUEngine.o: GPU/GPUEngine.cu
	$(NVCC) -DWITHGPU -G -maxrregcount=0 --ptxas-options=-v --compile --compiler-options -fPIC -ccbin $(CXXCUDA) -m64 -g -I$(CUDA)/include \
		-gencode=arch=compute_$(ccap),code=sm_$(ccap) -gencode=arch=compute_$(ccap),code=compute_$(ccap) \
		-o $(OBJDIR)/GPU/GPUEngine.o -c GPU/GPUEngine.cu
else
$(OBJDIR)/GPU/GPUEngine.o: GPU/GPUEngine.cu
	$(NVCC) -DWITHGPU -maxrregcount=0 --ptxas-options=-v --compile --compiler-options -fPIC -ccbin $(CXXCUDA) -m64 -O3 -I$(CUDA)/include \
		-gencode=arch=compute_$(ccap),code=sm_$(ccap) -gencode=arch=compute_$(ccap),code=compute_$(ccap) \
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

