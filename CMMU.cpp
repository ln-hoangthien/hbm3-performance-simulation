//---------------------------------------------------------------------------------------------------------------------------------------------------------
// FileName	: CMMU.cpp
// Version	: 0.861
// Date		: 25 Apr 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: MMU class definition
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// Spec
//	1. Arch				: ARM v7 MMU 32-bit architecture
//	2. Page table walk		: Single fetch, Block fetch
//	3. Address			: VA 64-bit. PA 32-bit
//	3. Page size                 	: 4kB
//	4. AR, AW arbitration        	: Round-robin. AR and AW shared.
//	5. Multiple outstanding count	: 16 (in SI)
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// Flow control
//	1. Requests are served in order.
//	2. "Demand-PTW" support
//	   One "demand-PTW" oustanding allowed.
//	   If any "demand-PTW" on going, stall "Normal-Ax". 	FIXME Be careful
//	   "Demand-PTW" is PTW for "normal-Ax" initated by master.
//	3. Prefetch support
//	   One PF outstanding allowed.
//
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// Flow control (Ver 0.85)
//	1. Requests are served in order.
//	2. If on-going any PTW, stall demaind-Ax (to simplify implementation)
//	   Note: We need care "AR + AW" mixed cases.
//	   	(1) "1-st level PTW" for AR is on going. AW is miss. PTW for AW
// is 2nd-level. We should care ordering. 	   	    "PTW-for-AR" and
// "PTW-for-AW" can have different ID. Then it is complex. FIXME
//	3. No support PF
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// ID outstanding policy
//	1. Hit-under-hit		: Allow
//	2. Hit-under-miss		: Not allow. If same ID was miss, stall
// Ax.
//	3. Miss-under-hit 		: Allow
//	4. Miss-under-miss		: Allow. Currently not support FIXME Be
// careful order of 1st-level and 2nd-level PTW. Check ID-head
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// Tracker push info
//	1. Hit				: Original. VA
//	2. PTW				: Original. VA
//	3. RPTW				: Original. VA aligned 16kB (four
// pages). Four because contiguity evenly distributed every 4 pages. FIXME
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// MMU arch options
//	1. Traditional			:
//	2. Traditional + DBPF		: Double-buffer prefetch 	(US16.
// Hong)
//	3. PCA				: Page Contiguity ascending 	(TVLSI19
// paper. Hur)
//	4. BCT				: Buddy block contiguity 	(IEICE19
// paper. Hur)
//	5. PCAD 			: PCA + descending 		(TVLSI19
// paper. Hur)
//	6. RMM				: Redundant Memory Mapping 	(ISCA15
// paper. Karakostas)
//	7. AT				: Anchor translation 		(ISCA17
// paper. Park)
//	8. CAMB				: Page table compaction 	(Hur)
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// AxID (2-bits)
//	1. PTW (Demanding)		: ARID 2
//	2. PF 				: ARID 0 (Buf0), 1 (Buf1)
//	2. RPTW (RMM)			: ARID 0
//	3. APTW	(AT)			: ARID 0. Issue APTW earlier than PTW.
//	4. Normal			: Shift left 2 (request), Shift right 2
//(response)
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// Prefetch
// 	1. DBPF				: Double-buffer. "buf0" (TLB) and
// "buf1"(TLB1). 					  Buffer size =
// "NUM_PTE_PTW" 					  Background (ping-pong)
// prefetch 					  (1) On TLB-miss, initiate
// block-fetch (for "buf0") 					  (2) On TLB-hit
// "first" time (in "one buf"), initiate next "block-prefetch" (for "the other
// buf"). 					      Ascending or descending is configurable.
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// Page table format : "PAGE_TABLE_GRANULE" 4
// 	Contiguity representation page table. TVLSI19. Block size any.
//	Contiguity evenly distributed in page table.
//	"-" indicates no llocation to TLB
//
//	   VPN			BlockSize	0  1  2  3   |	  4  5  6  7  |
// 8
//	1. Contiguity	  0%	   1		0  0  0  0   |	  0  0  0  0  |
// 0 ... --> (0),   (1),   (2), ...
//	2. Contiguity	 25%	   2		1  -  0  0   |	  1  -  0  0  |
// 1 ... --> (0,1), (4,5), (8, 9), ...
//	3. Contiguity	 50%	   3		2  -  -  0   |	  2  -  -  0  |
// 2 ... --> (0-2), (4-6), (8-10), (12-14), ...
//	4. Contiguity	 75%	   4		3  -  -  -   |	  3  -  -  -
// 3 ... --> (0-3), (4-7), (8-11), (12-15), (16-19), (20-23), ...
//	5. Contiguity	100%			MAX ...
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// Page table format : "PAGE_TABLE_8_GRANULE" 8
// 	Contiguity representation page table.
//
//	   VPN			BlockSize	0  1  2  3  4  5  6  7 |  8  9
// 10 11 12 13 14 15 |  16
//	1. Contiguity	  0%	   1		0  0  0  0  0  0  0  0 |  0  0
// 0  0  0  0  0  0 |   0 ... --> (0),   (1),   (2), ...
//	2. Contiguity	 25%	   3		2  -  -  0  0  -  0  0 |  2  -
//-  0  0  0  0  0 |   2 ... --> (0-2), (8-10), (16-18), ...
//	3. Contiguity	 50%	   5		4  -  -  -  -  0  0  0 |  4  -
//-  -  -  0  0  0 |   4 ... --> (0-4), (8-12), (16-20), ...
//	4. Contiguity	 75%	   7		6  -  -  -  -  -  -  0 |  6  -
//-  -  -  -  -  0 |   6 ... --> (0-6), (8-14), (16-22), ...
//	5. Contiguity	100%			MAX
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// Buddy page table
// 	1. Assumption
// 		(1) Single thread
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// Known issues FIXME
//	1. W FIFO			: In random (corner) case, when
// simulation finishes, "FIFO_W" may not be empty.
// This is bug. Work-around is not to use "W_channel". We believe this is
// insignificant issue because of "early-write-response".
//	2. AR-AW interference		: Often "AR_fwd", "AW_fwd", "R_fwd"
// seems create read/write interference.
// When PTW MO is allowed, this is issue. When "1-st level PTW" is on going and
// new PTW requires 2nd-level, then ordering is complex.
// When a single MMU does not conduct read and write concurrently, this is not
// issue.
//	3. W data 			: W data is not referred. Data value is
// not relevant. Work-arond is not to use "W_Channel".
//	4. Addr dependency		: Flow control. Addr dependency. Assume
// no address dependency.
//
//	5. Stall due to PTW		: When PTW for AR is on-going and AW is
// TLB-hit, we stall AW (because PTW on-going). This is performance issue.
//	6. Tracker alloc for hit	: When TLB is hit, tracker entry is
// allocated. This is not bug. We do it for debug.
//	7. VA format			: In TLB hardwrae, format is "tag +
// index + tag" (if contiguity enable) or "tag + index" (if contiguity disable).
//					  In perf model, we use "vpn" so tag
// number not necessary.
//	8. Block fetch			: PCA supports "Block fetch", "Half
// fetch", "Quarter fetch". 					  CAMB supports
// "Half fetch", "Quarter fetch".
//	9. Page table			: Contiguity represented every 4 or 8
// pages (as above). 					  Random generation with
// different contiguity is future work. Random (weighted) number generation
// routine can be used.
//	10. CAMB			: "QUARTER_FETCH" and "HALF_FETCH" only
// supported. When TLB fill, allocate 4 or 8 PTEs.
// Assume BCT-style. 					  In TLB entry, "VPN"
// meaans "StartVPN". "PPN" means "StartPPN".
//	11. RMM support "Page Granule 4": RMM supports only
//"PAGE_TABLE_GRANULE4" format
//	12. PCAD			: Not implemented. When ascending
// pattern, PCAD, PCA, BCT are same.
//	13. PTW multiple-outstanding	: Currently, any PTW MO is 1.
//
//	14. DBPF ascending		: DBPF supports only ascending
// direction.
//	15. DBPF AR + AW		: PF oustanding is 1. When PF for AR is
// outstanding, PF for AW is not initiated. Work-around is to connect either AR
// or AW.
//	16. DBPF TLB size		: PTW (or PF) fills entire "buf0" or
//"buf1". TLB size should be equal to "NUM_PTE_PTW". For example, buf has 4
// entries for  "QUARTER_FETCH".
//---------------------------------------------------------------------------------------------------------------------------------------------------------
// To do
//	1. PTW multiple-outstanding	: AR and AW seprately controls. "AR-PTW
// ID" and "AW-PTW ID" can be different.
// Be careful when new "2nd-level PTW" required when "1st-level PTW" on going.
//	2. PF with CAMB (or BCT)
//	3. Replacement			: Currently, using "timestamp". May need
// improvement (to consider contiguity)
//	4. Buddy support		: Currently, "conventional" and "BCT"
// only support Buddy.
//	5. AT scheme: How to pop from Tracker when the AW transaction is
// complete while remaining one type of PTW in progress (maybe AR transaction,
// but currently only AW transaction) 		current solution, create a new
// state: ECACHE_STATE_TYPE_PTW_HIT to indicate that the AW transaction is
// complete before completing remain PTW 		in detail: Do_B_bwd will
// update ECACHE_STATE_FOURTH_PTW_DONE to ECACHE_STATE_PTW_HIT
// APTW in Do_R_fwd_AT will check state, if HIT then pop all PTW from Tracker,
// else do normal
//---------------------------------------------------------------------------------------------------------------------------------------------------------

#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "CMMU.h"

// Construct
CMMU::CMMU(string cName) {

  // Generate and initialize
  this->cName = cName;

  // Generate ports
  // Slave interface
  this->cpRx_AR = new CTRx("MMU_Rx_AR", ETRX_TYPE_RX, EPKT_TYPE_AR);
  this->cpRx_AW = new CTRx("MMU_Rx_AW", ETRX_TYPE_RX, EPKT_TYPE_AW);
  this->cpTx_R = new CTRx("MMU_Tx_R", ETRX_TYPE_TX, EPKT_TYPE_R);
  this->cpRx_W = new CTRx("MMU_Rx_W", ETRX_TYPE_RX, EPKT_TYPE_W);
  this->cpTx_B = new CTRx("MMU_Tx_B", ETRX_TYPE_TX, EPKT_TYPE_B);

  // Master interface
  this->cpTx_AR = new CTRx("MMU_Tx_AR", ETRX_TYPE_TX, EPKT_TYPE_AR);
  this->cpTx_AW = new CTRx("MMU_Tx_AW", ETRX_TYPE_TX, EPKT_TYPE_AW);
  this->cpRx_R = new CTRx("MMU_Rx_R", ETRX_TYPE_RX, EPKT_TYPE_R);
  this->cpTx_W = new CTRx("MMU_Tx_W", ETRX_TYPE_TX, EPKT_TYPE_W);
  this->cpRx_B = new CTRx("MMU_Rx_B", ETRX_TYPE_RX, EPKT_TYPE_B);

  // FIFO
  this->cpFIFO_AR = new CFIFO("MMU_FIFO_AR", EUD_TYPE_AR, 40);
  // this->cpFIFO_R = new CFIFO("MMU_FIFO_R",  EUD_TYPE_R,  40);
  // this->cpFIFO_B = new CFIFO("MMU_FIFO_B",  EUD_TYPE_B,  40);
  this->cpFIFO_W = new CFIFO("MMU_FIFO_W", EUD_TYPE_W, 160);

  // Generate TLB ("buf0" in DBPF)
  for (int i = 0; i < MMU_NUM_SET; i++) {
    for (int j = 0; j < MMU_NUM_WAY; j++) {
      this->spTLB[i][j] = new SMMUEntry;
      this->spTLB[i][j]->nValid = -1;
      this->spTLB[i][j]->nVPN = -1;
      this->spTLB[i][j]->nPPN = -1;
      this->spTLB[i][j]->ePageSize = EPAGE_SIZE_TYPE_UNDEFINED;
      this->spTLB[i][j]->nTimeStamp = -1;
      this->spTLB[i][j]->nContiguity = -1;
      this->spTLB[i][j]->N_flag = -1;
    };
  };

  // Generate TLB1 ("buf1" in DBPF)
  for (int i = 0; i < MMU_NUM_SET; i++) {
    for (int j = 0; j < MMU_NUM_WAY; j++) {
      this->spTLB1[i][j] = new SMMUEntry;
      this->spTLB1[i][j]->nValid = -1;
      this->spTLB1[i][j]->nVPN = -1;
      this->spTLB1[i][j]->nPPN = -1;
      this->spTLB1[i][j]->ePageSize = EPAGE_SIZE_TYPE_UNDEFINED;
      this->spTLB1[i][j]->nTimeStamp = -1;
      this->spTLB1[i][j]->nContiguity = -1;
    };
  };

  // Generate RTLB memory. RMM
  for (int i = 0; i < MMU_NUM_SET; i++) {
    for (int j = 0; j < MMU_NUM_WAY; j++) {
      this->spRTLB_RMM[i][j] = new SMMUEntry;
      this->spRTLB_RMM[i][j]->nValid = -1;
      this->spRTLB_RMM[i][j]->nVPN = -1;
      this->spRTLB_RMM[i][j]->nPPN = -1;
      this->spRTLB_RMM[i][j]->ePageSize = EPAGE_SIZE_TYPE_UNDEFINED;
      this->spRTLB_RMM[i][j]->nTimeStamp = -1;
      this->spRTLB_RMM[i][j]->nContiguity = -1;
    };
  };

  // Generate FLPD memory
  for (int j = 0; j < NUM_FLPD_ENTRY; j++) {
    this->spFLPD[j] = new SMMUEntry;
    this->spFLPD[j]->nValid = -1;
    this->spFLPD[j]->nVPN = -1;
    this->spFLPD[j]->nPPN = -1;
    this->spFLPD[j]->ePageSize = EPAGE_SIZE_TYPE_UNDEFINED;
    this->spFLPD[j]->nTimeStamp = -1;
    this->spFLPD[j]->nContiguity = -1;
  };

  // MO tracker
  this->cpTracker = new CTracker("MMU_MO_tracker", 40);

  // W control
  // this->IsPTW_AW_ongoing = ERESULT_TYPE_NO;

  this->nAR_START_ADDR = -1; // RMM. nRTE calculation
  this->nAW_START_ADDR = -1;

  this->START_VPN = -1; // Buddy page-table

  this->spPageTable = NULL;

  // Stat
  this->nMO_AR = -1;
  this->nMO_AW = -1;
  this->nMO_Ax = -1;

  this->nMax_MO_AR = -1;
  this->nMax_MO_AW = -1;
  this->nMax_MO_Ax = -1;

  this->nTotal_MO_AR = -1;
  this->nTotal_MO_AW = -1;
  this->nTotal_MO_Ax = -1;

  this->nMax_MOTracker_Occupancy = -1;
  this->nTotal_MOTracker_Occupancy = -1;

  // Stat
  this->nAR_SI = -1;
  this->nAR_MI = -1;
  this->nAW_SI = -1;
  this->nAW_MI = -1;
  this->nHit_AR_TLB = -1;
  this->nHit_AW_TLB = -1;
  this->nHit_AR_FLPD = -1;
  this->nHit_AW_FLPD = -1;
  this->nHit_AR_TLB1_DBPF = -1;
  this->nHit_AR_TLB1_DBPF = -1;
  this->nHit_TLB_After_Fill_DBPF = -1;
  this->nHit_TLB1_After_Fill_DBPF = -1;
  this->nPTW_total = -1;
  this->nPTW_1st = -1;
  this->nPTW_2nd = -1;
  this->nPTW_3rd = -1;
  this->nPTW_4th = -1;

  this->nPF_total = -1;
  this->nPF_1st = -1;
  this->nPF_2nd = -1;

  this->nRPTW_total_RMM = -1; // RMM
  this->nRPTW_1st_RMM = -1;
  this->nRPTW_2nd_RMM = -1;
  this->nRPTW_3rd_RMM = -1;

  this->nAPTW_total_AT = -1; // AT
  this->nAPTW_1st_AT = -1;
  this->nAPTW_2nd_AT = -1;
  this->nAPTW_3rd_AT = -1;
  this->nAPTW_4th_AT = -1;

  this->nHit_AR_RTLB_RMM = -1; // RMM
  this->nHit_AW_RTLB_RMM = -1;

  this->nTLB_evict = -1;
  this->nFLPD_evict = -1;

  this->nRTLB_evict_RMM = -1; // RMM

  this->nMax_Tracker_Wait_Ax = -1;
  this->nTotal_Tracker_Wait_Ax = -1;
  this->nTotal_Tracker_Wait_AR = -1;
  this->nTotal_Tracker_Wait_AW = -1;

  this->IsPTW_AR_ongoing = ERESULT_TYPE_NO;
  this->IsPTW_AW_ongoing = ERESULT_TYPE_NO;

  this->IsRPTW_AR_ongoing_RMM = ERESULT_TYPE_NO; // RMM
  this->IsRPTW_AW_ongoing_RMM = ERESULT_TYPE_NO;

  this->Is_AR_priority = ERESULT_TYPE_NO;
  this->Is_AW_priority = ERESULT_TYPE_NO;

  this->nPTW_AR_ongoing_cycles = -1;
  this->nPTW_AW_ongoing_cycles = -1;

  this->nAR_stall_cycles_SI = -1;
  this->nAW_stall_cycles_SI = -1;

  this->nTotalPages_TLB_capacity = -1;

  this->AR_min_VPN = -1;
  this->AW_min_VPN = -1;
  this->AR_max_VPN = -1;
  this->AW_max_VPN = -1;

  this->anchorDistance = 0; // DUONGTRAN add
};

// Destruct
CMMU::~CMMU() {

  // Port
  delete (this->cpRx_AR);
  delete (this->cpRx_AW);
  delete (this->cpTx_R);
  delete (this->cpRx_W);
  delete (this->cpTx_B);

  delete (this->cpTx_AR);
  delete (this->cpTx_AW);
  delete (this->cpRx_R);
  delete (this->cpTx_W);
  delete (this->cpRx_B);

  this->cpRx_AR = NULL;
  this->cpRx_AW = NULL;
  this->cpTx_R = NULL;
  this->cpRx_W = NULL;
  this->cpTx_B = NULL;

  this->cpTx_AR = NULL;
  this->cpTx_AW = NULL;
  this->cpRx_R = NULL;
  this->cpTx_W = NULL;
  this->cpRx_B = NULL;

  // FIFO
  delete (this->cpFIFO_AR);
  // delete (this->cpFIFO_R);
  // delete (this->cpFIFO_B);
  delete (this->cpFIFO_W);

  this->cpFIFO_AR = NULL;
  // this->cpFIFO_R = NULL;
  // this->cpFIFO_B = NULL;
  this->cpFIFO_W = NULL;

  // TLB memory ("buf0" in DBPF)
  for (int i = 0; i < MMU_NUM_SET; i++) {
    for (int j = 0; j < MMU_NUM_WAY; j++) {
      delete (this->spTLB[i][j]);
      this->spTLB[i][j] = NULL;
    };
  };

  // TLB ("buf1" in DBPF)
  for (int i = 0; i < MMU_NUM_SET; i++) {
    for (int j = 0; j < MMU_NUM_WAY; j++) {
      delete (this->spTLB1[i][j]);
      this->spTLB1[i][j] = NULL;
    };
  };

  // RTLB memory. RMM
  for (int i = 0; i < MMU_NUM_SET; i++) {
    for (int j = 0; j < MMU_NUM_WAY; j++) {
      delete (this->spRTLB_RMM[i][j]);
      this->spRTLB_RMM[i][j] = NULL;
    };
  };

  // FLPD memory
  for (int j = 0; j < NUM_FLPD_ENTRY; j++) {
    delete (this->spFLPD[j]);
    this->spFLPD[j] = NULL;
  };

  // MO tracker
  delete (this->cpTracker);
  this->cpTracker = NULL;
};

// Reset
EResultType CMMU::Reset() {

  // Initialize
  // Port
  this->cpRx_AR->Reset();
  this->cpRx_AW->Reset();
  this->cpTx_R->Reset();
  this->cpRx_W->Reset();
  this->cpTx_B->Reset();

  this->cpTx_AR->Reset();
  this->cpTx_AW->Reset();
  this->cpRx_R->Reset();
  this->cpTx_W->Reset();
  this->cpTx_B->Reset();

  // FIFO
  this->cpFIFO_AR->Reset();
  // this->cpFIFO_R->Reset();
  // this->cpFIFO_B->Reset();
  this->cpFIFO_W->Reset();

  // TLB memory ("buf0" in DBPF)
  for (int i = 0; i < MMU_NUM_SET; i++) {
    for (int j = 0; j < MMU_NUM_WAY; j++) {
      this->spTLB[i][j]->nValid = 0;
      this->spTLB[i][j]->nVPN = -1;
      this->spTLB[i][j]->nPPN = -1;
      this->spTLB[i][j]->ePageSize = EPAGE_SIZE_TYPE_UNDEFINED;
      this->spTLB[i][j]->nTimeStamp = 0;
      this->spTLB[i][j]->nContiguity = 0;
      this->spTLB[i][j]->N_flag = 0;
    };
  };

  // TLB memory ("buf1" in DBPF)
  for (int i = 0; i < MMU_NUM_SET; i++) {
    for (int j = 0; j < MMU_NUM_WAY; j++) {
      this->spTLB1[i][j]->nValid = 0;
      this->spTLB1[i][j]->nVPN = -1;
      this->spTLB1[i][j]->nPPN = -1;
      this->spTLB1[i][j]->ePageSize = EPAGE_SIZE_TYPE_UNDEFINED;
      this->spTLB1[i][j]->nTimeStamp = 0;
      this->spTLB1[i][j]->nContiguity = 0;
    };
  };

  // RTLB memory. RMM
  for (int i = 0; i < MMU_NUM_SET; i++) {
    for (int j = 0; j < MMU_NUM_WAY; j++) {
      this->spRTLB_RMM[i][j]->nValid = 0;
      this->spRTLB_RMM[i][j]->nVPN = -1;
      this->spRTLB_RMM[i][j]->nPPN = -1;
      this->spRTLB_RMM[i][j]->ePageSize = EPAGE_SIZE_TYPE_UNDEFINED;
      this->spRTLB_RMM[i][j]->nTimeStamp = 0;
      this->spRTLB_RMM[i][j]->nContiguity = 0;
    };
  };

  // FLPD memory
  for (int j = 0; j < NUM_FLPD_ENTRY; j++) {
    this->spFLPD[j]->nValid = 0;
    this->spFLPD[j]->nVPN = -1;
    this->spFLPD[j]->nPPN = -1;
    this->spFLPD[j]->ePageSize = EPAGE_SIZE_TYPE_UNDEFINED;
    this->spFLPD[j]->nTimeStamp = 0;
    this->spFLPD[j]->nContiguity = 0;
  };

  // MO tracker
  this->cpTracker->Reset();

  // Control
  this->nMO_AR = 0;
  this->nMO_AW = 0;
  this->nMO_Ax = 0;

  // this->nAR_START_ADDR = 0; // Set before reset
  // this->nAW_START_ADDR = 0;

  // Stat
  this->nMax_MO_AR = 0;
  this->nMax_MO_AW = 0;
  this->nMax_MO_Ax = 0;

  this->nTotal_MO_AR = 0;
  this->nTotal_MO_AW = 0;
  this->nTotal_MO_Ax = 0;

  this->nMax_MOTracker_Occupancy = 0;
  this->nTotal_MOTracker_Occupancy = 0;

  this->nAR_SI = 0;
  this->nAR_MI = 0;
  this->nAW_SI = 0;
  this->nAW_MI = 0;
  this->nHit_AR_TLB = 0;
  this->nHit_AW_TLB = 0;
  this->nHit_AR_FLPD = 0;
  this->nHit_AW_FLPD = 0;
  this->nHit_AR_TLB1_DBPF = 0;
  this->nHit_AW_TLB1_DBPF = 0;
  this->nHit_TLB_After_Fill_DBPF = -1;
  this->nHit_TLB1_After_Fill_DBPF = -1;
  this->nPTW_total = 0;
  this->nPTW_1st = 0;
  this->nPTW_2nd = 0;
  this->nPTW_3rd = 0;
  this->nPTW_4th = 0;
  this->nPF_total = 0;
  this->nPF_1st = 0;
  this->nPF_2nd = 0;

  this->nRPTW_total_RMM = 0; // RMM
  this->nRPTW_1st_RMM = 0;
  this->nRPTW_2nd_RMM = 0;
  this->nRPTW_3rd_RMM = 0;

  this->nAPTW_total_AT = 0; // AT
  this->nAPTW_1st_AT = 0;
  this->nAPTW_2nd_AT = 0;
  this->nAPTW_3rd_AT = 0;
  this->nAPTW_4th_AT = 0;

  this->nHit_AR_RTLB_RMM = 0; // RMM
  this->nHit_AW_RTLB_RMM = 0;

  this->nTLB_evict = 0;
  this->nFLPD_evict = 0;

  this->nRTLB_evict_RMM = 0; // RMM

  this->nMax_Tracker_Wait_Ax = 0;
  this->nTotal_Tracker_Wait_Ax = 0;
  this->nTotal_Tracker_Wait_AR = 0;
  this->nTotal_Tracker_Wait_AW = 0;

  this->IsPTW_AR_ongoing = ERESULT_TYPE_NO;
  this->IsPTW_AW_ongoing = ERESULT_TYPE_NO;

  this->IsRPTW_AR_ongoing_RMM = ERESULT_TYPE_NO; // RMM
  this->IsRPTW_AW_ongoing_RMM = ERESULT_TYPE_NO;

  this->nPTW_AR_ongoing_cycles = 0;
  this->nPTW_AW_ongoing_cycles = 0;

  this->Is_AR_priority = ERESULT_TYPE_YES; // Check AR first SI TLB check
  this->Is_AW_priority = ERESULT_TYPE_NO;

  this->nAR_stall_cycles_SI = 0;
  this->nAW_stall_cycles_SI = 0;

  this->nTotalPages_TLB_capacity = 0;

  this->AR_min_VPN = 0x7FFFFFFFFFFFF;
  this->AW_min_VPN = 0x7FFFFFFFFFFFF;
  this->AR_max_VPN = 0;
  this->AW_max_VPN = 0;

  return (ERESULT_TYPE_SUCCESS);
};

// AR valid
EResultType CMMU::Do_AR_fwd_MMU_OFF(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid
  CPAxPkt cpAR = this->cpRx_AR->GetPair()->GetAx();
  if (cpAR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(this->cpRx_AR->IsBusy() == ERESULT_TYPE_NO);
  assert(this->cpRx_AR->GetPair()->IsBusy() == ERESULT_TYPE_YES);
// cpAR->CheckPkt();
#endif

  // Put Rx
  this->cpRx_AR->PutAx(cpAR);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AR_fwd_MMU_OFF] %s put Rx_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
  assert(cpAR != NULL);
#endif

  // Put Tx
  this->cpTx_AR->PutAx(cpAR);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AR_fwd_MMU_OFF] %s put Tx_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// AW valid
EResultType CMMU::Do_AW_fwd_MMU_OFF(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid
  CPAxPkt cpAW = this->cpRx_AW->GetPair()->GetAx();
  if (cpAW == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(this->cpRx_AW->IsBusy() == ERESULT_TYPE_NO);
  assert(this->cpRx_AW->GetPair()->IsBusy() == ERESULT_TYPE_YES);
// cpAW->CheckPkt();
#endif

  // Put Rx
  this->cpRx_AW->PutAx(cpAW);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AW_fwd_MMU_OFF] %s put Rx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
#endif

#ifdef DEBUG_MMU
  assert(cpAW != NULL);
#endif

  // Put Tx
  this->cpTx_AW->PutAx(cpAW);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AW_fwd_MMU_OFF] %s put Tx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// W valid
EResultType CMMU::Do_W_fwd_MMU_OFF(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid
  CPAxPkt cpW = this->cpRx_W->GetPair()->GetAx();
  if (cpW == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(this->cpRx_W->IsBusy() == ERESULT_TYPE_NO);
  assert(this->cpRx_W->GetPair()->IsBusy() == ERESULT_TYPE_YES);
// cpW->CheckPkt();
#endif

  // Put Rx
  this->cpRx_W->PutAx(cpW);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_W_fwd_MMU_OFF] %s put Rx_W.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
  assert(cpW != NULL);
#endif

  // Put Tx
  this->cpTx_W->PutAx(cpW);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_W_fwd_MMU_OFF] %s put Tx_W.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// AR ready
EResultType CMMU::Do_AR_bwd_MMU_OFF(int64_t nCycle) {

  // Check Tx ready
  if (this->cpTx_AR->GetAcceptResult() == ERESULT_TYPE_REJECT) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check Rx valid
  if (this->cpRx_AR->IsBusy() == ERESULT_TYPE_NO) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Set ready
  this->cpRx_AR->SetAcceptResult(ERESULT_TYPE_ACCEPT);

  return (ERESULT_TYPE_SUCCESS);
};

// AW ready
EResultType CMMU::Do_AW_bwd_MMU_OFF(int64_t nCycle) {

  // Check Tx ready
  if (this->cpTx_AW->GetAcceptResult() == ERESULT_TYPE_REJECT) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check Rx valid
  if (this->cpRx_AW->IsBusy() == ERESULT_TYPE_NO) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Set ready
  this->cpRx_AW->SetAcceptResult(ERESULT_TYPE_ACCEPT);

  return (ERESULT_TYPE_SUCCESS);
};

// W ready
EResultType CMMU::Do_W_bwd_MMU_OFF(int64_t nCycle) {

  // Check Tx ready
  if (this->cpTx_W->GetAcceptResult() == ERESULT_TYPE_REJECT) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check Rx valid
  if (this->cpRx_W->IsBusy() == ERESULT_TYPE_NO) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Set ready
  this->cpRx_W->SetAcceptResult(ERESULT_TYPE_ACCEPT);

  return (ERESULT_TYPE_SUCCESS);
};

// R valid
EResultType CMMU::Do_R_fwd_MMU_OFF(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_R->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid
  CPRPkt cpR = this->cpRx_R->GetPair()->GetR();
  if (cpR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(cpR != NULL);
// cpR->CheckPkt();
#endif

  // Put Rx
  this->cpRx_R->PutR(cpR);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_R_fwd_MMU_OFF] %s put Rx_R.\n", nCycle, this->cName.c_str(), cpR->GetName().c_str());
#endif

  // Put Tx
  this->cpTx_R->PutR(cpR);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_R_fwd_MMU_OFF] %s put Tx_R.\n", nCycle, this->cName.c_str(), cpR->GetName().c_str());
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// B valid
EResultType CMMU::Do_B_fwd_MMU_OFF(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_B->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid
  CPBPkt cpB = this->cpRx_B->GetPair()->GetB();
  if (cpB == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(cpB != NULL);
// cpB->CheckPkt();
#endif

  // Put Rx
  this->cpRx_B->PutB(cpB);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_B_fwd_MMU_OFF] %s put Rx_B.\n", nCycle, this->cName.c_str(), cpB->GetName().c_str());
#endif

  // Put Tx
  this->cpTx_B->PutB(cpB);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_B_fwd_MMU_OFF] %s put Tx_B.\n", nCycle, this->cName.c_str(), cpB->GetName().c_str());
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// R ready
EResultType CMMU::Do_R_bwd_MMU_OFF(int64_t nCycle) {

  // Check Tx ready
  if (this->cpTx_R->GetAcceptResult() == ERESULT_TYPE_REJECT) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check Rx valid
  if (this->cpRx_R->IsBusy() == ERESULT_TYPE_NO) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Set ready
  this->cpRx_R->SetAcceptResult(ERESULT_TYPE_ACCEPT);

  return (ERESULT_TYPE_SUCCESS);
};

// B ready
EResultType CMMU::Do_B_bwd_MMU_OFF(int64_t nCycle) {

  // Check Tx ready
  if (this->cpTx_B->GetAcceptResult() == ERESULT_TYPE_REJECT) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check Rx valid
  if (this->cpRx_B->IsBusy() == ERESULT_TYPE_NO) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Set ready
  this->cpRx_B->SetAcceptResult(ERESULT_TYPE_ACCEPT);

  return (ERESULT_TYPE_SUCCESS);
};

