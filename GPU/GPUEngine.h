#ifndef GPUENGINEH
#define GPUENGINEH

#include <vector>
#include "../SECP256k1.h"

#ifdef WITHGPU
#include <cuda_runtime.h>
#endif

#define SEARCH_COMPRESSED 0
#define SEARCH_UNCOMPRESSED 1
#define SEARCH_BOTH 2

static const char *searchModes[] = {"Compressed","Uncompressed","Compressed or Uncompressed"};

// Number of key per thread (must be a multiple of GRP_SIZE) per kernel call
#define STEP_SIZE 1024

// Number of thread per block
// 256 tends to give better occupancy/reg pressure tradeoffs on modern NVIDIA GPUs.
#define NB_TRHEAD_PER_GROUP 256
#define ITEM_SIZE 28
#define ITEM_SIZE32 (ITEM_SIZE/4)
#define _64K 65536

typedef uint16_t prefix_t;
typedef uint32_t prefixl_t;

typedef struct {
  uint32_t thId;
  int16_t  incr;
  int16_t  endo;
  uint8_t  *hash;
  bool mode;
} ITEM;

// Second level lookup
typedef struct {
  prefix_t sPrefix;
  std::vector<prefixl_t> lPrefixes;
} LPREFIX;

class GPUEngine {

public:

  GPUEngine(int nbThreadGroup,int gpuId,uint32_t maxFound,bool rekey); 
  ~GPUEngine();
  void SetPrefix(std::vector<prefix_t> prefixes);
  void SetPrefix(std::vector<LPREFIX> prefixes,uint32_t totalPrefix);
  bool SetKeys(Point *p);
  void SetSearchMode(int searchMode);
  void SetSearchType(int searchType);
  void SetPattern(const char *pattern);
  bool Launch(std::vector<ITEM> &prefixFound,bool spinWait=false);
  int GetNbThread();
  int GetGroupSize();

  bool Check(Secp256K1 *secp);
  std::string deviceName;

  static void PrintCudaInfo();
  static void GenerateCode(Secp256K1 *secp, int size);

private:

  bool callKernel();
  static void ComputeIndex(std::vector<int> &s, int depth, int n);
  static void Browse(FILE *f,int depth, int max, int s);
  bool CheckHash(uint8_t *h, std::vector<ITEM>& found, int tid, int incr, int endo, int *ok);

  int nbThread;
  prefix_t *inputPrefix;
  prefix_t *inputPrefixPinned;
  uint32_t *inputPrefixLookUp;
  uint32_t *inputPrefixLookUpPinned;
  uint64_t *inputKey;
  uint64_t *inputKeyPinned;
  uint32_t *outputPrefix;
  uint32_t *outputPrefixPinned;
  bool initialised;
  uint32_t searchMode;
  uint32_t searchType;
  bool littleEndian;
  bool lostWarning;
  bool rekey;
  uint32_t maxFound;
  uint32_t outputSize;
  std::string pattern;
  bool hasPattern;

#ifdef WITHGPU
  cudaStream_t stream;
  cudaEvent_t  evt;
#endif

};

#endif // GPUENGINEH
