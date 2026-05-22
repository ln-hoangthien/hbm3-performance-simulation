//------------------------------------------------------------
// FileName	: CMMU.h
// Version	: 0.86
// Date 	: 25 Apr 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: MMU header
//------------------------------------------------------------
// Terms
// 	(1) PTW	: "Demand-PTW" due to TLB miss
// 	(2) PF	: Prefetch (background PTW) initiated in MMU
//------------------------------------------------------------
#ifndef CMMU_H
#define CMMU_H

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "CFIFO.h"
#include "CTRx.h"
#include "CTracker.h"
#include "Top.h"

#include "./Buddy/CBuddy.h"

using namespace std;

//---------------------------------------------------
// Replacement policy
//---------------------------------------------------
// #define MMU_REPLACEMENT_RANDOM
// #define MMU_REPLACEMENT_FIFO						// Find
// entry allocated earliest. Not implemented yet.
#define MMU_REPLACEMENT_LRU // Find entry used oldest

//---------------------------------------------------
// Page table walk (defined in Top.h)
//---------------------------------------------------
// #define SINGLE_FETCH
// #define BLOCK_FETCH							// Full
// #define HALF_FETCH							// Half
// 1/2 #define QUARTER_FETCH						//
// Quarter 1/4

//---------------------------------------------------
// Page size
//---------------------------------------------------
#define PAGE_SIZE_4KB
// #define PAGE_SIZE_1MB

//---------------------------------------------------
// MMU page size
//---------------------------------------------------
#define BIT_4K_PAGE 12 // Bits. 4K = 2^12
#define BIT_1M_PAGE 20 // Bits. 1M = 2^20

//---------------------------------------------------
// TLB demension 2D array (defined in Top.h)
//---------------------------------------------------
// #define MMU_NUM_WAY 		16					//
// Number of TLB entries (power-of-2) for fully associative #define MMU_NUM_SET
// 32					// 1 for fully associative

//---------------------------------------------------
// Number of PTEs a PTW obtain (defined in Top.h)
//---------------------------------------------------
// #if defined SINGLE_FETCH
//	#define NUM_PTE_PTW	1
// #elif defined BLOCK_FETCH
//	#define NUM_PTE_PTW	16					//
// Power-of-2. Fully associative. Assume 16. FIXME
// #elif defined HALF_FETCH
// 	#define NUM_PTE_PTW	8
// #elif defined QUARTER_FETCH
// 	#define NUM_PTE_PTW	4
// #endif

//---------------------------------------------------
// Number of bits for set
//---------------------------------------------------
#define MMU_BIT_SET ((int)(ceilf(log2f(MMU_NUM_SET)))) // 0 for fully associative
// #define MMU_BIT_SET		0					// DEBUG

//---------------------------------------------------
// Number of bits for tag
//---------------------------------------------------
// #define BIT_TAG		(32 - MMU_BIT_SET - BIT_4K_PAGE)	//
// 20-bits for 4kB page. Arch32 #define BIT_TAG		(64 - MMU_BIT_SET -
// BIT_4K_PAGE)	// 20-bits for 4kB page. Arch64

//---------------------------------------------------
// FLPD cache size
//---------------------------------------------------
#define NUM_FLPD_ENTRY 64 // Duong change from 8 to 64 to support multi-level page table

//---------------------------------------------------
// ARID PTW
//---------------------------------------------------
#define ARID_PTW 2     // Normal demanding PTW
#define ARID_PF_BUF0 0 // DBPF
#define ARID_PF_BUF1 1

//---------------------------------------------------
// ARID APTW. AT
//---------------------------------------------------
#define ARID_APTW 0 // anchor APTW

//---------------------------------------------------
// Translation Table Base Address
//	(1) FIRST_TTBA:  Total 512 tables. Each 4KiB
//	(2) SECOND_TTBA: Total 512 tables. Each 4KiB.
//	(3) THỈD_TTBA:   Total 512 tables. Each 4KiB.
//	(4) FOURTH_TTBA: Total 512 tables. Each 4KiB.
//	Note: Addr is signed int64_t type. MSB is supposed to 0.
//---------------------------------------------------
#define FIRST_TTBA 0x100000000
#define SECOND_TTBA 0x200000000
#define THIRD_TTBA 0x300000000
#define FOURTH_TTBA 0x400000000