//-------------------------------------------------------------------------------------------------------
// AR valid.  AT (Anchor Translation)
// 	TLB lookup
//-------------------------------------------------------------------------------------------------------
// Algorithm:
//		   Regular 	Anchor		Contiguity	Operation
// Allocation
//		1. Hit		-		-		Translate, or:
//		   Miss		Allocated	Match		Translate, end.
//
//		2. -		Allocated	Not match		(1) PTW,
//(2) Fill "regular" in TLB (after do PTW, do step 3 below).
//
//		3. -		Not allocated	-			(1)
// APTW. 		3.1 match (2) Fill "anchor"  in TLB 		3.2 Not match	(2) Fill "regular" in TLB 		Duong: if PTW done
// first, response, wait APTW to determine fill anchor or regular TLB 			   if APTW done first, check contiguity
// matching, if match then do response and fill anchor TLB if not match then wait PTW 			   for both step2 and step3:
// if PTW done, check Tracker, if have no APTW then fill regular TLB, pop from Tracker, end. if have APTW then wait
// APTW, ..., pop both PTW and APTW from Tracker, end if APTW done, check Tracker, if have no PTW -> ERROR!!! because
// APTW never alone if have PTW then check the contiguity match to determine fill anchor or regular TLB, pop both PTW
// and APTW from Tracker, end
//-------------------------------------------------------------------------------------------------------
// Operation
// 		0. Get Ax. Check TLB hit
// 		1. If hit, translate address
// 		2. Miss. Suppose "regular" or "anchor". If "anchor"  not in TLB,
// generate "anchor" APTW
// 		3. Miss. Suppose "regular".             If "regular" not in TLB,
// generate "regular" PTW
//-------------------------------------------------------------------------------------------------------
// Note
// 		1. TLB entry types "regular" or "anchor".
//		2. APTW and PTW same 1MB boundary.
// 		   Issue APTW earlier than PTW.
// 		   FIXME Finish APTW always earlier than PTW.
// 		3. When both PTW and APTW FLPD miss, one should wait until other
// PTW finishes. 		   To improve performance, issue both "1st PTW"
// and "1st APTW".
// FIXME
//-------------------------------------------------------------------------------------------------------
EResultType CMMU::Do_AR_fwd_SI_AT(int64_t nCycle) {

  //----------------------------------------------------
  // 0. Get Ax. Check TLB hit
  //----------------------------------------------------

  // Check AW priority and candidate
  if (this->Is_AW_priority == ERESULT_TYPE_YES and this->cpRx_AW->GetPair()->GetAx() != NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check FIFO AR
  if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid. Get.
  CPAxPkt cpAR = this->cpRx_AR->GetPair()->GetAx();
  if (cpAR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // AR info original
  //
  int nID = cpAR->GetID();
  int64_t nVA = cpAR->GetVA();

#ifdef STAT_DETAIL
  int64_t nVPN = GetVPNNum(nVA);
  if (nVPN < this->AR_min_VPN) {
    this->AR_min_VPN = nVPN;
  };
  if (nVPN > this->AR_max_VPN) {
    this->AR_max_VPN = nVPN;
  };
#endif

  // Check MO
  if (this->GetMO_Ax() >= MAX_MO_COUNT) { // AR + AW
    return (ERESULT_TYPE_SUCCESS);
  };

  // // Check ID dependent node doing PTW
  // if (this->cpTracker->IsUDMatchPTW(EUD_TYPE_AR) == ERESULT_TYPE_YES) {
  // // Check any PTW FIXME performance if (this->cpTracker->IsIDMatchPTW(nID,
  // EUD_TYPE_AR) == ERESULT_TYPE_YES) {		// Check ID PTW
  if (this->cpTracker->IsAnyPTW() == ERESULT_TYPE_YES) { // Check any PTW
    // if (this->cpTracker->IsIDMatch(nID, EUD_TYPE_AR) == ERESULT_TYPE_YES) {
    // // Check ID dependent node is outstanding
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check address dependency
  if (this->cpTracker->IsAnyPTW_Match_VPN(nVA) == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  if (this->cpTracker->IsFull() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_FAIL);
  }

#ifdef DEBUG_MMU
  assert(this->cpRx_AR->IsBusy() == ERESULT_TYPE_NO);
#endif

  // Put Rx
  this->cpRx_AR->PutAx(cpAR);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s put Rx_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
// cpAR->Display();
#endif

  // Check regular or anchor
  // EResultType IsVA_Regular = this->IsVA_Regular_AT(nVA);
  // EResultType IsVA_Anchor  = this->IsVA_Anchor_AT(nVA);

  // Lookup TLB (regular or anchor)
  EResultType IsTLB_Allocate = this->IsTLB_Allocate_AT(nVA, nCycle);
  // Check anchor allocated
  EResultType IsTLB_Allocate_Anchor = this->IsTLB_Allocate_Anchor_AT(nVA);
  EResultType IsTLB_Contiguity_Match = this->IsTLB_Contiguity_Match_AT(nVA, nCycle);

#ifdef DEBUG_MMU
  if (IsTLB_Allocate == ERESULT_TYPE_YES) {
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s TLB allocate VA.\n", nCycle, this->cName.c_str(),
           cpAR->GetName().c_str());
  } else {
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s TLB not allocate VA.\n", nCycle, this->cName.c_str(),
           cpAR->GetName().c_str());
  };

  if (IsTLB_Allocate_Anchor == ERESULT_TYPE_YES) {
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s TLB_AT allocate VA.\n", nCycle, this->cName.c_str(),
           cpAR->GetName().c_str());
  } else {
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s TLB_AT not allocate VA.\n", nCycle, this->cName.c_str(),
           cpAR->GetName().c_str());
  };

  if (IsTLB_Contiguity_Match == ERESULT_TYPE_YES) {
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s TLB contiguity matched.\n", nCycle, this->cName.c_str(),
           cpAR->GetName().c_str());
  } else {
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s TLB contiguity not matched.\n", nCycle, this->cName.c_str(),
           cpAR->GetName().c_str());
  };
#endif

  // // Check anchor allocated but not contiguity match
  // EResultType IsAnchor_InTLB_But_Not_ContiguityMatch = ERESULT_TYPE_NO;
  //
  // if (IsTLB_Allocate_Anchor == ERESULT_TYPE_YES and IsTLB_Contiguity_Match ==
  // ERESULT_TYPE_NO) { 	IsAnchor_InTLB_But_Not_ContiguityMatch =
  // ERESULT_TYPE_YES;
  // };

  //-------------------------------------------------
  // 1. Hit. Translate address
  //-------------------------------------------------
  // if (IsTLB_Allocate == ERESULT_TYPE_YES or IsTLB_Contiguity_Match ==
  // ERESULT_TYPE_YES) {  // DUONGTRAN comment
  if ((IsTLB_Allocate == ERESULT_TYPE_YES) ||
      ((IsTLB_Contiguity_Match == ERESULT_TYPE_YES) && (IsTLB_Allocate_Anchor == ERESULT_TYPE_YES))) { // DUONGTRAN add

    // Push tracker
    UPUD upAR_new = new UUD;
    upAR_new->cpAR = Copy_CAxPkt(cpAR); // Original VA
    this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_HIT);

#ifdef DEBUG_MMU
// printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s push MO_tracker.\n", nCycle,
// this->cName.c_str(), cpAR->GetName().c_str());
// this->cpTracker->CheckTracker();
#endif

    // Maintain
    Delete_UD(upAR_new, EUD_TYPE_AR);

    // Push FIFO AR
    UPUD upAR_new2 = new UUD;
    upAR_new2->cpAR = Copy_CAxPkt(cpAR);

    // Get PA
    int nPA = this->GetPA_TLB(nVA);

#ifdef DEBUG_MMU
    assert(nPA >= MIN_ADDR);
    assert(nPA <= MAX_ADDR);
#endif

    // Set PA
    upAR_new2->cpAR->SetAddr(nPA);

    // Encode ID
    nID = nID << 2; // PTW ID 0x1
    upAR_new2->cpAR->SetID(nID);

#ifdef DEBUG_MMU
    assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif

    this->cpFIFO_AR->Push(upAR_new2, MMU_FIFO_AR_TLB_HIT_LATENCY); // Translated, ID-encoded

#ifdef DEBUG_MMU
    assert(upAR_new2 != NULL);
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
// cpAR->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upAR_new2, EUD_TYPE_AR);

    // Stat
    this->nHit_AR_TLB++;

    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert((IsTLB_Allocate == ERESULT_TYPE_NO) &&
         ((IsTLB_Contiguity_Match == ERESULT_TYPE_NO) || (IsTLB_Allocate_Anchor == ERESULT_TYPE_NO)));
#endif

  char cPktName[50];

  //-------------------------------------------------
  // 2. TLB miss. Suppose "regular".
  //    If "regular" not in TLB, generate PTW
  //-------------------------------------------------
  // Miss. Generate "regular" PTW
  // if (IsVA_Regular == ERESULT_TYPE_YES and IsTLB_Allocate == ERESULT_TYPE_NO)
  // {
  if (IsTLB_Allocate == ERESULT_TYPE_NO) {
    // Check FLPD
    EResultType IsTLPD_Hit = this->IsTLPD_Hit(nVA, nCycle);
    EResultType IsSLPD_Hit = this->IsSLPD_Hit(nVA, nCycle);
    EResultType IsFLPD_Hit = this->IsFLPD_Hit(nVA, nCycle);

    // Generate AR PTW
    char cPktName[50];
    CPAxPkt cpAR_new = NULL;

    int64_t nAddr_PTW = -1;

    // Generate "regular" PTW
    if (IsTLPD_Hit == ERESULT_TYPE_YES) { // TLPD hit

      // Get address for 4th-level table
      nAddr_PTW = GetFourth_PTWAddr(nVA);

      // Set pkt
      sprintf(cPktName, "4th_PTW_for_%s", cpAR->GetName().c_str());
      cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

      //                  nID,       nAddr,      nLen
      // cpAR_new->SetPkt(ARID_PTW,  nAddr_PTW,  0);
      cpAR_new->SetID(ARID_PTW);
      cpAR_new->SetAddr(nAddr_PTW);
      cpAR_new->SetSrcName(this->cName);
      cpAR_new->SetTransType(ETRANS_TYPE_FOURTH_PTW);
      cpAR_new->SetVA(nVA);

#if defined SINGLE_FETCH
      cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes
                                              // and BURST_SIZE 16 bytes
#elif defined HALF_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

      // Stat
      this->nHit_AR_FLPD++;
      this->nPTW_4th++;
    } else if (IsSLPD_Hit == ERESULT_TYPE_YES) { // SLPD hit

      // Get address for 3rd-level table
      nAddr_PTW = GetThird_PTWAddr(nVA);
      // Set pkt
      sprintf(cPktName, "3rd_PTW_for_%s", cpAR->GetName().c_str());
      cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

      //                  nID,       nAddr,      nLen
      // cpAR_new->SetPkt(ARID_PTW,  nAddr_PTW,  0);
      cpAR_new->SetID(ARID_PTW);
      cpAR_new->SetAddr(nAddr_PTW);
      cpAR_new->SetSrcName(this->cName);
      cpAR_new->SetTransType(ETRANS_TYPE_THIRD_PTW);
      cpAR_new->SetVA(nVA);

#if defined SINGLE_FETCH
      cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes
                                              // and BURST_SIZE 16 bytes
#elif defined HALF_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

      // Stat
      this->nHit_AR_FLPD++;
      this->nPTW_3rd++;
    } else if (IsFLPD_Hit == ERESULT_TYPE_YES) { // FLPD hit

      // Get address for 2nd-level table
      nAddr_PTW = GetSecond_PTWAddr(nVA);

      // Set pkt
      sprintf(cPktName, "2nd_PTW_for_%s", cpAR->GetName().c_str());
      cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

      //                  nID,       nAddr,      nLen
      // cpAR_new->SetPkt(ARID_PTW,  nAddr_PTW,  0);
      cpAR_new->SetID(ARID_PTW);
      cpAR_new->SetAddr(nAddr_PTW);
      cpAR_new->SetSrcName(this->cName);
      cpAR_new->SetTransType(ETRANS_TYPE_SECOND_PTW);
      cpAR_new->SetVA(nVA);

#if defined SINGLE_FETCH
      cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes
                                              // and BURST_SIZE 16 bytes
#elif defined HALF_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

      // Stat
      this->nHit_AR_FLPD++;
      this->nPTW_2nd++;
    } else if (IsFLPD_Hit == ERESULT_TYPE_NO) { // FLPD miss

      // Get address for 1st-level table
      nAddr_PTW = GetFirst_PTWAddr(nVA);

      // Set pkt
      sprintf(cPktName, "1st_PTW_for_%s", cpAR->GetName().c_str());
      cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

      //               nID,       nAddr,      nLen
      cpAR_new->SetPkt(ARID_PTW, nAddr_PTW, 0);
      cpAR_new->SetSrcName(this->cName);
      cpAR_new->SetTransType(ETRANS_TYPE_FIRST_PTW);
      cpAR_new->SetVA(nVA);

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

      // Stat
      this->nPTW_1st++;
    } else {
      assert(0);
    };

    // Push tracker "regular"
    UPUD upAR_new = new UUD;
    upAR_new->cpAR = Copy_CAxPkt(cpAR); // Original

    if (IsTLPD_Hit == ERESULT_TYPE_YES) {
      this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_FOURTH_PTW);
    } else if (IsSLPD_Hit == ERESULT_TYPE_YES) {
      this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_THIRD_PTW);
    } else if (IsFLPD_Hit == ERESULT_TYPE_YES) {
      this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_SECOND_PTW);
    } else if (IsFLPD_Hit == ERESULT_TYPE_NO) {
      this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_FIRST_PTW);
    } else {
      assert(0);
    };

#ifdef DEBUG_MMU
// printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s push MO_tracker.\n", nCycle,
// this->cName.c_str(), cpAR->GetName().c_str());
//  this->cpTracker->CheckTracker();
#endif

    // Maintain "regular"
    Delete_UD(upAR_new, EUD_TYPE_AR);

    UPUD upAR_new2 = new UUD;
    upAR_new2->cpAR = cpAR_new; // PTW

    // Push FIFO AR "regular"
    this->cpFIFO_AR->Push(upAR_new2, MMU_FIFO_AR_TLB_MISS_LATENCY);

#ifdef DEBUG_MMU
    // printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s push FIFO_AR.\n", nCycle,
    // this->cName.c_str(), cpAR->GetName().c_str());
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain "regular"
    Delete_UD(upAR_new2, EUD_TYPE_AR);

    // Stat
    this->nPTW_total++;
    this->IsPTW_AR_ongoing = ERESULT_TYPE_YES;
  };

  //-------------------------------------------------
  // 3. TLB miss. Suppose "regular" or "anchor".
  //    If "anchor" not in TLB, generate APTW
  //-------------------------------------------------
  // Check "anchor" not in TLB. Generate APTW
  if (IsTLB_Allocate_Anchor == ERESULT_TYPE_NO) {

    // Get VA anchor
    int64_t nVA_anchor = GetVA_Anchor_AT(nVA);

    if (nVA_anchor == nVA) { // DUONGTRAN add this condition to improve AT
      return (ERESULT_TYPE_SUCCESS);
    }

    // Check FLPD
    EResultType IsTLPD_Hit = this->IsTLPD_Hit(nVA_anchor, nCycle); // Check level-3
    // EResultType IsSLPD_Hit = this->IsSLPD_Hit(nVA_anchor, nCycle);
    // // Check level-2 EResultType IsFLPD_Hit = this->IsFLPD_Hit(nVA_anchor,
    // nCycle);			// Check level-1

    // Generate APTW
    CPAxPkt cpAR_new = NULL;

    int64_t nAddr_PTW = -1;

    // Generate "anchor" APTW
    if (IsTLPD_Hit == ERESULT_TYPE_YES) { // Hit in level-3

      // Get address for 4th-level table
      nAddr_PTW = GetFourth_PTWAddr(nVA_anchor);

      // Set pkt
      sprintf(cPktName, "4th_APTW_for_%s", cpAR->GetName().c_str());
      cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

      //                  nID,       nAddr,      nLen
      // cpAR_new->SetPkt(ARID_APTW, nAddr_PTW,  0);
      cpAR_new->SetID(ARID_APTW);
      cpAR_new->SetAddr(nAddr_PTW);
      cpAR_new->SetSrcName(this->cName);
      cpAR_new->SetTransType(ETRANS_TYPE_FOURTH_APTW);
      // cpAR_new->SetVA(nVA_anchor);
      cpAR_new->SetVA(nVA);

#if defined SINGLE_FETCH
      cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes
                                              // and BURST_SIZE 16 bytes
#elif defined HALF_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

      // Stat
      this->nHit_AR_FLPD++;
      this->nAPTW_4th_AT++;

      // Push tracker "anchor"
      UPUD upAR_new = new UUD;
      upAR_new->cpAR = Copy_CAxPkt(cpAR); // Original

      if (IsTLPD_Hit == ERESULT_TYPE_YES) {
        this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_FOURTH_APTW);
      }

      // Maintain "anchor"
      Delete_UD(upAR_new, EUD_TYPE_AR);

      UPUD upAR_new2 = new UUD;
      upAR_new2->cpAR = cpAR_new; // PTW

      // Push FIFO AR "anchor"
      this->cpFIFO_AR->Push(upAR_new2, MMU_FIFO_AR_TLB_MISS_LATENCY);

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

      // Maintain "anchor"
      Delete_UD(upAR_new2, EUD_TYPE_AR);

      // Stat
      this->nAPTW_total_AT++;
      this->IsPTW_AR_ongoing = ERESULT_TYPE_YES;
    }
  };

  return (ERESULT_TYPE_SUCCESS);
};

//----------------------------------------------
// AR valid
//----------------------------------------------
// 	TLB lookup
//----------------------------------------------
EResultType CMMU::Do_AR_fwd_SI(int64_t nCycle) { // Master -> MMU

  // Check AW priority and candidate
  if (this->Is_AW_priority == ERESULT_TYPE_YES and
      this->cpRx_AW->GetPair()->GetAx() != NULL) { // AW priority and candidate
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check FIFO AR
  if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx. Get.
  CPAxPkt cpAR = this->cpRx_AR->GetPair()->GetAx();
  if (cpAR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Get AR info original
  int nID = cpAR->GetID();
  int64_t nVA = cpAR->GetAddr();

#ifdef STAT_DETAIL
  int64_t nVPN = GetVPNNum(nVA);
  if (nVPN < this->AR_min_VPN) {
    this->AR_min_VPN = nVPN;
  };
  if (nVPN > this->AR_max_VPN) {
    this->AR_max_VPN = nVPN;
  };
#endif

  // Check MO
  if (this->GetMO_Ax() >= MAX_MO_COUNT) { // AR + AW
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check ID-dependent PTW
  if (this->cpTracker->IsAnyPTW() == ERESULT_TYPE_YES) { // Check any PTW FIXME Default
    // if (this->cpTracker->IsIDMatchPTW(nID, EUD_TYPE_AR) == ERESULT_TYPE_YES)
    // {		// Check ID PTW FIXME try. This is reasonable.

    // if (this->cpTracker->IsUDMatchPTW(EUD_TYPE_AR) == ERESULT_TYPE_YES) {
    // // Check any PTW FIXME Not good performance
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check address-dependent PTW
  if (this->cpTracker->IsAnyPTW_Match_VPN(nVA) == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(this->cpRx_AR->IsBusy() == ERESULT_TYPE_NO);
#endif

  // Put Rx
  this->cpRx_AR->PutAx(cpAR);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s put Rx_AR.\n", nCycle, this->cName.c_str(),
         cpAR->GetName().c_str()); // DUONGTRAN uncomment
// cpAR->Display();
#endif

  // Lookup TLB
  EResultType IsTLB_Hit = this->IsTLB_Hit(nVA, nCycle);

#ifdef DEBUG_MMU
  if (IsTLB_Hit == ERESULT_TYPE_YES) {
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s TLB hit.\n", nCycle, this->cName.c_str(),
           cpAR->GetName().c_str()); // DUONGTRAN uncomment
  } else {
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s TLB miss.\n", nCycle, this->cName.c_str(),
           cpAR->GetName().c_str()); // DUONGTRAN uncomment
  };
#endif

  // Hit. Translate address
  if (IsTLB_Hit == ERESULT_TYPE_YES) {

    // Push tracker
    UPUD upAR_new = new UUD;
    upAR_new->cpAR = Copy_CAxPkt(cpAR); // Original VA
    this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_HIT);

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s, ID %d push MO_tracker. TLBHit\n", nCycle, this->cName.c_str(),
           cpAR->GetName().c_str(), cpAR->GetID());
// this->cpTracker->CheckTracker();
#endif

    // Maintain
    Delete_UD(upAR_new, EUD_TYPE_AR);

    // Push FIFO AR
    UPUD upAR_new2 = new UUD;
    upAR_new2->cpAR = Copy_CAxPkt(cpAR);

    // Get PA
    int nPA = this->GetPA_TLB(nVA);

#ifdef DEBUG_MMU
    assert(nPA >= MIN_ADDR);
    assert(nPA <= MAX_ADDR);
#endif

    // Set PA
    upAR_new2->cpAR->SetAddr(nPA);

    // Encode ID
    nID = nID << 2; // PTW ID is 0x2
    upAR_new2->cpAR->SetID(nID);

#ifdef DEBUG_MMU
    assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif
    this->cpFIFO_AR->Push(upAR_new2,
                          MMU_FIFO_AR_TLB_HIT_LATENCY); // Translated, ID-encoded original

#ifdef DEBUG_MMU
    assert(upAR_new2 != NULL);
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s, ID %d push FIFO_AR. TLBHit\n", nCycle, this->cName.c_str(),
           cpAR->GetName().c_str(), cpAR->GetID());
// cpAR->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upAR_new2, EUD_TYPE_AR);

    // Stat
    this->nHit_AR_TLB++;

    return (ERESULT_TYPE_SUCCESS);
  };

// Miss. Generate AR PTW
#ifdef DEBUG_MMU
  assert(IsTLB_Hit == ERESULT_TYPE_NO); // Miss in level-4
#endif

  // Check in FLPD cache
  EResultType IsTLPD_Hit = this->IsTLPD_Hit(nVA, nCycle); // Check level-3
  EResultType IsSLPD_Hit = this->IsSLPD_Hit(nVA, nCycle); // Check level-2
  EResultType IsFLPD_Hit = this->IsFLPD_Hit(nVA, nCycle); // Check level-1

  // Generate AR PTW
  char cPktName[50];
  CPAxPkt cpAR_new = NULL;

  // int64_t nVA = cpAR->GetAddr();
  int64_t nAddr_PTW = -1;

  // Generate PTW
  if (IsTLPD_Hit == ERESULT_TYPE_YES) { // Hit in level-3

    // Get address for 4th-level table
    nAddr_PTW = GetFourth_PTWAddr(nVA);

    // Set pkt
    sprintf(cPktName, "4th_PTW_for_%s", cpAR->GetName().c_str());
    cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

    //                  nID,       nAddr,      nLen
    // cpAR_new->SetPkt(ARID_PTW,  nAddr_PTW,  0);
    cpAR_new->SetID(ARID_PTW);
    cpAR_new->SetAddr(nAddr_PTW);
    cpAR_new->SetSrcName(this->cName);
    cpAR_new->SetTransType(ETRANS_TYPE_FOURTH_PTW);
    cpAR_new->SetVA(nVA);

#if defined SINGLE_FETCH
    cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes and
                                            // BURST_SIZE 16 bytes
#elif defined HALF_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes and
                                                // BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes and
                                                // BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s generated.\n", nCycle, this->cName.c_str(),
           cPktName); // DUONGTRAN uncomment
// cpAR_new->Display();
#endif

    // Stat
    this->nHit_AR_FLPD++;
    this->nPTW_4th++;

  } else if (IsTLPD_Hit == ERESULT_TYPE_NO) {
    if (IsSLPD_Hit == ERESULT_TYPE_YES) { // Hit in level-2

      // Get address for 3rd-level table
      nAddr_PTW = GetThird_PTWAddr(nVA);

      // Set pkt
      sprintf(cPktName, "3rd_PTW_for_%s", cpAR->GetName().c_str());
      cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

      //                  nID,       nAddr,      nLen
      // cpAR_new->SetPkt(ARID_PTW,  nAddr_PTW,  0);
      cpAR_new->SetID(ARID_PTW);
      cpAR_new->SetAddr(nAddr_PTW);
      cpAR_new->SetSrcName(this->cName);
      cpAR_new->SetTransType(ETRANS_TYPE_THIRD_PTW);
      cpAR_new->SetVA(nVA);

#if defined SINGLE_FETCH
      cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes
                                              // and BURST_SIZE 16 bytes
#elif defined HALF_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s generated.\n", nCycle, this->cName.c_str(),
             cPktName); // DUONGTRAN uncomment
// cpAR_new->Display();
#endif

      // Stat
      this->nHit_AR_FLPD++;
      this->nPTW_3rd++;

    } else if (IsSLPD_Hit == ERESULT_TYPE_NO) {
      if (IsFLPD_Hit == ERESULT_TYPE_YES) { // Hit in level-1

        // Get address for 2nd-level table
        nAddr_PTW = GetSecond_PTWAddr(nVA);

        // Set pkt
        sprintf(cPktName, "2nd_PTW_for_%s", cpAR->GetName().c_str());
        cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

        //                  nID,       nAddr,      nLen
        // cpAR_new->SetPkt(ARID_PTW,  nAddr_PTW,  0);
        cpAR_new->SetID(ARID_PTW);
        cpAR_new->SetAddr(nAddr_PTW);
        cpAR_new->SetSrcName(this->cName);
        cpAR_new->SetTransType(ETRANS_TYPE_SECOND_PTW);
        cpAR_new->SetVA(nVA);

#if defined SINGLE_FETCH
        cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
        cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes
                                                // and BURST_SIZE 16 bytes
#elif defined HALF_FETCH
        cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes
                                                    // and BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
        cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes
                                                    // and BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
        printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s generated.\n", nCycle, this->cName.c_str(),
               cPktName); // DUONGTRAN uncomment
// cpAR_new->Display();
#endif

        // Stat
        this->nHit_AR_FLPD++;
        this->nPTW_2nd++;

      } else if (IsFLPD_Hit == ERESULT_TYPE_NO) {

        // Get address for 1st-level table
        nAddr_PTW = GetFirst_PTWAddr(nVA);

        // Set pkt
        sprintf(cPktName, "1st_PTW_for_%s", cpAR->GetName().c_str());
        cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

        //               nID,       nAddr,      nLen
        cpAR_new->SetPkt(ARID_PTW, nAddr_PTW, 0);
        cpAR_new->SetSrcName(this->cName);
        cpAR_new->SetTransType(ETRANS_TYPE_FIRST_PTW);
        cpAR_new->SetVA(nVA);

#ifdef DEBUG_MMU
        printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s generated.\n", nCycle, this->cName.c_str(),
               cPktName); // DUONGTRAN uncomment
// cpAR_new->Display();
#endif

        // Stat
        this->nPTW_1st++;

      } else {
        assert(0);
      }
    } else {
      assert(0);
    }
  } else {
    assert(0);
  }

  // Push tracker
  UPUD upAR_new = new UUD;
  upAR_new->cpAR = Copy_CAxPkt(cpAR); // Original

  if (IsTLPD_Hit == ERESULT_TYPE_YES) {
    this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_FOURTH_PTW);
  } else if (IsTLPD_Hit == ERESULT_TYPE_NO) {
    if (IsSLPD_Hit == ERESULT_TYPE_YES) {
      this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_THIRD_PTW);
    } else if (IsSLPD_Hit == ERESULT_TYPE_NO) {
      if (IsFLPD_Hit == ERESULT_TYPE_YES) {
        this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_SECOND_PTW);
      } else if (IsFLPD_Hit == ERESULT_TYPE_NO) {
        this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_FIRST_PTW);
      } else {
        assert(0);
      }
    } else {
      assert(0);
    }
  } else {
    assert(0);
  }

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s, ID %d push MO_tracker. TLBMiss\n", nCycle, this->cName.c_str(),
         cpAR->GetName().c_str(), cpAR->GetID());
// this->cpTracker->CheckTracker();
#endif

  // Maintain
  Delete_UD(upAR_new, EUD_TYPE_AR);

  UPUD upAR_new2 = new UUD; // PTW
  upAR_new2->cpAR = cpAR_new;

  // Push FIFO AR
  this->cpFIFO_AR->Push(upAR_new2, MMU_FIFO_AR_TLB_MISS_LATENCY); // PTW

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s, ID %d push FIFO_AR. TLBMiss\n", nCycle, this->cName.c_str(),
         upAR_new2->cpAR->GetName().c_str(), upAR_new2->cpAR->GetID());
// cpAR_new->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

  // Maintain
  Delete_UD(upAR_new2, EUD_TYPE_AR);

  // Stat
  this->nPTW_total++;
  this->IsPTW_AR_ongoing = ERESULT_TYPE_YES;

  return (ERESULT_TYPE_SUCCESS);
};

//--------------------------------
// AR valid
//--------------------------------
// 	Master interface
//--------------------------------
EResultType CMMU::Do_AR_fwd_MI(int64_t nCycle) { // MMU -> Slave

  // Check Tx valid
  if (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_FAIL);
  };

  // Check FIFO AR
  if (this->cpFIFO_AR->IsEmpty() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_FAIL);
  };

  // Debug
  // this->cpFIFO_AR->CheckFIFO();

  // Pop
  UPUD upAR_new = this->cpFIFO_AR->Pop();

  // Check
  if (upAR_new == NULL) {
    return (ERESULT_TYPE_FAIL);
  };

#ifdef DEBUG_MMU
  assert(upAR_new != NULL);
  printf("[Cycle %3ld: %s.Do_AR_fwd_MI] (%s) popped FIFO_AR.\n", nCycle, this->cName.c_str(),
         upAR_new->cpAR->GetName().c_str());
  // this->cpFIFO_AR->CheckFIFO();
  // this->cpFIFO_AR->Display();
  assert(this->cpTx_AR->IsBusy() == ERESULT_TYPE_NO);
#endif

  // Put Tx
  CPAxPkt cpAR_new = upAR_new->cpAR;
  this->cpTx_AR->PutAx(cpAR_new);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AR_fwd_MI] (%s) put Tx_AR.\n", nCycle, this->cName.c_str(),
         cpAR_new->GetName().c_str()); // DUONGTRAN uncomment
// cpAR_new->Display();
// cpAR_new->CheckPkt();
#endif

  // Stat
  this->nAR_MI++;

  // Maintain
  Delete_UD(upAR_new, EUD_TYPE_AR);

  return (ERESULT_TYPE_SUCCESS);
};

