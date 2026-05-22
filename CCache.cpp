//------------------------------------------------------------
// FileName     : CCache.cpp
// Version      : 0.73
// DATE         : 24 Nov 2021
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Cacheiter class definition
//-----------------------------------------------------------
// Spec
//	1. Cache address mapping	: Tag, Set, Offset
//	2. Miss policy	 		: Read allocate, Write allocate
//	3. Line size			: 64 B
//	4. AR, AW arbitration		: Round-robin. AR and AW shared.
//	5. Multiple outstanding count	: 16
//-----------------------------------------------------------
// Flow control
//	1. Requests are served in order.
//	2. If there is any on-going line-fill or evict with "address (same cache
// line) dependency", stall Ax.
//-----------------------------------------------------------
// ID outstanding policy
//	1. Hit-under-hit		: Allow
//	2. Hit-under-miss		: Not allow. If same ID was miss, stall
// Ax.
//	3. Miss-under-hit		: Allow
//	4. Miss-under-miss		: Allow
//-----------------------------------------------------------
// Known issues
//	1. W data is not referred. Data value is not relevant. Work-arond is not
// to use "W_Channel".
//-----------------------------------------------------------
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "CCache.h"

// Construct
CCache::CCache(string cName) {

  // Generate and initialize
  this->cName = cName;

  // Generate ports
  // Slave interface
  this->cpRx_AR = new CTRx("CACHE_Rx_AR", ETRX_TYPE_RX, EPKT_TYPE_AR);
  this->cpRx_AW = new CTRx("CACHE_Rx_AW", ETRX_TYPE_RX, EPKT_TYPE_AW);
  this->cpTx_R = new CTRx("CACHE_Tx_R", ETRX_TYPE_TX, EPKT_TYPE_R);
  this->cpRx_W = new CTRx("CACHE_Rx_W", ETRX_TYPE_RX, EPKT_TYPE_W);
  this->cpTx_B = new CTRx("CACHE_Tx_B", ETRX_TYPE_TX, EPKT_TYPE_B);

  // Master interface
  this->cpTx_AR = new CTRx("CACHE_Tx_AR", ETRX_TYPE_TX, EPKT_TYPE_AR);
  this->cpTx_AW = new CTRx("CACHE_Tx_AW", ETRX_TYPE_TX, EPKT_TYPE_AW);
  this->cpRx_R = new CTRx("CACHE_Rx_R", ETRX_TYPE_RX, EPKT_TYPE_R);
  this->cpTx_W = new CTRx("CACHE_Tx_W", ETRX_TYPE_TX, EPKT_TYPE_W);
  this->cpRx_B = new CTRx("CACHE_Rx_B", ETRX_TYPE_RX, EPKT_TYPE_B);

  // FIFO
  this->cpFIFO_AR = new CFIFO("CACHE_FIFO_AR", EUD_TYPE_AR, 40);
  this->cpFIFO_AW = new CFIFO("CACHE_FIFO_AW", EUD_TYPE_AR, 40);
  this->cpFIFO_R = new CFIFO("CACHE_FIFO_R", EUD_TYPE_R, 80); // DUONG change from 40 to 80
  this->cpFIFO_B = new CFIFO("CACHE_FIFO_B", EUD_TYPE_B, 40);
  this->cpFIFO_W = new CFIFO("CACHE_FIFO_W", EUD_TYPE_B, 40);

  // Generate cache memory
  for (int i = 0; i < CACHE_NUM_SET; i++) {
    for (int j = 0; j < CACHE_NUM_WAY; j++) {
      this->spCache[i][j] = new SCacheLine;
      this->spCache[i][j]->nValid = -1;
      this->spCache[i][j]->nTag = -1;
      this->spCache[i][j]->nData = -1;
      this->spCache[i][j]->nDirty = -1;
      this->spCache[i][j]->nTimeStamp = -1;
    };
  };

  // Tracker
  this->cpTracker = new CTracker("Cache_Tracker", 80); // DUONG change from 40 to 80

  // Control
  this->nMO_AR = -1;
  this->nMO_AW = -1;

  this->Is_AR_priority = ERESULT_TYPE_NO;
  this->Is_AW_priority = ERESULT_TYPE_NO;

  // Stat
  this->nMax_MOTracker_Occupancy = -1;
  this->nTotal_MOTracker_Occupancy = -1;
  this->nAR_SI = -1;
  this->nAR_MI = -1;
  this->nAW_SI = -1;
  this->nAW_MI = -1;
  this->nHit_AR = -1;
  this->nHit_AW = -1;
  this->nLineFill = -1;
  this->nEvict = -1;
  this->spPageTable = NULL;
  ;
};

// Destruct
CCache::~CCache() {

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

  // FIFO
  delete (this->cpFIFO_AR);
  delete (this->cpFIFO_AW);
  delete (this->cpFIFO_R);
  delete (this->cpFIFO_B);
  delete (this->cpFIFO_W);

  // Cache memory
  for (int i = 0; i < CACHE_NUM_SET; i++) {
    for (int j = 0; j < CACHE_NUM_WAY; j++) {
      delete (this->spCache[i][j]);
    };
  };

  // Tracker
  delete (this->cpTracker);

  // Port
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
  this->cpFIFO_AR = NULL;
  this->cpFIFO_AW = NULL;
  this->cpFIFO_R = NULL;
  this->cpFIFO_B = NULL;
  this->cpFIFO_W = NULL;

  // Cache memory
  for (int i = 0; i < CACHE_NUM_SET; i++) {
    for (int j = 0; j < CACHE_NUM_WAY; j++) {
      this->spCache[i][j] = NULL;
    };
  };

  // Tracker
  this->cpTracker = NULL;
};

// Reset
EResultType CCache::Reset() {

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
  this->cpRx_B->Reset();

  // FIFO
  this->cpFIFO_AR->Reset();
  this->cpFIFO_AW->Reset();
  this->cpFIFO_R->Reset();
  this->cpFIFO_B->Reset();
  this->cpFIFO_W->Reset();

  // Cache memory
  for (int i = 0; i < CACHE_NUM_SET; i++) {
    for (int j = 0; j < CACHE_NUM_WAY; j++) {
      this->spCache[i][j]->nValid = 0;
      this->spCache[i][j]->nTag = 0;
      this->spCache[i][j]->nData = 0;
      this->spCache[i][j]->nDirty = 0;
      this->spCache[i][j]->nTimeStamp = 0;
    };
  };

  // Tracker
  this->cpTracker->Reset();

  // Control
  this->nMO_AR = 0;
  this->nMO_AW = 0;

  this->Is_AR_priority = ERESULT_TYPE_YES; // Check AR first SI TLB check
  this->Is_AW_priority = ERESULT_TYPE_NO;

  // Stat
  this->nMax_MOTracker_Occupancy = 0;
  this->nTotal_MOTracker_Occupancy = 0;
  this->nAR_SI = 0;
  this->nAR_MI = 0;
  this->nAW_SI = 0;
  this->nAW_MI = 0;
  this->nHit_AR = 0;
  this->nHit_AW = 0;
  this->nLineFill = 0;
  this->nEvict = 0;

  return (ERESULT_TYPE_SUCCESS);
};

// Update state
EResultType CCache::UpdateState(int64_t nCycle) {

  // Control
  // Ax priority SI
  if (this->cpRx_AR->IsBusy() == ERESULT_TYPE_YES) { // AR accepted this cycle. Next AW higher priority
    this->Is_AR_priority = ERESULT_TYPE_NO;
    this->Is_AW_priority = ERESULT_TYPE_YES;
  } else if (this->cpRx_AW->IsBusy() == ERESULT_TYPE_YES) {
    this->Is_AR_priority = ERESULT_TYPE_YES;
    this->Is_AW_priority = ERESULT_TYPE_NO;
  };

  // Port
  this->cpRx_AR->UpdateState();
  this->cpRx_AW->UpdateState();
  this->cpTx_R->UpdateState();
  this->cpRx_W->UpdateState();
  this->cpTx_B->UpdateState();

  this->cpTx_AR->UpdateState();
  this->cpTx_AW->UpdateState();
  this->cpRx_R->UpdateState();
  this->cpTx_W->UpdateState();
  this->cpRx_B->UpdateState();

  // FIFO
  this->cpFIFO_AR->UpdateState();
  this->cpFIFO_AW->UpdateState();
  this->cpFIFO_R->UpdateState();
  this->cpFIFO_B->UpdateState();
  // this->cpFIFO_W ->UpdateState();

  // Max MO tracker occupancy
  if (this->cpTracker->GetCurNum() > this->nMax_MOTracker_Occupancy) {
    // Debug
    this->nMax_MOTracker_Occupancy++;
  };

  // Avg MO tracker occupancy
  this->nTotal_MOTracker_Occupancy += this->cpTracker->GetCurNum();

  return (ERESULT_TYPE_SUCCESS);
};