//---------------------------------------------------
// RMM
//	Redundant Memory Mapping
//---------------------------------------------------
#define ARID_RPTW_RMM 0               // RMM RPTW
#define START_RT_ADDR_RMM 0x500000000 // Start range table address
#define MAX_RTE_NUM_RMM 16384         // Max number of range entries  FIXME Optimistic for RMM

//---------------------------------------------------
// Contiguity (TLB)
//---------------------------------------------------
#define CONTIGUITY_ENABLE // DUONGTRAN uncomment
// #define CONTIGUITY_DISABLE             // DUONGTRAN comment
//  #define CONTIGUITY_0_PERCENT
//  #define CONTIGUITY_25_PERCENT
//  #define CONTIGUITY_50_PERCENT
//  #define CONTIGUITY_75_PERCENT
//  #define CONTIGUITY_100_PERCENT

//--------------------------------------------
// Page-table format
// 	Contiguity value represent every "PAGE_TABLE_GRANULE" pages
//--------------------------------------------
#define PAGE_TABLE_GRANULE 4 // Contiguity represent every 4 pages
// #define PAGE_TABLE_GRANULE				8		//
// Contiguity represent every 8 pages

//--------------------------------------------
// Max block size
// 	2^(L2_MAX_ORDER)
//--------------------------------------------
#define TOTAL_PAGE_NUM 128 //  2^(L2_MAX_ORDER) 4kB-sized pages.
#define L2_MAX_ORDER 7

// #if defined PAGE_SIZE_4KB
//	#define TOTAL_PAGE_NUM  ( (IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE *
// BYTE_PER_PIXEL) >> 12 )
// #elif defined PAGE_SIZE_1MB
//	#define TOTAL_PAGE_NUM  ( (IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE *
// BYTE_PER_PIXEL) >> 20 )
// #endif

//---------------------------------------------------
// VPN same PPN
// 	If not same, generate PPN randomly
//---------------------------------------------------
// #define VPN_SAME_PPN

//---------------------------------------------------
// Cycles to serve TLB hit (FIFO_AR)
//---------------------------------------------------
#ifdef CONTIGUITY_ENABLE

#ifdef RMM_ENABLE
#define MMU_FIFO_AR_TLB_HIT_LATENCY 3
#else
#define MMU_FIFO_AR_TLB_HIT_LATENCY 2
#endif
#else
#define MMU_FIFO_AR_TLB_HIT_LATENCY 1
#endif

//---------------------------------------------------
// Cycles to serve TLB miss (FIFO_AR)
//---------------------------------------------------
#ifdef CONTIGUITY_ENABLE

#ifdef RMM_ENABLE
#define MMU_FIFO_AR_TLB_MISS_LATENCY 4
#else
#define MMU_FIFO_AR_TLB_MISS_LATENCY 3
#endif
#else
#define MMU_FIFO_AR_TLB_MISS_LATENCY 2
#endif

//---------------------------------------------------
// Cycles to finish PTW (FIFO_AR)
//---------------------------------------------------
#ifdef CONTIGUITY_ENABLE
#define MMU_FIFO_AR_PTW_FINISH_LATENCY 2
#else
#define MMU_FIFO_AR_PTW_FINISH_LATENCY 1
#endif

//---------------------------------------------------
// Cycles to serve 2nd, 3rd level PTW (FIFO_AR)
//---------------------------------------------------
#define MMU_FIFO_AR_2ND_PTW_LATENCY 2
#define MMU_FIFO_AR_3RD_PTW_LATENCY 2
#define MMU_FIFO_AR_4TH_PTW_LATENCY 2

//---------------------------------------------------
// Page size
//---------------------------------------------------
typedef enum {
  EPAGE_SIZE_TYPE_4KB,
  EPAGE_SIZE_TYPE_1MB,
  EPAGE_SIZE_TYPE_2MB, // Duong add 3 new huge pages 2MB, 1GB, and 512GB
  EPAGE_SIZE_TYPE_1GB,
  EPAGE_SIZE_TYPE_512GB,
  EPAGE_SIZE_TYPE_UNDEFINED
} EPageSizeType;