//-------------------------------------------------------------------------------------------------------
// AW valid.  AT (Anchor Translation)
// 	TLB lookup
//-------------------------------------------------------------------------------------------------------
// Algorithm "regular"
//		   Regular 	Anchor		Contiguity	Operation
// Allocation
//		1. Hit		-		-		Translate
//		2. Miss		Allocated	Match		Translate
//
//		3. Miss		Allocated	Not match	(1) PTW
//(2) Fill "regular" in TLB
//		4. Miss		Not allocated	Match		(1) PTW, APTW
//(2) Fill "anchor"  in TLB
//		5. Miss		Not allocated	Not match	(1) PTW, APTW
//(2) Fill "regular" in TLB
//
//-------------------------------------------------------------------------------------------------------
// Algorithm "anchor"
//		   Regular 	Anchor		Contiguity	Operation
// Allocation
//		1. -		Allocated	-		Translate
//
//		2. -		Not allocated	-		(1) APTW
//(2) Fill "anchor" in TLB
//-------------------------------------------------------------------------------------------------------
// Operation
// 		0. Get Ax. Check TLB hit
// 		1. If hit, translate address
// 		2. Miss. Suppose "regular" or "anchor". If "anchor"  not in TLB,
// generate "anchor" APTW
// 		3. Miss. Suppose "regular".             If "regular" not in TLB,
// generate "regular" PTW
//-------------------------------------------------------------------------------------------------------
// Note
// 		1. Algorithm same as AR.
// 		   Only difference is to put Tx when hit.
//
//		2. AR and AW share a TLB. Priority between AR and AW is switched
//(to do round-robin arbitration).
//		3. When MMU receives AW, the MMU checks whether hit or miss.
//		4. Then MMU allocates in the tracker.
//		5. In the Do_W_fwd(), we wait for the WLAST. After that, MMU
// provides a hit service (to translate address) or miss service (to generate
// PTW) FIXME
//		6. This routine is independent of W
//		7. This function is similar to AR
//		8. Do we allow multiple outstanding PTW? In this work, we allow
// one PTW because a single master operates a single process.
//		9. When PTW on going for AR, do we allow outstanding AW? In this
// work, we stall when any PTW is on going. FIXME
//-------------------------------------------------------------------------------------------------------
EResultType CMMU::Do_AW_fwd_AT(int64_t nCycle) {

  //----------------------------------------------------
  // 0. Get Ax. Check TLB hit
  //----------------------------------------------------

  // Check Tx valid
  if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check AR priority and busy
  if (this->Is_AR_priority == ERESULT_TYPE_YES and this->cpRx_AR->GetPair()->GetAx() != NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check FIFO AR
  if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid. Get.
  CPAxPkt cpAW = this->cpRx_AW->GetPair()->GetAx();
  if (cpAW == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Get AW info original
  //
  // int  nID = cpAW->GetID();
  // int64_t nVA = cpAW->GetAddr();
  int64_t nVA = cpAW->GetVA();

#ifdef STAT_DETAIL
  int64_t nVPN = GetVPNNum(nVA);
  if (nVPN < this->AR_min_VPN) {
    this->AR_min_VPN = nVPN;
  };
  if (nVPN > this->AR_max_VPN) {
    this->AR_max_VPN = nVPN;
  };
#endif

  // Check MO
  if (this->GetMO_Ax() >= MAX_MO_COUNT) { // AR + AW
    return (ERESULT_TYPE_SUCCESS);
  };

  // // Check ID dependent node doing PTW
  // if (this->cpTracker->IsUDMatchPTW(EUD_TYPE_AR) == ERESULT_TYPE_YES) {
  // // Check any PTW FIXME performance if (this->cpTracker->IsIDMatchPTW(nID,
  // EUD_TYPE_AR) == ERESULT_TYPE_YES) {		// Check ID PTW
  if (this->cpTracker->IsAnyPTW() == ERESULT_TYPE_YES) { // Check any PTW
    // if (this->cpTracker->IsIDMatch(nID, EUD_TYPE_AR) == ERESULT_TYPE_YES) {
    // // Check ID dependent node is outstanding
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check address dependency
  if (this->cpTracker->IsAnyPTW_Match_VPN(nVA) == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  if (this->cpTracker->IsFull() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_FAIL);
  }

#ifdef DEBUG_MMU
  assert(this->cpRx_AW->IsBusy() == ERESULT_TYPE_NO);
#endif

  // Put Rx
  this->cpRx_AW->PutAx(cpAW);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s put Rx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
// cpAW->Display();
#endif

  // Check regular or anchor
  // EResultType IsVA_Regular = this->IsVA_Regular_AT(nVA);
  // EResultType IsVA_Anchor  = this->IsVA_Anchor_AT(nVA);

  // Lookup TLB (regular or anchor)
  EResultType IsTLB_Allocate = this->IsTLB_Allocate_AT(nVA, nCycle);
  // Check anchor allocated
  EResultType IsTLB_Allocate_Anchor = this->IsTLB_Allocate_Anchor_AT(nVA);
  EResultType IsTLB_Contiguity_Match = this->IsTLB_Contiguity_Match_AT(nVA, nCycle);

#ifdef DEBUG_MMU
  if (IsTLB_Allocate == ERESULT_TYPE_YES) {
    printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s TLB allocate VA.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
  } else {
    printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s TLB not allocate VA.\n", nCycle, this->cName.c_str(),
           cpAW->GetName().c_str());
  };

  if (IsTLB_Allocate_Anchor == ERESULT_TYPE_YES) {
    printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s TLB_AT allocate VA.\n", nCycle, this->cName.c_str(),
           cpAW->GetName().c_str());
  } else {
    printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s TLB_AT not allocate VA.\n", nCycle, this->cName.c_str(),
           cpAW->GetName().c_str());
  };

  if (IsTLB_Contiguity_Match == ERESULT_TYPE_YES) {
    printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s TLB_AT contiguity matched.\n", nCycle, this->cName.c_str(),
           cpAW->GetName().c_str());
  } else {
    printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s TLB_AT contiguity not matched.\n", nCycle, this->cName.c_str(),
           cpAW->GetName().c_str());
  };
#endif

  // // Check anchor allocated but not contiguity match
  // EResultType IsAnchor_InTLB_But_Not_ContiguityMatch = ERESULT_TYPE_NO;
  //
  // if (IsTLB_Allocate_Anchor == ERESULT_TYPE_YES and IsTLB_Contiguity_Match ==
  // ERESULT_TYPE_NO) { 	IsAnchor_InTLB_But_Not_ContiguityMatch =
  // ERESULT_TYPE_YES;
  // };

  //-------------------------------------------------
  // 1. Hit. Translate address
  //-------------------------------------------------
  // if (IsTLB_Allocate == ERESULT_TYPE_YES or IsTLB_Contiguity_Match ==
  // ERESULT_TYPE_YES) {  //DUONGTRAN comment
  if ((IsTLB_Allocate == ERESULT_TYPE_YES) ||
      ((IsTLB_Contiguity_Match == ERESULT_TYPE_YES) && (IsTLB_Allocate_Anchor == ERESULT_TYPE_YES))) {

    // Push tracker
    UPUD upAW_new = new UUD;
    upAW_new->cpAR = Copy_CAxPkt(cpAW); // Original VA
    this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_HIT);

#ifdef DEBUG_MMU
// printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s push MO_tracker.\n", nCycle,
// this->cName.c_str(), cpAW->GetName().c_str());
// this->cpTracker->CheckTracker();
#endif

    // Maintain
    Delete_UD(upAW_new, EUD_TYPE_AW);

    // Get PA
    int nPA = this->GetPA_TLB(nVA);

#ifdef DEBUG_MMU
    assert(nPA >= MIN_ADDR);
    assert(nPA <= MAX_ADDR);
#endif

    // Set PA
    cpAW->SetAddr(nPA);

#ifdef DEBUG_MMU
    assert(this->cpTx_AW->IsBusy() == ERESULT_TYPE_NO);
#endif

    // Put Tx
    this->cpTx_AW->PutAx(cpAW);

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_AW_fwd_AT] (%s) put Tx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
// cpAW->Display();
#endif

    // Debug FIXME
    // printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s push FIFO_AR.\n", nCycle,
    // this->cName.c_str(), cpAR->GetName().c_str()); cpAW->Display();
    // this->cpFIFO_AR->Display();
    // this->cpFIFO_AR->CheckFIFO();

    // Stat
    this->nHit_AW_TLB++;
    this->nAW_MI++;

    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert((IsTLB_Allocate == ERESULT_TYPE_NO) &&
         ((IsTLB_Contiguity_Match == ERESULT_TYPE_NO) || (IsTLB_Allocate_Anchor == ERESULT_TYPE_NO)));
#endif

  //-------------------------------------------------
  //    Steps 2,3,..,6 are same as AR.
  //-------------------------------------------------

  //-------------------------------------------------
  // 2. TLB miss. Suppose "regular".
  //    If "regular" not in TLB, generate PTW
  //-------------------------------------------------

  // Miss. Generate "regular" PTW
  // if (IsVA_Regular == ERESULT_TYPE_YES and IsTLB_Allocate == ERESULT_TYPE_NO)
  // {
  if (IsTLB_Allocate == ERESULT_TYPE_NO) {

    // Check FLPD
    EResultType IsTLPD_Hit = this->IsTLPD_Hit(nVA, nCycle);
    EResultType IsSLPD_Hit = this->IsSLPD_Hit(nVA, nCycle);
    EResultType IsFLPD_Hit = this->IsFLPD_Hit(nVA, nCycle);

    // Generate AR PTW
    char cPktName[50];
    CPAxPkt cpAR_new = NULL;

    int64_t nAddr_PTW = -1;

    // Generate "regular" PTW
    if (IsTLPD_Hit == ERESULT_TYPE_YES) { // TLPD hit in level-3

      // Get address for 4th-level table
      nAddr_PTW = GetFourth_PTWAddr(nVA);

      // Set pkt
      sprintf(cPktName, "4th_PTW_for_%s", cpAW->GetName().c_str());
      cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

      //                  nID,       nAddr,      nLen
      // cpAR_new->SetPkt(ARID_PTW,  nAddr_PTW,  0);
      cpAR_new->SetID(ARID_PTW);
      cpAR_new->SetAddr(nAddr_PTW);
      cpAR_new->SetSrcName(this->cName);
      cpAR_new->SetTransType(ETRANS_TYPE_FOURTH_PTW);
      cpAR_new->SetVA(nVA);

#if defined SINGLE_FETCH
      cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes
                                              // and BURST_SIZE 16 bytes
#elif defined HALF_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

      // Stat
      this->nHit_AW_FLPD++;
      this->nPTW_4th++;
    } else if (IsSLPD_Hit == ERESULT_TYPE_YES) { // SLPD hit in level-2

      // Get address for 3rd-level table
      nAddr_PTW = GetThird_PTWAddr(nVA);

      // Set pkt
      sprintf(cPktName, "3rd_PTW_for_%s", cpAW->GetName().c_str());
      cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

      //                  nID,       nAddr,      nLen
      // cpAR_new->SetPkt(ARID_PTW,  nAddr_PTW,  0);
      cpAR_new->SetID(ARID_PTW);
      cpAR_new->SetAddr(nAddr_PTW);
      cpAR_new->SetSrcName(this->cName);
      cpAR_new->SetTransType(ETRANS_TYPE_THIRD_PTW);
      cpAR_new->SetVA(nVA);

#if defined SINGLE_FETCH
      cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes
                                              // and BURST_SIZE 16 bytes
#elif defined HALF_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

      // Stat
      this->nHit_AW_FLPD++;
      this->nPTW_3rd++;
    } else if (IsFLPD_Hit == ERESULT_TYPE_YES) { // FLPD hit

      // Get address for 2nd-level table
      nAddr_PTW = GetSecond_PTWAddr(nVA);

      // Set pkt
      sprintf(cPktName, "2nd_PTW_for_%s", cpAW->GetName().c_str());
      cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

      //                  nID,       nAddr,      nLen
      // cpAR_new->SetPkt(ARID_PTW,  nAddr_PTW,  0);
      cpAR_new->SetID(ARID_PTW);
      cpAR_new->SetAddr(nAddr_PTW);
      cpAR_new->SetSrcName(this->cName);
      cpAR_new->SetTransType(ETRANS_TYPE_SECOND_PTW);
      cpAR_new->SetVA(nVA);

#if defined SINGLE_FETCH
      cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes
                                              // and BURST_SIZE 16 bytes
#elif defined HALF_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

      // Stat
      this->nHit_AW_FLPD++;
      this->nPTW_2nd++;
    } else if (IsFLPD_Hit == ERESULT_TYPE_NO) { // FLPD miss

      // Get address for 1st-level table
      nAddr_PTW = GetFirst_PTWAddr(nVA);

      // Set pkt
      sprintf(cPktName, "1st_PTW_for_%s", cpAW->GetName().c_str());
      cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

      //               nID,       nAddr,      nLen
      cpAR_new->SetPkt(ARID_PTW, nAddr_PTW, 0);
      cpAR_new->SetSrcName(this->cName);
      cpAR_new->SetTransType(ETRANS_TYPE_FIRST_PTW);
      cpAR_new->SetVA(nVA);

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

      // Stat
      this->nPTW_1st++;
    } else {
      assert(0);
    };

    // Push tracker "regular"
    UPUD upAW_new = new UUD;
    upAW_new->cpAW = Copy_CAxPkt(cpAW); // Original

    if (IsTLPD_Hit == ERESULT_TYPE_YES) {
      this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_FOURTH_PTW);
    } else if (IsSLPD_Hit == ERESULT_TYPE_YES) {
      this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_THIRD_PTW);
    } else if (IsFLPD_Hit == ERESULT_TYPE_YES) {
      this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_SECOND_PTW);
    } else if (IsFLPD_Hit == ERESULT_TYPE_NO) {
      this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_FIRST_PTW);
    } else {
      assert(0);
    };

#ifdef DEBUG_MMU
// printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s push MO_tracker.\n", nCycle,
// this->cName.c_str(), cpAW->GetName().c_str());
// this->cpTracker->CheckTracker();
#endif

    // Maintain "regular"
    Delete_UD(upAW_new, EUD_TYPE_AW);

    UPUD upAR_new2 = new UUD;
    upAR_new2->cpAR = cpAR_new; // PTW

    // Push FIFO AR "regular"
    this->cpFIFO_AR->Push(upAR_new2, MMU_FIFO_AR_TLB_MISS_LATENCY);

#ifdef DEBUG_MMU
    // printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s push FIFO_AR.\n", nCycle,
    // this->cName.c_str(), cpAW->GetName().c_str());
    printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain "regular"
    Delete_UD(upAR_new2, EUD_TYPE_AR);

    // Stat
    this->nPTW_total++;
    this->IsPTW_AW_ongoing = ERESULT_TYPE_YES;
  };

  //-------------------------------------------------
  // 3. TLB miss. Suppose "regular" or "anchor".
  //    If "anchor" not in TLB, generate APTW
  //-------------------------------------------------

  // Check "anchor" not in TLB. Generate APTW
  if (IsTLB_Allocate_Anchor == ERESULT_TYPE_NO) {
    // Get VA anchor
    int64_t nVA_anchor = GetVA_Anchor_AT(nVA);

    if (nVA_anchor == nVA) { // DUONGTRAN add this condition to improve AT
      return (ERESULT_TYPE_SUCCESS);
    }

    // Check FLPD
    EResultType IsTLPD_Hit = this->IsTLPD_Hit(nVA_anchor, nCycle);
    // EResultType IsSLPD_Hit = this->IsSLPD_Hit(nVA_anchor, nCycle);
    // EResultType IsFLPD_Hit = this->IsFLPD_Hit(nVA_anchor, nCycle);

    // Generate APTW
    char cPktName[50];
    CPAxPkt cpAR_new = NULL;

    int64_t nAddr_PTW = -1;

    // Generate "anchor" APTW
    if (IsTLPD_Hit == ERESULT_TYPE_YES) { // hit in level-3

      // Get address for 4th-level table
      nAddr_PTW = GetFourth_PTWAddr(nVA_anchor);

      // Set pkt
      sprintf(cPktName, "4th_APTW_for_%s", cpAW->GetName().c_str());
      cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

      //                  nID,       nAddr,      nLen
      // cpAR_new->SetPkt(ARID_APTW, nAddr_PTW,  0);
      cpAR_new->SetID(ARID_APTW);
      cpAR_new->SetAddr(nAddr_PTW);
      cpAR_new->SetSrcName(this->cName);
      cpAR_new->SetTransType(ETRANS_TYPE_FOURTH_APTW);
      cpAR_new->SetVA(nVA_anchor);

#if defined SINGLE_FETCH
      cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes
                                              // and BURST_SIZE 16 bytes
#elif defined HALF_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

      // Stat
      this->nHit_AW_FLPD++;
      this->nAPTW_4th_AT++;

      // Push tracker "anchor"
      UPUD upAW_new = new UUD;
      upAW_new->cpAW = Copy_CAxPkt(cpAW); // Original

      if (IsTLPD_Hit == ERESULT_TYPE_YES) {
        this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_FOURTH_APTW);
      }

      // Maintain "anchor"
      Delete_UD(upAW_new, EUD_TYPE_AW);

      UPUD upAR_new2 = new UUD;
      upAR_new2->cpAR = cpAR_new; // PTW

      // Push FIFO AR "anchor"
      this->cpFIFO_AR->Push(upAR_new2, MMU_FIFO_AR_TLB_MISS_LATENCY);

#ifdef DEBUG_MMU
      // printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s push FIFO_AR.\n", nCycle,
      // this->cName.c_str(), cpAW->GetName().c_str());
      printf("[Cycle %3ld: %s.Do_AW_fwd_AT] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

      // Maintain "anchor"
      Delete_UD(upAR_new2, EUD_TYPE_AR);

      // Stat
      this->nAPTW_total_AT++;
      this->IsPTW_AW_ongoing = ERESULT_TYPE_YES;
    }
  };

  return (ERESULT_TYPE_SUCCESS);
};

//----------------------------------------------------------------------------------------------
// AW valid
//----------------------------------------------------------------------------------------------
//	AR and AW share a TLB. Priority between AR and AW is switched (to do
// round-robin arbitration). 	When MMU receives AW, the MMU checks whether hit
// or miss. 	Then MMU allocates AW in tracker. 	In the Do_W_fwd(), we
// wait for the WLAST. After that, MMU provides a hit service (to translate
// address) or miss service (to generate PTW) FIXME 	This routine is
// independent of W. 	This function is similar to AR.
//----------------------------------------------------------------------------------------------
//	Do we allow multiple outstanding PTW? In this work, we allow one PTW
// because a single master operates a single process. 	When PTW on going for
// AR, do we allow outstanding AW? In this work, we stall when any PTW is on
// going.
// FIXME
//----------------------------------------------------------------------------------------------
EResultType CMMU::Do_AW_fwd(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check Rx_AR
  // FIXME FIXME FIXME When AR accepted, stall AW
  // if (this->cpRx_AR->IsBusy() == ERESULT_TYPE_YES) {
  // 	return (ERESULT_TYPE_SUCCESS);
  // };

  // Check AR priority and busy
  if (this->Is_AR_priority == ERESULT_TYPE_YES and
      this->cpRx_AR->IsBusy() == ERESULT_TYPE_YES) { // AR priority and accepted
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx. Get.
  CPAxPkt cpAW = this->cpRx_AW->GetPair()->GetAx();
  if (cpAW == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Get AW info
  // int nID = cpAW->GetID();
  int64_t nVA = cpAW->GetAddr();

  // Stat
  int64_t nVPN = GetVPNNum(nVA);
  if (nVPN < this->AW_min_VPN) {
    this->AW_min_VPN = nVPN;
  };
  if (nVPN > this->AW_max_VPN) {
    this->AW_max_VPN = nVPN;
  };

  // Check MO
  // if (this->GetMO_Ax() >= MAX_MO_COUNT) {	// AR + AW. Be careful
  //	return (ERESULT_TYPE_SUCCESS);
  // };

  // Check ID-dependent PTW
  if (this->cpTracker->IsAnyPTW() == ERESULT_TYPE_YES) { // Check any PTW FIXME performance FIXME Default
    // if (this->cpTracker->IsIDMatchPTW(nID, EUD_TYPE_AW) == ERESULT_TYPE_YES)
    // {		// Check ID PTW FIXME This is reasonable.

    // if (this->cpTracker->IsUDMatchPTW(EUD_TYPE_AW) == ERESULT_TYPE_YES) {
    // // Check any PTW FIXME Not good performance
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check address-dependent PTW
  if (this->cpTracker->IsAnyPTW_Match_VPN(nVA) == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(this->cpRx_AW->IsBusy() == ERESULT_TYPE_NO);
#endif

  // Put Rx
  this->cpRx_AW->PutAx(cpAW);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) put Rx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
// cpAW->Display();
#endif

  // Lookup TLB
  EResultType IsTLB_Hit = this->IsTLB_Hit(nVA, nCycle);

#ifdef DEBUG_MMU
  if (IsTLB_Hit == ERESULT_TYPE_YES) {
    printf("[Cycle %3ld: %s.Do_AW_fwd] %s TLB hit.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
  } else {
    printf("[Cycle %3ld: %s.Do_AW_fwd] %s TLB miss.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
  };
#endif

  // TLB hit. Translate address
  if (IsTLB_Hit == ERESULT_TYPE_YES) {

    // Push tracker
    UPUD upAW_new = new UUD;
    upAW_new->cpAW = Copy_CAxPkt(cpAW); // Original VA
    this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_HIT);

#ifdef DEBUG_MMU
// printf("[Cycle %3ld: %s.Do_AW_fwd] %s push MO_tracker.\n", nCycle,
// this->cName.c_str(), cpAW->GetName().c_str());
// this->cpTracker->CheckTracker();
#endif

    // Maintain
    Delete_UD(upAW_new, EUD_TYPE_AW);

    // Get PA
    int nPA = this->GetPA_TLB(nVA);

#ifdef DEBUG_MMU
    assert(nPA >= MIN_ADDR);
    assert(nPA <= MAX_ADDR);
#endif

    // Set PA
    cpAW->SetAddr(nPA);

#ifdef DEBUG_MMU
    assert(this->cpTx_AW->IsBusy() == ERESULT_TYPE_NO);
#endif

    // Put Tx
    this->cpTx_AW->PutAx(cpAW);

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) put Tx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
// cpAW->Display();
#endif

    // Stat
    this->nHit_AW_TLB++;
    this->nAW_MI++;

    return (ERESULT_TYPE_SUCCESS);
  };

// TLB miss. Generate AR PTW
#ifdef DEBUG_MMU
  assert(IsTLB_Hit == ERESULT_TYPE_NO); // Miss in level-4
#endif

  // Check FLPD cache
  EResultType IsTLPD_Hit = this->IsTLPD_Hit(nVA, nCycle); // Check level-3
  EResultType IsSLPD_Hit = this->IsSLPD_Hit(nVA, nCycle); // Check level-2
  EResultType IsFLPD_Hit = this->IsFLPD_Hit(nVA, nCycle); // Check level-1

  // Generate AR PTW
  char cPktName[50];
  CPAxPkt cpAR_new = NULL;

  int64_t nAddr_PTW = -1;

  // Generate PTW
  if (IsTLPD_Hit == ERESULT_TYPE_YES) { // Hit in level-3

    sprintf(cPktName, "4th_PTW_for_%s", cpAW->GetName().c_str());

    // Set pkt
    cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

    // Get address 4th-level table
    nAddr_PTW = GetFourth_PTWAddr(nVA);

    //                  nID,       nAddr,      nLen
    // cpAR_new->SetPkt(ARID_PTW,  nAddr_PTW,  0);
    cpAR_new->SetAddr(nAddr_PTW);
    cpAR_new->SetID(ARID_PTW);
    cpAR_new->SetSrcName(this->cName);
    cpAR_new->SetTransType(ETRANS_TYPE_FOURTH_PTW);
    cpAR_new->SetVA(nVA);
#if defined SINGLE_FETCH
    cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes and
                                            // BURST_SIZE 16 bytes
#elif defined HALF_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes and
                                                // BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes and
                                                // BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

    // Stat
    this->nHit_AW_FLPD++;
    this->nPTW_4th++;

  } else if (IsTLPD_Hit == ERESULT_TYPE_NO) {
    if (IsSLPD_Hit == ERESULT_TYPE_YES) { // Hit in level-2

      sprintf(cPktName, "3rd_PTW_for_%s", cpAW->GetName().c_str());

      // Set pkt
      cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

      // Get address 3rd-level table
      nAddr_PTW = GetThird_PTWAddr(nVA);

      //                  nID,       nAddr,      nLen
      // cpAR_new->SetPkt(ARID_PTW,  nAddr_PTW,  0);
      cpAR_new->SetAddr(nAddr_PTW);
      cpAR_new->SetID(ARID_PTW);
      cpAR_new->SetSrcName(this->cName);
      cpAR_new->SetTransType(ETRANS_TYPE_THIRD_PTW);
      cpAR_new->SetVA(nVA);
#if defined SINGLE_FETCH
      cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes
                                              // and BURST_SIZE 16 bytes
#elif defined HALF_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
      cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes
                                                  // and BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

      // Stat
      this->nHit_AW_FLPD++;
      this->nPTW_3rd++;

    } else if (IsSLPD_Hit == ERESULT_TYPE_NO) {
      if (IsFLPD_Hit == ERESULT_TYPE_YES) { // Hit in level-1

        sprintf(cPktName, "2nd_PTW_for_%s", cpAW->GetName().c_str());

        // Set pkt
        cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

        // Get address 2ndd-level table
        nAddr_PTW = GetSecond_PTWAddr(nVA);

        //                  nID,       nAddr,      nLen
        // cpAR_new->SetPkt(ARID_PTW,  nAddr_PTW,  0);
        cpAR_new->SetAddr(nAddr_PTW);
        cpAR_new->SetID(ARID_PTW);
        cpAR_new->SetSrcName(this->cName);
        cpAR_new->SetTransType(ETRANS_TYPE_SECOND_PTW);
        cpAR_new->SetVA(nVA);
#if defined SINGLE_FETCH
        cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
        cpAR_new->SetLen(MAX_BURST_LENGTH - 1); // MAX_BURST_LENGTH 4 when MAX_TRANS_SIZE 64 bytes
                                                // and BURST_SIZE 16 bytes
#elif defined HALF_FETCH
        cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1); // BURST_LENGTH     2 when MAX_TRANS_SIZE 64 bytes
                                                    // and BURST_SIZE 16 bytes
#elif defined QUARTER_FETCH
        cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1); // BURST_LENGTH     1 when MAX_TRANS_SIZE 64 bytes
                                                    // and BURST_SIZE 16 bytes
#endif

#ifdef DEBUG_MMU
        printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

        // Stat
        this->nHit_AW_FLPD++;
        this->nPTW_2nd++;

      } else if (IsFLPD_Hit == ERESULT_TYPE_NO) {

        sprintf(cPktName, "1st_PTW_for_%s", cpAW->GetName().c_str());

        // Set pkt
        cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

        // Get address 1st-level page table
        nAddr_PTW = GetFirst_PTWAddr(nVA);

        //               nID,       nAddr,      nLen
        cpAR_new->SetPkt(ARID_PTW, nAddr_PTW, 0);
        cpAR_new->SetSrcName(this->cName);
        cpAR_new->SetTransType(ETRANS_TYPE_FIRST_PTW);
        cpAR_new->SetVA(nVA);

#ifdef DEBUG_MMU
        printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

        // Stat
        this->nPTW_1st++;

      } else {
        assert(0);
      }
    } else {
      assert(0);
    }
  } else {
    assert(0);
  }

  // Push tracker PTW
  UPUD upAW_new = new UUD;
  upAW_new->cpAW = Copy_CAxPkt(cpAW); // Original

  if (IsTLPD_Hit == ERESULT_TYPE_YES) {
    this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_FOURTH_PTW);
  } else if (IsTLPD_Hit == ERESULT_TYPE_NO) {
    if (IsSLPD_Hit == ERESULT_TYPE_YES) {
      this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_THIRD_PTW);
    } else if (IsSLPD_Hit == ERESULT_TYPE_NO) {
      if (IsFLPD_Hit == ERESULT_TYPE_YES) {
        this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_SECOND_PTW);
      } else if (IsFLPD_Hit == ERESULT_TYPE_NO) {
        this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_FIRST_PTW);
      } else {
        assert(0);
      }
    } else {
      assert(0);
    }
  } else {
    assert(0);
  }

  // W control
  // this->IsPTW_AW_ongoing = ERESULT_TYPE_YES;

#ifdef DEBUG_MMU
// printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) push MO_tracker.\n", nCycle,
// this->cName.c_str(), cpAW->GetName().c_str());
// this->cpTracker->CheckTracker();
#endif

  // Maintain
  Delete_UD(upAW_new, EUD_TYPE_AW);

  // Push FIFO AR
  UPUD upAR_new = new UUD;
  upAR_new->cpAR = cpAR_new; // PTW

#ifdef DEBUG_MMU
  assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
  assert(upAR_new != NULL);
#endif

  this->cpFIFO_AR->Push(upAR_new, MMU_FIFO_AR_TLB_MISS_LATENCY); // PTW

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AW_fwd] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cpAR_new->GetName().c_str());
// cpAR_new->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

  // Maintain
  Delete_UD(upAR_new, EUD_TYPE_AR);

  // Stat
  this->nPTW_total++;
  this->IsPTW_AW_ongoing = ERESULT_TYPE_YES;

  return (ERESULT_TYPE_SUCCESS);
};