// AR valid
EResultType CCache::Do_AR_fwd_Cache_OFF(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid
  CPAxPkt cpAR = this->cpRx_AR->GetPair()->GetAx();
  if (cpAR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Debug
  assert(this->cpRx_AR->IsBusy() == ERESULT_TYPE_NO);
  assert(this->cpRx_AR->GetPair()->IsBusy() == ERESULT_TYPE_YES);
  // cpAR->CheckPkt();

  // Put Rx
  this->cpRx_AR->PutAx(cpAR);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_AR_fwd_Cache_OFF] %s put Rx_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
#endif
  assert(cpAR != NULL);

  // Put Tx
  this->cpTx_AR->PutAx(cpAR);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_AR_fwd_Cache_OFF] %s put Tx_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// AW valid
EResultType CCache::Do_AW_fwd_Cache_OFF(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid
  CPAxPkt cpAW = this->cpRx_AW->GetPair()->GetAx();
  if (cpAW == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Debug
  assert(this->cpRx_AW->IsBusy() == ERESULT_TYPE_NO);
  assert(this->cpRx_AW->GetPair()->IsBusy() == ERESULT_TYPE_YES);
  // cpAW->CheckPkt();

  // Put Rx
  this->cpRx_AW->PutAx(cpAW);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_AW_fwd_Cache_OFF] %s put Rx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
#endif
  assert(cpAW != NULL);

  // Put Tx
  this->cpTx_AW->PutAx(cpAW);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_AW_fwd_Cache_OFF] %s put Tx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// W valid
EResultType CCache::Do_W_fwd_Cache_OFF(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid
  CPAxPkt cpW = this->cpRx_W->GetPair()->GetAx();
  if (cpW == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Debug
  assert(this->cpRx_W->IsBusy() == ERESULT_TYPE_NO);
  assert(this->cpRx_W->GetPair()->IsBusy() == ERESULT_TYPE_YES);
  // cpW->CheckPkt();

  // Put Rx
  this->cpRx_W->PutAx(cpW);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_W_fwd_Cache_OFF] %s put Rx_W.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
#endif
  assert(cpW != NULL);

  // Put Tx
  this->cpTx_W->PutAx(cpW);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_W_fwd_Cache_OFF] %s put Tx_W.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// AR ready
EResultType CCache::Do_AR_bwd_Cache_OFF(int64_t nCycle) {

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
EResultType CCache::Do_AW_bwd_Cache_OFF(int64_t nCycle) {

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
EResultType CCache::Do_W_bwd_Cache_OFF(int64_t nCycle) {

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
EResultType CCache::Do_R_fwd_Cache_OFF(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_R->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid
  CPRPkt cpR = this->cpRx_R->GetPair()->GetR();
  if (cpR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Debug
  assert(cpR != NULL);
  // cpR->CheckPkt();

  // Put Rx
  this->cpRx_R->PutR(cpR);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_R_fwd_Cache_OFF] %s put Rx_R.\n", nCycle, this->cName.c_str(), cpR->GetName().c_str());
#endif

  // Put Tx
  this->cpTx_R->PutR(cpR);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_R_fwd_Cache_OFF] %s put Tx_R.\n", nCycle, this->cName.c_str(), cpR->GetName().c_str());
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// B valid
EResultType CCache::Do_B_fwd_Cache_OFF(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_B->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid
  CPBPkt cpB = this->cpRx_B->GetPair()->GetB();
  if (cpB == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Debug
  assert(cpB != NULL);
  // cpB->CheckPkt();

  // Put Rx
  this->cpRx_B->PutB(cpB);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_B_fwd_Cache_OFF] %s put Rx_B.\n", nCycle, this->cName.c_str(), cpB->GetName().c_str());
#endif

  // Put Tx
  this->cpTx_B->PutB(cpB);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_B_fwd_Cache_OFF] %s put Tx_B.\n", nCycle, this->cName.c_str(), cpB->GetName().c_str());
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// R ready
EResultType CCache::Do_R_bwd_Cache_OFF(int64_t nCycle) {

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
EResultType CCache::Do_B_bwd_Cache_OFF(int64_t nCycle) {

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

// AR valid SI
EResultType CCache::Do_AR_fwd_SI(int64_t nCycle) { // Master -> Cache

  // Check AW priority and candidate
  if (this->Is_AW_priority == ERESULT_TYPE_YES and this->cpRx_AW->GetPair()->GetAx() != NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid. Get.
  CPAxPkt cpAR = this->cpRx_AR->GetPair()->GetAx();
  if (cpAR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check FIFO_AR
  if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Get AR info
  // int     nID   = cpAR->GetID();
  int64_t nAddr = cpAR->GetAddr();

  // Check MO
  if (this->GetMO_Ax() >= MAX_MO_COUNT) { // AR + AW
    return (ERESULT_TYPE_FAIL);
  };

  // Check cache-hit
  EResultType IsHit = this->IsHit(nCycle, nAddr);

  // Check ID dependency
  // if (this->cpTracker->IsIDMatch(nID, EUD_TYPE_AR) == ERESULT_TYPE_YES) {
  //	return (ERESULT_TYPE_SUCCESS);
  //};

  // Check address dependency
  if (this->cpTracker->IsAddrMatch_Cache(nAddr) == ERESULT_TYPE_YES) { // Same line-fill or evict on-going
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check line-fill on going
  // if (this->cpTracker->IsAnyLineFill() == ERESULT_TYPE_YES) {
  // // FIXME
  if (this->cpTracker->IsSameID_LineFill(cpAR->GetID(), EUD_TYPE_AR) == ERESULT_TYPE_YES and
      IsHit == ERESULT_TYPE_YES) { // FIXME
    return (ERESULT_TYPE_SUCCESS);
  };

  // Put Rx
  this->cpRx_AR->PutAx(cpAR);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s put Rx_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
// cpAR->Display();
#endif

  // Hit. Generate R. Push FIFO R
  if (IsHit == ERESULT_TYPE_YES) {

    // Set time
    this->Hit_SetTime(nCycle, nAddr); // LRU

    // Push tracker
    UPUD upAR_new = new UUD;
    upAR_new->cpAR = Copy_CAxPkt(cpAR);
    this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_HIT);
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s, ID %d is pushed into tracker. "
           "CacheHit.\n",
           nCycle, this->cName.c_str(), cpAR->GetName().c_str(), cpAR->GetID());
// this->cpTracker->CheckTracker();
#endif

    // Maintain
    Delete_UD(upAR_new, EUD_TYPE_AR);

    // Get AR info
    int nLen = cpAR->GetLen();
    int nID = cpAR->GetID();
    int nCacheCh = cpAR->GetCacheCh();
    EResultType eFinalTrans = cpAR->IsFinalTrans();

    // Push FIFO R
    for (int nIndex = 0; nIndex <= nLen; nIndex++) {

      // Generate R
      CPRPkt cpR_new;
      char cPktName[50];
      if (nIndex == nLen) {
        sprintf(cPktName, "RLAST_R%d_for_%s", nIndex, cpAR->GetName().c_str());
      } else {
        sprintf(cPktName, "R%d_for_%s", nIndex, cpAR->GetName().c_str());
      };
      cpR_new = new CRPkt(cPktName);
      cpR_new->SetID(nID);
      cpR_new->SetData(nIndex);
      cpR_new->SetCacheCh(nCacheCh);
      cpR_new->SetFinalTrans(eFinalTrans);
      if (nIndex == nLen) {
        cpR_new->SetLast(ERESULT_TYPE_YES);
      } else {
        cpR_new->SetLast(ERESULT_TYPE_NO);
      };

      UPUD upR_new = new UUD;
      upR_new->cpR = cpR_new;

      // Push R
      this->cpFIFO_R->Push(upR_new, CACHE_FIFO_R_LATENCY);

// Debug
#ifdef DEBUG_Cache
      printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s, ID %d is pushed into FIFO_R.\n", nCycle, this->cName.c_str(),
             upR_new->cpR->GetName().c_str(), nID);
// this->cpFIFO_R->Display();
// this->cpFIFO_R->CheckFIFO();
#endif

      // Maintain
      Delete_UD(upR_new, EUD_TYPE_R);
    };

    // Stat
    this->nHit_AR++;
  };

  // Miss. Generate AR line-fill. Push FIFO AR
  if (IsHit == ERESULT_TYPE_NO) {

    // Push tracker
    UPUD upAR_new = new UUD;
    upAR_new->cpAR = Copy_CAxPkt(cpAR);
    this->cpTracker->Push(upAR_new, EUD_TYPE_AR, ECACHE_STATE_TYPE_FILL);
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s, ID %d is pushed into tracker. "
           "CacheMiss\n",
           nCycle, this->cName.c_str(), cpAR->GetName().c_str(), cpAR->GetID());
// this->cpTracker->CheckTracker();
// this->cpTracker->Display();
#endif

    // Maintain
    Delete_UD(upAR_new, EUD_TYPE_AR);

    // Generate AR line-fill
    char cPktName[50];
    sprintf(cPktName, "AR_pkt_fill_for_%s", cpAR->GetName().c_str());
    CPAxPkt cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

    int64_t nAddr = cpAR->GetAddr();
    nAddr = (nAddr >> CACHE_BIT_LINE) << CACHE_BIT_LINE; // Align

    cpAR_new->SetPkt(ARID_LINEFILL, nAddr, 3);
    cpAR_new->SetSrcName(this->cName);
    cpAR_new->SetVA(nAddr);
    cpAR_new->SetTransType(ETRANS_TYPE_LINE_FILL);

#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s is generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

    // Push FIFO_AR
    UPUD upAR_new2 = new UUD;
    upAR_new2->cpAR = cpAR_new;

    assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);

    this->cpFIFO_AR->Push(upAR_new2,
                          CACHE_FIFO_AR_LATENCY); // Line-fill. Original

    // Debug
    assert(upAR_new2 != NULL);
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_AR_fwd_SI] %s push FIFO_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
// cpAR->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upAR_new2, EUD_TYPE_AR);

    // Stat
    this->nLineFill++;
  };

  return (ERESULT_TYPE_SUCCESS);
};

// AR valid MI
EResultType CCache::Do_AR_fwd_MI(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_FAIL);
  };

  // Check FIFO_AR
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

  // Debug
  assert(upAR_new != NULL);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_AR_fwd_MI] (%s) popped FIFO_AR.\n", nCycle, this->cName.c_str(),
         upAR_new->cpAR->GetName().c_str());
