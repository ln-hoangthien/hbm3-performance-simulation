//------------------------------------------------------------
// FileName	: CCache.h
// Version	: 0.73
// DATE 	: 1 Nov 2021
// Contact	: JaeYoung.Hur@gmail.com
// Description	: cache header
//------------------------------------------------------------
#ifndef CCACHE_H
#define CCACHE_H

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "./Buddy/CBuddy.h"
#include "CFIFO.h"
#include "CMMU.h"
#include "CTRx.h"
#include "CTracker.h"
#include "Top.h"

using namespace std;

//------------------------------------------------------------
// Cache OFF (defined in Top.h)
//------------------------------------------------------------
// #define Cache_OFF
// #define Cache_ON

//------------------------------------------------------------
// Replacement policy
//------------------------------------------------------------
#define CACHE_REPLACEMENT_LRU
// #define CACHE_REPLACEMENT_RANDOM
// #define CACHE_REPLACEMENT_FIFO
// // not yet support

//------------------------------------------------------------
// Cache demension 2D array (defined in Top.h)
//------------------------------------------------------------
// #define CACHE_NUM_WAY 	2
// #define CACHE_NUM_SET 	1

//------------------------------------------------------------
// Number of set bits (defined in Top.h)
//------------------------------------------------------------
// #define CACHE_BIT_SET	((int)(ceilf(log2f(CACHE_NUM_SET))))

//------------------------------------------------------------
// Cache line 64 byte (defined in Top.h)
//------------------------------------------------------------
// #define CACHE_BIT_LINE	6

//------------------------------------------------------------
// Number of tag bits (defined in Top.h)
//------------------------------------------------------------
// // #define CACHE_BIT_TAG	(32 - CACHE_BIT_SET - CACHE_BIT_LINE)
// // Arch32 #define CACHE_BIT_TAG	(64 - CACHE_BIT_SET - CACHE_BIT_LINE)
// // Arch64

//------------------------------------------------------------
// AxID line fill
//------------------------------------------------------------
#define ARID_LINEFILL 1

//------------------------------------------------------------
// AxID evict
//------------------------------------------------------------
#define AWID_EVICT 1

//------------------------------------
// FIFO R, B latency
//------------------------------------
#define CACHE_FIFO_R_LATENCY 3
#define CACHE_FIFO_B_LATENCY 3
#define CACHE_FIFO_AR_LATENCY 3
#define CACHE_FIFO_AW_LATENCY 3
#define CACHE_FIFO_W_LATENCY 3

//------------------------------------------------------------
// Cache line
//------------------------------------------------------------
typedef struct tagSCacheLine *SPCacheLine;
typedef struct tagSCacheLine {
  int nValid;
  int nTag;
  int nData;
  SPTE *spPageTableEntry; // DUONGTRAN add this field to support TLB in CacheL3
  int nDirty;
  uint64_t nTimeStamp; // DUONGTRAN modify from int to uint64_t due to assert
                       // occur when run with large matrix size (from 512x512)
} SCacheLine;

typedef class CCache *CPCache;
class CCache {

public:
  // 1. Contructor and Destructor
  CCache(string cName);
  ~CCache();

  // 2. Control

  // Handler
  EResultType Do_AR_fwd_SI(int64_t nCycle); // Slave interface
  EResultType Do_AR_fwd_MI(int64_t nCycle); // Master interface
  EResultType Do_AR_bwd(int64_t nCycle);
  EResultType Do_AW_fwd_SI(int64_t nCycle);
  EResultType Do_AW_fwd_MI(int64_t nCycle);
  EResultType Do_AW_bwd(int64_t nCycle);
  EResultType Do_R_fwd_SI(int64_t nCycle);
  EResultType Do_R_fwd_MI(int64_t nCycle);
  EResultType Do_R_bwd_SI(int64_t nCycle);
  EResultType Do_R_bwd_MI(int64_t nCycle);
  EResultType Do_W_fwd(int64_t nCycle);
  EResultType Do_W_bwd(int64_t nCycle);
  EResultType Do_B_fwd_SI(int64_t nCycle);
  EResultType Do_B_fwd_MI(int64_t nCycle);
  EResultType Do_B_bwd_SI(int64_t nCycle);
  EResultType Do_B_bwd_MI(int64_t nCycle);