// W valid SI
EResultType CMMU::Do_W_fwd_SI(int64_t nCycle) {

  // Check FIFO_W
  if (this->cpFIFO_W->IsFull() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid. Get.
  CPWPkt cpW = this->cpRx_W->GetPair()->GetW();
  if (cpW == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Put Rx
  this->cpRx_W->PutW(cpW);

#ifdef DEBUG_MMU
  if (cpW->IsLast() == ERESULT_TYPE_YES) {
    // printf("[Cycle %3ld: %s.Do_W_fwd_SI] (%s) WLAST put Rx_W.\n", nCycle,
    // this->cName.c_str(), cpW->GetName().c_str());
  } else {
    // printf("[Cycle %3ld: %s.Do_W_fwd_SI] (%s) put Rx_W.\n", nCycle,
    // this->cName.c_str(), cpW->GetName().c_str());
  };
#endif

  // Push W
  UPUD upW_new = new UUD;
  upW_new->cpW = Copy_CWPkt(cpW);

  // Push W
  this->cpFIFO_W->Push(upW_new, 0);

#ifdef DEBUG_MMU
  if (cpW->IsLast() == ERESULT_TYPE_YES) {
    // printf("[Cycle %3ld: %s.Do_W_fwd_SI] (%s) WLAST push FIFO_W.\n", nCycle,
    // this->cName.c_str(), upW_new->cpW->GetName().c_str());
  } else {
    // printf("[Cycle %3ld: %s.Do_W_fwd_SI] (%s) push FIFO_W.\n", nCycle,
    // this->cName.c_str(), upW_new->cpW->GetName().c_str());
  };
// this->cpFIFO_W->Display();
// this->cpFIFO_W->CheckFIFO();
#endif

  // Maintain
  Delete_UD(upW_new, EUD_TYPE_W);

  return (ERESULT_TYPE_SUCCESS);
};

// W valid MI
EResultType CMMU::Do_W_fwd_MI(int64_t nCycle) {

  // Check Tx
  if (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check FIFO_W
  if (this->cpFIFO_W->IsEmpty() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Get
  CPWPkt cpW = this->cpFIFO_W->GetTop()->cpW;
  int nID = cpW->GetID();

  // Check MO tracker
  if (this->cpTracker->GetNode(nID, EUD_TYPE_AW) == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(cpW != NULL);
  assert(this->cpTracker->GetNode(nID, EUD_TYPE_AW) != NULL);
#endif

  // Check state
  ECacheStateType eState = this->cpTracker->GetNode(nID, EUD_TYPE_AW)->eState;

  // Hit. Translate address. Put Ax. Put W.
  if (eState == ECACHE_STATE_TYPE_HIT) { // FIXME Be careful. Progress W only when AW is
                                         // doing HIT. This makes W dependent on AW.

    // Pop
    UPUD upW_new = this->cpFIFO_W->Pop();

#ifdef DEBUG_MMU
    assert(upW_new != NULL);
// this->cpFIFO_W->CheckFIFO();
// this->cpFIFO_W->Display();
#endif

    // Get W
    CPWPkt cpW_new = upW_new->cpW;

    // Debug
    // cpW_new->CheckPkt();

    // Put W
    this->cpTx_W->PutW(cpW_new);

#ifdef DEBUG_MMU
    if (cpW_new->IsLast() == ERESULT_TYPE_YES) {
      // printf("[Cycle %3ld: %s.Do_W_fwd_MI] (%s) WLAST put Tx_W.\n", nCycle,
      // this->cName.c_str(), cpW_new->GetName().c_str());
    } else {
      // printf("[Cycle %3ld: %s.Do_W_fwd_MI] (%s) put Tx_W.\n", nCycle,
      // this->cName.c_str(), cpW_new->GetName().c_str());
    };
#endif

    // Maintain
    Delete_UD(upW_new, EUD_TYPE_W);
  };

  return (ERESULT_TYPE_SUCCESS);
};

// AR ready
EResultType CMMU::Do_AR_bwd(int64_t nCycle) {

  // Check Rx
  if (this->cpRx_AR->IsBusy() == ERESULT_TYPE_NO) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(this->cpRx_AR->IsBusy() == ERESULT_TYPE_YES);
#endif

  // Set ready
  this->cpRx_AR->SetAcceptResult(ERESULT_TYPE_ACCEPT);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AR_bwd] (%s) is handshaked at SI.\n", nCycle, this->cName.c_str(),
         this->cpRx_AR->GetAx()->GetName().c_str());
#endif

  // MO stat
  this->Increase_MO_AR();
  this->Increase_MO_Ax();

  // Stat
  this->nAR_SI++;

  // Stat Max MO
  if (this->nMO_AR > this->nMax_MO_AR) {
    this->nMax_MO_AR = this->nMO_AR;
  };
  if (this->nMO_Ax > this->nMax_MO_Ax) {
    this->nMax_MO_Ax = this->nMO_Ax;
  };

  return (ERESULT_TYPE_SUCCESS);
};

// AW ready
EResultType CMMU::Do_AW_bwd(int64_t nCycle) {

  // Check Rx
  if (this->cpRx_AW->IsBusy() == ERESULT_TYPE_NO) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(this->cpRx_AW->IsBusy() == ERESULT_TYPE_YES);
#endif

  // Set ready
  this->cpRx_AW->SetAcceptResult(ERESULT_TYPE_ACCEPT);

#ifdef DEBUG_MMU
  printf("[Cycle %3ld: %s.Do_AW_bwd] (%s) is handshaked at SI.\n", nCycle, this->cName.c_str(),
         this->cpRx_AW->GetAx()->GetName().c_str());
#endif

  // MO stat
  this->Increase_MO_AW();
  this->Increase_MO_Ax();

  // Stat
  this->nAW_SI++;

  // Stat Max MO
  if (this->nMO_AW > this->nMax_MO_AW) {
    this->nMax_MO_AW = this->nMO_AW;
  };
  if (this->nMO_Ax > this->nMax_MO_Ax) {
    this->nMax_MO_Ax = this->nMO_Ax;
  };

  return (ERESULT_TYPE_SUCCESS);
};

// W ready
EResultType CMMU::Do_W_bwd(int64_t nCycle) {

  // Check Rx
  if (this->cpRx_W->IsBusy() == ERESULT_TYPE_YES) {
    // Set ready
    this->cpRx_W->SetAcceptResult(ERESULT_TYPE_ACCEPT);
  };

  return (ERESULT_TYPE_SUCCESS);
};

//--------------------------------------------------------------------------
// R valid
//--------------------------------------------------------------------------
//	1. MMU receives R transaction
//	2. If not PTW, send to SI
//	3. If 4th PTW, (1) Update TLB. (2) Translate address. (3) Send to MI
//	4. If 3rd PTW, (1) Update FLPD. (2) Generate 4th PTW
//	5. If 2nd PTW, (1) Update FLPD. (2) Generate 3rd PTW
//	6. If 1st PTW, (1) Update FLPD. (2) Generate 2nd PTW
//--------------------------------------------------------------------------
EResultType CMMU::Do_R_fwd(int64_t nCycle) {

  //------------------------------
  // 1. MMU receives R transaction
  //------------------------------
  // Check Tx valid
  if (this->cpTx_R->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid
  CPRPkt cpR = this->cpRx_R->GetPair()->GetR();
  if (cpR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(this->cpRx_R->GetPair()->IsBusy() == ERESULT_TYPE_YES);
#endif

  // Check FIFO AR PTW
  if (cpR->GetID() == ARID_PTW and this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Put Rx
  this->cpRx_R->PutR(cpR);

#ifdef DEBUG_MMU
  assert(cpR != NULL);
#endif

  //------------------------------
  // 2. If not PTW, forward to SI
  //------------------------------
  int nID = cpR->GetID();
  if (nID != ARID_PTW) {

#ifdef DEBUG_MMU
    if (cpR->IsLast() == ERESULT_TYPE_YES) {
      printf("[Cycle %3ld: %s.Do_R_fwd] (%s) RID 0x%x RLAST put Rx_R.\n", nCycle, this->cName.c_str(),
             cpR->GetName().c_str(), cpR->GetID());
      // cpR->Display();
    } else {
      printf("[Cycle %3ld: %s.Do_R_fwd] (%s) RID 0x%x put Rx_R.\n", nCycle, this->cName.c_str(), cpR->GetName().c_str(),
             cpR->GetID());
      // cpR->Display();
    };
#endif

    // DUONG fix Physical Cache check old ID
    UPUD cpR_new = new UUD;
    cpR_new->cpR = Copy_CRPkt(cpR);

    // Decode ID
    nID = nID >> 2;
    cpR_new->cpR->SetID(nID);

    // Put Tx
    this->cpTx_R->PutR(cpR_new->cpR);
    Delete_UD(cpR_new, EUD_TYPE_R);

    return (ERESULT_TYPE_SUCCESS);
  };

  // Check RLAST PTW
  if (cpR->IsLast() != ERESULT_TYPE_YES) { // FIXME wait RLAST
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(cpR->IsLast() == ERESULT_TYPE_YES);
  assert(cpR->GetID() == ARID_PTW);
  printf("[Cycle %3ld: %s.Do_R_fwd] (%s) RID 0x%x RLAST PTW put Rx_R.\n", nCycle, this->cName.c_str(),
         cpR->GetName().c_str(), cpR->GetID());
#endif

  // Get Ax info
  int64_t nVA = -1;

  // Get tracker node first matching PTW	FIXME Careful when PF
  SPLinkedCUD spNode = this->cpTracker->GetNode(ECACHE_STATE_TYPE_FIRST_PTW);
  if (spNode == NULL) {
    spNode = this->cpTracker->GetNode(ECACHE_STATE_TYPE_SECOND_PTW);
    if (spNode == NULL) {
      spNode = this->cpTracker->GetNode(ECACHE_STATE_TYPE_THIRD_PTW);
      if (spNode == NULL) {
        spNode = this->cpTracker->GetNode(ECACHE_STATE_TYPE_FOURTH_PTW);
      }
    }
  };

#ifdef DEBUG_MMU
  assert(spNode != NULL);
#endif

  // Get Ax info original
  CPAxPkt cpAR = NULL;
  CPAxPkt cpAW = NULL;
  EUDType eUDType = spNode->eUDType;
  if (eUDType == EUD_TYPE_AR) {
    cpAR = spNode->upData->cpAR;
    nID = spNode->upData->cpAR->GetID();
    nVA = spNode->upData->cpAR->GetAddr();
  } else if (eUDType == EUD_TYPE_AW) {
    cpAW = spNode->upData->cpAW;
    nID = spNode->upData->cpAW->GetID();
    nVA = spNode->upData->cpAW->GetAddr();
  } else {
    assert(0);
  };

#ifdef DEBUG_MMU
  assert(nVA != -1);
  assert(nID != -1);
  assert(eUDType != EUD_TYPE_UNDEFINED);
#endif
  //---------------------------------------------------------------------
  // 3. If 4th PTW, (1) Update TLB. (2) Translate address. (3) Send to MI.
  //---------------------------------------------------------------------
  if (spNode->eState == ECACHE_STATE_TYPE_FOURTH_PTW) { // PTW finished

// Update TLB
#if defined SINGLE_FETCH
    this->FillTLB_SF(nVA, nCycle);
#elif defined BLOCK_FETCH
#ifdef CAMB_ENABLE
    this->FillTLB_CAMB(nVA, nCycle);
#elif defined RCPT_ENABLE
    this->FillTLB_RCPT(nVA, nCycle);
#elif defined cRCPT_ENABLE
    this->FillTLB_cRCPT(nVA, nCycle);
#else
    this->FillTLB_BF(nVA, nCycle);
#endif
#elif defined HALF_FETCH
#ifdef CAMB_ENABLE
    this->FillTLB_CAMB(nVA, nCycle);
#elif defined RCPT_ENABLE
    this->FillTLB_RCPT(nVA, nCycle);
#elif defined cRCPT_ENABLE
    this->FillTLB_cRCPT(nVA, nCycle);
#else
    this->FillTLB_BF(nVA, nCycle);
#endif
#elif defined QUARTER_FETCH
#ifdef CAMB_ENABLE
#ifdef CAMB_WRITE_SF_ENABLE // SF for AW
    if (eUDType == EUD_TYPE_AW) {
      this->FillTLB_SF(nVA, nCycle);
    } else {
      this->FillTLB_CAMB(nVA, nCycle); // QF for AR
    };
#else
    this->FillTLB_CAMB(nVA, nCycle);
#endif
#elif defined RCPT_ENABLE
    this->FillTLB_RCPT(nVA, nCycle);
#elif defined cRCPT_ENABLE
    this->FillTLB_cRCPT(nVA, nCycle);
#else
    this->FillTLB_BF(nVA, nCycle);
#endif
#endif

    // Update tracker state
    // 	(1) Search head node matching ID, UDType, state
    // 	(2) Update state
    this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_FOURTH_PTW, ECACHE_STATE_TYPE_HIT);

    // Translate addr
    int nPA = GetPA_TLB(nVA);

#ifdef DEBUG_MMU
    assert(nPA >= MIN_ADDR);
    assert(nPA <= MAX_ADDR);
#endif

    // Set transaction info
    if (eUDType == EUD_TYPE_AR) {

#ifdef DEBUG_MMU
      assert(cpAR != NULL);
#endif

      // Encode ID
      cpAR->SetID(nID << 2);
      cpAR->SetAddr(nPA);

      // Push FIFO AR
      UPUD upAR_new = new UUD;
      upAR_new->cpAR = Copy_CAxPkt(cpAR); // Translated original

#ifdef DEBUG_MMU
      assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif

      this->cpFIFO_AR->Push(upAR_new, MMU_FIFO_AR_PTW_FINISH_LATENCY);

#ifdef DEBUG_MMU
      assert(upAR_new != NULL);
      printf("[Cycle %3ld: %s.Do_R_fwd] %s, ID %d push FIFO_AR.\n", nCycle, this->cName.c_str(),
             cpAR->GetName().c_str(), cpAR->GetID());
// cpAR->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

      // Maintain
      Delete_UD(upAR_new, EUD_TYPE_AR);

      // Stat
      this->IsPTW_AR_ongoing = ERESULT_TYPE_NO;
    } else if (eUDType == EUD_TYPE_AW) {

#ifdef DEBUG_MMU
      assert(cpAW != NULL);
#endif

      cpAW->SetAddr(nPA);

#ifdef DEBUG_MMU
      assert(this->cpTx_AW->IsBusy() == ERESULT_TYPE_NO);
#endif

      // Put Tx_AW
      this->cpTx_AW->PutAx(cpAW);

      // Stat
      this->nAW_MI++;

      // W control
      this->IsPTW_AW_ongoing = ERESULT_TYPE_NO;
    } else {
      assert(0);
    };
  };

  //-----------------------------------------------------
  // 4. If 3rd PTW, (1) Update FLPD. (2) Generate 4th PTW
  //-----------------------------------------------------
  if (spNode->eState == ECACHE_STATE_TYPE_THIRD_PTW) { // PTW not finished

    // Generate AR PTW
    char cPktName[50];
    if (eUDType == EUD_TYPE_AR) {
      sprintf(cPktName, "4th_PTW_for_%s", cpAR->GetName().c_str());
    } else if (eUDType == EUD_TYPE_AW) {
      sprintf(cPktName, "4th_PTW_for_%s", cpAW->GetName().c_str());
    } else {
      assert(0);
    };

    // Get address 4th-level page table
    int64_t nAddr_FourthPTW = GetFourth_PTWAddr(nVA);

    // Generate PTW
    CPAxPkt cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

    //                  nID,       nAddr,           nLen
    // cpAR_new->SetPkt(ARID_PTW,  nAddr_SecondPTW, 0);
    cpAR_new->SetID(ARID_PTW);
    cpAR_new->SetAddr(nAddr_FourthPTW);
    cpAR_new->SetSrcName(this->cName);
    cpAR_new->SetTransType(ETRANS_TYPE_FOURTH_PTW);
    cpAR_new->SetVA(nVA);
#if defined SINGLE_FETCH
    cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH - 1);
#elif defined HALF_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1);
#elif defined QUARTER_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1);
#endif

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_R_fwd] (%s) generated for fourth PTW.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

    // Push FIFO AR
    UPUD upAR_new = new UUD;
    upAR_new->cpAR = cpAR_new;

#ifdef DEBUG_MMU
    assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif

    this->cpFIFO_AR->Push(upAR_new, MMU_FIFO_AR_4TH_PTW_LATENCY); // PTW

#ifdef DEBUG_MMU
    assert(upAR_new != NULL);
    printf("[Cycle %3ld: %s.Do_R_fwd] %s push FIFO_AR.\n", nCycle, this->cName.c_str(),
           cpAR_new->GetName().c_str()); // DUONGTRAN cpAR  --> cpAR_new
// cpAR->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upAR_new, EUD_TYPE_AR);

    // Get victim entry FLPD
    int nVictim_FLPD = GetVictim_FLPD();
    int64_t nVPN = nVA >> BIT_2MB_PAGE; // 2MB page. BIT_2MB_PAGE 21

    // Update FLPD
    this->FillTLPD(nVictim_FLPD, nVPN, nCycle);

    // Update tracker state
    // 	(1) Search head node matching ID, UDType, state
    // 	(2) Update state
    this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_THIRD_PTW, ECACHE_STATE_TYPE_FOURTH_PTW);

    // Stat
    this->nPTW_total++;
    this->nPTW_4th++;
  };

  //-----------------------------------------------------
  // 5. If 2nd PTW, (1) Update FLPD. (2) Generate 3rd PTW
  //-----------------------------------------------------
  if (spNode->eState == ECACHE_STATE_TYPE_SECOND_PTW) { // PTW not finished

    // Generate AR PTW
    char cPktName[50];
    if (eUDType == EUD_TYPE_AR) {
      sprintf(cPktName, "3rd_PTW_for_%s", cpAR->GetName().c_str());
    } else if (eUDType == EUD_TYPE_AW) {
      sprintf(cPktName, "3rd_PTW_for_%s", cpAW->GetName().c_str());
    } else {
      assert(0);
    };

    // Get address 2nd-level page table
    int64_t nAddr_ThirdPTW = GetThird_PTWAddr(nVA);

    // Generate PTW
    CPAxPkt cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

    //                  nID,       nAddr,           nLen
    // cpAR_new->SetPkt(ARID_PTW,  nAddr_SecondPTW, 0);
    cpAR_new->SetID(ARID_PTW);
    cpAR_new->SetAddr(nAddr_ThirdPTW);
    cpAR_new->SetSrcName(this->cName);
    cpAR_new->SetTransType(ETRANS_TYPE_THIRD_PTW);
    cpAR_new->SetVA(nVA);
#if defined SINGLE_FETCH
    cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH - 1);
#elif defined HALF_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1);
#elif defined QUARTER_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1);
#endif

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_R_fwd] (%s) generated for third PTW.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

    // Push FIFO AR
    UPUD upAR_new = new UUD;
    upAR_new->cpAR = cpAR_new;

#ifdef DEBUG_MMU
    assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif

    this->cpFIFO_AR->Push(upAR_new, MMU_FIFO_AR_3RD_PTW_LATENCY); // PTW

#ifdef DEBUG_MMU
    assert(upAR_new != NULL);
    printf("[Cycle %3ld: %s.Do_R_fwd] %s push FIFO_AR.\n", nCycle, this->cName.c_str(),
           cpAR_new->GetName().c_str()); // DUONGTRAN cpAR  --> cpAR_new
// cpAR->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upAR_new, EUD_TYPE_AR);

    // Get victim entry FLPD
    int nVictim_FLPD = GetVictim_FLPD();
    int64_t nVPN = nVA >> BIT_1GB_PAGE; // 1GB page. BIT_1GB_PAGE 30

    // Update FLPD
    this->FillSLPD(nVictim_FLPD, nVPN, nCycle);

    // Update tracker state
    // 	(1) Search head node matching ID, UDType, state
    // 	(2) Update state
    this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_SECOND_PTW, ECACHE_STATE_TYPE_THIRD_PTW);

    // Stat
    this->nPTW_total++;
    this->nPTW_3rd++;
  };

  //-----------------------------------------------------
  // 6. If 1st PTW, (1) Update FLPD. (2) Generate 2nd PTW
  //-----------------------------------------------------
  if (spNode->eState == ECACHE_STATE_TYPE_FIRST_PTW) { // PTW not finished

    // Generate AR PTW
    char cPktName[50];
    if (eUDType == EUD_TYPE_AR) {
      sprintf(cPktName, "2nd_PTW_for_%s", cpAR->GetName().c_str());
    } else if (eUDType == EUD_TYPE_AW) {
      sprintf(cPktName, "2nd_PTW_for_%s", cpAW->GetName().c_str());
    } else {
      assert(0);
    };

    // Get address 2nd-level page table
    int64_t nAddr_SecondPTW = GetSecond_PTWAddr(nVA);

    // Generate PTW
    CPAxPkt cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

    //                  nID,       nAddr,           nLen
    // cpAR_new->SetPkt(ARID_PTW,  nAddr_SecondPTW, 0);
    cpAR_new->SetID(ARID_PTW);
    cpAR_new->SetAddr(nAddr_SecondPTW);
    cpAR_new->SetSrcName(this->cName);
    cpAR_new->SetTransType(ETRANS_TYPE_SECOND_PTW);
    cpAR_new->SetVA(nVA);
#if defined SINGLE_FETCH
    cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH - 1);
#elif defined HALF_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1);
#elif defined QUARTER_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1);
#endif

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_R_fwd] (%s) generated for second PTW.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

    // Push FIFO AR
    UPUD upAR_new = new UUD;
    upAR_new->cpAR = cpAR_new;

#ifdef DEBUG_MMU
    assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif

    this->cpFIFO_AR->Push(upAR_new, MMU_FIFO_AR_2ND_PTW_LATENCY); // PTW

#ifdef DEBUG_MMU
    assert(upAR_new != NULL);
    printf("[Cycle %3ld: %s.Do_R_fwd] %s push FIFO_AR.\n", nCycle, this->cName.c_str(),
           cpAR_new->GetName().c_str()); // DUONGTRAN cpAR  --> cpAR_new
// cpAR->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upAR_new, EUD_TYPE_AR);

    // Get victim entry FLPD
    int nVictim_FLPD = GetVictim_FLPD();
    int64_t nVPN = nVA >> BIT_512GB_PAGE; // 512GB page. BIT_512GB_PAGE 39

    // Update FLPD
    this->FillFLPD(nVictim_FLPD, nVPN, nCycle);

    // Update tracker state
    // 	(1) Search head node matching ID, UDType, state
    // 	(2) Update state
    this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_FIRST_PTW, ECACHE_STATE_TYPE_SECOND_PTW);

    // Stat
    this->nPTW_total++;
    this->nPTW_2nd++;
  };

  return (ERESULT_TYPE_SUCCESS);
};

//----------------------------------------------------------------------------------------------------------------------
// R valid. AT
//----------------------------------------------------------------------------------------------------------------------
//	Algorithm "regular"
//		   Regular	Anchor		Contiguity	Operation
// Allocation			Tracker pop
//
//		1. Miss		Allocated	Not match	(1) PTW
//(2) Fill "regular" in TLB	-
//		2. Miss		Not allocated	Match		(1) PTW, APTW
//(2) Fill "anchor"  in TLB	PTW "regular"
//		3. Miss		Not allocated	Not match	(1) PTW, APTW
//(2) Fill "regular" in TLB	APTW "anchor"
//
//----------------------------------------------------------------------------------------------------------------------
//	Algorithm "anchor"
//		  Regular	Anchor		Contiguity	Operation
// Allocation
//
//		1. -		Not allocated	-		(1) APTW
//(2) Fill "anchor"  in TLB	-
//----------------------------------------------------------------------------------------------------------------------
// 	Operation
//		1. MMU receives R transaction
//		2. If not (A)PTW, send to SI
//
//		   Get PTW info.
//		3. (3.1) If "1st APTW", update FLPD. Generate "2nd APTW".
//		   (3.2) If "2nd APTW", update FLPD. Generate "3rd APTW".
//		   (3.3) If "3rd APTW", update FLPD. Generate "4th APTW".
//
//		4. (4.1) If "1st PTW",  update FLPD. Generate "2nd PTW".
//		   (4.2) If "2nd PTW",  update FLPD. Generate "3rd PTW".
//		   (4.3) If "3rd PTW",  update FLPD. Generate "4th PTW".
//
//		5. If "4th APTW", update tracker state "DONE".
//		   Check contiguity match.
//
//		   (5.1) If match, update TLB.
//		         Check "pair PTW" in tracker.
//		   	 (5.1a) If there is no pair, translate address. Send to
// MI.
//
//		   (5.2) Otherwise, wait "4th PTW" finish.
//
//		6. If "4th PTW", check "pair APTW".
//		   (6.1) If there is no "pair APTW" in tracker, update TLB.
// Translate address. Send to MI.
//
//		   (6.2) If there is "pair APTW", check "4th APTW" finished.
//		       Check APTW contiguity match.
//		       (6.2a) If     match, do not update TLB for PTW. Translate
// address. Send to MI. Pop APTW tracker. 		       (6.2b) If not
// match,        update TLB for PTW. Translate address. Send to MI. Pop APTW
// tracker.
//----------------------------------------------------------------------------------------------------------------------
EResultType CMMU::Do_R_fwd_AT(int64_t nCycle) {

  //------------------------------
  // 1. MMU receives R transaction
  //------------------------------
  // Check Tx valid
  if (this->cpTx_R->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid. Get.
  CPRPkt cpR = this->cpRx_R->GetPair()->GetR();
  if (cpR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(this->cpRx_R->GetPair()->IsBusy() == ERESULT_TYPE_YES);
#endif

  // Check FIFO AR PTW
  if (cpR->GetID() == ARID_PTW or cpR->GetID() == ARID_APTW) {
    if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
      return (ERESULT_TYPE_SUCCESS);
    };
  };

  // Put Rx
  this->cpRx_R->PutR(cpR);
#ifdef DEBUG_MMU
  assert(cpR != NULL);
#endif

#ifdef DEBUG_MMU
  if (cpR->IsLast() == ERESULT_TYPE_YES) {
    printf("[Cycle %3ld: %s.Do_R_fwd_AT] (%s) RID 0x%x RLAST put Rx_R.\n", nCycle, this->cName.c_str(),
           cpR->GetName().c_str(), cpR->GetID());
    // cpR->Display();
  } else {
    printf("[Cycle %3ld: %s.Do_R_fwd_AT] (%s) RID 0x%x put Rx_R.\n", nCycle, this->cName.c_str(),
           cpR->GetName().c_str(), cpR->GetID());
    // cpR->Display();
  };
#endif

  //------------------------------
  // 2. If not (A)PTW, forward to SI
  //------------------------------
  int nID = cpR->GetID();
  if (nID != ARID_PTW and nID != ARID_APTW) {

    // ID decode
    nID = nID >> 2;
    cpR->SetID(nID);

    // Put Tx
    this->cpTx_R->PutR(cpR);

    return (ERESULT_TYPE_SUCCESS);
  };

  // Check RLAST PTW
  if (cpR->IsLast() != ERESULT_TYPE_YES) { // FIXME wait RLAST
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(cpR->IsLast() == ERESULT_TYPE_YES);
  assert(nID == ARID_PTW or nID == ARID_APTW);
#endif

  //------------------------------
  // 3.0. Get PTW info
  //------------------------------
  // Get Ax info
  int64_t nVA = -1;

  // Get tracker node first matching (A)PTW
  SPLinkedCUD spNode = NULL;

  if (nID == ARID_PTW) {
    spNode = this->cpTracker->GetNode_PTW();
  } else if (nID == ARID_APTW) {
    spNode = this->cpTracker->GetNode_APTW_AT();
  } else {
    assert(0);
  };
  // SPLinkedCUD spNode = this->cpTracker->GetNode(nID);
  // SPLinkedCUD spNode = this->cpTracker->GetNode_anyPTW();

  // SPLinkedCUD spNode = this->cpTracker->GetNode(ECACHE_STATE_TYPE_FIRST_PTW);
  // if (spNode == NULL) {
  // 	spNode = this->cpTracker->GetNode(ECACHE_STATE_TYPE_SECOND_PTW);
  // };

#ifdef DEBUG_MMU
  assert(spNode != NULL);
#endif

  // Get original Ax info
  CPAxPkt cpAR = NULL;
  CPAxPkt cpAW = NULL;
  EUDType eUDType = spNode->eUDType;
  if (eUDType == EUD_TYPE_AR) {
    cpAR = spNode->upData->cpAR;
    nID = spNode->upData->cpAR->GetID();
    nVA = spNode->upData->cpAR->GetAddr();
  } else if (eUDType == EUD_TYPE_AW) {
    cpAW = spNode->upData->cpAW;
    nID = spNode->upData->cpAW->GetID();
    nVA = spNode->upData->cpAW->GetAddr();
  } else {
    assert(0);
  };

#ifdef DEBUG_MMU
  assert(nVA >= 0);
  assert(nID >= 0);
  assert(eUDType != EUD_TYPE_UNDEFINED);
#endif

  //------------------------------------------------------
  // 4.1 If "1st PTW", update FLPD if miss. Generate "2nd PTW".
  //------------------------------------------------------
  if (spNode->eState == ECACHE_STATE_TYPE_FIRST_PTW) { // Doing PTW

    // Debug
    // assert (nID == ARID_PTW);  // FIXME

    // Generate 2nd PTW
    char cPktName[50];
    if (eUDType == EUD_TYPE_AR) {
      sprintf(cPktName, "2nd_PTW_for_%s", cpAR->GetName().c_str());
    } else if (eUDType == EUD_TYPE_AW) {
      sprintf(cPktName, "2nd_PTW_for_%s", cpAW->GetName().c_str());
    } else {
      assert(0);
    };

    // Get address 2nd-level page table
    int64_t nAddr_SecondPTW = GetSecond_PTWAddr(nVA);

    // Generate PTW
    CPAxPkt cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

    //                  nID,       nAddr,           nLen
    // cpAR_new->SetPkt(ARID_PTW,  nAddr_SecondPTW, 0);
    cpAR_new->SetID(ARID_PTW);
    cpAR_new->SetAddr(nAddr_SecondPTW);
    cpAR_new->SetSrcName(this->cName);
    cpAR_new->SetTransType(ETRANS_TYPE_SECOND_PTW);
    cpAR_new->SetVA(nVA);
#if defined SINGLE_FETCH
    cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH - 1);
#elif defined HALF_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1);
#elif defined QUARTER_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1);
#endif

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_R_fwd_AT] (%s) generated for second PTW.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

    // Push FIFO AR
    UPUD upAR_new = new UUD;
    upAR_new->cpAR = cpAR_new;
#ifdef DEBUG_MMU
    assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif

    this->cpFIFO_AR->Push(upAR_new, MMU_FIFO_AR_2ND_PTW_LATENCY);

#ifdef DEBUG_MMU
    assert(upAR_new != NULL);
    // printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s push FIFO_AR.\n", nCycle,
    // this->cName.c_str(), cpAR->GetName().c_str()); //DUONGTRAN comment
    printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cPktName); // DUONGTRAN add
// cpAR->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upAR_new, EUD_TYPE_AR);

    // Fill FLPD
    // Check hit. 1st APTW could fill already.
    EResultType IsFLPD_Hit = this->IsFLPD_Hit(nVA, nCycle);
    if (IsFLPD_Hit == ERESULT_TYPE_NO) {
      // Get victim entry number FLPD
      int nVictim_entry = GetVictim_FLPD();
      int64_t nVPN = nVA >> BIT_512GB_PAGE; // 512GB page. BIT_512GB_PAGE 39

      // Update FLPD
      this->FillFLPD(nVictim_entry, nVPN, nCycle);
    };

    // Update tracker state
    //	(1) Search head node matching ID, UDType, state
    //	(2) Update state
    this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_FIRST_PTW, ECACHE_STATE_TYPE_SECOND_PTW);

    // Stat
    this->nPTW_total++;
    this->nPTW_2nd++;

    return (ERESULT_TYPE_SUCCESS);
  };

  //------------------------------------------------------
  // 4.2 If "2nd PTW", update FLPD if miss. Generate "3rd PTW".
  //------------------------------------------------------
  if (spNode->eState == ECACHE_STATE_TYPE_SECOND_PTW) { // Doing PTW

    // Debug
    // assert (nID == ARID_PTW);  // FIXME

    // Generate 3rd PTW
    char cPktName[50];
    if (eUDType == EUD_TYPE_AR) {
      sprintf(cPktName, "3rd_PTW_for_%s", cpAR->GetName().c_str());
    } else if (eUDType == EUD_TYPE_AW) {
      sprintf(cPktName, "3rd_PTW_for_%s", cpAW->GetName().c_str());
    } else {
      assert(0);
    };

    // Get address 3rd-level page table
    int64_t nAddr_ThirdPTW = GetThird_PTWAddr(nVA);

    // Generate PTW
    CPAxPkt cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

    //                  nID,       nAddr,           nLen
    // cpAR_new->SetPkt(ARID_PTW,  nAddr_SecondPTW, 0);
    cpAR_new->SetID(ARID_PTW);
    cpAR_new->SetAddr(nAddr_ThirdPTW);
    cpAR_new->SetSrcName(this->cName);
    cpAR_new->SetTransType(ETRANS_TYPE_THIRD_PTW);
    cpAR_new->SetVA(nVA);
#if defined SINGLE_FETCH
    cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH - 1);
#elif defined HALF_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1);
#elif defined QUARTER_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1);
#endif

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_R_fwd_AT] (%s) generated for third PTW.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

    // Push FIFO AR
    UPUD upAR_new = new UUD;
    upAR_new->cpAR = cpAR_new;
#ifdef DEBUG_MMU
    assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif

    this->cpFIFO_AR->Push(upAR_new, MMU_FIFO_AR_3RD_PTW_LATENCY);

#ifdef DEBUG_MMU
    assert(upAR_new != NULL);
    // printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s push FIFO_AR.\n", nCycle,
    // this->cName.c_str(), cpAR->GetName().c_str()); //DUONGTRAN comment
    printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cPktName); // DUONGTRAN add
// cpAR->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upAR_new, EUD_TYPE_AR);

    // Fill FLPD
    // Check hit. 1st APTW could fill already.
    EResultType IsSLPD_Hit = this->IsSLPD_Hit(nVA, nCycle);
    if (IsSLPD_Hit == ERESULT_TYPE_NO) {
      // Get victim entry number FLPD
      int nVictim_entry = GetVictim_FLPD();
      int64_t nVPN = nVA >> BIT_1GB_PAGE; // 1GB page. BIT_1GB_PAGE 30

      // Update FLPD
      this->FillSLPD(nVictim_entry, nVPN, nCycle);
    };

    // Update tracker state
    //	(1) Search head node matching ID, UDType, state
    //	(2) Update state
    this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_SECOND_PTW, ECACHE_STATE_TYPE_THIRD_PTW);

    // Stat
    this->nPTW_total++;
    this->nPTW_3rd++;

    return (ERESULT_TYPE_SUCCESS);
  };
  //------------------------------------------------------
  // 4.3 If "3rd PTW", update FLPD if miss. Generate "4th PTW".
  //------------------------------------------------------
  if (spNode->eState == ECACHE_STATE_TYPE_THIRD_PTW) { // Doing PTW

    // Debug
    // assert (nID == ARID_PTW);  // FIXME

    // Generate 4th PTW
    char cPktName[50];
    if (eUDType == EUD_TYPE_AR) {
      sprintf(cPktName, "4th_PTW_for_%s", cpAR->GetName().c_str());
    } else if (eUDType == EUD_TYPE_AW) {
      sprintf(cPktName, "4th_PTW_for_%s", cpAW->GetName().c_str());
    } else {
      assert(0);
    };

    // Get address 4th-level page table
    int64_t nAddr_FourthPTW = GetFourth_PTWAddr(nVA);

    // Generate PTW
    CPAxPkt cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

    //                  nID,       nAddr,           nLen
    // cpAR_new->SetPkt(ARID_PTW,  nAddr_SecondPTW, 0);
    cpAR_new->SetID(ARID_PTW);
    cpAR_new->SetAddr(nAddr_FourthPTW);
    cpAR_new->SetSrcName(this->cName);
    cpAR_new->SetTransType(ETRANS_TYPE_FOURTH_PTW);
    cpAR_new->SetVA(nVA);
#if defined SINGLE_FETCH
    cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH - 1);