// this->cpFIFO_AR->CheckFIFO();
// this->cpFIFO_AR->Display();
#endif
  assert(this->cpTx_AR->IsBusy() == ERESULT_TYPE_NO);

  // Put Tx
  CPAxPkt cpAR_new = upAR_new->cpAR;
  this->cpTx_AR->PutAx(cpAR_new);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_AR_fwd_MI] (%s) put Tx_AR.\n", nCycle, this->cName.c_str(), cpAR_new->GetName().c_str());
// cpAR_new->Display();
// cpAR_new->CheckPkt();
#endif

  // Stat
  this->nAR_MI++;

  // Maintain
  Delete_UD(upAR_new, EUD_TYPE_AR);

  return (ERESULT_TYPE_SUCCESS);
};

//-------------------------------------------------------------
// AW valid
//-------------------------------------------------------------
//	AR and AW share a cache. Therefore, we need an arbiter.
//	When the cache receives AW, the cache checks whether hit or miss.
//	Then the cache allocates in the tracker.
//	After that, cache provides a hit service (to generate B) or miss service
//(to generate line-fill)
//-------------------------------------------------------------
EResultType CCache::Do_AW_fwd_SI(int64_t nCycle) {

  // Check AR priority and busy
  if (this->Is_AR_priority == ERESULT_TYPE_YES and
      this->cpRx_AR->IsBusy() == ERESULT_TYPE_YES) { // AR priority and accepted
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check remote-Tx valid. Get.
  CPAxPkt cpAW = this->cpRx_AW->GetPair()->GetAx();
  if (cpAW == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Get AW info
  int nID = cpAW->GetID();
  int64_t nAddr = cpAW->GetAddr();

  // Check MO
  if (this->GetMO_Ax() >= MAX_MO_COUNT) { // AR + AW
    return (ERESULT_TYPE_FAIL);
  };

  // Check cache-hit
  EResultType IsHit = this->IsHit(nCycle, nAddr);

  // Check ID dependency
  // if (this->cpTracker->IsIDMatch(nID, EUD_TYPE_AW) == ERESULT_TYPE_YES) {
  //	return (ERESULT_TYPE_SUCCESS);
  //};

  // Check address dependency
  if (this->cpTracker->IsAddrMatch_Cache(nAddr) == ERESULT_TYPE_YES) { // Same line-fill or evict on-going
    return (ERESULT_TYPE_SUCCESS);
  };

  // Check line-fill on going
  // if (this->cpTracker->IsAnyLineFill() == ERESULT_TYPE_YES) {
  if (this->cpTracker->IsSameID_LineFill(nID, EUD_TYPE_AW) == ERESULT_TYPE_YES and IsHit == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Put Rx
  this->cpRx_AW->PutAx(cpAW);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_AW_fwd_SI] %s put Rx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
#endif

  // Hit. Generate B. Push FIFO B
  if (IsHit == ERESULT_TYPE_YES) {

    // Set dirty. Set time
    this->Hit_SetDirtyTime(nCycle, nAddr, EUD_TYPE_AW);

    // Push tracker
    UPUD upAW_new = new UUD;
    upAW_new->cpAW = Copy_CAxPkt(cpAW);
    this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_HIT);
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_AW_fwd_SI] %s is pushed into tracker.\n", nCycle, this->cName.c_str(),
           cpAW->GetName().c_str());
// this->cpTracker->CheckTracker();
#endif

    // Maintain
    Delete_UD(upAW_new, EUD_TYPE_AW);

    // Get AW info
    int nID = cpAW->GetID();
    int nCacheCh = cpAW->GetCacheCh();
    EResultType eFinalTrans = cpAW->IsFinalTrans();

    // Generate B
    CPBPkt cpB_new;
    char cPktName[50];
    sprintf(cPktName, "B_for_%s", cpAW->GetName().c_str());
    cpB_new = new CBPkt(cPktName);
    cpB_new->SetID(nID);
    cpB_new->SetFinalTrans(eFinalTrans);
    cpB_new->SetCacheCh(nCacheCh);

    UPUD upB_new = new UUD;
    upB_new->cpB = cpB_new;

    // Push FIFO B
    this->cpFIFO_B->Push(upB_new, CACHE_FIFO_B_LATENCY);

// Debug
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_AW_fwd_SI] %s is pushed into FIFO_B.\n", nCycle, this->cName.c_str(),
           upB_new->cpB->GetName().c_str());
// this->cpFIFO_B->CheckFIFO();
// this->cpFIFO_B->Display();
#endif

    // Maintain
    Delete_UD(upB_new, EUD_TYPE_B);

    // Stat
    this->nHit_AW++;
  };

  // Miss. Generate AR line-fill. Push FIFO AR
  if (IsHit == ERESULT_TYPE_NO) {

    // Push tracker
    UPUD upAW_new = new UUD;
    upAW_new->cpAW = Copy_CAxPkt(cpAW);
    this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_FILL);
    // printf("[Cycle %3ld: %s.Do_AW_fwd_SI] %s is pushed into tracker.\n",
    // nCycle, this->cName.c_str(), upAW_new->cpAW->GetName().c_str());
    // this->cpTracker->CheckTracker();

    // Maintain
    Delete_UD(upAW_new, EUD_TYPE_AW);

    // Generate AR line-fill
    char cPktName[50];
    sprintf(cPktName, "AR_pkt_fill_for_%s", cpAW->GetName().c_str());
    CPAxPkt cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

    int64_t nAddr = cpAW->GetAddr();
    nAddr = (nAddr >> CACHE_BIT_LINE) << CACHE_BIT_LINE; // Align

    //               nID,            nAddr,  nLen
    cpAR_new->SetPkt(ARID_LINEFILL, nAddr, 3);
    cpAR_new->SetSrcName(this->cName);
    cpAR_new->SetVA(nAddr);
    cpAR_new->SetTransType(ETRANS_TYPE_LINE_FILL);
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_AW_fwd_SI] %s is generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

    // Push FIFO_AR
    UPUD upAR_new2 = new UUD;
    upAR_new2->cpAR = cpAR_new;

    assert(this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO);

    this->cpFIFO_AR->Push(upAR_new2, CACHE_FIFO_AR_LATENCY); // Line-fill

    // Debug
    assert(upAR_new2 != NULL);
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_AW_fwd_SI] %s push FIFO_AR.\n", nCycle, this->cName.c_str(),
           cpAR_new->GetName().c_str());