//------------------------------------------------------------------------------
// TLB entry
// 	Store VPN (instead of TAG number).  By doing this, we simplify tag
// calculation.
//------------------------------------------------------------------------------
// 	1. VPN	(1) Traditional	: Requested VPN
// 			(2) PCA		: Requested VPN
// 			(3) BCT		: StartVPN of block
// 			(4) PCAD	: StartVPN of block
// 			(5) RMM		: StartVPN of block
// 			(6) AT		: StartVPN of block (distance)
// 			(7) CAMB	: StartVPN of block
//------------------------------------------------------------------------------
// 	2. PPN	(1) Traditional	: PPN
// 			(2) PCA		: PPN
// 			(3) BCT		: StartPPN of block
// 			(4) PCAD	: StartPPN of block
// 			(5) RMM		: StartPPN of block
// 			(6) AT		: PPN
// 			(7) CAMB	: StartPPN of block
//------------------------------------------------------------------------------
// 	3. Contiguity	(1) Traditional	: 0
// 			(2) PCA		: How many next pages contiguous
// ascending 			(3) BCT		: BlockSize - 1 (pages)
// (4) PCAD	: BlockSize - 1 (pages) 			(5) RMM :
// BlockSize - 1 (pages) 			(6) AT : How many pages
// contiguous ascending 			(7) CAMB	: BlockSize - 1
// (pages)
//------------------------------------------------------------------------------
typedef struct tagSMMUEntry *SPMMUEntry;
typedef struct tagSMMUEntry {
  int nValid;
  // int64_t 		nTag;
  int64_t nVPN; // Start VPN
  int64_t nPPN;
  EPageSizeType ePageSize;

  int nContiguity;
  uint64_t nTimeStamp; // Allocation or access time. Replacement.
  int N_flag;
} SMMUEntry;

//---------------------------------------------------
// MMU
//---------------------------------------------------
typedef class CMMU *CPMMU;
class CMMU {

public:
  // 1. Contructor and Destructor
  CMMU(string cName);
  ~CMMU();

  // 2. Control

  // Handler
  EResultType Do_AR_fwd_SI(int64_t nCycle); // Traditional
  EResultType Do_AR_fwd_MI(int64_t nCycle);
  EResultType Do_AR_bwd(int64_t nCycle);
  EResultType Do_AW_fwd(int64_t nCycle);
  EResultType Do_AW_bwd(int64_t nCycle);
  EResultType Do_R_fwd(int64_t nCycle);
  EResultType Do_R_bwd(int64_t nCycle);
  EResultType Do_W_fwd_SI(int64_t nCycle);
  EResultType Do_W_fwd_MI(int64_t nCycle);
  EResultType Do_W_bwd(int64_t nCycle);
  EResultType Do_B_fwd(int64_t nCycle);
  EResultType Do_B_bwd(int64_t nCycle);

  EResultType Do_AR_fwd_SI_AT(int64_t nCycle); // AT
  EResultType Do_AW_fwd_AT(int64_t nCycle);
  EResultType Do_R_fwd_AT(int64_t nCycle);

  EResultType Do_AR_fwd_MMU_OFF(int64_t nCycle); // MMU OFF
  EResultType Do_AR_bwd_MMU_OFF(int64_t nCycle);
  EResultType Do_AW_fwd_MMU_OFF(int64_t nCycle);
  EResultType Do_AW_bwd_MMU_OFF(int64_t nCycle);
  EResultType Do_R_fwd_MMU_OFF(int64_t nCycle);
  EResultType Do_R_bwd_MMU_OFF(int64_t nCycle);
  EResultType Do_W_fwd_MMU_OFF(int64_t nCycle);
  EResultType Do_W_bwd_MMU_OFF(int64_t nCycle);
  EResultType Do_B_fwd_MMU_OFF(int64_t nCycle);
  EResultType Do_B_bwd_MMU_OFF(int64_t nCycle);

  // Set value
  EResultType Reset();
  EResultType UpdateState(int64_t nCycle);

  EResultType FillTLB_SF(int64_t nVA, int64_t nCycle); // Single fetch
  EResultType FillTLB_BF(int64_t nVA,
                         int64_t nCycle); // Block fetch. In DBPF, "buf0"

  EResultType FillTLB_CAMB(int64_t nVA, int64_t nCycle);  // CAMB
  EResultType FillTLB_RCPT(int64_t nVA, int64_t nCycle);  // RCPT
  EResultType FillTLB_cRCPT(int64_t nVA, int64_t nCycle); // cRCPT