#elif defined HALF_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1);
#elif defined QUARTER_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1);
#endif

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_R_fwd_AT] (%s) generated for fourth PTW.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

    // Push FIFO AR
    UPUD upAR_new = new UUD;
    upAR_new->cpAR = cpAR_new;
#ifdef DEBUG_MMU
    assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif

    this->cpFIFO_AR->Push(upAR_new, MMU_FIFO_AR_4TH_PTW_LATENCY);

#ifdef DEBUG_MMU
    assert(upAR_new != NULL);
    // printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s push FIFO_AR.\n", nCycle,
    // this->cName.c_str(), cpAR->GetName().c_str()); //DUONGTRAN comment
    printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cPktName); // DUONGTRAN add
// cpAR->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upAR_new, EUD_TYPE_AR);

    // Fill FLPD
    // Check hit. 1st APTW could fill already.
    EResultType IsTLPD_Hit = this->IsTLPD_Hit(nVA, nCycle);
    if (IsTLPD_Hit == ERESULT_TYPE_NO) {
      // Get victim entry number FLPD
      int nVictim_entry = GetVictim_FLPD();
      int64_t nVPN = nVA >> BIT_2MB_PAGE; // 2MB page. BIT_2MB_PAGE 21

      // Update TLPD
      this->FillTLPD(nVictim_entry, nVPN, nCycle);
    };

    // Update tracker state
    //	(1) Search head node matching ID, UDType, state
    //	(2) Update state
    this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_THIRD_PTW, ECACHE_STATE_TYPE_FOURTH_PTW);

    // Stat
    this->nPTW_total++;
    this->nPTW_4th++;

    // //////////////////////////////////////////////////////////////////////////
    // Issue 4th APTW
    /////////////////////////////////////////////////////////////////////////////

    if (eUDType == EUD_TYPE_AR) {
      sprintf(cPktName, "4th_APTW_for_%s", cpAR->GetName().c_str());
    } else if (eUDType == EUD_TYPE_AW) {
      sprintf(cPktName, "4th_APTW_for_%s", cpAW->GetName().c_str());
    } else {
      assert(0);
    };

    // Get VA anchor
    int64_t nVA_anchor = GetVA_Anchor_AT(nVA);

    if (nVA_anchor == nVA) {
      return (ERESULT_TYPE_SUCCESS);
    }

    // Get address 4th-level page table
    int64_t nAddr_FourthAPTW = GetFourth_PTWAddr(nVA);

    // Generate APTW
    cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

    //                  nID,       nAddr,           nLen
    // cpAR_new->SetPkt(ARID_APTW, nAddr_SecondPTW, 0);
    cpAR_new->SetID(ARID_APTW);
    cpAR_new->SetAddr(nAddr_FourthAPTW);
    cpAR_new->SetSrcName(this->cName);
    cpAR_new->SetTransType(ETRANS_TYPE_FOURTH_APTW);
    cpAR_new->SetVA(nVA);
#if defined SINGLE_FETCH
    cpAR_new->SetLen(0);
#elif defined BLOCK_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH - 1);
#elif defined HALF_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 2 - 1);
#elif defined QUARTER_FETCH
    cpAR_new->SetLen(MAX_BURST_LENGTH / 4 - 1);
#endif

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_R_fwd_AT] (%s) generated at 0x%lx for fourth "
           "APTW.\n",
           nCycle, this->cName.c_str(), cPktName, nVA_anchor);
// cpAR_new->Display();
#endif

    // Push FIFO AR
    upAR_new = new UUD;
    upAR_new->cpAR = cpAR_new;
#ifdef DEBUG_MMU
    assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif

    this->cpFIFO_AR->Push(upAR_new, MMU_FIFO_AR_4TH_PTW_LATENCY);

#ifdef DEBUG_MMU
    assert(upAR_new != NULL);
    // printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s push FIFO_AR.\n", nCycle,
    // this->cName.c_str(), cpAR->GetName().c_str());
    printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cPktName); // DUONGTRAN add
// cpAR->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upAR_new, EUD_TYPE_AR);

    // Fill FLPD
    // Check hit. 1st PTW could fill already.
    IsTLPD_Hit = this->IsTLPD_Hit(nVA, nCycle);
    if (IsTLPD_Hit == ERESULT_TYPE_NO) {
      // Get victim entry number FLPD
      int nVictim_entry = GetVictim_FLPD();
      int64_t nVPN = nVA >> BIT_2MB_PAGE; // 2MB page. BIT_2MB_PAGE 21

      // Update FLPD
      this->FillTLPD(nVictim_entry, nVPN, nCycle);
    };

    // Push tracker "anchor"
    upAR_new = new UUD;

    if (eUDType == EUD_TYPE_AR) {
      upAR_new->cpAR = Copy_CAxPkt(cpAR); // Original
      this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_FOURTH_APTW);
      // Maintain "anchor"
      Delete_UD(upAR_new, EUD_TYPE_AR);
    } else if (eUDType == EUD_TYPE_AW) {
      upAR_new->cpAW = Copy_CAxPkt(cpAW); // Original
      this->cpTracker->Push(upAR_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_FOURTH_APTW);
      // Maintain "anchor"
      Delete_UD(upAR_new, EUD_TYPE_AW);
    } else {
      assert(0);
    };

#ifdef DEBUG_MMU
// printf("[Cycle %3ld: %s.Do_AR_fwd_SI_AT] %s push MO_tracker.\n", nCycle,
// this->cName.c_str(), cpAR->GetName().c_str());
//  this->cpTracker->CheckTracker();
#endif

    // Stat
    this->nAPTW_total_AT++;
    this->nAPTW_4th_AT++;

    return (ERESULT_TYPE_SUCCESS);
  };

  //---------------------------------------------------------------
  // 5. If "4th APTW", change to APTW_DONE
  //   Check contiguity match.
  //
  //   (5.1) If have no PTW pair ---> ERROR!!!
  //   (5.2) if PTW on going
  //			 (5.2a) if "4th APTW" match contiguity, fill anchor,
  // translate address, send to MI, else: 			 (5.2b) Pop APTW
  // else (5.3) 	 (5.3) (PTW_DONE), Pop APTW 			 (5.3a)
  // if "4th APTW" match contiguity, fill anchor, else: 			 (5.3b)
  // if not match, fill regular
  //---------------------------------------------------------------
  // FIXME What if APTW only?		Duong fixed
  //       How to POP tracker?
  //---------------------------------------------------------------

  if (spNode->eState == ECACHE_STATE_TYPE_FOURTH_APTW) { // APTW finished
    // Update tracker state
    this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_FOURTH_APTW, ECACHE_STATE_TYPE_FOURTH_APTW_DONE);
    // Debug
    // assert (nID == ARID_APTW); // FIXME

    // (5.1) check APTW only --> ERROR!!! Currently, at anytime, Tracker only
    // have a (A)PTW belong to a AR/AW transaction FIXME Get tracker node is PTW
    SPLinkedCUD spNode_PTW = this->cpTracker->GetNode_Pair_PTW_VA_AT(nVA, eUDType);
    if (spNode_PTW == NULL) {
      printf("APTW never alone, please check your system and sure issue both "
             "APTW and PTW!!!\n");
      assert(0);
    }

    // (5.2) if PTW on going
    SPLinkedCUD spNode_PTW_DONE = this->cpTracker->GetNode(ECACHE_STATE_TYPE_FOURTH_PTW_DONE);
    SPLinkedCUD spNode_PTW_HIT = this->cpTracker->GetNode(ECACHE_STATE_TYPE_PTW_HIT);
    if (spNode_PTW_DONE == NULL && spNode_PTW_HIT == NULL) {

      // (5.2a) if match contiguity, fill anchor, translate address, send to MI
      EResultType IsAnchor_Contiguity_Match = IsAnchor_Cover_AT(nVA);
      // if match contiguity, fill anchor, translate address, send to MI
      if (IsAnchor_Contiguity_Match == ERESULT_TYPE_YES) {
        // Update TLB, translate address. Send to MI
        // Update TLB
        // Get VA aligned
        int64_t nVA_anchor = GetVA_Anchor_AT(nVA);
#ifdef DEBUG_MMU
        printf("in Do_R_fwd_AT - nVA_anchor: 0x%lx\n", nVA_anchor);
#endif

// Update TLB
#if defined SINGLE_FETCH
        this->FillTLB_SF(nVA_anchor, nCycle);
#elif defined BLOCK_FETCH
        this->FillTLB_BF(nVA_anchor, nCycle);
#elif defined HALF_FETCH
        this->FillTLB_BF(nVA_anchor, nCycle);
#elif defined QUARTER_FETCH
        this->FillTLB_BF(nVA_anchor, nCycle);
#endif

        // Translate address. Send to MI.
        // Translate addr
        int nPA = GetPA_TLB(nVA);

#ifdef DEBUG_MMU
        assert(nPA >= MIN_ADDR);
        assert(nPA <= MAX_ADDR);
#endif

        // Set transaction info
        if (eUDType == EUD_TYPE_AR) {
#ifdef DEBUG_MMU
          assert(cpAR != NULL);
#endif
          cpAR->SetID(nID << 2);
          cpAR->SetAddr(nPA);

          // Push FIFO AR
          UPUD upAR_new = new UUD;
          upAR_new->cpAR = Copy_CAxPkt(cpAR); // Translated original
#ifdef DEBUG_MMU
          assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif

          this->cpFIFO_AR->Push(upAR_new, MMU_FIFO_AR_PTW_FINISH_LATENCY);

#ifdef DEBUG_MMU
          assert(upAR_new != NULL);
          printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s push FIFO_AR.\n", nCycle, this->cName.c_str(),
                 cpAR->GetName().c_str());
#endif

          // Maintain
          Delete_UD(upAR_new, EUD_TYPE_AR);

          // Stat
          // this->IsPTW_AR_ongoing = ERESULT_TYPE_NO;
        } else if (eUDType == EUD_TYPE_AW) {

#ifdef DEBUG_MMU
          assert(cpAW != NULL);
#endif

          cpAW->SetAddr(nPA);

#ifdef DEBUG_MMU
          assert(this->cpTx_AW->IsBusy() == ERESULT_TYPE_NO);
#endif

          // Put Tx_AW
          this->cpTx_AW->PutAx(cpAW);

#ifdef DEBUG_MMU
          printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s put TxAW\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
#endif

          // Stat
          this->nAW_MI++;

          // W control
          // this->IsPTW_AW_ongoing = ERESULT_TYPE_NO;
        } else {
          assert(0);
        };
      } else {
//----------------------------------------
//  (5.2b) not match, pop APTW and wait "4th PTW" finish.
//----------------------------------------
#ifdef DEBUG_MMU
        assert(spNode_PTW != NULL);
#endif
        // Pop APTW tracker
        SPLinkedCUD spNode_APTW = this->cpTracker->GetNode(nVA, ECACHE_STATE_TYPE_FOURTH_APTW_DONE);

        UPUD upAR_new2 = this->cpTracker->Pop(spNode_APTW);

#ifdef DEBUG_MMU
        // printf("[Cycle %3ld: %s.Do_R_fwd_AT] (%s) pop MO_tracker.\n", nCycle,
        // this->cName.c_str(), upAR_new2->GetName().c_str());
        //  this->cpTracker->CheckTracker();
        assert(upAR_new2 != NULL);
#endif

        // Maintain
        Delete_UD(upAR_new2, eUDType);
      }
    } else {
      //(5.3) (PTW_DONE / PTW_HIT) Pop APTW
      // Pop APTW tracker
      SPLinkedCUD spNode_APTW = this->cpTracker->GetNode(ECACHE_STATE_TYPE_FOURTH_APTW_DONE);
      UPUD upAR_new2 = this->cpTracker->Pop(spNode_APTW);
#ifdef DEBUG_MMU
      if (eUDType == EUD_TYPE_AR) {
        printf("[Cycle %3ld: %s.Do_R_fwd_AT] (%s) pop MO_tracker.\n", nCycle, this->cName.c_str(),
               upAR_new2->cpAR->GetName().c_str());
      } else {
        printf("[Cycle %3ld: %s.Do_R_fwd_AT] (%s) pop MO_tracker.\n", nCycle, this->cName.c_str(),
               upAR_new2->cpAW->GetName().c_str());
      }
      this->cpTracker->Display();
      assert(upAR_new2 != NULL);
#endif

      // Maintain
      Delete_UD(upAR_new2, eUDType);

      if (spNode_PTW_HIT != NULL) {
        UPUD upAR_new3 = this->cpTracker->Pop(spNode_PTW_HIT);
#ifdef DEBUG_MMU
        if (eUDType == EUD_TYPE_AR) {
          printf("[Cycle %3ld: %s.Do_R_fwd_AT] (%s) pop MO_tracker.\n", nCycle, this->cName.c_str(),
                 upAR_new3->cpAR->GetName().c_str());
        } else {
          printf("[Cycle %3ld: %s.Do_R_fwd_AT] (%s) pop MO_tracker.\n", nCycle, this->cName.c_str(),
                 upAR_new3->cpAW->GetName().c_str());
        }
        this->cpTracker->Display();
        assert(upAR_new3 != NULL);
#endif
        // Maintain
        Delete_UD(upAR_new3, eUDType);
      } else {
        // change PTW_DONE to HIT
        this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_FOURTH_PTW_DONE, ECACHE_STATE_TYPE_HIT);
      }

      //	 (5.3a) if match contiguity, fill anchor
      EResultType IsAnchor_Contiguity_Match = IsAnchor_Cover_AT(nVA);
      if (IsAnchor_Contiguity_Match == ERESULT_TYPE_YES) {
        int64_t nVA_anchor = GetVA_Anchor_AT(nVA);
#ifdef DEBUG_MMU
        printf("in Do_R_fwd_AT - nVA_anchor: 0x%lx\n", nVA_anchor);
#endif

// Update TLB
#if defined SINGLE_FETCH
        this->FillTLB_SF(nVA_anchor, nCycle);
#elif defined BLOCK_FETCH
        this->FillTLB_BF(nVA_anchor, nCycle);
#elif defined HALF_FETCH
        this->FillTLB_BF(nVA_anchor, nCycle);
#elif defined QUARTER_FETCH
        this->FillTLB_BF(nVA_anchor, nCycle);
#endif

      } else {
//	 (5.3b) if not match, fill regular
#ifdef DEBUG_MMU
        printf("in Do_R_fwd_AT - nVA: 0x%lx\n", nVA);
#endif

// Update TLB
#if defined SINGLE_FETCH
        this->FillTLB_SF(nVA, nCycle);
#elif defined BLOCK_FETCH
        this->FillTLB_BF(nVA, nCycle);
#elif defined HALF_FETCH
        this->FillTLB_BF(nVA, nCycle);
#elif defined QUARTER_FETCH
        this->FillTLB_BF(nVA, nCycle);
#endif
      }
      this->IsPTW_AR_ongoing = ERESULT_TYPE_NO;
      this->IsPTW_AW_ongoing = ERESULT_TYPE_NO;
    }

    return (ERESULT_TYPE_SUCCESS);
  };

  //------------------------------------------------------------------------------------------
  // 6. If "4th PTW", change to PTW_DONE, check "pair APTW".
  //   (6.1) If there is no "pair APTW" in tracker, update TLB. Translate
  //   address. Send to MI. change to HIT
  //
  //   (6.2) If there is "pair APTW",
  //       (6.2a) if APTW_DONE,  Pop APTW else:
  //       (6.2b) APTW on going, Translate address. Send to MI
  //------------------------------------------------------------------------------------------

#ifdef DEBUG_MMU
  assert(spNode->eState == ECACHE_STATE_TYPE_FOURTH_PTW);
#endif

  if (spNode->eState == ECACHE_STATE_TYPE_FOURTH_PTW) { // PTW finished
    this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_FOURTH_PTW, ECACHE_STATE_TYPE_FOURTH_PTW_DONE);

    // Debug
    // assert (nID == ARID_PTW); // FIXME

    //------------------------------------------------------------------------------------------
    // (6.1) Check there is no "pair APTW" in tracker, update TLB. Translate
    // address. Send to MI.
    //------------------------------------------------------------------------------------------
    // int64_t nVA_anchor = GetVA_Anchor_AT(nVA);
    SPLinkedCUD spNode_APTW = this->cpTracker->GetNode_Pair_APTW_VA_AT(nVA, eUDType); // FIXME
    // No pair. Update TLB. Translate address. Send to MI.
    if (spNode_APTW == NULL) { // No pair

// Update TLB
#if defined SINGLE_FETCH
      this->FillTLB_SF(nVA, nCycle);
#elif defined BLOCK_FETCH
      this->FillTLB_BF(nVA, nCycle);
#elif defined HALF_FETCH
      this->FillTLB_BF(nVA, nCycle);
#elif defined QUARTER_FETCH
      this->FillTLB_BF(nVA, nCycle);
#endif

      // Update tracker state
      // 	(1) Search head node matching ID, UDType, state
      // 	(2) Update state
      this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_FOURTH_PTW_DONE, ECACHE_STATE_TYPE_HIT);
#ifdef DEBUG_MMU
      if (eUDType == EUD_TYPE_AR) {
        printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s update "
               "ECACHE_STATE_TYPE_FOURTH_PTW_DONE to ECACHE_STATE_TYPE_HIT\n",
               nCycle, this->cName.c_str(), cpAR->GetName().c_str());
      } else if (eUDType == EUD_TYPE_AW) {
        printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s update "
               "ECACHE_STATE_TYPE_FOURTH_PTW_DONE to ECACHE_STATE_TYPE_HIT\n",
               nCycle, this->cName.c_str(), cpAW->GetName().c_str());
      } else
        assert(0);
#endif

      // Translate addr
      int nPA = GetPA_TLB(nVA);

#ifdef DEBUG_MMU
      assert(nPA >= MIN_ADDR);
      assert(nPA <= MAX_ADDR);
#endif

      // Set transaction info
      if (eUDType == EUD_TYPE_AR) {
#ifdef DEBUG_MMU
        assert(cpAR != NULL);
#endif
        cpAR->SetID(nID << 2);
        cpAR->SetAddr(nPA);

        // Push FIFO AR
        UPUD upAR_new = new UUD;
        upAR_new->cpAR = Copy_CAxPkt(cpAR); // Translated original
#ifdef DEBUG_MMU
        assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif

        this->cpFIFO_AR->Push(upAR_new, MMU_FIFO_AR_PTW_FINISH_LATENCY);

#ifdef DEBUG_MMU
        assert(upAR_new != NULL);
        printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
// cpAR->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

        // Maintain
        Delete_UD(upAR_new, EUD_TYPE_AR);

        // Stat
        this->IsPTW_AR_ongoing = ERESULT_TYPE_NO;
      } else if (eUDType == EUD_TYPE_AW) {
#ifdef DEBUG_MMU
        assert(cpAW != NULL);
#endif

        cpAW->SetAddr(nPA);

#ifdef DEBUG_MMU
        assert(this->cpTx_AW->IsBusy() == ERESULT_TYPE_NO);
#endif

        // Put Tx_AW
        this->cpTx_AW->PutAx(cpAW);
#ifdef DEBUG_MMU
        printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s put Tx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
#endif

        // Stat
        this->nAW_MI++;

        // W control
        this->IsPTW_AW_ongoing = ERESULT_TYPE_NO;
      } else {
        assert(0);
      };

      return (ERESULT_TYPE_SUCCESS);
    };

#ifdef DEBUG_MMU
    assert(spNode_APTW != NULL);
#endif

    // Check "4th APTW" finish
    if (spNode_APTW->eState == ECACHE_STATE_TYPE_FOURTH_APTW_DONE) {
      //---------------------------------------------------------------------------------------------
      // (6.2a) APTW_DONE,  Pop APTW, change PTW to HIT
      //---------------------------------------------------------------------------------------------

      // Pop APTW tracker
      UPUD upAR_new2 = this->cpTracker->Pop(spNode_APTW);

#ifdef DEBUG_MMU
      // printf("[Cycle %3ld: %s.Do_R_fwd_AT] (%s) pop MO_tracker.\n", nCycle,
      // this->cName.c_str(), upAR_new2->GetName().c_str());
      printf("[Cycle %3ld: %s.Do_R_fwd_AT] pop MO_tracker.\n", nCycle, this->cName.c_str());
      // this->cpTracker->CheckTracker();
      assert(upAR_new2 != NULL);
#endif

      // Maintain
      Delete_UD(upAR_new2, eUDType);

      this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_FOURTH_PTW_DONE, ECACHE_STATE_TYPE_HIT);
#ifdef DEBUG_MMU
      if (eUDType == EUD_TYPE_AR) {
        printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s update "
               "ECACHE_STATE_TYPE_FOURTH_PTW_DONE to ECACHE_STATE_TYPE_HIT\n",
               nCycle, this->cName.c_str(), cpAR->GetName().c_str());
      } else if (eUDType == EUD_TYPE_AW) {
        printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s update "
               "ECACHE_STATE_TYPE_FOURTH_PTW_DONE to ECACHE_STATE_TYPE_HIT\n",
               nCycle, this->cName.c_str(), cpAW->GetName().c_str());
      } else
        assert(0);
#endif

      this->IsPTW_AR_ongoing = ERESULT_TYPE_NO;
      this->IsPTW_AW_ongoing = ERESULT_TYPE_NO;

      return (ERESULT_TYPE_SUCCESS);
    } else {
      //(6.2b) APTW on going, Translate address. Send to MI
      // Translate addr
      // int nPA = GetPA_TLB(nVA);

      int64_t nVPN = this->GetVPNNum(nVA);
      int nIndexPTE = nVPN - (this->START_VPN);
      int nPPN = this->spPageTable[nIndexPTE]->nPPN;
      assert(nIndexPTE >= 0);
      assert(nPPN >= 0);
      int nOffset = nVA & 0x000000000FFF; // Offset 12 bit
      int nPA = (nPPN << BIT_4K_PAGE) + nOffset;
#ifdef DEBUG_MMU
      assert(nPA >= MIN_ADDR);
      assert(nPA <= MAX_ADDR);
#endif

      // Set transaction info
      if (eUDType == EUD_TYPE_AR) {
#ifdef DEBUG_MMU
        assert(cpAR != NULL);
#endif
        cpAR->SetID(nID << 2);
        cpAR->SetAddr(nPA);
        // Push FIFO AR
        UPUD upAR_new = new UUD;
        upAR_new->cpAR = Copy_CAxPkt(cpAR); // Translated original
#ifdef DEBUG_MMU
        assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);
#endif
        this->cpFIFO_AR->Push(upAR_new, MMU_FIFO_AR_PTW_FINISH_LATENCY);
#ifdef DEBUG_MMU
        assert(upAR_new != NULL);
        printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
#endif
        // Maintain
        Delete_UD(upAR_new, EUD_TYPE_AR);
        // Stat
        // this->IsPTW_AR_ongoing = ERESULT_TYPE_NO;
      } else if (eUDType == EUD_TYPE_AW) {
#ifdef DEBUG_MMU
        assert(cpAW != NULL);
#endif
        cpAW->SetAddr(nPA);
#ifdef DEBUG_MMU
        assert(this->cpTx_AW->IsBusy() == ERESULT_TYPE_NO);
#endif
        // Put Tx_AW
        this->cpTx_AW->PutAx(cpAW);
#ifdef DEBUG_MMU
        printf("[Cycle %3ld: %s.Do_R_fwd_AT] %s put Tx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
#endif
        // Stat
        this->nAW_MI++;
        // W control
        // this->IsPTW_AW_ongoing = ERESULT_TYPE_NO;
      } else {
        assert(0);
      };
    }
    return (ERESULT_TYPE_SUCCESS);
  };

  return (ERESULT_TYPE_SUCCESS);
};

// B valid
EResultType CMMU::Do_B_fwd(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_B->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx. Get.
  CPBPkt cpB = this->cpRx_B->GetPair()->GetB();
  if (cpB == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

#ifdef DEBUG_MMU
  assert(cpB != NULL);
#endif

  // Put Rx
  this->cpRx_B->PutB(cpB);

  // Put Tx
  this->cpTx_B->PutB(cpB);

  return (ERESULT_TYPE_SUCCESS);
};

// R ready
EResultType CMMU::Do_R_bwd(int64_t nCycle) {

  // Check Rx valid
  CPRPkt cpR = this->cpRx_R->GetR();
  if (cpR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  int nID = cpR->GetID();

  // Check PTW
  // if (nID == ARID_PTW or nID == ARID_PF_BUF0 or nID == ARID_PF_BUF1) {
  // //FIXME //DUONGTRAN comment
  if (nID == ARID_PTW) { // DUONGTRAN add
    // Set ready
    this->cpRx_R->SetAcceptResult(ERESULT_TYPE_ACCEPT);
#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_R_bwd] (%s) is handshaked PTW Rx_R.\n", nCycle, this->cName.c_str(),
           cpR->GetName().c_str());
#endif
  };

#ifdef AT_ENABLE
  // Check APTW. AT
  if (nID == ARID_APTW) {
    // Set ready
    this->cpRx_R->SetAcceptResult(ERESULT_TYPE_ACCEPT);
#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_R_bwd] (%s) is handshaked AnchorPTW Rx_R.\n", nCycle, this->cName.c_str(),
           cpR->GetName().c_str());
#endif
  };
#endif

#ifdef RMM_ENABLE
  // Check RPTW. RMM
  if (nID == ARID_RPTW_RMM) {
    // Set ready
    this->cpRx_R->SetAcceptResult(ERESULT_TYPE_ACCEPT);
#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_R_bwd] (%s) is handshaked RMM_PTW Rx_R.\n", nCycle, this->cName.c_str(),
           cpR->GetName().c_str());
#endif
  };
#endif

  // Check Tx ready
  EResultType eAcceptResult = this->cpTx_R->GetAcceptResult();

  // Traditional
  // if (nID != ARID_PTW) {
  if (nID >= 4) {
    if (eAcceptResult == ERESULT_TYPE_ACCEPT) {
      // Set ready
      this->cpRx_R->SetAcceptResult(ERESULT_TYPE_ACCEPT);
    };
  };

  // Check valid SI
  if (this->cpTx_R->IsBusy() == ERESULT_TYPE_YES) {
    cpR = this->cpTx_R->GetR();
  } else {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Maintain tracker
  if (eAcceptResult == ERESULT_TYPE_ACCEPT) {
    if (cpR->IsLast() == ERESULT_TYPE_YES) {

#ifdef DEBUG_MMU
      printf("[Cycle %3ld: %s.Do_R_bwd] RLAST sent to SI.\n", nCycle, this->cName.c_str());
#endif

      // Stat
      this->Decrease_MO_AR();
      this->Decrease_MO_Ax();

      int nID = cpR->GetID(); // Original AxID. Not PTW ID. Be careful.

#ifdef STAT_DETAIL
      // Stat
      SPLinkedCUD spNode = this->cpTracker->GetNode(nID, EUD_TYPE_AR);

#ifdef DEBUG_MMU
      assert(spNode->nCycle_wait >= 0);
#endif

      this->nTotal_Tracker_Wait_Ax += spNode->nCycle_wait;
      this->nTotal_Tracker_Wait_AR += spNode->nCycle_wait;
// printf("[Cycle %3ld: %s.Do_R_bwd] %d cycles allocated tracker (%s) \n",
// nCycle, this->cName.c_str(), spNode->nCycle_wait,
// spNode->upData->cpAR->GetName().c_str());
#endif
      // Maintain tracker
      UPUD upAR_new = this->cpTracker->Pop(nID, EUD_TYPE_AR, ECACHE_STATE_TYPE_HIT);

#ifdef DEBUG_MMU
// printf("[Cycle %3ld: %s.Do_R_bwd] (%s), ID %d pop MO_tracker.\n", nCycle,
// this->cName.c_str(), upAR_new->cpAR->GetName().c_str(), nID);
//  this->cpTracker->CheckTracker();
#endif

      if (this->cpTracker->IsAnyPTW() == ERESULT_TYPE_NO) {
        assert(upAR_new != NULL);
        // Maintain
        Delete_UD(upAR_new, EUD_TYPE_AR);
      }
    };

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_R_bwd] R sent to master.\n", nCycle, this->cName.c_str());
#endif
  };

  return (ERESULT_TYPE_SUCCESS);
};

// B ready
EResultType CMMU::Do_B_bwd(int64_t nCycle) {

  // Check Tx ready
  if (this->cpTx_B->GetAcceptResult() == ERESULT_TYPE_REJECT) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check Rx valid
  if (this->cpRx_B->IsBusy() == ERESULT_TYPE_NO) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Set ready
  this->cpRx_B->SetAcceptResult(ERESULT_TYPE_ACCEPT);

  // Get B
  CPBPkt cpB = this->cpTx_B->GetB();
  if (cpB == NULL) {
    assert(0);
    return (ERESULT_TYPE_SUCCESS);
  };

  // Get ready
  EResultType eAcceptResult = this->cpTx_B->GetAcceptResult();

  // Maintain tracker
  if (eAcceptResult == ERESULT_TYPE_ACCEPT) {

#ifdef DEBUG_MMU
    printf("[Cycle %3ld: %s.Do_B_bwd] B sent to SI.\n", nCycle, this->cName.c_str());
    printf("[Cycle %3ld: %s.Do_B_bwd] (%s) B handshake Rx_B.\n", nCycle, this->cName.c_str(), cpB->GetName().c_str());
#endif

    // MO
    this->Decrease_MO_AW();
    this->Decrease_MO_Ax();

    int nID = cpB->GetID();

#ifdef STAT_DETAIL
    // Stat
    SPLinkedCUD spNode = this->cpTracker->GetNode(nID, EUD_TYPE_AW);
    assert(spNode->nCycle_wait >= 0);
    this->nTotal_Tracker_Wait_Ax += spNode->nCycle_wait;
    this->nTotal_Tracker_Wait_AW += spNode->nCycle_wait;
// printf("[Cycle %3ld: %s.Do_B_bwd] %d cycles allocated tracker (%s) \n",
// nCycle, this->cName.c_str(), spNode->nCycle_wait,
// spNode->upData->cpAW->GetName().c_str());
#endif
    SPLinkedCUD spNode_PTW_DONE = this->cpTracker->GetNode(nID, EUD_TYPE_AW, ECACHE_STATE_TYPE_FOURTH_PTW_DONE);
    if (spNode_PTW_DONE == NULL) {
      UPUD upAW_new = this->cpTracker->Pop(nID, EUD_TYPE_AW, ECACHE_STATE_TYPE_HIT);
      // printf("[Cycle %3ld: %s.Do_B_bwd] (%s) pop MO_tracker.\n", nCycle,
      // this->cName.c_str(), upAW_new->cpAW->GetName().c_str());
      // this->cpTracker->CheckTracker();
      assert(upAW_new != NULL);
      // Maintain
      Delete_UD(upAW_new, EUD_TYPE_AW);
    } else { // only (A)PTW
      this->cpTracker->SetStateNode(nID, EUD_TYPE_AW, ECACHE_STATE_TYPE_FOURTH_PTW_DONE, ECACHE_STATE_TYPE_PTW_HIT);
    }
  };

  return (ERESULT_TYPE_SUCCESS);
};