// cpAR_new->Display();
// this->cpFIFO_AR->Display();
// this->cpFIFO_AR->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upAR_new2, EUD_TYPE_AR);

    // Stat
    this->nLineFill++;
  };

  return (ERESULT_TYPE_SUCCESS);
};

// AW valid MI
EResultType CCache::Do_AW_fwd_MI(int64_t nCycle) {

  // Check Tx valid
  if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_FAIL);
  };

  // Check FIFO_AW
  if (this->cpFIFO_AW->IsEmpty() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_FAIL);
  };

  // Debug
  // this->cpFIFO_AW->CheckFIFO();

  // Pop
  UPUD upAW_new = this->cpFIFO_AW->Pop();

  // Check
  if (upAW_new == NULL) {
    return (ERESULT_TYPE_FAIL);
  };

  // Debug
  assert(upAW_new != NULL);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_AW_fwd_MI] (%s) popped FIFO_AW.\n", nCycle, this->cName.c_str(),
         upAW_new->cpAW->GetName().c_str());
// this->cpFIFO_AW->CheckFIFO();
// this->cpFIFO_AW->Display();
#endif
  assert(this->cpTx_AW->IsBusy() == ERESULT_TYPE_NO);

  // Put Tx
  CPAxPkt cpAW_new = upAW_new->cpAW;
  this->cpTx_AW->PutAx(cpAW_new);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_AW_fwd_MI] (%s) put Tx_AW.\n", nCycle, this->cName.c_str(), cpAW_new->GetName().c_str());
// cpAW_new->Display();
// cpAW_new->CheckPkt();
#endif

  // Stat
  this->nAW_MI++;

  // Maintain
  Delete_UD(upAW_new, EUD_TYPE_AW);

  return (ERESULT_TYPE_SUCCESS);
};

// W valid
EResultType CCache::Do_W_fwd(int64_t nCycle) { // FIXME check

  // Check Tx idle
  if (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_FAIL);
  };

  // Check remote-Tx valid. Get.
  CPWPkt cpW = this->cpRx_W->GetPair()->GetW();
  if (cpW == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Get W info
  int nID = cpW->GetID();

  // Check AW associated
  if (this->cpTracker->GetNode(nID, EUD_TYPE_AW) == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };
  assert(this->cpTracker->GetNode(nID, EUD_TYPE_AW) != NULL);

  // Check WLAST
  if (cpW->IsLast() != ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_SUCCESS);
  };
  assert(cpW->IsLast() == ERESULT_TYPE_YES);

  // Get AW associated
  CPAxPkt cpAW = this->cpTracker->GetNode(nID, EUD_TYPE_AW)->upData->cpAW;

  // Debug
  assert(nID == cpAW->GetID());
  assert(this->GetMO_Ax() <= MAX_MO_COUNT);
  assert(cpAW->GetLen() == 3); // Burst 4

  // Get AW info
  int64_t nAddr = cpAW->GetAddr();

  // Put Rx
  this->cpRx_W->PutW(cpW);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_W_fwd] %s put Rx_W.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
#endif

  // Check state
  ECacheStateType eState = this->cpTracker->GetNode(nID, EUD_TYPE_AW)->eState;

  // Hit. Generate B. Push FIFO B
  if (eState == ECACHE_STATE_TYPE_HIT) {

    // Get B info
    EResultType eFinalTrans = cpAW->IsFinalTrans();

    // Generate B
    CPBPkt cpB_new;
    char cPktName[50];
    sprintf(cPktName, "B_pkt_for_%s", cpAW->GetName().c_str());
    cpB_new = new CBPkt(cPktName);
    cpB_new->SetID(nID);
    cpB_new->SetFinalTrans(eFinalTrans);

    UPUD upB_new = new UUD;
    upB_new->cpB = cpB_new;

    // Push B
    this->cpFIFO_B->Push(upB_new, CACHE_FIFO_B_LATENCY);
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_W_fwd] %s is pushed into FIFO_B.\n", nCycle, this->cName.c_str(),
           upB_new->cpB->GetName().c_str());
// this->cpFIFO_B->CheckFIFO();
// this->cpFIFO_B->Display();
#endif

    // Maintain
    Delete_UD(upB_new, EUD_TYPE_B);
  };

  // Miss. Generate AR line-fill
  if (eState == ECACHE_STATE_TYPE_FILL) {

    // Generate AR line-fill
    char cPktName[50];
    sprintf(cPktName, "AR_pkt_fill_for_%s", cpAW->GetName().c_str());
    CPAxPkt cpAR_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_READ);

    nAddr = (nAddr >> CACHE_BIT_LINE) << CACHE_BIT_LINE; // Align

    //               nID,            nAddr,  nLen
    cpAR_new->SetPkt(ARID_LINEFILL, nAddr, 3);
    cpAR_new->SetSrcName(this->cName);
    cpAR_new->SetVA(nAddr);
    cpAR_new->SetTransType(ETRANS_TYPE_LINE_FILL);

#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_W_fwd] %s is generated.\n", nCycle, this->cName.c_str(), cPktName);
// cpAR_new->Display();
#endif

    // Put Tx
    this->cpTx_AR->PutAx(cpAR_new);
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_W_fwd] %s put Tx_AR.\n", nCycle, this->cName.c_str(), cpAR_new->GetName().c_str());
#endif

    // Maintain
    delete (cpAR_new); // Be careful
    cpAR_new = NULL;

    // Stat
    this->nLineFill++;
  };

  return (ERESULT_TYPE_SUCCESS);
};

// AR ready
EResultType CCache::Do_AR_bwd(int64_t nCycle) {

  // Stat
  if (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES and this->cpTx_AR->GetAcceptResult() == ERESULT_TYPE_ACCEPT) {
    this->nAR_MI++;
  };

  // Check Rx valid
  if (this->cpRx_AR->IsBusy() == ERESULT_TYPE_NO) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Set ready
  this->cpRx_AR->SetAcceptResult(ERESULT_TYPE_ACCEPT);
  // MO
  this->Increase_MO_AR();
  // this->cpTracker->Display();
  this->Increase_MO_Ax();

  // Stat
  this->nAR_SI++;

  return (ERESULT_TYPE_SUCCESS);
};

// AW ready
EResultType CCache::Do_AW_bwd(int64_t nCycle) {

  // Stat
  if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES and this->cpTx_AW->GetAcceptResult() == ERESULT_TYPE_ACCEPT) {
    this->nAW_MI++;
  };

  // Check Rx valid
  if (this->cpRx_AW->IsBusy() == ERESULT_TYPE_NO) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Set ready
  this->cpRx_AW->SetAcceptResult(ERESULT_TYPE_ACCEPT);

  // MO
  this->Increase_MO_AW();
  this->Increase_MO_Ax();

  // Stat
  this->nAW_SI++;

  return (ERESULT_TYPE_SUCCESS);
};

// W ready
EResultType CCache::Do_W_bwd(int64_t nCycle) {

  // Check Tx ready
  if (this->cpTx_W->GetAcceptResult() == ERESULT_TYPE_REJECT) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Stat
  // if (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES and
  // this->cpTx_W->GetAcceptResult() == ERESULT_TYPE_ACCEPT) {
  //	this->nW_MI++;
  // };

  // Check Rx valid
  if (this->cpRx_W->IsBusy() == ERESULT_TYPE_NO) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Set ready
  this->cpRx_W->SetAcceptResult(ERESULT_TYPE_ACCEPT);

  // Stat
  // this->nW_SI++;

  return (ERESULT_TYPE_SUCCESS);
};