  EResultType FillTLB1_BF_DBPF(int64_t nVA,
                               int64_t nCycle); // Block fetch. In DBPF, "buf1"

  EResultType FillFLPD(int nEntry, int64_t nVPN, int64_t nCycle);
  EResultType FillSLPD(int nEntry, int64_t nVPN, int64_t nCycle);
  EResultType FillTLPD(int nEntry, int64_t nVPN, int64_t nCycle);

  EResultType Set_nAR_START_ADDR(int64_t nVA); // Start VA. RMM
  EResultType Set_nAW_START_ADDR(int64_t nVA);

  EResultType Set_START_VPN(int64_t nVPN); // Start VPN. Buddy page-table

  EResultType Set_PageTable(SPPTE *spPageTable);

  EResultType Increase_MO_AR();
  EResultType Decrease_MO_AR();
  EResultType Increase_MO_AW();
  EResultType Decrease_MO_AW();
  EResultType Increase_MO_Ax(); // AR + AW
  EResultType Decrease_MO_Ax();

  // Get value
  string GetName();
  EResultType IsTLB_Hit(int64_t nVA,
                        int64_t nCycle); // (1) Check hit (2) Update time.
  EResultType IsTLB_Hit(int64_t nVPN);   // Check hit. VPN
  EResultType IsFLPD_Hit(int64_t nVA, int64_t nCycle);
  EResultType IsSLPD_Hit(int64_t nVA, int64_t nCycle);
  EResultType IsTLPD_Hit(int64_t nVA, int64_t nCycle);

  EResultType IsTLB1_Hit_DBPF(int64_t nVA, int64_t nCycle); // "buf1" DBPF
  EResultType IsTLB1_Hit_DBPF(int64_t nVPN);

  EResultType IsVA_Regular_AT(int64_t nVA); // AT. VA original
  EResultType IsVA_Anchor_AT(int64_t nVA);
  EResultType IsTLB_Allocate_Anchor_AT(int64_t nVA);
  EResultType IsTLB_Allocate_AT(int64_t nVA, int64_t nCycle);
  EResultType IsAnchor_Cover_AT(int64_t nVA);
  EResultType IsTLB_Contiguity_Match_AT(int64_t nVA, int64_t nCycle);

  int64_t GetVPNNum(int64_t nVA); // TLB
  int64_t GetTagNum(int64_t nVA);
  int GetSetNum(int64_t nVPN);

  int GetPA_TLB(int64_t nVA);
  int GetPA_TLB1_DBPF(int64_t nVA); // "buf1" DBPF

  int64_t GetVA_Anchor_AT(int64_t nVA); // AT

  int64_t GetFirst_PTWAddr(int64_t nVA); // PTW addr
  int64_t GetSecond_PTWAddr(int64_t nVA);
  int64_t GetThird_PTWAddr(int64_t nVA);
  int64_t GetFourth_PTWAddr(int64_t nVA);

  int GetVictim_TLB(int64_t nVPN);       // Replacement TLB ("buf0" in DBPF)
  int GetVictim_TLB1_DBPF(int64_t nVPN); // Replacement TLB ("buf1" in DBPF)

  int GetVictim_FLPD();

  // Stat
  int GetMO_AR();
  int GetMO_AW();
  int GetMO_Ax();

  float GetTLBHitRate();
  float GetTLB_reach(int64_t nCycle); // TLB reach
  int Get_nAR_SI();
  int Get_nAW_SI();
  int Get_nHit_AR_TLB();
  int Get_nHit_AW_TLB();

  // Control

  // Debug
  EResultType CheckMMU();
  EResultType PrintStat(int64_t nCycle, FILE *fp);
  EResultType CheckLink();
  EResultType Display_TLB();
  // EResultType	Display();

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

  int anchorDistance; // DUONGTRAN add
  int nPTW_total;     // PTW AR total DUONG move to public to print in main ^_^
  int nAPTW_total_AT; // PTW AR total. AT, DUONG move to public to print in main
                      // ^_^

private:
  // Original
  string cName;

  SPMMUEntry spTLB[MMU_NUM_SET][MMU_NUM_WAY];  // TLB memory. "buf0" in DBPF.
  SPMMUEntry spTLB1[MMU_NUM_SET][MMU_NUM_WAY]; // TLB "buf1" in DBPF