//----------------------------------------------------------
// Fill TLB
// 	Single fetch
// 	1. PCA
// 	2. BCT
// 	3. AT
//  4. NAPOT
//----------------------------------------------------------
EResultType CMMU::FillTLB_SF(int64_t nVA, int64_t nCycle) {

  // Debug
  // this->CheckMMU();

  int64_t nVPN = this->GetVPNNum(nVA);

#ifdef DEBUG_MMU
#ifdef AT_ENABLE
  assert(this->IsTLB_Allocate_AT(nVA, nCycle) || this->IsTLB_Allocate_Anchor_AT(nVA) ||
         this->IsTLB_Contiguity_Match_AT(nVA, nCycle) == ERESULT_TYPE_NO); // DUONGTRAN add
#else
  assert(this->IsTLB_Hit(nVPN) == ERESULT_TYPE_NO);
#endif
#endif

  int nSet = this->GetSetNum(nVPN);
  int nWay = this->GetVictim_TLB(nVPN);

#ifdef DEBUG_MMU
  printf("in FillTLB_SF - VPN: 0x%lx, nSet: %d, nWay: %d\n", nVPN, nSet, nWay);
#endif

  // Fill new entry
  this->spTLB[nSet][nWay]->nValid = 1;
  this->spTLB[nSet][nWay]->nVPN = nVPN;

#ifdef PAGE_SIZE_4KB
  this->spTLB[nSet][nWay]->ePageSize = EPAGE_SIZE_TYPE_4KB;
#endif

  this->spTLB[nSet][nWay]->nPPN = rand() % 0x3FFFF; // Mapping table random 20-bit. nPPN is signed int type.
                                                    // MSB suppose 0. PPN 20-bit

#ifdef BUDDY_ENABLE
  int nIndexPTE = nVPN - (this->START_VPN);
  int nPPN = this->spPageTable[nIndexPTE]->nPPN;
  assert(nIndexPTE >= 0);
  assert(nPPN >= 0);
  this->spTLB[nSet][nWay]->nPPN = nPPN;
#endif

#ifdef PAGE_SIZE_1MB
  this->spTLB[nSet][nWay]->ePageSize = EPAGE_SIZE_TYPE_1MB;
  this->spTLB[nSet][nWay]->nPPN = rand() % 0x7FF; // Mapping table random 12-bit. nPPN signed int type. MSB
                                                  // suppose 0. PPN 20-bit
#endif

#ifdef VPN_SAME_PPN
  this->spTLB[nSet][nWay]->nPPN = nVPN; // VPN equal to PPN
#endif

  // Allocate time
  this->spTLB[nSet][nWay]->nTimeStamp = nCycle; // Replacement

#ifdef CONTIGUITY_ENABLE
#ifdef BUDDY_ENABLE

// DUONGTRAN add for RangeBuddy
// (1)
#ifdef PCAD
#ifdef PBS
  int mulPage = 1 << (this->spPageTable[nIndexPTE]->nCodedPageSize << 2);
  this->spTLB[nSet][nWay]->nVPN =
      nVPN - (this->spPageTable[nIndexPTE]->nPPN % mulPage) - this->spPageTable[nIndexPTE]->nDescend * mulPage;
  this->spTLB[nSet][nWay]->nPPN = this->spPageTable[nIndexPTE]->nPPN - (this->spPageTable[nIndexPTE]->nPPN % mulPage) -
                                  this->spPageTable[nIndexPTE]->nDescend * mulPage;
  this->spTLB[nSet][nWay]->nContiguity =
      (this->spPageTable[nIndexPTE]->nAscend + this->spPageTable[nIndexPTE]->nDescend + 1) * mulPage - 1;
#else
  this->spTLB[nSet][nWay]->nVPN = nVPN - this->spPageTable[nIndexPTE]->nDescend;
  this->spTLB[nSet][nWay]->nPPN = this->spPageTable[nIndexPTE]->nPPN - this->spPageTable[nIndexPTE]->nDescend;
  this->spTLB[nSet][nWay]->nContiguity =
      this->spPageTable[nIndexPTE]->nAscend + this->spPageTable[nIndexPTE]->nDescend; // DuongTran, don't plus 1
#endif // PBS
// (2)
#elif defined BCT_ENABLE
                                                // int blockSize =  1 <<
                                                // this->spPageTable[nIndexPTE]->nBlockSize;
  int blockSize = this->spPageTable[nIndexPTE]->nBlockSize;
  this->spTLB[nSet][nWay]->nVPN = nVPN - (nVPN % blockSize);
  this->spTLB[nSet][nWay]->nPPN = nPPN - (nPPN % blockSize);
  this->spTLB[nSet][nWay]->nContiguity = blockSize - 1;
  // (3)
#elif defined AT_ENABLE
  this->spTLB[nSet][nWay]->nContiguity = this->spPageTable[nIndexPTE]->nContiguity;
// (4)
#elif defined NAPOT
  this->spTLB[nSet][nWay]->N_flag = this->spPageTable[nIndexPTE]->N_flag;
  if (this->spPageTable[nIndexPTE]->N_flag == 1) {
    this->spTLB[nSet][nWay]->nVPN = nVPN & (~((1 << NAPOT_BITS) - 1));
  }
#endif
#endif // BUDDY_ENABLE
#endif // "CONTIGUITY_ENABLE"

  return (ERESULT_TYPE_SUCCESS);
};

//----------------------------------------------------------
// Fill TLB cRCPT
// Fetch GROUP_SIZE  PTEs
//----------------------------------------------------------
EResultType CMMU::FillTLB_cRCPT(int64_t nVA, int64_t nCycle) {
#if defined CONTIGUITY_ENABLE && defined BUDDY_ENABLE && defined cRCPT_ENABLE
  int64_t nVPN = this->GetVPNNum(nVA);
  int64_t nAVPN = nVPN & (~(NUM_PTE_PTW - 1));

  int nSet = -1;
  int nWay = -1;

  int nIndexPTE = nAVPN - (this->START_VPN);
  nAVPN = nAVPN & (~(this->spPageTable[nIndexPTE]->nBlockSize - 1)); // Solve redundant group, example, if GROUP_SIZE is
                                                                     // 4 and BlockSize of block is 8
  int64_t nNewVPN = nAVPN;

  // Aligned PTE
  if (this->spPageTable[nIndexPTE]->nBlockSize != 0) { // ignore invalid PTE
    // Check TLB hit (to avoid redundant allocation)
    EResultType IsTLB_Hit = this->IsTLB_Hit(nNewVPN);
    if (IsTLB_Hit != ERESULT_TYPE_YES) {
      // Find victim to replace
      nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nAVPN
      nSet = this->GetSetNum(nNewVPN);

      this->spTLB[nSet][nWay]->nContiguity = this->spPageTable[nIndexPTE]->nBlockSize - 1;
      this->spTLB[nSet][nWay]->nValid = 1;
      this->spTLB[nSet][nWay]->nPPN = this->spPageTable[nIndexPTE]->nPPN;
      this->spTLB[nSet][nWay]->nVPN = nNewVPN;

      this->spTLB[nSet][nWay]->ePageSize = EPAGE_SIZE_TYPE_4KB;
      this->spTLB[nSet][nWay]->nTimeStamp = nCycle;

      if (nVPN == nNewVPN) {
        this->spTLB[nSet][nWay]->nTimeStamp = nCycle; // Demanding VPN priority
      }
    }
    nNewVPN += this->spPageTable[nIndexPTE]->nBlockSize;
  } else {
    printf("ERROR!!! APTE must always be valid!\n");
    assert(0); // APTE must always be valid
  }

  // Next PTEs
  for (int j = 1; j < NUM_PTE_PTW; j++) {
    if (this->spPageTable[nIndexPTE + j]->nBlockSize_1 != 0) { // ignore invalid Block
      // Check TLB hit (to avoid redundant allocation)
      EResultType IsTLB_Hit = this->IsTLB_Hit(nNewVPN);
      if (IsTLB_Hit != ERESULT_TYPE_YES) {
        // Find victim to replace
        nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nAVPN
        nSet = this->GetSetNum(nNewVPN);

        this->spTLB[nSet][nWay]->nContiguity = this->spPageTable[nIndexPTE + j]->nBlockSize_1 - 1;
        this->spTLB[nSet][nWay]->nValid = 1;
        this->spTLB[nSet][nWay]->nPPN = this->spPageTable[nIndexPTE + j]->nPPN_1;
        this->spTLB[nSet][nWay]->nVPN = nNewVPN;

        this->spTLB[nSet][nWay]->ePageSize = EPAGE_SIZE_TYPE_4KB;
        this->spTLB[nSet][nWay]->nTimeStamp = nCycle;

        if (nVPN == nNewVPN) {
          this->spTLB[nSet][nWay]->nTimeStamp = nCycle; // Demanding VPN priority
        }
      }
      nNewVPN += this->spPageTable[nIndexPTE + j]->nBlockSize_1;
    }

    if (this->spPageTable[nIndexPTE + j]->nBlockSize != 0) { // ignore invalid Block
      // Check TLB hit (to avoid redundant allocation)
      EResultType IsTLB_Hit = this->IsTLB_Hit(nNewVPN);
      if (IsTLB_Hit != ERESULT_TYPE_YES) {
        // Find victim to replace
        nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nAVPN
        nSet = this->GetSetNum(nNewVPN);

        this->spTLB[nSet][nWay]->nContiguity = this->spPageTable[nIndexPTE + j]->nBlockSize - 1;
        this->spTLB[nSet][nWay]->nValid = 1;
        this->spTLB[nSet][nWay]->nPPN = this->spPageTable[nIndexPTE + j]->nPPN;
        this->spTLB[nSet][nWay]->nVPN = nNewVPN;

        this->spTLB[nSet][nWay]->ePageSize = EPAGE_SIZE_TYPE_4KB;
        this->spTLB[nSet][nWay]->nTimeStamp = nCycle;

        if (nVPN == nNewVPN) {
          this->spTLB[nSet][nWay]->nTimeStamp = nCycle; // Demanding VPN priority
        }
      }
      nNewVPN += this->spPageTable[nIndexPTE + j]->nBlockSize;
    }

    if (this->spPageTable[nIndexPTE + j]->nCluster == true) {    // cRCPT PTE: 3 Blocks
      if (this->spPageTable[nIndexPTE + j]->nBlockSize_3 != 0) { // ignore invalid Block
        // Check TLB hit (to avoid redundant allocation)
        EResultType IsTLB_Hit = this->IsTLB_Hit(nNewVPN);
        if (IsTLB_Hit != ERESULT_TYPE_YES) {
          // Find victim to replace
          nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nAVPN
          nSet = this->GetSetNum(nNewVPN);

          this->spTLB[nSet][nWay]->nContiguity = this->spPageTable[nIndexPTE + j]->nBlockSize_3 - 1;
          this->spTLB[nSet][nWay]->nValid = 1;
          this->spTLB[nSet][nWay]->nPPN = this->spPageTable[nIndexPTE + j]->nPPN_3;
          this->spTLB[nSet][nWay]->nVPN = nNewVPN;

          this->spTLB[nSet][nWay]->ePageSize = EPAGE_SIZE_TYPE_4KB;
          this->spTLB[nSet][nWay]->nTimeStamp = nCycle;

          if (nVPN == nNewVPN) {
            this->spTLB[nSet][nWay]->nTimeStamp = nCycle; // Demanding VPN priority
          }
        }
        nNewVPN += this->spPageTable[nIndexPTE + j]->nBlockSize_3;
      }
    }
  }

#else
  assert(0);
#endif // CONTIGUITY_ENABLE && BUDDY_ENABLE && cRCPT_ENABLE

  return (ERESULT_TYPE_SUCCESS);
}

//----------------------------------------------------------
// Fill TLB RCPT
// Fetch GROUP_SIZE  PTEs
//----------------------------------------------------------
EResultType CMMU::FillTLB_RCPT(int64_t nVA, int64_t nCycle) {
#if defined CONTIGUITY_ENABLE && defined BUDDY_ENABLE && defined RCPT_ENABLE
  int64_t nVPN = this->GetVPNNum(nVA);
  int64_t nAVPN = nVPN & (~(NUM_PTE_PTW - 1));

  int nSet = -1;
  int nWay = -1;
  int nSet_1 = -1;
  int nWay_1 = -1;

  int nIndexPTE = nAVPN - (this->START_VPN);
  nAVPN = nAVPN & (~(this->spPageTable[nIndexPTE]->nBlockSize - 1)); // Solve redundant group, example, if GROUP_SIZE is
                                                                     // 4 and BlockSize of block is 8
  int64_t nNewVPN = nAVPN;
  int j = 0;
  while (true) {

    if (this->spPageTable[nIndexPTE + j]->nBlockSize != 0) { // ignore invalid PTE

      // Check TLB hit (to avoid redundant allocation)
      EResultType IsTLB_Hit = this->IsTLB_Hit(nNewVPN);
      if (IsTLB_Hit != ERESULT_TYPE_YES) {

        // Find victim to replace
        nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nAVPN
        nSet = this->GetSetNum(nNewVPN);

        this->spTLB[nSet][nWay]->nContiguity = this->spPageTable[nIndexPTE + j]->nBlockSize - 1;
        this->spTLB[nSet][nWay]->nValid = 1;
        this->spTLB[nSet][nWay]->nPPN = this->spPageTable[nIndexPTE + j]->nPPN;
        this->spTLB[nSet][nWay]->nVPN = nNewVPN;

        this->spTLB[nSet][nWay]->ePageSize = EPAGE_SIZE_TYPE_4KB;
        this->spTLB[nSet][nWay]->nTimeStamp = nCycle - j;

        if (nVPN == nNewVPN) {
          this->spTLB[nSet][nWay]->nTimeStamp = nCycle; // Demanding VPN priority
        }
      }
      nNewVPN += this->spPageTable[nIndexPTE + j]->nBlockSize;
    }

    j++;
    if (j >= NUM_PTE_PTW)
      break;

    if (this->spPageTable[nIndexPTE + j]->nBlockSize_1 != 0) { // ignore invalid PTE

      // Check TLB hit (to avoid redundant allocation)
      EResultType IsTLB_Hit = this->IsTLB_Hit(nNewVPN);

      if (IsTLB_Hit != ERESULT_TYPE_YES) {

        // Find victim to replace
        nWay_1 = this->GetVictim_TLB(nNewVPN); // Be careful. Not nAVPN
        nSet_1 = this->GetSetNum(nNewVPN);

        this->spTLB[nSet_1][nWay_1]->nContiguity = this->spPageTable[nIndexPTE + j]->nBlockSize_1 - 1;
        this->spTLB[nSet_1][nWay_1]->nValid = 1;
        this->spTLB[nSet_1][nWay_1]->nPPN = this->spPageTable[nIndexPTE + j]->nPPN_1;
        this->spTLB[nSet_1][nWay_1]->nVPN = nNewVPN;

        this->spTLB[nSet_1][nWay_1]->ePageSize = EPAGE_SIZE_TYPE_4KB;
        this->spTLB[nSet_1][nWay_1]->nTimeStamp = nCycle - j;

        if (nVPN == nNewVPN) {
          this->spTLB[nSet_1][nWay_1]->nTimeStamp = nCycle; // Demanding VPN priority
        }
      }
      nNewVPN += this->spPageTable[nIndexPTE + j]->nBlockSize_1;
    }

  } // end while

#else
  assert(0);
#endif // CONTIGUITY_ENABLE && BUDDY_ENABLE && RCPT_ENABLE

  return (ERESULT_TYPE_SUCCESS);
}

//----------------------------------------------------------
// Fill TLB CAMB
// 	Fetch GROUP_SIZE  PTEs
//----------------------------------------------------------
EResultType CMMU::FillTLB_CAMB(int64_t nVA, int64_t nCycle) {
#if defined CONTIGUITY_ENABLE && defined BUDDY_ENABLE
  int64_t nVPN = this->GetVPNNum(nVA);
  int64_t nAVPN = nVPN & (~(NUM_PTE_PTW - 1));

  int nSet = -1;
  int nWay = -1;

  int nIndexPTE = nAVPN - (this->START_VPN);
  nAVPN = nAVPN & (~(this->spPageTable[nIndexPTE]->nBlockSize - 1)); // Solve redundant group, example, if GROUP_SIZE is
                                                                     // 4 and BlockSize of block is 8
  int64_t nNewVPN = nAVPN;

  for (int j = 0; j < NUM_PTE_PTW; j++) {
    if (this->spPageTable[nIndexPTE + j]->nBlockSize == 0) // ignore invalid PTE
      continue;

    // Check TLB hit (to avoid redundant allocation)
    EResultType IsTLB_Hit = this->IsTLB_Hit(nNewVPN);
    if (IsTLB_Hit != ERESULT_TYPE_YES) {

      // Find victim to replace
      nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nAVPN
      nSet = this->GetSetNum(nNewVPN);

      this->spTLB[nSet][nWay]->nContiguity = this->spPageTable[nIndexPTE + j]->nBlockSize - 1;
      this->spTLB[nSet][nWay]->nValid = 1;
      this->spTLB[nSet][nWay]->nPPN = this->spPageTable[nIndexPTE + j]->nPPN;
      this->spTLB[nSet][nWay]->nVPN = nNewVPN;

      this->spTLB[nSet][nWay]->ePageSize = EPAGE_SIZE_TYPE_4KB;
      this->spTLB[nSet][nWay]->nTimeStamp = nCycle - j;

      if (nVPN == nNewVPN) {
        this->spTLB[nSet][nWay]->nTimeStamp = nCycle; // Demanding VPN priority
      }
    }

    nNewVPN += this->spPageTable[nIndexPTE + j]->nBlockSize;
  }

#else
  assert(0);
#endif // CONTIGUITY_ENABLE && BUDDY_ENABLE

  return (ERESULT_TYPE_SUCCESS);
}

//----------------------------------------------------------
// Fill TLB
// 	Fetch multiple PTEs
// 	(1) Traditional (also PF)
// 	(2) PCA
//----------------------------------------------------------
EResultType CMMU::FillTLB_BF(int64_t nVA, int64_t nCycle) {

  // Debug
  // this->CheckMMU();

  int nSet = -1;
  int nWay = -1;
  int64_t nVPN = -1;

  int NUM_ENTRY = NUM_PTE_PTW; // "BLOCK_FETCH": 16 PTEs per PTW, when TLB entries 16

  nVPN = this->GetVPNNum(nVA);
  nSet = this->GetSetNum(nVPN);

  nVPN = nVPN & (~(NUM_PTE_PTW - 1)); // Align VPNs, 0xFFFF_FFF0, when NUM_PTE_PTW 16

  for (int j = 0; j < NUM_ENTRY; j++) {

    int64_t nNewVPN = nVPN + j;

#ifdef CONTIGUITY_ENABLE

#ifdef PCA_ENABLE
//------------------------------------------------------------
// PCA
//------------------------------------------------------------
#ifdef CONTIGUITY_0_PERCENT
    //---------------------------------------------------
    // VPN		0  1  2	 3	4  5  6  7	8 ...
    // Contiguity	0  0  0	 0	0  0  0  0	0 ...
    //---------------------------------------------------
    // Find victim to replace
    nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nVPN

    this->spTLB[nSet][nWay]->nContiguity = 0; // No page contiguous
#endif

#ifdef CONTIGUITY_25_PERCENT
    if (PAGE_TABLE_GRANULE == 4) {
      //---------------------------------------------------
      // VPN		0  1  2	 3	4  5  6  7	8 ...
      // Contiguity	1  -  0	 0	1  -  0  0	1 ...
      //---------------------------------------------------
      if (nNewVPN % 4 == 1)
        continue; // Do not allocate

      // Find victim to replace
      nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nVPN

      this->spTLB[nSet][nWay]->nContiguity = 0;

      if (nNewVPN % 4 == 0) {
        this->spTLB[nSet][nWay]->nContiguity = 1; // Next 1 page contiguous
      };
    } else if (PAGE_TABLE_GRANULE == 8) {
      //---------------------------------------------------
      // VPN		0  1  2	 3  4  5  6  7	   8 ...
      // Contiguity	2  -  -	 0  0  0  0  0	   2 ...
      //---------------------------------------------------
      if (nNewVPN % 8 == 1)
        continue; // Do not allocate
      if (nNewVPN % 8 == 2)
        continue;

      // Find victim to replace
      nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nVPN

      this->spTLB[nSet][nWay]->nContiguity = 0;

      if (nNewVPN % 8 == 0) {
        this->spTLB[nSet][nWay]->nContiguity = 2; // Next 2 page contiguous
      };
    } else {
      assert(0);
    };
#endif

#ifdef CONTIGUITY_50_PERCENT
    if (PAGE_TABLE_GRANULE == 4) {
      //---------------------------------------------------
      // VPN		0  1  2	 3	4  5  6  7	8 ...
      // Contiguity	2  -  -	 0	2  -  -  0	2 ...
      //---------------------------------------------------
      if (nNewVPN % 4 == 1)
        continue;
      if (nNewVPN % 4 == 2)
        continue;

      // Find victim to replace
      nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nVPN

      this->spTLB[nSet][nWay]->nContiguity = 0;

      if (nNewVPN % 4 == 0) {
        this->spTLB[nSet][nWay]->nContiguity = 2; // Next 2 pages contiguous
      };
    } else if (PAGE_TABLE_GRANULE == 8) {
      //---------------------------------------------------
      // VPN		0  1  2	 3  4  5  6  7	   8 ...
      // Contiguity	4  -  -	 -  -  0  0  0	   4 ...
      //---------------------------------------------------
      if (nNewVPN % 8 == 1)
        continue;
      if (nNewVPN % 8 == 2)
        continue;
      if (nNewVPN % 8 == 3)
        continue;
      if (nNewVPN % 8 == 4)
        continue;

      // Find victim to replace
      nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nVPN

      this->spTLB[nSet][nWay]->nContiguity = 0;

      if (nNewVPN % 8 == 0) {
        this->spTLB[nSet][nWay]->nContiguity = 4; // Next 4 pages contiguous
      };
    } else {
      assert(0);
    };
#endif

#ifdef CONTIGUITY_75_PERCENT
    if (PAGE_TABLE_GRANULE == 4) {
      //---------------------------------------------------
      // VPN		0  1  2	 3	4  5  6  7	8 ...
      // Contiguity	3  -  -	 -	3  -  -  -	3 ...
      //---------------------------------------------------
      if (nNewVPN % 4 == 1)
        continue;
      if (nNewVPN % 4 == 2)
        continue;
      if (nNewVPN % 4 == 3)
        continue;

      // Find victim to replace
      nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nVPN

      this->spTLB[nSet][nWay]->nContiguity = 0;

      if (nNewVPN % 4 == 0) {
        this->spTLB[nSet][nWay]->nContiguity = 3; // Next 3 pages contiguous
      };
    } else if (PAGE_TABLE_GRANULE == 8) {
      //---------------------------------------------------
      // VPN		0  1  2	 3  4  5  6  7	   8 ...
      // Contiguity	6  -  -	 -  -  -  -  0	   6 ...
      //---------------------------------------------------
      if (nNewVPN % 8 == 1)
        continue;
      if (nNewVPN % 8 == 2)
        continue;
      if (nNewVPN % 8 == 3)
        continue;
      if (nNewVPN % 8 == 4)
        continue;
      if (nNewVPN % 8 == 5)
        continue;
      if (nNewVPN % 8 == 6)
        continue;

      // Find victim to replace
      nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nVPN

      this->spTLB[nSet][nWay]->nContiguity = 0;

      if (nNewVPN % 8 == 0) {
        this->spTLB[nSet][nWay]->nContiguity = 6; // Next 6 pages contiguous
      };
    } else {
      assert(0);
    };
#endif

#ifdef CONTIGUITY_100_PERCENT
    //---------------------------------------------------
    // VPN		0		  1  2  3   4  5  6  7  8 ...
    // Contiguity   TOTAL_PAGE_NUM-1  -  -  -   -  -  -
    //---------------------------------------------------
    int64_t nVPN_aligned = nNewVPN >> L2_MAX_ORDER << L2_MAX_ORDER; // 2^(L2_MAX_ORDER) page aligned
    int64_t nVPN_diff = nNewVPN - nVPN_aligned;
    if (nVPN_diff > 0 and nVPN_diff < TOTAL_PAGE_NUM) {
      continue;
    };

    // Find victim to replace
    nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nVPN

    this->spTLB[nSet][nWay]->nContiguity = 0;

    if (nVPN_diff < TOTAL_PAGE_NUM) {
      this->spTLB[nSet][nWay]->nContiguity = TOTAL_PAGE_NUM - nVPN_diff - 1; // Next pages contiguous
    };
#endif

#endif // "PCA_ENABLE"
#endif // "CONTIGUITY_ENABLE"

    // Find victim to replace
    nWay = this->GetVictim_TLB(nNewVPN); // Be careful. Not nVPN

#ifdef DBPF_ENABLE
    nWay = j;
#endif

    // Fill new entry
    this->spTLB[nSet][nWay]->nValid = 1;
    this->spTLB[nSet][nWay]->nVPN = nNewVPN;

#if defined PAGE_SIZE_4KB
    this->spTLB[nSet][nWay]->ePageSize = EPAGE_SIZE_TYPE_4KB;
    this->spTLB[nSet][nWay]->nPPN = rand() % 0x3FFFF; // Mapping table random 20-bit. nPPN is signed int
                                                      // type. MSB suppose 0
#elif defined PAGE_SIZE_1MB
    this->spTLB[nSet][nWay]->ePageSize = EPAGE_SIZE_TYPE_1MB;
    this->spTLB[nSet][nWay]->nPPN = rand() % 0x7FF; // Mapping table random 12-bit. nPPN signed int type.
                                                    // MSB suppose 0.
#endif

#ifdef VPN_SAME_PPN
    this->spTLB[nSet][nWay]->nPPN = nNewVPN; // VPN equal to PPN
#endif

    // Allocate time
    // this->spTLB[nSet][nWay]->nTimeStamp = nCycle + j;
    // // For replacement. Add j to make order anyway
    this->spTLB[nSet][nWay]->nTimeStamp = nCycle - j - 1; //

    if (nNewVPN == GetVPNNum(nVA)) {
      this->spTLB[nSet][nWay]->nTimeStamp = nCycle; // Demanding VPN priority
    };
  };

#ifdef DBPF_ENABLE
  this->nHit_TLB_After_Fill_DBPF = 0;
#endif

  return (ERESULT_TYPE_SUCCESS);
};