// R valid MI
EResultType CCache::Do_R_fwd_MI(int64_t nCycle) {

  // Check remote-Tx valid
  CPRPkt cpR = this->cpRx_R->GetPair()->GetR();
  if (cpR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Debug
  assert(this->cpRx_R->GetPair()->IsBusy() == ERESULT_TYPE_YES);

  // Put Rx
  this->cpRx_R->PutR(cpR);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_R_fwd_MI] %s, ID %d put Rx_R.\n", nCycle, this->cName.c_str(), cpR->GetName().c_str(),
         cpR->GetID());
// cpR->Display();
#endif
  assert(cpR != NULL);

// Debug
#ifdef DEBUG_Cache
  if (cpR->IsLast() == ERESULT_TYPE_YES) {
    printf("[Cycle %3ld: LastLastLast %s.Do_R_fwd_MI] (%s) RID 0x%x put Rx_R.\n", nCycle, this->cName.c_str(),
           cpR->GetName().c_str(), cpR->GetID());
    // cpR->Display();
  } else {
    printf("[Cycle %3ld: %s.Do_R_fwd_MI] (%s) RID 0x%x put Rx_R.\n", nCycle, this->cName.c_str(),
           cpR->GetName().c_str(), cpR->GetID());
    // cpR->Display();
  };
#endif

  // Check RLAST
  if (cpR->IsLast() == ERESULT_TYPE_NO) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Debug
  assert(cpR->IsLast() == ERESULT_TYPE_YES);

  // Get Ax info
  int nID = -1;
  int64_t nAddr = -1;
  int nLen = -1;
  int nCacheCh = -1;
  int64_t nVA = -1;
  EResultType eFinalTrans = ERESULT_TYPE_NO;
  ETransType eTransType = ETRANS_TYPE_UNDEFINED;

  // Get tracker node
  SPLinkedCUD spNode = this->cpTracker->GetNode(ECACHE_STATE_TYPE_FILL);
  // this->cpTracker->Display();
  assert(spNode != NULL);

  // Get original Ax info
  EUDType eUDType = spNode->eUDType;
  if (eUDType == EUD_TYPE_AR) {
    nID = spNode->upData->cpAR->GetID();
    nAddr = spNode->upData->cpAR->GetAddr();
    nCacheCh = spNode->upData->cpAR->GetCacheCh();
    nLen = spNode->upData->cpAR->GetLen();
    eFinalTrans = spNode->upData->cpAR->IsFinalTrans();
    nVA = spNode->upData->cpAR->GetVA();
    eTransType = spNode->upData->cpAR->GetTransType();
  } else if (eUDType == EUD_TYPE_AW) {
    nID = spNode->upData->cpAW->GetID();
    nAddr = spNode->upData->cpAW->GetAddr();
    nCacheCh = spNode->upData->cpAW->GetCacheCh();
    nLen = spNode->upData->cpAW->GetLen();
    eFinalTrans = spNode->upData->cpAW->IsFinalTrans();
    nVA = spNode->upData->cpAW->GetVA();
    eTransType = spNode->upData->cpAW->GetTransType();
  } else {
    assert(0);
  };

  // Debug
  assert(nAddr != -1);
  assert(nID != -1);
  assert(nCacheCh != -1);
  assert(eUDType != EUD_TYPE_UNDEFINED);
  assert(nVA != -1);

  // Get victim
  int nSet = this->GetSetNum(nAddr);
  int nWay = this->GetWayNum(nAddr);
  int64_t nTag = this->GetTagNum(nAddr);

  int64_t nVPN = nVA >> 12;    // this->GetVPNNum(nVA);
  int64_t nAVPN = nVPN & ~(7); // (~(NUM_PTE_PTW-1));

  int nIndexPTE = nAVPN - 0; // this->START_VPN;
  SPTE *spPageTableEntry = this->spPageTable[nIndexPTE];

  // Check victim dirty
  EResultType IsDirty = this->IsDirty(nSet, nWay);

  // Clean victim
  if (IsDirty == ERESULT_TYPE_NO) {
    // Fill line
    if (eUDType == EUD_TYPE_AR and (eTransType == ETRANS_TYPE_FIRST_PTW or eTransType == ETRANS_TYPE_SECOND_PTW or
                                    eTransType == ETRANS_TYPE_THIRD_PTW or eTransType == ETRANS_TYPE_FOURTH_PTW or
                                    eTransType == ETRANS_TYPE_FIRST_APTW or eTransType == ETRANS_TYPE_SECOND_APTW or
                                    eTransType == ETRANS_TYPE_THIRD_APTW or eTransType == ETRANS_TYPE_FOURTH_APTW)) {
      this->FillLine(nCycle, nSet, nWay, nTag, spPageTableEntry);
    } else {
      this->FillLine(nCycle, nSet, nWay, nTag);
    }
  };

  // Generate evict
  if (IsDirty == ERESULT_TYPE_YES) {

    // Get dirty addr
    int64_t nAddrDirty = GetAddrDirty(nSet, nWay);

    // Fill line
    if (eUDType == EUD_TYPE_AR and (eTransType == ETRANS_TYPE_FIRST_PTW or eTransType == ETRANS_TYPE_SECOND_PTW or
                                    eTransType == ETRANS_TYPE_THIRD_PTW or eTransType == ETRANS_TYPE_FOURTH_PTW or
                                    eTransType == ETRANS_TYPE_FIRST_APTW or eTransType == ETRANS_TYPE_SECOND_APTW or
                                    eTransType == ETRANS_TYPE_THIRD_APTW or eTransType == ETRANS_TYPE_FOURTH_APTW)) {
      this->FillLine(nCycle, nSet, nWay, nTag, spPageTableEntry);
    } else {
      this->FillLine(nCycle, nSet, nWay, nTag);
    }

    // Generate AW evict
    char cPktName[50];
    sprintf(cPktName, "AW_pkt_evict_for_0x%lx", nAddrDirty);
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_R_fwd_MI] %s is generated.\n", nCycle, this->cName.c_str(), cPktName);
#endif
    CPAxPkt cpAW_new = new CAxPkt(cPktName, ETRANS_DIR_TYPE_WRITE);

    //               nID,          nAddr,       nLen
    cpAW_new->SetPkt(AWID_EVICT, nAddrDirty, 3);
    cpAW_new->SetVA(nAddrDirty);
    cpAW_new->SetSrcName(this->cName.c_str());
    cpAW_new->SetTransType(ETRANS_TYPE_EVICT);
    // cpAW_new->Display();

    // Debug
    assert(cpAW_new->GetID() == AWID_EVICT);

    // Push FIFO_AW
    UPUD upAW_new2 = new UUD;
    upAW_new2->cpAW = Copy_CAxPkt(cpAW_new);

    assert(this->cpFIFO_AW->IsFull() == ERESULT_TYPE_NO);

    this->cpFIFO_AW->Push(upAW_new2, CACHE_FIFO_AW_LATENCY); // Evict. Original

    // Debug
    assert(upAW_new2 != NULL);
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_R_fwd_MI] %s push FIFO_AW.\n", nCycle, this->cName.c_str(), cpAW_new->GetName().c_str());
// cpAW_new->Display();
// this->cpFIFO_AW->Display();
// this->cpFIFO_AW->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upAW_new2, EUD_TYPE_AW);

    // Push tracker AW evict
    UPUD upAW_new = new UUD;
    upAW_new->cpAW = cpAW_new;
    this->cpTracker->Push(upAW_new, EUD_TYPE_AW, ECACHE_STATE_TYPE_EVICT);
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_R_fwd_MI] %s, ID %d is pushed into tracker.\n", nCycle, this->cName.c_str(),
           cpAW_new->GetName().c_str(), cpAW_new->GetID());
// this->cpTracker->CheckTracker();
#endif

    // Maintain
    Delete_UD(upAW_new, EUD_TYPE_AW);

    // Stat
    this->nEvict++;