  EResultType Do_AR_fwd_Cache_OFF(int64_t nCycle); // Cache OFF
  EResultType Do_AR_bwd_Cache_OFF(int64_t nCycle);
  EResultType Do_AW_fwd_Cache_OFF(int64_t nCycle);
  EResultType Do_AW_bwd_Cache_OFF(int64_t nCycle);
  EResultType Do_R_fwd_Cache_OFF(int64_t nCycle);
  EResultType Do_R_bwd_Cache_OFF(int64_t nCycle);
  EResultType Do_W_fwd_Cache_OFF(int64_t nCycle);
  EResultType Do_W_bwd_Cache_OFF(int64_t nCycle);
  EResultType Do_B_fwd_Cache_OFF(int64_t nCycle);
  EResultType Do_B_bwd_Cache_OFF(int64_t nCycle);

  // Set value
  EResultType Reset();
  EResultType UpdateState(int64_t nCycle);

  EResultType FillLine(int64_t nCycle, int nSet, int nWay, int64_t nTag);
  EResultType FillLine(int64_t nCycle, int nSet, int nWay, int64_t nTag, SPTE *spPageTableEntry);

  EResultType Increase_MO_AR();
  EResultType Decrease_MO_AR();
  EResultType Increase_MO_AW();
  EResultType Decrease_MO_AW();
  EResultType Increase_MO_Ax(); // AR + AW
  EResultType Decrease_MO_Ax();

  EResultType Set_PageTable(SPPTE *spPageTable);

  // Get value
  string GetName();
  EResultType IsHit(int64_t nCycle, int64_t nAddr);       // Lookup
  EResultType Hit_SetTime(int64_t nCycle, int64_t nAddr); // LRU
  EResultType Hit_SetDirtyTime(int64_t nCycle, int64_t nAddr, EUDType eUDType);
  EResultType IsDirty(int nSet, int nWay);
  int64_t GetTagNum(int64_t nAddr);
  int GetSetNum(int64_t nAddr);
  int64_t GetAddrDirty(int nSet, int nWay);

  int GetMO_AR();
  int GetMO_AW();
  int GetMO_Ax();

  int GetWayNum(int64_t nAddr); // Replacement
  int GetWayNum_FIFO(int64_t nAddr);
  int GetWayNum_LRU(int64_t nAddr);
  int GetWayNum_Random(int64_t nAddr);

  // Control

  // Debug
  EResultType CheckCache();
  EResultType CheckLink();
  EResultType Display();
  EResultType PrintStat(int64_t nCycle, FILE *fp);

  // Port Slave interface
  CPTRx cpRx_AR;
  CPTRx cpTx_R;
  CPTRx cpRx_AW;
  CPTRx cpRx_W;
  CPTRx cpTx_B;

  // Port Master interface
  CPTRx cpTx_AR;
  CPTRx cpRx_R;
  CPTRx cpTx_AW;
  CPTRx cpTx_W;
  CPTRx cpRx_B;

private:
  // Original info
  string cName;

  SPCacheLine spCache[CACHE_NUM_SET][CACHE_NUM_WAY]; // Cache memory

  CPTracker cpTracker;

  CPFIFO cpFIFO_AR; // Line-fill
  CPFIFO cpFIFO_AW; // Evict
  CPFIFO cpFIFO_R;  // Response generated by cache
  CPFIFO cpFIFO_B;
  CPFIFO cpFIFO_W; // Victim generated by cache

  // Control info
  int nMO_Ax; // MO count AR + AW
  int nMO_AR;
  int nMO_AW;

  EResultType Is_AR_priority; // AR, AW arbiter
  EResultType Is_AW_priority;

  // Stat
  int nMax_MOTracker_Occupancy; // Occupancy
  int nTotal_MOTracker_Occupancy;

  int nAR_SI; // Number of AR
  int nAR_MI;
  int nAW_SI;
  int nAW_MI;
  int nHit_AR; // Number of cache-hit
  int nHit_AW;
  int nLineFill; // Number of line-fill AR transactions
  int nEvict;    // Number of evicts transactions

  SPPTE *spPageTable;
};

#endif