//----------------------------------------------------------
// Fill TLB1
// 	Same logic as TLB
// 	Only supports "no contiguity"
// 	DBPF must be enabled.
//----------------------------------------------------------
EResultType CMMU::FillTLB1_BF_DBPF(int64_t nVA, int64_t nCycle) {

  // Debug
  // this->CheckMMU();

#ifndef DBPF_ENABLE
  assert(0);
#endif

  int nSet = -1;
  int nWay = -1;
  int64_t nVPN = -1;

  int NUM_ENTRY = NUM_PTE_PTW; // "BLOCK_FETCH": 16 PTEs per PTW, when TLB entries 16

  nVPN = this->GetVPNNum(nVA);
  nSet = this->GetSetNum(nVPN);

  nVPN = nVPN & (~(NUM_PTE_PTW - 1)); // Align VPNs, 0xFFFF_FFF0, when NUM_PTE_PTW 16

  for (int j = 0; j < NUM_ENTRY; j++) {

    int64_t nNewVPN = nVPN + j;

    // Find victim to replace
    nWay = this->GetVictim_TLB1_DBPF(nNewVPN); // Be careful. Not nVPN

#ifdef DBPF_ENABLE
    nWay = j;
#endif

    // Fill new entry
    this->spTLB1[nSet][nWay]->nValid = 1;
    this->spTLB1[nSet][nWay]->nVPN = nNewVPN;

#if defined PAGE_SIZE_4KB
    this->spTLB1[nSet][nWay]->ePageSize = EPAGE_SIZE_TYPE_4KB;
    this->spTLB1[nSet][nWay]->nPPN = rand() % 0x3FFFF; // Mapping table random 20-bit. nPPN is signed int
                                                       // type. MSB suppose 0
#elif defined PAGE_SIZE_1MB
    this->spTLB1[nSet][nWay]->ePageSize = EPAGE_SIZE_TYPE_1MB;
    this->spTLB1[nSet][nWay]->nPPN = rand() % 0x7FF; // Mapping table random 12-bit. nPPN signed int type.
                                                     // MSB suppose 0.
#endif

#ifdef VPN_SAME_PPN
    this->spTLB1[nSet][nWay]->nPPN = nNewVPN; // VPN equal to PPN
#endif

    // Allocate time
    this->spTLB1[nSet][nWay]->nTimeStamp = nCycle + j; // For replacement. Add j to make order anyway
  };

#ifdef DBPF_ENABLE
  this->nHit_TLB1_After_Fill_DBPF = 0;
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// Fill FLPD
EResultType CMMU::FillFLPD(int nEntry, int64_t nVPN, int64_t nCycle) {

// Debug
// this->CheckMMU();

// Fill new entry
#ifdef DEBUG_MMU
  printf("in FillFLPD - VPN[47:39]: 0x%lx, nEntry: %d\n", nVPN, nEntry);
#endif // DEBUG_MMU

  this->spFLPD[nEntry]->nValid = 1;
  this->spFLPD[nEntry]->nVPN = nVPN;
  this->spFLPD[nEntry]->nPPN = rand() % 0x8000000000;
  this->spFLPD[nEntry]->ePageSize = EPAGE_SIZE_TYPE_512GB;
  this->spFLPD[nEntry]->nTimeStamp = nCycle;

#ifdef VPN_SAME_PPN
  this->spFLPD[nEntry]->nPPN = nVPN; // VPN equals to PPN
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// Fill SLPD
EResultType CMMU::FillSLPD(int nEntry, int64_t nVPN, int64_t nCycle) {

// Debug
// this->CheckMMU();

// Fill new entry
#ifdef DEBUG_MMU
  printf("in FillSLPD - VPN[47:30]: 0x%lx, nEntry: %d\n", nVPN, nEntry);
#endif // DEBUG_MMU

  this->spFLPD[nEntry]->nValid = 1;
  this->spFLPD[nEntry]->nVPN = nVPN;
  this->spFLPD[nEntry]->nPPN = rand() % 0x40000000;
  this->spFLPD[nEntry]->ePageSize = EPAGE_SIZE_TYPE_1GB;
  this->spFLPD[nEntry]->nTimeStamp = nCycle;

#ifdef VPN_SAME_PPN
  this->spFLPD[nEntry]->nPPN = nVPN; // VPN equals to PPN
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// Fill TLPD
EResultType CMMU::FillTLPD(int nEntry, int64_t nVPN, int64_t nCycle) {

// Debug
// this->CheckMMU();

// Fill new entry
#ifdef DEBUG_MMU
  printf("in FillTLPD - VPN[47:21]: 0x%lx, nEntry: %d\n", nVPN, nEntry);
#endif // DEBUG_MMU

  this->spFLPD[nEntry]->nValid = 1;
  this->spFLPD[nEntry]->nVPN = nVPN;
  this->spFLPD[nEntry]->nPPN = rand() % 0x20000;
  this->spFLPD[nEntry]->ePageSize = EPAGE_SIZE_TYPE_2MB;
  this->spFLPD[nEntry]->nTimeStamp = nCycle;

#ifdef VPN_SAME_PPN
  this->spFLPD[nEntry]->nPPN = nVPN; // VPN equals to PPN
#endif

  return (ERESULT_TYPE_SUCCESS);
};

//-------------------------------------------------------
// Get Tag number
// 	Tag number is the upper (32 - MMU_BIT_SET - BIT_4K_PAGE) bits. Arch32
// 	Tag number is the upper (64 - MMU_BIT_SET - BIT_4K_PAGE) bits. Arch64
//
// 	Note	: Tag number is calculated considering "Tag + Index" structure.
// In perf model, we use "vpn" so tag number not necessary.
//-------------------------------------------------------
int64_t CMMU::GetTagNum(int64_t nVA) {

  // Debug
  // this->CheckMMU();

  int64_t nTagNum = nVA >> (MMU_BIT_SET + BIT_4K_PAGE);

  return (nTagNum);
};

//-------------------------------------------------------
// Check TLB hit
//-------------------------------------------------------
//	Check hit
//		"Requested VPN" within "(VPN, VPN + contiguity)"
//	Assume
//		Page size 4kB
//---------------------------------------------------
EResultType CMMU::IsTLB_Hit(int64_t nVPN) {

  // Debug
  // this->CheckMMU();

  // Set number
  int nSet = this->GetSetNum(nVPN);

  // Find Tag
  for (int j = 0; j < MMU_NUM_WAY; j++) {
    // Check valid and Tag
    if (this->spTLB[nSet][j]->nValid != 1) {
      continue;
    };

    // Check requested VPN hit
    int64_t nVPN_low = this->spTLB[nSet][j]->nVPN;
    int64_t nVPN_high = this->spTLB[nSet][j]->nVPN + this->spTLB[nSet][j]->nContiguity;

    if (nVPN_low <= nVPN and nVPN_high >= nVPN) {
      return (ERESULT_TYPE_YES);
    };
  };

  // #ifdef DBPF_ENABLE
  // // Find Tag. TLB1
  // for (int j=0; j<MMU_NUM_WAY; j++) {
  // 	// Check valid and Tag
  // 	if (this->spTLB1[nSet][j]->nValid != 1) {
  // 		continue;
  // 	};
  //
  // 	// Check requested VPN hit
  // 	int64_t nVPN_low  = this->spTLB1[nSet][j]->nVPN;
  // 	int64_t nVPN_high = this->spTLB1[nSet][j]->nVPN +
  // this->spTLB1[nSet][j]->nContiguity;
  //
  // 	if (nVPN_low <= nVPN and nVPN_high >= nVPN ) {
  // 		return (ERESULT_TYPE_YES);
  // 	};
  // };
  // #endif

  return (ERESULT_TYPE_NO);
};

//-------------------------------------------------------
// Check TLB1 hit
//-------------------------------------------------------
// 	Same logic as TLB
// 	Only used as "buf1" DBPF
// 	DBPF must be enabled.
//---------------------------------------------------
EResultType CMMU::IsTLB1_Hit_DBPF(int64_t nVPN) {

// Debug
// this->CheckMMU();
//
#ifndef DBPF_ENABLE
  assert(0);
#endif

  // Set number
  int nSet = this->GetSetNum(nVPN);

  // Find Tag. TLB1
  for (int j = 0; j < MMU_NUM_WAY; j++) {
    // Check valid and Tag
    if (this->spTLB1[nSet][j]->nValid != 1) {
      continue;
    };

    // Check requested VPN hit
    int64_t nVPN_low = this->spTLB1[nSet][j]->nVPN;
    int64_t nVPN_high = this->spTLB1[nSet][j]->nVPN + this->spTLB1[nSet][j]->nContiguity;

    if (nVPN_low <= nVPN and nVPN_high >= nVPN) {
      return (ERESULT_TYPE_YES);
    };
  };

  return (ERESULT_TYPE_NO);
};

//-------------------------------------------------------
// Check TLB hit
// 	If LRU, update timestamp (to remember access time)
//-------------------------------------------------------
//	Check hit
//		1. Traditional	: "Requested VPN" = "VPN"
//		2. PCA		: "Requested VPN" within "(VPN, VPN +
// contiguity)"
//		3. BCT		: "Requested VPN" within "(StartVPN, StartVPN +
// contiguity)". Contiguity 2^n
//		4. PCAD		: "Requested VPN" within "(StartVPN, StartVPN +
// contiguity)"
//		5. AT		: "Requested VPN" within "(StartVPN, StartVPN +
// contiguity)"
//		6. NAPOT	:  "Requested VPN" within [StartVPN, StartVPN +
//(1 << NAPOT_BITS))"
//---------------------------------------------------
EResultType CMMU::IsTLB_Hit(int64_t nVA, int64_t nCycle) {

  // Debug
  // this->CheckMMU();

  int64_t nVPN = GetVPNNum(nVA);

  // Set number
  int nSet = this->GetSetNum(nVPN);

  // Find Tag
  for (int j = 0; j < MMU_NUM_WAY; j++) {
    // Check valid and Tag
    if (this->spTLB[nSet][j]->nValid != 1) {
      continue;
    };

    //---------------------------------
    // Contiguity enable
    //---------------------------------
    // #ifdef CONTIGUITY_ENABLE
    // #ifdef PAGE_SIZE_4KB
    // Check requested VPN hit
    int64_t nVPN_low = this->spTLB[nSet][j]->nVPN;
    int64_t nVPN_high = this->spTLB[nSet][j]->nVPN + this->spTLB[nSet][j]->nContiguity;
#ifdef NAPOT
    if (this->spTLB[nSet][j]->N_flag == 1) {
      nVPN_high = this->spTLB[nSet][j]->nVPN + (1 << NAPOT_BITS) - 1;
    } else {
      nVPN_high = this->spTLB[nSet][j]->nVPN;
    }
#endif // NAPOT

    if (nVPN_low <= nVPN and nVPN_high >= nVPN) { // Hit. Within VPN range

#ifdef MMU_REPLACEMENT_LRU
      this->spTLB[nSet][j]->nTimeStamp = nCycle;
#endif

#ifdef DBPF_ENABLE
      this->nHit_TLB_After_Fill_DBPF++;
#endif

      return (ERESULT_TYPE_YES);
    };

    // #endif
    // #endif

    // //---------------------------------
    // // Traditional case. Contiguity disable.
    // //---------------------------------
    // if (this->spTLB[nSet][j]->ePageSize == EPAGE_SIZE_TYPE_4KB) {
    //
    // 	if (this->spTLB[nSet][j]->nVPN == nVPN) {
    // // Hit. same VPN. 		#ifdef MMU_REPLACEMENT_LRU
    // 		this->spTLB[nSet][j]->nTimeStamp = nCycle;
    // 		#endif
    // 		return (ERESULT_TYPE_YES);
    // 	};
    // }
    // else if (this->spTLB[nSet][j]->ePageSize == EPAGE_SIZE_TYPE_1MB) {
    // 	if (this->spTLB[nSet][j]->nVPN == nVPN) {
    // // Hit. same VPN. 		#ifdef MMU_REPLACEMENT_LRU
    // 		this->spTLB[nSet][j]->nTimeStamp = nCycle;
    // 		#endif
    // 		return (ERESULT_TYPE_YES);
    // 	};
    // }
    // else {
    // 	assert (0);
    // };
  };

  // #ifdef DBPF_ENABLE
  // // Find Tag
  // for (int j=0; j<MMU_NUM_WAY; j++) {
  // 	// Check valid and Tag
  // 	if (this->spTLB1[nSet][j]->nValid != 1) {
  // 		continue;
  // 	};
  //
  // 	// Check requested VPN hit
  // 	int64_t nVPN_low  = this->spTLB1[nSet][j]->nVPN;
  // 	int64_t nVPN_high = this->spTLB1[nSet][j]->nVPN +
  // this->spTLB1[nSet][j]->nContiguity;
  //
  // 	if ( nVPN_low <= nVPN and nVPN_high >= nVPN ) {
  // // Hit. Within VPN range 		#ifdef MMU_REPLACEMENT_LRU
  // 		this->spTLB1[nSet][j]->nTimeStamp = nCycle;
  // 		#endif
  // 		return (ERESULT_TYPE_YES);
  // 	};
  // };
  // #endif

  return (ERESULT_TYPE_NO);
};

//-------------------------------------------------------
// Check TLB hit. AT. "Regular" or "Anchor"
// 	If LRU, update timestamp (to remember access time)
//	Traditional way : "Requested VPN" = "VPN"
//---------------------------------------------------
EResultType CMMU::IsTLB_Allocate_AT(int64_t nVA, int64_t nCycle) {

  // Debug
  // this->CheckMMU();

  int64_t nVPN = GetVPNNum(nVA);

  // Set number
  int nSet = this->GetSetNum(nVPN);

  // Find Tag
  for (int j = 0; j < MMU_NUM_WAY; j++) {
    // Check valid and Tag
    if (this->spTLB[nSet][j]->nValid != 1) {
      continue;
    };

    //---------------------------------
    // Traditional case. Contiguity disable.
    //---------------------------------
    if (this->spTLB[nSet][j]->ePageSize == EPAGE_SIZE_TYPE_4KB) {

      if (this->spTLB[nSet][j]->nVPN == nVPN) { // Hit. same VPN.
#ifdef DEBUG_MMU
        printf("match VNP 0x%lx at nSet %d, nWay %d\n", nVPN, nSet, j);
#endif

#ifdef MMU_REPLACEMENT_LRU
        this->spTLB[nSet][j]->nTimeStamp = nCycle;
#endif
        return (ERESULT_TYPE_YES);
      };
    } else if (this->spTLB[nSet][j]->ePageSize == EPAGE_SIZE_TYPE_1MB) {
      if (this->spTLB[nSet][j]->nVPN == nVPN) { // Hit. same VPN.

#ifdef MMU_REPLACEMENT_LRU
        this->spTLB[nSet][j]->nTimeStamp = nCycle;
#endif
        return (ERESULT_TYPE_YES);
      };
    } else {
      assert(0);
    };
  };

  return (ERESULT_TYPE_NO);
};

//-------------------------------------------------------
// Check contiguity match in TLB
// 	If LRU, update timestamp (to remember access time)
//-------------------------------------------------------
//	AT		: "Requested VPN" within "(StartVPN, StartVPN +
// contiguity)"
//---------------------------------------------------
EResultType CMMU::IsTLB_Contiguity_Match_AT(int64_t nVA, int64_t nCycle) {

  // Debug
  // this->CheckMMU();

  int64_t nVPN = GetVPNNum(nVA);

  // int64_t nVPN_anchor = nVPN - (nVPN % PAGE_TABLE_GRANULE);
  // // VPN align	 DUONGTRAN comment
  int64_t nVPN_anchor = nVPN - (nVPN % this->anchorDistance); // VPN align	 DUONGTRAN add

#ifdef CONTIGUITY_100_PERCENT
  nVPN_anchor = (nVPN >> L2_MAX_ORDER) << L2_MAX_ORDER; // 2^(L2_MAX_ORDER) page align
#endif

  // Set number
  // int nSet = this->GetSetNum(nVPN);  // FIXME DUONGTRAN comment
  int nSet = this->GetSetNum(nVPN_anchor); // FIXME DUONGTRAN add

  // Find Tag
  for (int j = 0; j < MMU_NUM_WAY; j++) {
    // Check valid and Tag
    if (this->spTLB[nSet][j]->nValid != 1) {
      continue;
    };

    // Check anchor allocated
    if (nVPN_anchor != this->spTLB[nSet][j]->nVPN) {
      continue;
    };

    int64_t nVPN_high = this->spTLB[nSet][j]->nVPN + this->spTLB[nSet][j]->nContiguity -
                        1; // DuongTran: -1 due to AT consider signle entry is contiguity-1

    // Check anchor hit
    // if (nVPN <= nVPN_high) {						// Hit.
    // Within VPN range
    if ((nVPN <= nVPN_high) && (this->spTLB[nSet][j]->nContiguity > 0)) { // Hit. Within VPN range   // DUONGTRAN add

#ifdef MMU_REPLACEMENT_LRU
      this->spTLB[nSet][j]->nTimeStamp = nCycle;
#endif
      return (ERESULT_TYPE_YES);
    };
  };

  return (ERESULT_TYPE_NO);
};

//-------------------------------------------------------
// Check Anchor entry allocated in TLB. AT.
//-------------------------------------------------------
EResultType CMMU::IsTLB_Allocate_Anchor_AT(int64_t nVA) { // VA original

  // Debug
  // this->CheckMMU();

  int64_t nVPN = GetVPNNum(nVA);

  // int64_t nVPN_anchor = nVPN - (nVPN % PAGE_TABLE_GRANULE);
  // // VPN align	DUONGTRAN comment
  int64_t nVPN_anchor = nVPN - (nVPN % this->anchorDistance); // VPN align	DUONGTRAN add

#ifdef CONTIGUITY_100_PERCENT
  nVPN_anchor = (nVPN >> L2_MAX_ORDER) << L2_MAX_ORDER; // 2^(L2_MAX_ORDER) page align
#endif

  // Set number
  int nSet = this->GetSetNum(nVPN);

  // Find Tag
  for (int j = 0; j < MMU_NUM_WAY; j++) {
    // Check valid and Tag
    if (this->spTLB[nSet][j]->nValid != 1) {
      continue;
    };

    // Check anchor allocated
    if (nVPN_anchor == this->spTLB[nSet][j]->nVPN) {
#ifdef DEBUG_MMU
      printf("match AT nVPN 0x%lx at nSet %d, nWay %d\n", nVPN, nSet, j);
#endif
      return (ERESULT_TYPE_YES);
    };
  };

  return (ERESULT_TYPE_NO);
};

//-------------------------------------------------------
// Check anchor (of VA) can cover
// 	Given page table and VA, possible to check.
//-------------------------------------------------------
EResultType CMMU::IsAnchor_Cover_AT(int64_t nVA) { // Can anchor cover VA

  // Debug
  // this->CheckMMU();

#ifdef CONTIGUITY_ENABLE // FIXME
  int64_t nVPN = GetVPNNum(nVA);

  // int64_t nVPN_anchor = nVPN - (nVPN % PAGE_TABLE_GRANULE);
  // // VPN align	DUONGTRAN comment
  int64_t nVPN_anchor = nVPN - (nVPN % this->anchorDistance); // VPN align	DUONGTRAN add

#endif

#ifdef CONTIGUITY_100_PERCENT
  nVPN_anchor = (nVPN >> L2_MAX_ORDER) << L2_MAX_ORDER; // 2^(L2_MAX_ORDER) page align
#endif

  if (nVPN - nVPN_anchor < TOTAL_PAGE_NUM)
    return ERESULT_TYPE_YES; // FIXME DUONGTRAN add FIXME URGENT

  return (ERESULT_TYPE_NO);
};

//-------------------------------------------------------
// Check regular entry. AT.
//-------------------------------------------------------
EResultType CMMU::IsVA_Regular_AT(int64_t nVA) {

  // Debug
  // this->CheckMMU();

  int64_t nVPN = GetVPNNum(nVA);

  // int64_t nVPN_anchor = nVPN - (nVPN % PAGE_TABLE_GRANULE);
  // // VPN align	DUONGTRAN comment
  int64_t nVPN_anchor = nVPN - (nVPN % this->anchorDistance); // VPN align	DUONGTRAN add

#ifdef CONTIGUITY_100_PERCENT
  nVPN_anchor = (nVPN >> L2_MAX_ORDER) << L2_MAX_ORDER; // 2^(L2_MAX_ORDER) page align
#endif

  // Check regular
  if (nVPN != nVPN_anchor) {
    return (ERESULT_TYPE_YES);
  };

  return (ERESULT_TYPE_NO);
};

//-------------------------------------------------------
// Check anchor entry. AT.
//-------------------------------------------------------
EResultType CMMU::IsVA_Anchor_AT(int64_t nVA) {

  // Debug
  // this->CheckMMU();

  int64_t nVPN = GetVPNNum(nVA);

  // int64_t nVPN_anchor = nVPN - (nVPN % PAGE_TABLE_GRANULE);
  // // VPN align	DUONGTRAN comment
  int64_t nVPN_anchor = nVPN - (nVPN % this->anchorDistance); // VPN align	DUONGTRAN add

#ifdef CONTIGUITY_100_PERCENT
  nVPN_anchor = (nVPN >> L2_MAX_ORDER) << L2_MAX_ORDER; // 2^(L2_MAX_ORDER) page align
#endif

  // Check regular
  if (nVPN == nVPN_anchor) {
    return (ERESULT_TYPE_YES);
  };

  return (ERESULT_TYPE_NO);
};

//--------------------------------------------------------
// Check FLPD hit
// 	If LRU, update timestamp (to remember access time)
//--------------------------------------------------------
EResultType CMMU::IsFLPD_Hit(int64_t nVA, int64_t nCycle) {

  // Debug
  // this->CheckMMU();

  // Find Tag
  for (int j = 0; j < NUM_FLPD_ENTRY; j++) {
    // Check valid and Tag
    if (this->spFLPD[j]->nValid != 1) {
      continue;
    };

    if ((this->spFLPD[j]->nVPN == (nVA >> BIT_512GB_PAGE)) &&
        (this->spFLPD[j]->ePageSize == EPAGE_SIZE_TYPE_512GB)) { // 512GB. 39 bits

#ifdef MMU_REPLACEMENT_LRU
      this->spFLPD[j]->nTimeStamp = nCycle;
#endif

      return (ERESULT_TYPE_YES);
    };
  };
  return (ERESULT_TYPE_NO);
};

//--------------------------------------------------------
// Check SLPD hit
// 	If LRU, update timestamp (to remember access time)
//--------------------------------------------------------
EResultType CMMU::IsSLPD_Hit(int64_t nVA, int64_t nCycle) {

  // Debug
  // this->CheckMMU();

  // Find Tag
  for (int j = 0; j < NUM_FLPD_ENTRY; j++) {
    // Check valid and Tag
    if (this->spFLPD[j]->nValid != 1) {
      continue;
    };

    if ((this->spFLPD[j]->nVPN == (nVA >> BIT_1GB_PAGE)) &&
        (this->spFLPD[j]->ePageSize == EPAGE_SIZE_TYPE_1GB)) { // 1GB. 30 bits

#ifdef MMU_REPLACEMENT_LRU
      this->spFLPD[j]->nTimeStamp = nCycle;
#endif

      return (ERESULT_TYPE_YES);
    };
  };
  return (ERESULT_TYPE_NO);
};

//--------------------------------------------------------
// Check TLPD hit
// 	If LRU, update timestamp (to remember access time)
//--------------------------------------------------------
EResultType CMMU::IsTLPD_Hit(int64_t nVA, int64_t nCycle) {

  // Debug
  // this->CheckMMU();

  // Find Tag
  for (int j = 0; j < NUM_FLPD_ENTRY; j++) {
    // Check valid and Tag
    if (this->spFLPD[j]->nValid != 1) {
      continue;
    };

    if ((this->spFLPD[j]->nVPN == (nVA >> BIT_2MB_PAGE)) &&
        (this->spFLPD[j]->ePageSize == EPAGE_SIZE_TYPE_2MB)) { // 2MB. 21 bits

#ifdef MMU_REPLACEMENT_LRU
      this->spFLPD[j]->nTimeStamp = nCycle;
#endif

      return (ERESULT_TYPE_YES);
    };
  };
  return (ERESULT_TYPE_NO);
};

//--------------------------------------
// Get VPN number
//--------------------------------------
int64_t CMMU::GetVPNNum(int64_t nVA) {

  // Debug
  // this->CheckMMU();

#if defined PAGE_SIZE_4KB
  int64_t nVPNNum = nVA >> BIT_4K_PAGE;
#elif defined PAGE_SIZE_1MB
  int64_t nVPNNum = nVA >> BIT_1M_PAGE;
#endif

  return (nVPNNum);
};

//--------------------------------------
// Get Set number
//--------------------------------------
//	1. Traditional	: Tag + Index
//	2. PCA		: Tag1 + index + tag2. Tag2 7 bits
//	3. BCT		: Tag1 + index + tag2. Tag2 7 bits
//	4. PCAD		: Tag1 + index + tag2. Tag2 7 bits
//	5. RMM		: Tag1 + index + tag2. Tag2 7 bits
//	6. AT		: Tag1 + index + tag2. Tag2 7 bits
//--------------------------------------
int CMMU::GetSetNum(int64_t nVPN) {

  // Debug
  // this->CheckMMU();

  // Set index (block number) modulo (number of sets)
  int nSet = nVPN % MMU_NUM_SET; // FIXME Check bug in SA cache. << Tag1 bits.

#ifdef CONTIGUITY_ENABLE
  nSet = (nVPN >> L2_MAX_ORDER) % MMU_NUM_SET;
#endif

  return (nSet);
};

//--------------------------------------
// Get physical address
//--------------------------------------
//	1. Traditional
//	2. PCA		: PPN = (Requested VPN - "StartVPN") + "PPN"
//	3. BCT		: PPN = (Requested VPN - "StartVPN") + "PPN"
//	4. PCAD		: PPN = (Requested VPN - "StartVPN") + "PPN"
//	5. AT		: PPN = (Requested VPN - "StartVPN") + "PPN"
//	6. NAPOT	:
//--------------------------------------
int CMMU::GetPA_TLB(int64_t nVA) { // VA original

  // Debug
  // this->CheckMMU();

  int64_t nVPN = GetVPNNum(nVA);

  // Get set number
  int nSet = this->GetSetNum(nVPN);

  int nPPN = -1;
  int nPA = -1;
  int nOffset = -1;

#ifdef PAGE_SIZE_4KB
  nOffset = nVA & 0x00000FFF; // Offset 12 bit
#endif
#ifdef PAGE_SIZE_1MB
  nOffset = nVA & 0x000FFFFF; // Offset 20 bit
#endif

  // Find Tag
  for (int j = 0; j < MMU_NUM_WAY; j++) {

    // Check valid and Tag
    if (this->spTLB[nSet][j]->nValid != 1) {
      continue;
    };

    int64_t nVPN_low = this->spTLB[nSet][j]->nVPN;
    int64_t nVPN_high = this->spTLB[nSet][j]->nVPN + this->spTLB[nSet][j]->nContiguity;
#ifdef NAPOT
    nVPN_high = this->spTLB[nSet][j]->nVPN + (1 << NAPOT_BITS) - 1;
#endif
    int64_t nVPN_diff = nVPN - this->spTLB[nSet][j]->nVPN;

#ifdef DEBUG_MMU
    assert(nVPN_low >= 0);
    assert(nVPN_high >= 0);
    assert(nVPN_high <= 0x7FFFFFFFFFFFF);
    assert(this->spTLB[nSet][j]->nPPN >= 0);
    assert(this->spTLB[nSet][j]->nPPN <= 0x3FFFF);
#endif

    if (nVPN_low <= nVPN and nVPN_high >= nVPN) {    // Hit. Within VPN range
      nPPN = nVPN_diff + this->spTLB[nSet][j]->nPPN; // PPN contiguous
      nPA = (nPPN << BIT_4K_PAGE) + nOffset;

#ifdef DEBUG_MMU
      assert(nPPN >= 0);
      assert(nPPN <= 0x7FFFF);
      assert(nPA >= MIN_ADDR);
      assert(nPA <= MAX_ADDR);
      assert(nVPN_diff >= 0);
#endif

      return (nPA);
    };
    // #endif
    // #endif
  };

#ifdef VPN_SAME_PPN
  assert(nPA == nVA);
#endif

#ifdef DEBUG_MMU
  if (nPA == -1)
    printf("This function call only when TLB is hit\n");
  assert(nPA != -1);
#endif

  return (nPA);
};

//--------------------------------------
// Get anchor VA (from requested VA)
//--------------------------------------
// 	Page size 4kB
//--------------------------------------
int64_t CMMU::GetVA_Anchor_AT(int64_t nVA) {

  // Debug
  // this->CheckMMU();

  int64_t nVPN = GetVPNNum(nVA);

  // int64_t nVPN_anchor = nVPN - (nVPN % PAGE_TABLE_GRANULE);
  // // VPN align	DUONGTRAN comment
  int64_t nVPN_anchor = nVPN - (nVPN % this->anchorDistance); // VPN align	DUONGTRAN add

#ifdef CONTIGUITY_100_PERCENT
  nVPN_anchor = (nVPN >> L2_MAX_ORDER) << L2_MAX_ORDER; // 2^(L2_MAX_ORDER) page align
#endif

  int64_t nOffset = nVA & 0x00000FFF; // Offset 12 bit

  int64_t nVA_anchor = (nVPN_anchor << BIT_4K_PAGE) + nOffset;

  return (nVA_anchor);
};

// Get 1st PTW address
int64_t CMMU::GetFirst_PTWAddr(int64_t nVA) {

  // Debug
  // this->CheckMMU();

  //                Upper 18 bits TTBA            VA[31:20)                   4
  //                byte align
  // int nAddr_PTW = ((FIRST_TTBA & 0xFFFFC000) + (((nVA & 0xFFF00000) >> 20) <<
  // 2));	// Max address 0xFFFF FFFF. nAddr unsigned int nAddr_PTW =
  // ((FIRST_TTBA & 0x7FFFC000) + (((nVA & 0x7FF00000) >> 20) << 2));	// Max
  // address 0x7FFF FFFF. nAddr int

  //                                                      VA[47:39] x 8
  int64_t nAddr_PTW = (FIRST_TTBA & 0x0000FFFFFFFFF000) + (((nVA & 0x0000FF8000000000) >> 39) << 3); // DUONG add

  return (nAddr_PTW);
};

// Get 2nd PTW address
int64_t CMMU::GetSecond_PTWAddr(int64_t nVA) {

  // Debug
  // this->CheckMMU();

  //-------------------------------------------
  // Single fetch
  //-------------------------------------------
  //                 Upper 22 bits TTBA            VA[31:20] x 1kB VA[19:12] 4
  //                 byte align
  // int nAddr_PTW = ((SECOND_TTBA & 0xFFFFFC00) + (((nVA & 0xFFF00000) >> 20)
  // << 10) +  (((nVA & 0x000FF000) >> 12) << 2));	// Max address 0xFFFF
  // FFFF. nAddr unsigned int nAddr_PTW = ((SECOND_TTBA & 0x7FFFFC00) + (((nVA &
  // 0x7FF00000) >> 20) << 10) +  (((nVA & 0x000FF000) >> 12) << 2));	// Max
  // address 0x7FFF FFFF. nAddr int

  // FIXME
  // the correct way: access at the  to get SECOND_TTBA, but this simulator
  // don't implement data, hence We use predefined SECOND_TTBA
  //                                                       VA[47:39] x 512 x 8
  //                                                       VA[38:30] x 8
  int64_t nAddr_PTW = (SECOND_TTBA & 0x0000FFFFFFFFF000) + (((nVA & 0x0000FF8000000000) >> 39) << 12) +
                      (((nVA & 0x0000007FC0000000) >> 30) << 3); // DUONG add

  return (nAddr_PTW);
};

// Get 3rd PTW address
int64_t CMMU::GetThird_PTWAddr(int64_t nVA) {

  // FIXME
  // the correct way: access at the second PTW address to get THIRD_TTBA, but
  // this simulator don't implement data, hence We use predefined THIRD_TTBA
  //                                                      VA[47:39] x 512 x 512
  //                                                      x 8 VA[38:30] x 512 x
  //                                                      8 VA[29:21] x 8
  int64_t nAddr_PTW = (THIRD_TTBA & 0x0000FFFFFFFFF000) + (((nVA & 0x0000FF8000000000) >> 39) << 21) +
                      (((nVA & 0x0000007FC0000000) >> 30) << 12) +
                      (((nVA & 0x000000003FE00000) >> 21) << 3); // DUONG add

  return (nAddr_PTW);
};

// Get 4td PTW address
int64_t CMMU::GetFourth_PTWAddr(int64_t nVA) {

  // FIXME
  // the correct way: access at the third PTW address to get FOURTH_TTBA, but
  // this simulator don't implement data, hence We use predefined FOURTH_TTBA
  //                                                       VA[47:39] x 512 x 512
  //                                                       x  512 x 8 VA[38:30]
  //                                                       x 512 x 512 x 8
  //                                                       VA[29:21] x 512 x 8
  //                                                       VA[20:12] x 8
  int64_t nAddr_PTW = (FOURTH_TTBA & 0x0000FFFFFFFFF000) + (((nVA & 0x0000FF8000000000) >> 39) << 30) +
                      (((nVA & 0x0000007FC0000000) >> 30) << 21) + (((nVA & 0x000000003FE00000) >> 21) << 12) +
                      (((nVA & 0x00000000001FF000) >> 12) << 3); // DUONG add

  return (nAddr_PTW);
};

//-----------------------------------------------
// Find way number to replace
//-----------------------------------------------
int CMMU::GetVictim_TLB(int64_t nVPN) {

  // Debug
  // this->CheckMMU();

  int nVictimWay = -1;
  int nSet = GetSetNum(nVPN);

  //----------------------------------
  // Check range hit
  //----------------------------------
#ifdef CONTIGUITY_ENABLE
  // Find Tag
  for (int j = 0; j < MMU_NUM_WAY; j++) {

    // Check valid and Tag
    if (this->spTLB[nSet][j]->nValid != 1) {
      continue;
    };

//---------------------------------
// Contiguity enable
//---------------------------------
#ifdef PAGE_SIZE_4KB
    int64_t nVPN_low = this->spTLB[nSet][j]->nVPN;
    int64_t nVPN_high = this->spTLB[nSet][j]->nVPN + this->spTLB[nSet][j]->nContiguity;
#ifdef AT_ENABLE
    nVPN_high -= 1; // DUONGTRAN: due to AT count from 1
#endif

    if (nVPN_low <= nVPN and nVPN_high >= nVPN) { // Range hit. Within VPN range
      nVictimWay = j;

      // Stat
      this->nTLB_evict++;

#ifdef DEBUG_MMU
      assert(nVictimWay < MMU_NUM_WAY);
      assert(nVictimWay >= 0);
#endif

      return (nVictimWay);
    };
#endif
  };
#endif

  // Get empty slot
  for (int j = 0; j < MMU_NUM_WAY; j++) {
    if (this->spTLB[nSet][j]->nValid != 1) {
      return (j);
    }
  };

#if defined MMU_REPLACEMENT_RANDOM
  // Choose victim way randomly (0,1,...,MMU_NUM_WAY-1)
  nVictimWay = rand() % (MMU_NUM_WAY);

#elif defined MMU_REPLACEMENT_LRU
  // LRU: Choose victim way with minimum (least recently used) timestamp
  nVictimWay = 0;
  uint64_t nMinTimeStamp = this->spTLB[nSet][0]->nTimeStamp;
  for (int j = 1; j < MMU_NUM_WAY; j++) {

#ifdef DEBUG_MMU
    assert(this->spTLB[nSet][j]->nValid == 1);
#endif

    if (nMinTimeStamp > this->spTLB[nSet][j]->nTimeStamp) {
      nMinTimeStamp = this->spTLB[nSet][j]->nTimeStamp;
      nVictimWay = j;
    };
  };

#ifdef DEBUG_MMU
  assert(nMinTimeStamp > 0);
#endif

#endif

  // Stat
  this->nTLB_evict++;

#ifdef DEBUG_MMU
  assert(nVictimWay < MMU_NUM_WAY);
  assert(nVictimWay >= 0);
  if (MMU_NUM_WAY == 1) {
    assert(nVictimWay == 0); // Direct mapped cache
  };
#endif

  return (nVictimWay);
};

//-----------------------------------------------------
// Find way number replacement FLPD cache
//-----------------------------------------------------
int CMMU::GetVictim_FLPD() {

  // Debug
  // this->CheckMMU();

  // Check empty slot
  for (int j = 0; j < NUM_FLPD_ENTRY; j++) {
    if (this->spFLPD[j]->nValid != 1) {
      return (j);
    };
  };

#if defined MMU_REPLACEMENT_RANDOM
  // Choose victim way randomly (0,1,...,NUM_FLPD_ENTRY -1)
  int nVictimWay = rand() % (NUM_FLPD_ENTRY);

#else
  // Choose victim way with minimum (least recently used) timestamp
  int nVictimWay = 0;
  uint64_t nMinTimeStamp = this->spFLPD[0]->nTimeStamp;
  for (int j = 1; j < NUM_FLPD_ENTRY; j++) {

#ifdef DEBUG_MMU
    assert(this->spFLPD[j]->nValid == 1);
#endif

    if (nMinTimeStamp > this->spFLPD[j]->nTimeStamp) {
      nMinTimeStamp = this->spFLPD[j]->nTimeStamp;
      nVictimWay = j;
    };
  };

#ifdef DEBUG_MMU
  if (MMU_NUM_WAY == 1) { // Direct mapped cache
    assert(nVictimWay == 0);
  };
  assert(nMinTimeStamp > 0);
#endif

  // Stat
  this->nFLPD_evict++;

#endif

#ifdef DEBUG_MMU
  assert(nVictimWay < NUM_FLPD_ENTRY);
  assert(nVictimWay >= 0);
#endif

  return (nVictimWay);
};

// Get AR MO count
int CMMU::GetMO_AR() {

#ifdef DEBUG_MMU
  assert(this->nMO_AR >= 0);
#endif
  return (this->nMO_AR);
};

// Get AW MO count
int CMMU::GetMO_AW() {

#ifdef DEBUG_MMU
  assert(this->nMO_AW >= 0);
#endif
  return (this->nMO_AW);
};

// Get Ax MO count
int CMMU::GetMO_Ax() {

#ifdef DEBUG_MMU
  assert(this->nMO_Ax >= 0);
#endif
  return (this->nMO_Ax);
};

// Increase AR MO count
EResultType CMMU::Increase_MO_AR() {

  this->nMO_AR++;
  return (ERESULT_TYPE_SUCCESS);
};

// Decrease AR MO count
EResultType CMMU::Decrease_MO_AR() {

  this->nMO_AR--;
#ifdef DEBUG_MMU
  assert(this->nMO_AR >= 0);
#endif
  return (ERESULT_TYPE_SUCCESS);
};

// Increase AW MO count
EResultType CMMU::Increase_MO_AW() {

  this->nMO_AW++;
  return (ERESULT_TYPE_SUCCESS);
};

// Decrease AW MO count
EResultType CMMU::Decrease_MO_AW() {

  this->nMO_AW--;
#ifdef DEBUG_MMU
  assert(this->nMO_AW >= 0);
#endif
  return (ERESULT_TYPE_SUCCESS);
};

// Increase Ax MO count
EResultType CMMU::Increase_MO_Ax() {

  this->nMO_Ax++;
  return (ERESULT_TYPE_SUCCESS);
};

// Decrease Ax MO count
EResultType CMMU::Decrease_MO_Ax() {

  this->nMO_Ax--;
#ifdef DEBUG_MMU
  assert(this->nMO_Ax >= 0);
#endif
  return (ERESULT_TYPE_SUCCESS);
};

// Set start VA AR
EResultType CMMU::Set_nAR_START_ADDR(int64_t nVA) {

  this->nAR_START_ADDR = nVA;
  return (ERESULT_TYPE_YES);
};

// Set start VA AW
EResultType CMMU::Set_nAW_START_ADDR(int64_t nVA) {

  this->nAW_START_ADDR = nVA;
  return (ERESULT_TYPE_YES);
};

// Set start VPN
EResultType CMMU::Set_START_VPN(int64_t nVPN) {

  this->START_VPN = nVPN;
  return (ERESULT_TYPE_YES);
};

// Set page-table
EResultType CMMU::Set_PageTable(SPPTE *spPageTable) {

  this->spPageTable = spPageTable;
  return (ERESULT_TYPE_YES);
};

// Get TLB reach
float CMMU::GetTLB_reach(int64_t nCycle) {

  float TLB_reach = (float)(this->nTotalPages_TLB_capacity) / nCycle;

  return (TLB_reach);
};

// Get TLB hit rate
float CMMU::GetTLBHitRate() {

  float TLB_hit_rate = (float)(this->nHit_AR_TLB + this->nHit_AW_TLB) / (this->nAR_SI + this->nAW_SI);
  return (TLB_hit_rate);
};

// Get nAR_SI
int CMMU::Get_nAR_SI() { return (this->nAR_SI); };

// Get nAW_SI
int CMMU::Get_nAW_SI() { return (this->nAW_SI); };

// Get nHit_AR_TLB
int CMMU::Get_nHit_AR_TLB() { return (this->nHit_AR_TLB); };

// Get nHit_AW_TLB
int CMMU::Get_nHit_AW_TLB() { return (this->nHit_AW_TLB); };

// Get Ax pkt name
string CMMU::GetName() {

  // Debug
  // this->CheckMMU();
  return (this->cName);
};

// Debug
EResultType CMMU::CheckMMU() {

  // Check Ax priority
  if (this->Is_AR_priority == ERESULT_TYPE_YES) {
    assert(this->Is_AW_priority == ERESULT_TYPE_NO);
  } else {
    assert(this->Is_AR_priority == ERESULT_TYPE_NO);
    assert(this->Is_AW_priority == ERESULT_TYPE_YES);
  };

  return (ERESULT_TYPE_SUCCESS);
};

// Debug
EResultType CMMU::CheckLink() {

  assert(this->cpTx_AR != NULL);
  assert(this->cpTx_AW != NULL);
  assert(this->cpTx_W != NULL);
  assert(this->cpRx_R != NULL);
  assert(this->cpRx_B != NULL);
  assert(this->cpTx_AR->GetPair() != NULL);
  assert(this->cpTx_AW->GetPair() != NULL);
  assert(this->cpTx_W->GetPair() != NULL);
  assert(this->cpRx_R->GetPair() != NULL);
  assert(this->cpRx_B->GetPair() != NULL);
  assert(this->cpTx_AR->GetTRxType() != this->cpTx_AR->GetPair()->GetTRxType());
  assert(this->cpTx_AW->GetTRxType() != this->cpTx_AW->GetPair()->GetTRxType());
  assert(this->cpTx_W->GetTRxType() != this->cpTx_W->GetPair()->GetTRxType());
  assert(this->cpRx_R->GetTRxType() != this->cpRx_R->GetPair()->GetTRxType());
  assert(this->cpRx_B->GetTRxType() != this->cpRx_B->GetPair()->GetTRxType());
  assert(this->cpTx_AR->GetPktType() == this->cpTx_AR->GetPair()->GetPktType());
  assert(this->cpTx_AW->GetPktType() == this->cpTx_AW->GetPair()->GetPktType());
  assert(this->cpTx_W->GetPktType() == this->cpTx_W->GetPair()->GetPktType());
  assert(this->cpRx_R->GetPktType() == this->cpRx_R->GetPair()->GetPktType());
  assert(this->cpRx_B->GetPktType() == this->cpRx_B->GetPair()->GetPktType());
  assert(this->cpTx_AR->GetPair()->GetPair() == this->cpTx_AR);
  assert(this->cpTx_AW->GetPair()->GetPair() == this->cpTx_AW);
  assert(this->cpTx_W->GetPair()->GetPair() == this->cpTx_W);
  assert(this->cpRx_R->GetPair()->GetPair() == this->cpRx_R);
  assert(this->cpRx_B->GetPair()->GetPair() == this->cpRx_B);

  return (ERESULT_TYPE_SUCCESS);
};

// EResultType CMMU::Display() {
//
//	// Debug
//	// this->CheckMMU();
//	return (ERESULT_TYPE_SUCCESS);
// };

EResultType CMMU::Display_TLB() {

  for (int i = 0; i < MMU_NUM_SET; i++) {
    for (int j = 0; j < MMU_NUM_WAY; j++) {
      printf("-------------------------------------\n");
      printf("\t %s.TLB[%d][%d]\n", this->cName.c_str(), i, j);
      printf("-------------------------------------\n");

      printf("\t TLB[%d][%d]->Valid \t : %d\n", i, j, this->spTLB[i][j]->nValid);
      printf("\t TLB[%d][%d]->VPN   \t : 0x%3lx\n", i, j, this->spTLB[i][j]->nVPN);

      // printf("\t TLB[%d][%d]->PPN   \t : 0x%x\n",   i, j,
      // this->spTLB[i][j]->nPPN);
      printf("\t TLB[%d][%d]->Contiguity\t : %d\n", i, j, this->spTLB[i][j]->nContiguity);
      printf("\t TLB[%d][%d]->Cycle \t : %ld\n", i, j, this->spTLB[i][j]->nTimeStamp);
    };
  };
  return (ERESULT_TYPE_SUCCESS);
};

// Update state
EResultType CMMU::UpdateState(int64_t nCycle) {

  // Ax priority SI
  if (this->cpRx_AR->IsBusy() == ERESULT_TYPE_YES) { // AR accepted this cycle. Next AW higher priority
    // this->Is_AR_priority = ERESULT_TYPE_NO;		// Duong comment to
    // ensure write after read this->Is_AW_priority = ERESULT_TYPE_YES;
  } else if (this->cpRx_AW->IsBusy() == ERESULT_TYPE_YES) {
    this->Is_AR_priority = ERESULT_TYPE_YES;
    this->Is_AW_priority = ERESULT_TYPE_NO;
  };

  // Port
  this->cpRx_AR->UpdateState();
  this->cpRx_AW->UpdateState();
  this->cpTx_R->UpdateState();
  // this->cpRx_W ->UpdateState();
  this->cpTx_B->UpdateState();

  this->cpTx_AR->UpdateState();
  this->cpTx_AW->UpdateState();
  this->cpRx_R->UpdateState();
  // this->cpTx_W ->UpdateState();
  this->cpRx_B->UpdateState();

  // FIFO
  this->cpFIFO_AR->UpdateState();
  // this->cpFIFO_R ->UpdateState();
  // this->cpFIFO_B ->UpdateState();
  // this->cpFIFO_W ->UpdateState();

  // Tracker
  this->cpTracker->UpdateState();

#ifdef STAT_DETAIL
  // Max MO tracker occupancy
  if (this->cpTracker->GetCurNum() > this->nMax_MOTracker_Occupancy) {
    // Debug
    // assert (this->cpTracker->GetCurNum() == 1+
    // this->nMax_MOTracker_Occupancy); // FIXME Yes even when multiple PTW
    // ongoing
    this->nMax_MOTracker_Occupancy++;
  };

  // Avg MO tracker occupancy
  this->nTotal_MOTracker_Occupancy += this->cpTracker->GetCurNum();

  // Total MO
  this->nTotal_MO_AR += this->nMO_AR;
  this->nTotal_MO_AW += this->nMO_AW;
  this->nTotal_MO_Ax += this->nMO_Ax;

  // Max allocation cycle tracker all entries
  if (this->cpTracker->GetMaxCycleWait() > this->nMax_Tracker_Wait_Ax) {
    this->nMax_Tracker_Wait_Ax = this->cpTracker->GetMaxCycleWait();
  };
#endif

#ifdef DEBUG_MMU
  if (this->nMax_Tracker_Wait_Ax >= STARVATION_CYCLE) { // Starvation. If wait too long, something wrong
    this->cpTracker->Display();
    printf("[Cycle %3ld: %s.UpdateState]  Starvation occurs (%d cycles) \n", nCycle, this->cName.c_str(),
           this->nMax_Tracker_Wait_Ax);
    assert(0);
  };
#endif

#ifdef STAT_DETAIL
  // PTW ongoing cycles
  if (this->IsPTW_AR_ongoing == ERESULT_TYPE_YES) {
    this->nPTW_AR_ongoing_cycles++;
  };
  if (this->IsPTW_AW_ongoing == ERESULT_TYPE_YES) {
    this->nPTW_AW_ongoing_cycles++;
  };

  // Stall cycles SI
  if (this->cpRx_AR->IsIdle() == ERESULT_TYPE_YES and this->cpRx_AR->GetPair()->IsBusy() == ERESULT_TYPE_YES) {
    this->nAR_stall_cycles_SI++;
  };
  if (this->cpRx_AW->IsIdle() == ERESULT_TYPE_YES and this->cpRx_AW->GetPair()->IsBusy() == ERESULT_TYPE_YES) {
    this->nAW_stall_cycles_SI++;
  };

  // TLB capacity
  for (int i = 0; i < MMU_NUM_SET; i++) {
    for (int j = 0; j < MMU_NUM_WAY; j++) {
      // if (this->spTLB[i][j]->nValid == 1) {
      this->nTotalPages_TLB_capacity += this->spTLB[i][j]->nContiguity;
      // };
    };
  };
  this->nTotalPages_TLB_capacity += (MMU_NUM_SET * MMU_NUM_WAY);
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// Stat
EResultType CMMU::PrintStat(int64_t nCycle, FILE *fp) {

  // Debug
  // this->CheckMMU();

  // printf("--------------------------------------------------------\n");
  // printf("\t MMU display\n");
  printf("--------------------------------------------------------\n");
  printf("\t Name : %s\n", this->cName.c_str());
  printf("--------------------------------------------------------\n");
#ifdef MMU_OFF
  printf("\t MMU                                : OFF \n");
#endif
#ifdef MMU_ON_
  printf("\t MMU                                : ON \n");
#endif
#ifdef RMM_ENABLE
  printf("\t RMM (Redundant Memory Mapping)     : ON \n");
#endif
  printf("\t Number of TLB sets                 : %d\n", MMU_NUM_SET);
  printf("\t Number of TLB ways                 : %d\n", MMU_NUM_WAY);
  printf("\t Number of FLPD entries             : %d\n", NUM_FLPD_ENTRY);
  printf("\t Max allowed Ax MO                  : %d\n", MAX_MO_COUNT);

#if defined MMU_REPLACEMENT_RANDOM
  printf("\t Replacement policy                 : Random \n");
#elif defined MMU_REPLACEMENT_FIFO
  printf("\t Replacement policy                 : FIFO \n");
#elif defined MMU_REPLACEMENT_LRU
  printf("\t Replacement policy                 : LRU \n");
#endif

#if defined SINGLE_FETCH
  printf("\t PTW policy                         : Single fetch \n");
#elif defined BLOCK_FETCH
  printf("\t PTW policy                         : Block fetch \n");
#elif defined HALF_FETCH
  printf("\t PTW policy                         : Half block fetch \n");
#elif defined QUARTER_FETCH
  printf("\t PTW policy                         : Quarter block fetch \n");
#endif

#ifdef CONTIGUITY_DISABLE
  printf("\t Contiguity                         : Disable \n");
#endif

#ifdef CONTIGUITY_ENABLE
#ifdef CONTIGUITY_0_PERCENT
  printf("\t Contiguity                         : 0 percent \n");
#endif
#ifdef CONTIGUITY_25_PERCENT
  printf("\t Contiguity                         : 25 percent \n");
#endif
#ifdef CONTIGUITY_50_PERCENT
  printf("\t Contiguity                         : 50 percent \n");
#endif
#ifdef CONTIGUITY_75_PERCENT
  printf("\t Contiguity                         : 75 percent \n");
#endif
#endif

#ifdef VPN_SAME_PPN
  printf("\t PPN                                : Same as VPN\n");
#else
  printf("\t PPN                                : Random generation\n\n");
#endif

  printf("\t Number of AR SI                    : %d\n", this->nAR_SI);
  printf("\t Number of AR MI                    : %d\n", this->nAR_MI);
  printf("\t Number of AW SI                    : %d\n", this->nAW_SI);
  printf("\t Number of AW MI                    : %d\n\n", this->nAW_MI);

  // printf("\t------------------------------------\n");
  if (this->nAR_SI > 0) {
    printf("\t Number of TLB hit AR               : %d\n", this->nHit_AR_TLB);
    printf("\t (%s) TLB hit rate AR             : %1.5f\n", this->cName.c_str(),
           (float)(this->nHit_AR_TLB) / (this->nAR_SI));

    printf("\t Number of FLPD hit AR              : %d\n", this->nHit_AR_FLPD);
    printf("\t FLPD hit rate AR                   : %1.5f\n",
           (float)(this->nHit_AR_FLPD) / (this->nAR_SI - this->nHit_AR_TLB));
  };
  if (this->nAW_SI > 0) {
    printf("\t Number of TLB hit AW               : %d\n", this->nHit_AW_TLB);
    printf("\t (%s) TLB hit rate AW             : %1.5f\n", this->cName.c_str(),
           (float)(this->nHit_AW_TLB) / (this->nAW_SI));

    printf("\t Number of FLPD hit AW              : %d\n\n", this->nHit_AW_FLPD);
    printf("\t FLPD hit rate AW                   : %1.5f\n",
           (float)(this->nHit_AW_FLPD) / (this->nAW_SI - this->nHit_AW_TLB));
  };

  printf("\t (%s) TLB hit rate Avg            : %1.5f\n", this->cName.c_str(),
         (float)(this->nHit_AR_TLB + this->nHit_AW_TLB) / (this->nAR_SI + this->nAW_SI));
  printf("\t (%s) FLPD hit rate Avg           : %1.5f\n", this->cName.c_str(),
         (float)(this->nHit_AR_FLPD + this->nHit_AW_FLPD) / (this->nAR_SI + this->nAW_SI));

  printf("\t (%s) Number of PTWs total         : %d\n", this->cName.c_str(), this->nPTW_total);
  printf("\t Number of PTWs 1st                : %d\n", this->nPTW_1st);
  printf("\t Number of PTWs 2nd                : %d\n", this->nPTW_2nd);
  printf("\t Number of PTWs 3rd                : %d\n", this->nPTW_3rd);
  printf("\t Number of PTWs 4th                : %d\n", this->nPTW_4th);

#ifdef STAT_DETAIL
  printf("\t Total cycles PTW for AR            : %d\n", this->nPTW_AR_ongoing_cycles);
  printf("\t Total cycles PTW for AW            : %d\n\n", this->nPTW_AW_ongoing_cycles);
#endif

  printf("\t Number of TLB evicts               : %d\n", this->nTLB_evict);
  printf("\t Number of FLPD evicts              : %d\n\n", this->nFLPD_evict);

#ifdef DBPF_ENABLE
  printf("\t------------------------------------\n");
  printf("\t Number of TLB1 hit AR              : %d\n", this->nHit_AR_TLB1_DBPF);
  printf("\t Number of TLB1 hit AW              : %d\n", this->nHit_AW_TLB1_DBPF);
  printf("\t (%s) Number of PF total          : %d\n", this->cName.c_str(), this->nPF_total);
  printf("\t Number of PF 1st                   : %d\n", this->nPF_1st);
  printf("\t Number of PF 2nd                   : %d\n\n", this->nPF_2nd);
  printf("\t------------------------------------\n");
#endif

#ifdef RMM_ENABLE
  printf("\t------------------------------------\n");
  printf("\t Number of RTLB hit AR              : %d\n", this->nHit_AR_RTLB_RMM);
  printf("\t Number of RTLB hit AW              : %d\n", this->nHit_AW_RTLB_RMM);
  printf("\t (%s) RTLB hit rate AR              : %1.5f\n" this->cName.c_str(),
         (float)(this->nHit_AR_RTLB_RMM) / (this->nAR_SI));
  printf("\t (%s) RTLB hit rate AW              : %1.5f\n" this->cName.c_str(),
         (float)(this->nHit_AW_RTLB_RMM) / (this->nAW_SI));

  printf("\t (%s) Number of RPTWs total         : %d\n", this->cName.c_str(), this->nRPTW_total_RMM);
  printf("\t Number of RPTWs 1st                : %d\n", this->nRPTW_1st_RMM);
  // printf("\t Number of RPTWs 2nd             : %d\n",
  // this->nRPTW_2nd_RMM); printf("\t Number of RPTWs 3rd             : %d\n",
  // this->nRPTW_3rd_RMM);

  printf("\t Number of RTLB evicts              : %d\n\n", this->nRTLB_evict_RMM);
  printf("\t------------------------------------\n");
#endif

#ifdef AT_ENABLE
  printf("\t------------------------------------\n");
  printf("\t (%s) Number of APTWs total         : %d\n", this->cName.c_str(), this->nAPTW_total_AT);
  printf("\t Number of APTWs 1st                : %d\n", this->nAPTW_1st_AT);
  printf("\t Number of APTWs 2nd                : %d\n\n", this->nAPTW_2nd_AT);
  printf("\t------------------------------------\n");
#endif

#ifdef STAT_DETAIL
// printf("\t Min AR VPN                      : 0x%lx\n",
// this->AR_min_VPN); printf("\t Max AR VPN                      : 0x%lx\n",
// this->AR_max_VPN); printf("\t Min AW VPN                      : 0x%lx\n",
// this->AW_min_VPN); printf("\t Max AW VPN                      : 0x%lx\n\n",
// this->AW_max_VPN);
#endif

#ifdef STAT_DETAIL
  printf("\t Stall cycles AR SI                 : %d\n", this->nAR_stall_cycles_SI);
  printf("\t Stall cycles AW SI                 : %d\n\n", this->nAW_stall_cycles_SI);

  // printf("\t Total TLB capacity              : %ld\n",
  // this->nTotalPages_TLB_capacity);
  printf("\t (%s) Avg TLB capacity            : %1.5f\n\n", this->cName.c_str(),
         (float)(this->nTotalPages_TLB_capacity) / nCycle);

  printf("\t Max MO tracker Occupancy           : %d\n", this->nMax_MOTracker_Occupancy);
  // printf("\t Total MO tracker Occupancy      : %d\n",
  // this->nTotal_MOTracker_Occupancy);
  printf("\t Avg MO tracker Occupancy           : %1.2f\n\n", (float)(this->nTotal_MOTracker_Occupancy) / nCycle);

  printf("\t Max MO tracker alloc cycles all Ax : %d\n", this->nMax_Tracker_Wait_Ax);
  printf("\t Avg MO tracker alloc cycles Ax     : %1.2f\n",
         (float)(this->nTotal_Tracker_Wait_Ax) / (this->nAR_SI + this->nAW_SI));
  printf("\t Avg MO tracker alloc cycles AR     : %1.2f\n", (float)(this->nTotal_Tracker_Wait_AR) / (this->nAR_SI));
  printf("\t Avg MO tracker alloc cycles AW     : %1.2f\n\n", (float)(this->nTotal_Tracker_Wait_AW) / (this->nAW_SI));

  printf("\t Max MO AR SI                       : %d\n", this->nMax_MO_AR);
  printf("\t Max MO AW SI                       : %d\n", this->nMax_MO_AW);
  printf("\t Max MO Ax SI                       : %d\n\n", this->nMax_MO_Ax);

  printf("\t Avg MO AR SI                       : %1.2f\n", (float)(this->nTotal_MO_AR) / nCycle);
  printf("\t Avg MO AW SI                       : %1.2f\n", (float)(this->nTotal_MO_AW) / nCycle);
  printf("\t Avg MO Ax SI                       : %1.2f\n", (float)(this->nTotal_MO_Ax) / nCycle);
#endif

  // printf("--------------------------------------------------------\n");

  // Debug
  assert(this->nMO_AR == 0);
  // assert (this->nMO_AW == 0);			// Cache evict may be in
  // on-going. This is not an issue. assert (this->nMO_Ax == 0); assert
  // (this->cpFIFO_R->GetCurNum() == 0);
  assert(this->cpFIFO_W->GetCurNum() == 0);
  // assert (this->cpFIFO_B->GetCurNum() == 0);

#ifndef BACKGROUND_TRAFFIC_ON
  assert(this->cpTracker->GetCurNum() == 0);
#endif

#ifdef BACKGROUND_TRAFFIC_ON
  assert(this->nAR_MI ==
         this->nAR_SI + this->nPTW_total + this->nPF_total + this->nAPTW_total_AT + this->nRPTW_total_RMM);
#else
  assert(this->nAR_MI == this->nAR_SI + this->nPTW_total + this->nPF_total + this->nAPTW_total_AT);
#endif

//--------------------------------------------------------------------------
// FILE out
//--------------------------------------------------------------------------
#ifdef FILE_OUT
  // fprintf(fp, "--------------------------------------------------------\n");
  // fprintf(fp, "\t MMU display\n");
  fprintf(fp, "--------------------------------------------------------\n");
  fprintf(fp, "\t Name: %s\n", this->cName.c_str());
  fprintf(fp, "--------------------------------------------------------\n");
#ifdef MMU_OFF
  fprintf(fp, "\t MMU OFF\n");
#endif
#ifdef MMU_ON_
  fprintf(fp, "\t MMU ON\n");
#endif
  fprintf(fp, "\t Number of TLB sets: %d\n", MMU_NUM_SET);
  fprintf(fp, "\t Number of TLB ways: %d\n", MMU_NUM_WAY);
  fprintf(fp, "\t Number of FLPD entries: %d\n", NUM_FLPD_ENTRY);
  fprintf(fp, "\t Max allowed Ax MO: %d\n", MAX_MO_COUNT);

#if defined SINGLE_FETCH
  fprintf(fp, "\t Page table walk policy: Single fetch\n");
#elif defined BLOCK_FETCH
  fprintf(fp, "\t Page table walk policy: Block fetch\n");
#elif defined HALF_FETCH
  fprintf(fp, "\t Page table walk policy: Half fetch\n");
#elif defined QUARTER_FETCH
  fprintf(fp, "\t Page table walk policy: Quarter fetch\n");
#endif

#if defined MMU_REPLACEMENT_RANDOM
  fprintf(fp, "\t Replacement policy: Random \n");
#elif defined MMU_REPLACEMENT_FIFO
  fprintf(fp, "\t Replacement policy: FIFO \n");
#elif defined MMU_REPLACEMENT_LRU
  fprintf(fp, "\t Replacement policy: LRU \n");
#endif

#ifdef CONTIGUITY_DISABLE
  fprintf(fp, "\t Contiguity: Disable (Reference)\n");
#endif

#ifdef CONTIGUITY_ENABLE
#ifdef CONTIGUITY_0_PERCENT
  fprintf(fp, "\t Contiguity: 0 percent \n");
#endif
#ifdef CONTIGUITY_25_PERCENT
  fprintf(fp, "\t Contiguity: 25 percent \n");
#endif
#ifdef CONTIGUITY_50_PERCENT
  fprintf(fp, "\t Contiguity: 50 percent \n");
#endif
#ifdef CONTIGUITY_75_PERCENT
  fprintf(fp, "\t Contiguity: 75 percent \n");
#endif
#endif

#ifdef VPN_SAME_PPN
  fprintf(fp, "\t PPN same as VPN\n");
#else
  fprintf(fp, "\t PPN random generation\n\n");
#endif

  fprintf(fp, "\t Number of AR SI:  %d\n", this->nAR_SI);
  fprintf(fp, "\t Number of AR MI: %d\n", this->nAR_MI);
  fprintf(fp, "\t Number of AW SI:  %d\n", this->nAW_SI);
  fprintf(fp, "\t Number of AW MI: %d\n\n", this->nAW_MI);

  fprintf(fp, "\t Number of TLB hit for AR: %d\n", this->nHit_AR_TLB);
  fprintf(fp, "\t Number of TLB hit for AW: %d\n", this->nHit_AW_TLB);
  fprintf(fp, "\t TLB hit rate for AR (%s): %1.5f\n", this->cName.c_str(), (float)(this->nHit_AR_TLB) / (this->nAR_SI));
  fprintf(fp, "\t TLB hit rate for AW (%s): %1.5f\n", this->cName.c_str(), (float)(this->nHit_AW_TLB) / (this->nAW_SI));
  fprintf(fp, "\t Number of FLPD hit for AR: %d\n", this->nHit_AR_FLPD);
  fprintf(fp, "\t Number of FLPD hit for AW: %d\n\n", this->nHit_AW_FLPD);

  fprintf(fp, "\t Number of PTWs total: %d\n", this->nPTW_total);
  fprintf(fp, "\t Number of PTWs 1st: %d\n", this->nPTW_1st);
  fprintf(fp, "\t Number of PTWs 2nd: %d\n", this->nPTW_2nd);
  fprintf(fp, "\t Number of PTWs 3rd: %d\n", this->nPTW_3rd);
  fprintf(fp, "\t Number of PTWs 4th: %d\n", this->nPTW_4th);
  fprintf(fp, "\t Total cycles PTW for AR: %d\n", this->nPTW_AR_ongoing_cycles);
  fprintf(fp, "\t Total cycles PTW for AW: %d\n\n", this->nPTW_AW_ongoing_cycles);

  fprintf(fp, "\t Number of TLB evicts: %d\n", this->nTLB_evict);
  fprintf(fp, "\t Number of FLPD evicts:%d\n\n", this->nFLPD_evict);

#ifdef RMM_ENABLE
  fprintf(fp, "\t-------------------------------\n");
  fprintf(fp, "\t Number of RTLB hit AR : %d\n", this->nHit_AR_RTLB_RMM);
  fprintf(fp, "\t Number of RTLB hit AW : %d\n", this->nHit_AW_RTLB_RMM);
  fprintf(fp, "\t %s RTLB hit rate AR : %1.5f\n", this->cName.c_str(),
          (float)(this->nHit_AR_RTLB_RMM) / (this->nAR_SI));
  fprintf(fp, "\t %s RTLB hit rate AW : %1.5f\n", this->cName.c_str(),
          (float)(this->nHit_AW_RTLB_RMM) / (this->nAW_SI));

  fprintf(fp, "\t Number of RPTWs 1st : %d\n", this->nRPTW_1st_RMM);
  fprintf(fp, "\t Number of RPTWs 2nd : %d\n", this->nRPTW_2nd_RMM);
  fprintf(fp, "\t Number of RPTWs 3rd : %d\n", this->nRPTW_3rd_RMM);

  fprintf(fp, "\t Number of RTLB evicts : %d\n\n", this->nRTLB_evict_RMM);
  fprintf(fp, "\t-------------------------------\n");
#endif

#ifdef AT_ENABLE
  fprintf(fp, "\t------------------------------------\n");
  fprintf(fp, "\t (%s) Number of APTWs total : %d\n", this->cName.c_str(), this->nAPTW_total_AT);
  fprintf(fp, "\t Number of APTWs 1st : %d\n", this->nAPTW_1st_AT);
  fprintf(fp, "\t Number of APTWs 2nd : %d\n", this->nAPTW_2nd_AT);
  fprintf(fp, "\t Number of APTWs 3rd : %d\n", this->nAPTW_3rd_AT);
  fprintf(fp, "\t Number of APTWs 4th : %d\n", this->nAPTW_4th_AT);

  fprintf(fp, "\t------------------------------------\n");
#endif

  fprintf(fp, "\t Stall cycles AR SI: %d\n", this->nAR_stall_cycles_SI);
  fprintf(fp, "\t Stall cycles AW SI: %d\n\n", this->nAW_stall_cycles_SI);

  fprintf(fp, "\t Max MO AR SI: %d\n", this->nMax_MO_AR);
  fprintf(fp, "\t Max MO AW SI: %d\n", this->nMax_MO_AW);
  fprintf(fp, "\t Max MO Ax SI: %d\n\n", this->nMax_MO_Ax);

  fprintf(fp, "\t Total TLB capacity: %ld\n", this->nTotalPages_TLB_capacity);
  fprintf(fp, "\t Avg TLB capacity (%s): %1.2f\n\n", this->cName.c_str(),
          (float)(this->nTotalPages_TLB_capacity) / nCycle);

  fprintf(fp, "\t Max MO tracker Occupancy: %d\n", this->nMax_MOTracker_Occupancy);
  // fprintf(fp, "\t Total MO tracker Occupancy: %d\n",
  // this->nTotal_MOTracker_Occupancy);
  fprintf(fp, "\t Avg MO tracker Occupancy: %1.2f\n\n", (float)(this->nTotal_MOTracker_Occupancy) / nCycle);

  fprintf(fp, "\t Max MO tracker alloc cycles all Ax: %d\n", this->nMax_Tracker_Wait_Ax);
  fprintf(fp, "\t Avg MO tracker alloc cycles Ax: %1.2f\n",
          (float)(this->nTotal_Tracker_Wait_Ax) / (this->nAR_SI + this->nAW_SI));
  fprintf(fp, "\t Avg MO tracker alloc cycles AR: %1.2f\n", (float)(this->nTotal_Tracker_Wait_AR) / (this->nAR_SI));
  fprintf(fp, "\t Avg MO tracker alloc cycles AW: %1.2f\n\n", (float)(this->nTotal_Tracker_Wait_AW) / (this->nAW_SI));

  fprintf(fp, "\t Avg MO AR Slave interface: %1.2f\n", (float)(this->nTotal_MO_AR) / nCycle);
  fprintf(fp, "\t Avg MO AW Slave interface: %1.2f\n", (float)(this->nTotal_MO_AW) / nCycle);
  fprintf(fp, "\t Avg MO Ax Slave interface: %1.2f\n", (float)(this->nTotal_MO_Ax) / nCycle);

// fprintf(fp, "--------------------------------------------------------\n");
#endif

  return (ERESULT_TYPE_SUCCESS);
};