#ifdef USE_W_CHANNEL
    // Get AW info for W
    int nLen = cpAW_new->GetLen();
    int nID = cpAW_new->GetID();

    // Debug
    assert(nLen == 3);
    assert(nID == AWID_EVICT);

    for (int i = 0; i <= nLen; i++) {

      char cWPktName[50];
      sprintf(cWPktName, "W_pkt_evict_for_0x%lx", nAddrDirty);
      CPWPkt cpW_new;
      cpW_new = new CWPkt(cWPktName);
      cpW_new->SetID(nID);
      cpW_new->SetData(i + 1);
      if (i == nLen) {
        cpW_new->SetLast(ERESULT_TYPE_YES);
      } else {
        cpW_new->SetLast(ERESULT_TYPE_NO);
      };

      // Push W
      UPUD upW_new = new UUD;
      upW_new->cpW = Copy_CWPkt(cpW_new);
      this->cpFIFO_W->Push(upW_new, CACHE_FIFO_W_LATENCY);

      // Debug
      // upW_new->cpW->CheckPkt();
      // printf("[Cycle %3ld: %s.Do_R_fwd_MI] %s is pushed into FIFO_W.\n",
      // nCycle, this->cName.c_str(), cWPktName); this->cpFIFO_W->Display();
      // this->cpFIFO_W->CheckFIFO();

      // Maintain
      Delete_UD(upW_new, EUD_TYPE_W);
      i
    };
#endif
  };

  // Update tracker state
  this->cpTracker->SetStateNode(nID, eUDType, ECACHE_STATE_TYPE_FILL, ECACHE_STATE_TYPE_HIT);
  // this->cpTracker->Display();

  // Serve hit
  if (eUDType == EUD_TYPE_AR) {

    // Generate R. Push
    for (int nIndex = 0; nIndex <= nLen; nIndex++) {

      // Generate R
      CPRPkt cpR_new = NULL;
      char cPktName[50];
      if (nIndex == nLen) {
        sprintf(cPktName, "RLAST_R%d_for_%s", nIndex, spNode->upData->cpAR->GetName().c_str());
      } else {
        sprintf(cPktName, "R%d_for_%s", nIndex, spNode->upData->cpAR->GetName().c_str());
      };
      cpR_new = new CRPkt(cPktName);
      cpR_new->SetID(nID);
      cpR_new->SetCacheCh(nCacheCh);
      cpR_new->SetData(nIndex);
      cpR_new->SetFinalTrans(eFinalTrans);
      if (nIndex == nLen) {
        cpR_new->SetLast(ERESULT_TYPE_YES);
      } else {
        cpR_new->SetLast(ERESULT_TYPE_NO);
      };

      UPUD upR_new = new UUD;
      upR_new->cpR = cpR_new;

      // Push R
      this->cpFIFO_R->Push(upR_new, CACHE_FIFO_R_LATENCY);

      // Debug
      // printf("[Cycle %3ld: %s.Do_R_fwd_MI] %s is pushed into FIFO_R.\n",
      // nCycle, this->cName.c_str(), upR_new->cpR->GetName().c_str());
      // this->cpFIFO_R->Display();
      // this->cpFIFO_R->CheckFIFO();

      // Maintain
      Delete_UD(upR_new, EUD_TYPE_R);
    };
  } else if (eUDType == EUD_TYPE_AW) {

    // Generate B
    CPBPkt cpB_new;
    char cPktName[50];
    sprintf(cPktName, "B_pkt_hit_for_%s", spNode->upData->cpAW->GetName().c_str());
    cpB_new = new CBPkt(cPktName);
    cpB_new->SetID(nID);
    cpB_new->SetFinalTrans(eFinalTrans);
    cpB_new->SetCacheCh(nCacheCh);

    UPUD upB_new = new UUD;
    upB_new->cpB = cpB_new;

    // Push FIFO B
    this->cpFIFO_B->Push(upB_new, CACHE_FIFO_B_LATENCY);

// Debug
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_R_fwd_MI] %s is pushed into FIFO_B.\n", nCycle, this->cName.c_str(),
           upB_new->cpB->GetName().c_str());
// this->cpFIFO_B->Display();
// this->cpFIFO_B->CheckFIFO();
#endif

    // Maintain
    Delete_UD(upB_new, EUD_TYPE_B);
  } else {
    assert(0);
  };

  return (ERESULT_TYPE_SUCCESS);
};

// R valid SI
EResultType CCache::Do_R_fwd_SI(int64_t nCycle) {

  // Check Tx busy
  if (this->cpTx_R->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_FAIL);
  };

  // Check FIFO
  if (this->cpFIFO_R->IsEmpty() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_FAIL);
  };

  // Pop
  UPUD upR_new = this->cpFIFO_R->Pop();

  // Debug
  assert(upR_new != NULL);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_R_fwd_SI] %s, ID %d is popped from FIFO_R.\n", nCycle, this->cName.c_str(),
         upR_new->cpR->GetName().c_str(), upR_new->cpR->GetID());
// this->cpFIFO_R->Display();
//  this->cpFIFO_R->CheckFIFO();
#endif

  // Get R
  CPRPkt cpR_new = upR_new->cpR;

  // Debug
  // cpR_new->CheckPkt();

  // Put Tx
  this->cpTx_R->PutR(cpR_new);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_R_fwd_SI] %s, ID %d put Tx_R.\n", nCycle, this->cName.c_str(), cpR_new->GetName().c_str(),
         cpR_new->GetID());
// cpR_new->Display();
#endif

  // Maintain
  Delete_UD(upR_new, EUD_TYPE_R);

  return (ERESULT_TYPE_SUCCESS);
};

//--------------------------------------------
// B valid MI
// 	Evict response
// 	Unconditionally get
//--------------------------------------------
EResultType CCache::Do_B_fwd_MI(int64_t nCycle) {

  // Check remote-Tx valid
  CPBPkt cpB = this->cpRx_B->GetPair()->GetB();
  if (cpB == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Debug
  assert(this->cpRx_B->GetPair()->IsBusy() == ERESULT_TYPE_YES);

  // Put Rx
  this->cpRx_B->PutB(cpB);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_B_fwd_MI] %s put Rx_B.\n", nCycle, this->cName.c_str(), cpB->GetName().c_str());
#endif
  assert(cpB != NULL);

  // Debug
  assert(cpB->GetID() == AWID_EVICT);

  return (ERESULT_TYPE_SUCCESS);
};

// B valid SI
EResultType CCache::Do_B_fwd_SI(int64_t nCycle) {

  // Check Tx
  if (this->cpTx_B->IsBusy() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_FAIL);
  };

  // Check FIFO
  if (this->cpFIFO_B->IsEmpty() == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_FAIL);
  };

  // Pop
  UPUD upB_new = this->cpFIFO_B->Pop();

  // Debug
  assert(upB_new != NULL);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_B_fwd_SI] %s is popped from FIFO_B.\n", nCycle, this->cName.c_str(),
         upB_new->cpB->GetName().c_str());
// this->cpFIFO_B->Display();
// this->cpFIFO_B->CheckFIFO();
#endif

  // Get B
  CPBPkt cpB_new = upB_new->cpB;

  // Debug
  // cpB_new->CheckPkt();

  // Put Tx
  this->cpTx_B->PutB(cpB_new);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_B_fwd_SI] %s put Tx_B.\n", nCycle, this->cName.c_str(), upB_new->cpB->GetName().c_str());
#endif

  // Maintain
  Delete_UD(upB_new, EUD_TYPE_B);

  return (ERESULT_TYPE_SUCCESS);
};