  SPMMUEntry spFLPD[NUM_FLPD_ENTRY]; // FLPD memory fully associative

  SPMMUEntry spRTLB_RMM[MMU_NUM_SET][MMU_NUM_WAY]; // RTLB memory. RMM

  CPTracker cpTracker; // MO tracker

  CPFIFO cpFIFO_AR; // Req
  // CPFIFO	cpFIFO_R; // Response CPFIFO	cpFIFO_B;
  CPFIFO cpFIFO_W; // Victim

  SPPTE *spPageTable;

  // Control
  EResultType Is_AR_priority; // Check AR first
  EResultType Is_AW_priority;

  int64_t nAR_START_ADDR; // Start VA. RMM.
  int64_t nAW_START_ADDR;

  int64_t START_VPN; // Start VPN. Buddy page-table.

  // Stat MO SI
  int nMO_Ax; // MO count AR + AW
  int nMO_AR;
  int nMO_AW;

  int nMax_MO_AR; // Max MO SI
  int nMax_MO_AW;
  int nMax_MO_Ax;

  int nTotal_MO_AR; // Avg MO SI
  int nTotal_MO_AW;
  int nTotal_MO_Ax;

  // Stat tracker
  int nMax_MOTracker_Occupancy;
  int nTotal_MOTracker_Occupancy;

  // Stat
  int nAR_SI; // AR slave interface
  int nAR_MI;
  int nAW_SI;
  int nAW_MI;

  int nHit_AR_TLB; // TLB hit AR
  int nHit_AW_TLB;
  int nHit_AR_FLPD; // FLPD hit AR
  int nHit_AW_FLPD;

  // int		nPTW_total;
  // // PTW AR total, DUONG move to public to print in main ^_^
  int nPTW_1st; // PTW AR level-1
  int nPTW_2nd;
  int nPTW_3rd; // Duong add 3rd and 4th
  int nPTW_4th;

  // Stat DBPF
  int nHit_AR_TLB1_DBPF; // TLB hit AR. DBPF
  int nHit_AW_TLB1_DBPF;
  int nHit_TLB_After_Fill_DBPF; // Hit counter after TLB fill
  int nHit_TLB1_After_Fill_DBPF;

  int nPF_total; // PF
  int nPF_1st;
  int nPF_2nd;

  // Stat RMM
  int nHit_AR_RTLB_RMM; // RTLB hit AR. RMM
  int nHit_AW_RTLB_RMM;

  int nRPTW_total_RMM; // PTW AR total. RMM
  int nRPTW_1st_RMM;
  int nRPTW_2nd_RMM;
  int nRPTW_3rd_RMM;

  EResultType IsRPTW_AR_ongoing_RMM; // RPTW ongoing. RMM
  EResultType IsRPTW_AW_ongoing_RMM;

  int nRTLB_evict_RMM; // Evict TLB entry. RMM

  // Stat AT
  // int		nAPTW_total_AT;
  // // PTW AR total. AT, DUONG move to public to print in main ^_^
  int nAPTW_1st_AT;
  int nAPTW_2nd_AT;
  int nAPTW_3rd_AT;
  int nAPTW_4th_AT;

  // Stat
  int nMax_Tracker_Wait_Ax; // Max waiting cycles tracker "all" entries

  int nTotal_Tracker_Wait_Ax; // Accumulate allocation cycle tracker "popped" Ax
                              // entry
  int nTotal_Tracker_Wait_AR;
  int nTotal_Tracker_Wait_AW;

  EResultType IsPTW_AR_ongoing; // PTW for AR ongoing
  EResultType IsPTW_AW_ongoing;

  int nPTW_AR_ongoing_cycles; // Accumulate cycles PTW ongoing
  int nPTW_AW_ongoing_cycles;

  int nAR_stall_cycles_SI; // Rx_AR empty, GetPair busy
  int nAW_stall_cycles_SI;

  int64_t nTotalPages_TLB_capacity; // How many pages TLB can cover
  int64_t AR_min_VPN;               // Min VPN experienced
  int64_t AW_min_VPN;
  int64_t AR_max_VPN;
  int64_t AW_max_VPN;

  // Stat replacement
  int nTLB_evict; // Evict TLB entry
  int nFLPD_evict;
};

#endif