// R ready MI
EResultType CCache::Do_R_bwd_MI(int64_t nCycle) {

  // Check Rx valid
  CPRPkt cpR = this->cpRx_R->GetR();
  if (cpR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Set ready
  this->cpRx_R->SetAcceptResult(ERESULT_TYPE_ACCEPT);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_R_bwd_MI] (%s) is handshaked Rx_R.\n", nCycle, this->cName.c_str(),
         cpR->GetName().c_str());
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// R ready SI
EResultType CCache::Do_R_bwd_SI(int64_t nCycle) {

  // Check Tx valid
  CPRPkt cpR = this->cpTx_R->GetR();
  if (cpR == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Debug
  assert(this->cpTx_R->IsBusy() == ERESULT_TYPE_YES);

  // Check ready
  EResultType eAcceptResult = this->cpTx_R->GetAcceptResult();

  // Set ready
  if (eAcceptResult == ERESULT_TYPE_ACCEPT) {
    if (cpR->IsLast() == ERESULT_TYPE_YES) {
      // MO
      // this->cpTracker->Display();
      this->Decrease_MO_AR();
      this->Decrease_MO_Ax();

      // Pop tracker
      UPUD upAR = this->cpTracker->Pop(cpR->GetID(), EUD_TYPE_AR);
#ifdef DEBUG_Cache
      printf("[Cycle %3ld: %s.Do_R_bwd_SI] %s, ID %d is popped from tracker.\n", nCycle, this->cName.c_str(),
             upAR->cpR->GetName().c_str(), upAR->cpR->GetID());
// this->cpTracker->CheckTracker();
#endif

      // Debug
      assert(upAR != NULL);

      // Maintain
      Delete_UD(upAR, EUD_TYPE_AR);
    };
  };

  return (ERESULT_TYPE_SUCCESS);
};

// B ready MI
EResultType CCache::Do_B_bwd_MI(int64_t nCycle) {

  // Check Rx valid
  CPBPkt cpB = this->cpRx_B->GetB();
  if (cpB == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Debug
  assert(this->cpRx_B->IsBusy() == ERESULT_TYPE_YES);
  assert(cpB->GetID() == AWID_EVICT);

  // Set ready
  this->cpRx_B->SetAcceptResult(ERESULT_TYPE_ACCEPT);

  // Pop tracker
  UPUD upAW = this->cpTracker->Pop(cpB->GetID(), EUD_TYPE_AW);
#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.Do_B_bwd_MI] %s, ID %d is popped from tracker.\n", nCycle, this->cName.c_str(),
         upAW->cpAW->GetName().c_str(), cpB->GetID());
// this->cpTracker->CheckTracker();
#endif

  // Debug
  assert(upAW != NULL);

  // Maintain
  Delete_UD(upAW, EUD_TYPE_AW);

  return (ERESULT_TYPE_SUCCESS);
};

// B ready SI
EResultType CCache::Do_B_bwd_SI(int64_t nCycle) {

  // Check Tx valid
  CPBPkt cpB = this->cpTx_B->GetB();
  if (cpB == NULL) {
    return (ERESULT_TYPE_SUCCESS);
  };

  // Debug
  assert(this->cpTx_B->IsBusy() == ERESULT_TYPE_YES);

  // Check ready
  EResultType eAcceptResult = this->cpTx_B->GetAcceptResult();

  // Set ready
  if (eAcceptResult == ERESULT_TYPE_ACCEPT) {
    // MO
    this->Decrease_MO_AW();
    this->Decrease_MO_Ax();

    // Pop tracker
    UPUD upAW = this->cpTracker->Pop(cpB->GetID(), EUD_TYPE_AW);
#ifdef DEBUG_Cache
    printf("[Cycle %3ld: %s.Do_B_bwd_SI] %s, ID %d is popped from tracker.\n", nCycle, this->cName.c_str(),
           upAW->cpAW->GetName().c_str(), cpB->GetID());
// this->cpTracker->CheckTracker();
#endif

    // Debug
    assert(upAW != NULL);

    // Maintain
    Delete_UD(upAW, EUD_TYPE_AW);
  };

  return (ERESULT_TYPE_SUCCESS);
};

// Check dirty
EResultType CCache::IsDirty(int nSet, int nWay) {

  // Debug
  // this->CheckCache();

  // Check
  if (this->spCache[nSet][nWay]->nDirty == 1) {
    return (ERESULT_TYPE_YES);
  };
  return (ERESULT_TYPE_NO);
};

// Get dirty addr
int64_t CCache::GetAddrDirty(int nSet, int nWay) {

  // Debug
  // this->CheckCache();
  assert(this->spCache[nSet][nWay]->nValid == 1);
  assert(this->spCache[nSet][nWay]->nDirty == 1);

  //
  int64_t nTag = this->spCache[nSet][nWay]->nTag;
  int64_t nAddrDirty = nTag << (CACHE_BIT_SET + CACHE_BIT_LINE); // Aligned line-size (64B)

  return (nAddrDirty);
};

// Fill cache line
EResultType CCache::FillLine(int64_t nCycle, int nSet, int nWay, int64_t nTag) {

  // Debug
  // this->CheckCache();

  // Fill new line
  this->spCache[nSet][nWay]->nValid = 1;
  this->spCache[nSet][nWay]->nTag = nTag;
  this->spCache[nSet][nWay]->nData = 1;
  this->spCache[nSet][nWay]->nDirty = 0;
  this->spCache[nSet][nWay]->nTimeStamp = nCycle;

  return (ERESULT_TYPE_SUCCESS);
};

// Fill cache line
EResultType CCache::FillLine(int64_t nCycle, int nSet, int nWay, int64_t nTag, SPTE *spPageTableEntry) {

  // Debug
  // this->CheckCache();

  // Fill new line
  this->spCache[nSet][nWay]->nValid = 1;
  this->spCache[nSet][nWay]->nTag = nTag;
  this->spCache[nSet][nWay]->nData = 1;
  this->spCache[nSet][nWay]->nDirty = 0;
  this->spCache[nSet][nWay]->nTimeStamp = nCycle;
  this->spCache[nSet][nWay]->spPageTableEntry = spPageTableEntry;

#ifdef DEBUG_Cache
  printf("[Cycle %3ld: %s.CacheFill] nSet: %d, nWay: %d, nTag: x%lx\n", nCycle, this->cName.c_str(), nSet, nWay, nTag);
#endif // DEBUG_Cache

  return (ERESULT_TYPE_SUCCESS);
};

// Get Tag number
int64_t CCache::GetTagNum(int64_t nAddr) {

  // Debug
  // this->CheckCache();

  // Get tag number
  // int nTagNum = nAddr >> (CACHE_BIT_SET + CACHE_BIT_LINE);
  int nTagNum = nAddr >> (CACHE_BIT_SET + CACHE_WIDTH + CACHE_BIT_LINE);

  return (nTagNum);
};

// Get Set number
int CCache::GetSetNum(int64_t nAddr) {

  // Debug
  // this->CheckCache();

  // Get cache-block number
  // int nBlock = nAddr >> CACHE_BIT_LINE;
  int nBlock = nAddr >> (CACHE_WIDTH + CACHE_BIT_LINE);

  // Get set index number
  int nSet = nBlock % CACHE_NUM_SET;

  return (nSet);
};

// Replacement FIFO. Not yet support
int CCache::GetWayNum_FIFO(int64_t nAddr) {

  // Debug
  // this->CheckCache();

  // Direct mapped cache
  if (CACHE_NUM_WAY == 1) {
    return (0);
  };
  return (-1);
};

// Replacement LRU
int CCache::GetWayNum_LRU(int64_t nAddr) {

  int nVictimWay = -1;
  int nSet = GetSetNum(nAddr);

  // Debug
  // this->CheckCache();

  // Direct mapped cache
  if (CACHE_NUM_WAY == 1) {
    return (0);
  };

  // Check empty slot
  for (int j = 0; j < CACHE_NUM_WAY; j++) {
    if (this->spCache[nSet][j]->nValid == 0) {
      return (j);
    }
  };

  // LRU: Choose victim way with minimum (least recently used) timestamp
  nVictimWay = 0;
  uint64_t nMinTimeStamp = this->spCache[nSet][0]->nTimeStamp; // DUONGTRAN modify from int to uint64_t
  for (int j = 1; j < CACHE_NUM_WAY; j++) {
    assert(this->spCache[nSet][j]->nValid == 1);
    if (nMinTimeStamp > this->spCache[nSet][j]->nTimeStamp) {
      nMinTimeStamp = this->spCache[nSet][j]->nTimeStamp;
      nVictimWay = j;
    };
  };

  // Debug
  // printf("%ld\n", nMinTimeStamp);
  assert(nMinTimeStamp > 0);

  // Debug
  assert(nVictimWay < CACHE_NUM_WAY);
  assert(nVictimWay >= 0);
  if (CACHE_NUM_WAY == 1) {
    assert(nVictimWay == 0); // Direct mapped cache
  };

  return (nVictimWay);
};

// Replacement random
int CCache::GetWayNum_Random(int64_t nAddr) {

  // Debug
  // this->CheckCache();

  int nSet = GetSetNum(nAddr);

  // Check empty slot
  for (int j = 0; j < CACHE_NUM_WAY; j++) {
    if (this->spCache[nSet][j]->nValid == 0) {
      return (j);
    }
  };

  // Choose victim way randomly (0,1,...,CACHE_NUM_WAY-1)
  int nVictimWay = rand() % (CACHE_NUM_WAY);

  return (nVictimWay);
};

// Replacement
int CCache::GetWayNum(int64_t nAddr) {

  // Debug
  // this->CheckCache();

  int nVictimWay = -1;

#if defined CACHE_REPLACEMENT_RANDOM
  nVictimWay = this->GetWayNum_Random(nAddr);
#elif defined CACHE_REPLACEMENT_FIFO
  nVictimWay = this->GetWayNum_FIFO(nAddr);
#elif defined CACHE_REPLACEMENT_LRU
  nVictimWay = this->GetWayNum_LRU(nAddr);
#endif

  // Debug
  assert(nVictimWay < CACHE_NUM_WAY);
  assert(nVictimWay >= 0);
  if (CACHE_NUM_WAY == 1)
    assert(nVictimWay == 0); // Direct mapped cache

  return (nVictimWay);
};

// Check cache hit
EResultType CCache::IsHit(int64_t nCycle, int64_t nAddr) {

  // Debug
  // this->CheckCache();

  // Get set number
  int nSet = this->GetSetNum(nAddr);

  // Get tag number
  int64_t nTag = this->GetTagNum(nAddr);

  // Find tag
  for (int j = 0; j < CACHE_NUM_WAY; j++) {
    // Check valid and Tag
    if (this->spCache[nSet][j]->nValid == 1 and this->spCache[nSet][j]->nTag == nTag) {
      return (ERESULT_TYPE_YES);
    } else {
      continue;
    };
  };
  return (ERESULT_TYPE_NO);
};

// Check cache hit. Set time
EResultType CCache::Hit_SetTime(int64_t nCycle, int64_t nAddr) {

  // Debug
  // this->CheckCache();

  // Get set number
  int nSet = this->GetSetNum(nAddr);

  // Get tag number
  int64_t nTag = this->GetTagNum(nAddr);

  // Find tag
  for (int j = 0; j < CACHE_NUM_WAY; j++) {
    // Check valid and Tag
    if (this->spCache[nSet][j]->nValid == 1 and this->spCache[nSet][j]->nTag == nTag) {

// Set time
#ifdef CACHE_REPLACEMENT_LRU
      this->spCache[nSet][j]->nTimeStamp = nCycle; // Replacement
#endif

      return (ERESULT_TYPE_YES);
    } else {
      continue;
    };
  };

  // Debug
  assert(0); // Should be hit

  return (ERESULT_TYPE_NO);
};

// Check cache hit. Set dirty. Set time
EResultType CCache::Hit_SetDirtyTime(int64_t nCycle, int64_t nAddr, EUDType eUDType) {

  // Debug
  // this->CheckCache();

  // Get set number
  int nSet = this->GetSetNum(nAddr);

  // Get Tag number
  int64_t nTag = this->GetTagNum(nAddr);

  // Find Tag
  for (int j = 0; j < CACHE_NUM_WAY; j++) {
    // Check valid and Tag
    if (this->spCache[nSet][j]->nValid == 1 and this->spCache[nSet][j]->nTag == nTag) {

      // Set dirty for write
      if (eUDType == EUD_TYPE_AW) {
        this->spCache[nSet][j]->nDirty = 1;
      };

// Set time
#ifdef CACHE_REPLACEMENT_LRU
      this->spCache[nSet][j]->nTimeStamp = nCycle; // Replacement
#endif

      return (ERESULT_TYPE_YES);
    } else {
      continue;
    };
  };

  // Debug
  assert(0); // Should be hit

  return (ERESULT_TYPE_NO);
};

// Get AR MO count
int CCache::GetMO_AR() {

  assert(this->nMO_AR >= 0);
  return (this->nMO_AR);
};

// Get AW MO count
int CCache::GetMO_AW() {

  assert(this->nMO_AW >= 0);
  return (this->nMO_AW);
};

// Get Ax MO count
int CCache::GetMO_Ax() {

  assert(this->nMO_Ax >= 0);
  return (this->nMO_Ax);
};

// Increase AR MO count
EResultType CCache::Increase_MO_AR() {

  this->nMO_AR++;
  return (ERESULT_TYPE_SUCCESS);
};

// Decrease AR MO count
EResultType CCache::Decrease_MO_AR() {

  this->nMO_AR--;
  assert(this->nMO_AR >= 0);
  return (ERESULT_TYPE_SUCCESS);
};

// Increase AW MO count
EResultType CCache::Increase_MO_AW() {

  this->nMO_AW++;
  return (ERESULT_TYPE_SUCCESS);
};

// Decrease AW MO count
EResultType CCache::Decrease_MO_AW() {

  this->nMO_AW--;
  assert(this->nMO_AW >= 0);
  return (ERESULT_TYPE_SUCCESS);
};

// Increase Ax MO count
EResultType CCache::Increase_MO_Ax() {

  this->nMO_Ax++;
  return (ERESULT_TYPE_SUCCESS);
};

// Decrease Ax MO count
EResultType CCache::Decrease_MO_Ax() {

  this->nMO_Ax--;
  assert(this->nMO_Ax >= 0);
  return (ERESULT_TYPE_SUCCESS);
};

// Get Ax pkt name
string CCache::GetName() {

  // Debug
  // this->CheckCache();
  return (this->cName);
};

// Debug
EResultType CCache::CheckCache() {

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
EResultType CCache::CheckLink() { return (ERESULT_TYPE_SUCCESS); };

EResultType CCache::Display() {

  // Debug
  // this->CheckCache();
  return (ERESULT_TYPE_SUCCESS);
};

EResultType CCache::Set_PageTable(SPPTE *spPageTable) {
  this->spPageTable = spPageTable;
  return (ERESULT_TYPE_YES);
}

// Stat
EResultType CCache::PrintStat(int64_t nCycle, FILE *fp) {

  // Debug
  // this->CheckCache();

  printf("--------------------------------------------------------\n");
  printf("\t Name: %s\n", this->cName.c_str());
  printf("--------------------------------------------------------\n");
#ifdef Cache_OFF
  printf("\t Cache                          : OFF\n");
#endif
#ifdef Cache_ON
  printf("\t Cache                          : ON\n");
#endif
  printf("\t Number of sets                 : %d\n", CACHE_NUM_SET);
  printf("\t Number of ways                 : %d\n", CACHE_NUM_WAY);
#if defined CACHE_REPLACEMENT_RANDOM
  printf("\t Replacement policy             : Random \n");
#elif defined CACHE_REPLACEMENT_FIFO
  printf("\t Replacement policy             : FIFO \n");
#elif defined CACHE_REPLACEMENT_LRU
  printf("\t Replacement policy             : LRU \n\n");
#endif
  printf("\t Number of AR SI                : %d\n", this->nAR_SI);
  printf("\t Number of AR MI                : %d\n", this->nAR_MI);
  printf("\t Number of AW SI                : %d\n", this->nAW_SI);
  printf("\t Number of AW MI                : %d\n", this->nAW_MI);
  // printf("\t Number of cache hit for AR  : %d\n",	this->nHit_AR);
  // printf("\t Number of cache hit for AW  : %d\n",	this->nHit_AW);
  printf("\t Cache hit rate AR              : %1.3f\n", (float)(this->nHit_AR) / (this->nAR_SI));
  printf("\t Cache hit rate AW              : %1.3f\n", (float)(this->nHit_AW) / (this->nAW_SI));
  printf("\t Number of line-fills           : %d\n", this->nLineFill);
  printf("\t Number of evicts               : %d\n", this->nEvict);
  printf("\t Max MO tracker Occupancy       : %d\n", this->nMax_MOTracker_Occupancy);
  // printf("\t Total MO tracker Occupancy  : %d\n",
  // this->nTotal_MOTracker_Occupancy);
  printf("\t Avg MO tracker Occupancy       : %1.2f\n\n", (float)(this->nTotal_MOTracker_Occupancy) / nCycle);

  // printf("--------------------------------------------------------\n");

  return (ERESULT_TYPE_SUCCESS);
};
