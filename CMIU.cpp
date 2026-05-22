//-----------------------------------------------------------
// Filename     : CMIU.cpp
// Version	: 0.125
// Date         : 5 Feb 2023
// Contact	: JaeYoung.Hur@gmail.com
// Description	: N x M MIU definition
//-----------------------------------------------------------
// Done
//  (1) Stat (AR,R)	 : MaxAllocCycle, Avg alloc cycles, Occupancy, Empty
//  cycles
//      Stat (AW,B)
//  (2) ROB pop latency
//  (3) MO SI only
//-----------------------------------------------------------
// To do
//  (1) Arbitration	 : R transaction level
//-----------------------------------------------------------
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "CMIU.h"

// Constructor
CMIU::CMIU(string cName, int NUM_PORT_SI, int NUM_PORT_MI) {

  // Name
  this->cName = cName;

  // Cache MIU mode
  this->Is_CACHE_MIU_MODE = ERESULT_TYPE_NO;

  // Num bits port (ID encode)
  this->NUM_PORT_SI = NUM_PORT_SI;
  this->BIT_PORT_SI = (int)(ceilf(log2f(NUM_PORT_SI)));

  this->NUM_PORT_MI = NUM_PORT_MI;

  // Port master interface
  this->cpTx_AR = new CTRx *[NUM_PORT_MI];
  for (int i = 0; i < NUM_PORT_MI; i++) {
    char cName[50];
    sprintf(cName, "MIU_Tx_AR[%d]", i);
    this->cpTx_AR[i] = new CTRx(cName, ETRX_TYPE_TX, EPKT_TYPE_AR);
  };

  this->cpRx_R = new CTRx *[NUM_PORT_MI];
  for (int i = 0; i < NUM_PORT_MI; i++) {
    char cName[50];
    sprintf(cName, "MIU_Rx_R[%d]", i);
    this->cpRx_R[i] = new CTRx(cName, ETRX_TYPE_RX, EPKT_TYPE_R);
  };

  this->cpTx_AW = new CTRx *[NUM_PORT_MI];
  for (int i = 0; i < NUM_PORT_MI; i++) {
    char cName[50];
    sprintf(cName, "MIU_Tx_AW[%d]", i);
    this->cpTx_AW[i] = new CTRx(cName, ETRX_TYPE_TX, EPKT_TYPE_AW);
  };

  this->cpTx_W = new CTRx *[NUM_PORT_MI];
  for (int i = 0; i < NUM_PORT_MI; i++) {
    char cName[50];
    sprintf(cName, "MIU_Tx_W[%d]", i);
    this->cpTx_W[i] = new CTRx(cName, ETRX_TYPE_TX, EPKT_TYPE_W);
  };

  this->cpRx_B = new CTRx *[NUM_PORT_MI];
  for (int i = 0; i < NUM_PORT_MI; i++) {
    char cName[50];
    sprintf(cName, "MIU_Rx_B[%d]", i);
    this->cpRx_B[i] = new CTRx(cName, ETRX_TYPE_RX, EPKT_TYPE_B);
  };

  // Port slave interface
  this->cpRx_AR = new CTRx *[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    char cName[50];
    sprintf(cName, "MIU_Rx_AR[%d]", i);
    this->cpRx_AR[i] = new CTRx(cName, ETRX_TYPE_RX, EPKT_TYPE_AR);
  };

  this->cpRx_AW = new CTRx *[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    char cName[50];
    sprintf(cName, "MIU_Rx_AW[%d]", i);
    this->cpRx_AW[i] = new CTRx(cName, ETRX_TYPE_RX, EPKT_TYPE_AW);
  };

  this->cpTx_R = new CTRx *[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    char cName[50];
    sprintf(cName, "MIU_Tx_R[%d]", i);
    this->cpTx_R[i] = new CTRx(cName, ETRX_TYPE_TX, EPKT_TYPE_R);
  };

  this->cpRx_W = new CTRx *[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    char cName[50];
    sprintf(cName, "MIU_Rx%d_W", i);
    this->cpRx_W[i] = new CTRx(cName, ETRX_TYPE_RX, EPKT_TYPE_W);
  };

  this->cpTx_B = new CTRx *[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    char cName[50];
    sprintf(cName, "MIU_Tx_B[%d]", i);
    this->cpTx_B[i] = new CTRx(cName, ETRX_TYPE_TX, EPKT_TYPE_B);
  };

  // Stat
  this->nAR_SI = new int[NUM_PORT_SI];
  this->nAW_SI = new int[NUM_PORT_SI];
  this->nR_SI = new int[NUM_PORT_SI];
  this->nB_SI = new int[NUM_PORT_SI];

  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nAR_SI[i] = -1;
  };
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nAW_SI[i] = -1;
  };
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nR_SI[i] = -1;
  };
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nB_SI[i] = -1;
  };

  // FIFO MI
  this->cpFIFO_AW = new CFIFO("MIU_FIFO_AW", EUD_TYPE_AW, 16);
  // this->cpFIFO_W  = new CFIFO("MIU_FIFO_W",  EUD_TYPE_W,  64);

  // ROB SI
  this->cpROB_AR_SI = new CROB *[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    char cName[50];
    sprintf(cName, "MIU_ROB_AR[%d]", i);
    this->cpROB_AR_SI[i] = new CROB(cName, EUD_TYPE_AR, 16); // MaxNum 16
  };

  this->cpROB_AW_SI = new CROB *[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    char cName[50];
    sprintf(cName, "MIU_ROB_AW[%d]", i);
    this->cpROB_AW_SI[i] = new CROB(cName, EUD_TYPE_AW, 16);
  };

  // ROB SI
  this->cpROB_R_SI = new CROB *[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    char cName[50];
    sprintf(cName, "MIU_ROB_R[%d]", i);
    this->cpROB_R_SI[i] = new CROB(cName, EUD_TYPE_R, 64); // 64 (= 16 x 4)
  };

  this->cpROB_B_SI = new CROB *[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    char cName[50];
    sprintf(cName, "MIU_ROB_B[%d]", i);
    this->cpROB_B_SI[i] = new CROB(cName, EUD_TYPE_B, 16);
  };

  // Arbiter MI
  this->cpArb_AR = new CArb *[NUM_PORT_MI];
  for (int i = 0; i < NUM_PORT_MI; i++) {
    char cName[50];
    sprintf(cName, "MIU_ARBITER_AR[%d]", i);
    this->cpArb_AR[i] = new CArb(cName, NUM_PORT_SI);
  };

  this->cpArb_AW = new CArb *[NUM_PORT_MI];
  for (int i = 0; i < NUM_PORT_MI; i++) {
    char cName[50];
    sprintf(cName, "MIU_ARBITER_AW[%d]", i);
    this->cpArb_AW[i] = new CArb(cName, NUM_PORT_SI);
  };

  // Arbiter SI
  this->cpArb_R = new CArb *[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    char cName[50];
    sprintf(cName, "MIU_ARBITER_R[%d]", i);
    this->cpArb_R[i] = new CArb(cName, NUM_PORT_MI);
  };

  this->cpArb_B = new CArb *[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    char cName[50];
    sprintf(cName, "MIU_ARBITER_B[%d]", i);
    this->cpArb_B[i] = new CArb(cName, NUM_PORT_MI);
  };

  // Stat MO SI
  this->nMO_AR_SI = new int[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nMO_AR_SI[i] = -1;
  }

  this->nMO_AW_SI = new int[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nMO_AW_SI[i] = -1;
  }

  // Stat alloc cycles SI
  this->nTotal_alloc_cycles_ROB_AR_SI = new int[NUM_PORT_SI];
  this->nTotal_alloc_cycles_ROB_R_SI = new int[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nTotal_alloc_cycles_ROB_AR_SI[i] = -1;
    this->nTotal_alloc_cycles_ROB_R_SI[i] = -1;
  };

  this->nMax_alloc_cycles_ROB_AR_SI = new int[NUM_PORT_SI];
  this->nMax_alloc_cycles_ROB_R_SI = new int[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nMax_alloc_cycles_ROB_AR_SI[i] = -1;
    this->nMax_alloc_cycles_ROB_R_SI[i] = -1;
  };

  this->nTotal_alloc_cycles_ROB_AW_SI = new int[NUM_PORT_SI];
  this->nTotal_alloc_cycles_ROB_B_SI = new int[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nTotal_alloc_cycles_ROB_AW_SI[i] = -1;
    this->nTotal_alloc_cycles_ROB_B_SI[i] = -1;
  };

  this->nMax_alloc_cycles_ROB_AW_SI = new int[NUM_PORT_SI];
  this->nMax_alloc_cycles_ROB_B_SI = new int[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nMax_alloc_cycles_ROB_AW_SI[i] = -1;
    this->nMax_alloc_cycles_ROB_B_SI[i] = -1;
  };

  // Stat occupancy SI
  this->nMax_Occupancy_ROB_AR_SI = new int[NUM_PORT_SI];
  this->nMax_Occupancy_ROB_R_SI = new int[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nMax_Occupancy_ROB_AR_SI[i] = -1;
    this->nMax_Occupancy_ROB_R_SI[i] = -1;
  };

  this->nTotal_Occupancy_ROB_AR_SI = new int[NUM_PORT_SI];
  this->nTotal_Occupancy_ROB_R_SI = new int[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nTotal_Occupancy_ROB_AR_SI[i] = -1;
    this->nTotal_Occupancy_ROB_R_SI[i] = -1;
  };

  this->nMax_Occupancy_ROB_AW_SI = new int[NUM_PORT_SI];
  this->nMax_Occupancy_ROB_B_SI = new int[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nMax_Occupancy_ROB_AW_SI[i] = -1;
    this->nMax_Occupancy_ROB_B_SI[i] = -1;
  };

  this->nTotal_Occupancy_ROB_AW_SI = new int[NUM_PORT_SI];
  this->nTotal_Occupancy_ROB_B_SI = new int[NUM_PORT_SI];
  for (int i = 0; i < NUM_PORT_SI; i++) {
    this->nTotal_Occupancy_ROB_AW_SI[i] = -1;
    this->nTotal_Occupancy_ROB_B_SI[i] = -1;
  };
};

// Destructor
CMIU::~CMIU() {

  // Port MI
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    delete (this->cpTx_AR[i]);
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    delete (this->cpRx_R[i]);
  };

  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpTx_AR[i] = NULL;
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpRx_R[i] = NULL;
  };

  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    delete (this->cpTx_AW[i]);
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    delete (this->cpTx_W[i]);
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    delete (this->cpRx_B[i]);
  };

  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpTx_AW[i] = NULL;
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpTx_W[i] = NULL;
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpRx_B[i] = NULL;
  };

  // Port SI
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    delete (this->cpRx_AR[i]);
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    delete (this->cpRx_AW[i]);
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    delete (this->cpRx_W[i]);
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    delete (this->cpTx_R[i]);
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    delete (this->cpTx_B[i]);
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpRx_AR[i] = NULL;
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpRx_AW[i] = NULL;
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpRx_W[i] = NULL;
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpTx_R[i] = NULL;
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpTx_B[i] = NULL;
  };

  // FIFO MI
  delete (this->cpFIFO_AW);
  // delete (this->cpFIFO_W);

  this->cpFIFO_AW = NULL;
  // this->cpFIFO_W  = NULL;

  // Arb MI
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    delete (this->cpArb_AR[i]);
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpArb_AR[i] = NULL;
  };

  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    delete (this->cpArb_AW[i]);
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpArb_AW[i] = NULL;
  };

  // Arb SI
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    delete (this->cpArb_R[i]);
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpArb_R[i] = NULL;
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    delete (this->cpArb_B[i]);
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpArb_B[i] = NULL;
  };

  // ROB SI
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    delete (this->cpROB_AR_SI[i]);
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    delete (this->cpROB_R_SI[i]);
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpROB_AR_SI[i] = NULL;
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpROB_R_SI[i] = NULL;
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    delete (this->cpROB_AW_SI[i]);
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    delete (this->cpROB_B_SI[i]);
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpROB_AW_SI[i] = NULL;
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpROB_B_SI[i] = NULL;
  };
};

// Initialize
EResultType CMIU::Reset() {

  // Port MI
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpTx_AR[i]->Reset();
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpRx_R[i]->Reset();
  };

  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpTx_AW[i]->Reset();
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpTx_W[i]->Reset();
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpRx_B[i]->Reset();
  };

  // Port SI
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpRx_AR[i]->Reset();
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpRx_AW[i]->Reset();
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpRx_W[i]->Reset();
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpTx_R[i]->Reset();
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpTx_B[i]->Reset();
  };

  // FIFO MI
  this->cpFIFO_AW->Reset();
  // this->cpFIFO_W->Reset();

  // Arb MI
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpArb_AR[i]->Reset();
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpArb_AW[i]->Reset();
  };

  // Arb SI
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpArb_R[i]->Reset();
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpArb_B[i]->Reset();
  };

  // ROB SI
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpROB_AR_SI[i]->Reset();
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpROB_AW_SI[i]->Reset();
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpROB_R_SI[i]->Reset();
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpROB_B_SI[i]->Reset();
  };

  // Stat SI
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->nMO_AR_SI[i] = 0;
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->nMO_AW_SI[i] = 0;
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->nAR_SI[i] = 0;
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->nAW_SI[i] = 0;
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->nR_SI[i] = 0;
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->nB_SI[i] = 0;
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->nTotal_alloc_cycles_ROB_AR_SI[i] = 0;
    this->nTotal_alloc_cycles_ROB_R_SI[i] = 0;

    this->nMax_alloc_cycles_ROB_AR_SI[i] = 0;
    this->nMax_alloc_cycles_ROB_R_SI[i] = 0;

    this->nTotal_alloc_cycles_ROB_AW_SI[i] = 0;
    this->nTotal_alloc_cycles_ROB_B_SI[i] = 0;

    this->nMax_alloc_cycles_ROB_AW_SI[i] = 0;
    this->nMax_alloc_cycles_ROB_B_SI[i] = 0;

    this->nMax_Occupancy_ROB_AR_SI[i] = 0;
    this->nMax_Occupancy_ROB_R_SI[i] = 0;

    this->nTotal_Occupancy_ROB_AR_SI[i] = 0;
    this->nTotal_Occupancy_ROB_R_SI[i] = 0;

    this->nMax_Occupancy_ROB_AW_SI[i] = 0;
    this->nMax_Occupancy_ROB_B_SI[i] = 0;

    this->nTotal_Occupancy_ROB_AW_SI[i] = 0;
    this->nTotal_Occupancy_ROB_B_SI[i] = 0;
  };

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// AR valid
// 	Same as BUS:
// 		(1) Buffer-less style
// 	Different from BUS:
// 		(1) After arbitration, push ROB
//------------------------------------------------------
EResultType CMIU::Do_AR_fwd(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_MI; i++) {

    // Check Tx valid
    if (this->cpTx_AR[i]->IsBusy() == ERESULT_TYPE_YES) {
      continue;
    };

    // Arbiter
    int nCandidateList[this->NUM_PORT_SI];
    for (int nPortSI = 0; nPortSI < this->NUM_PORT_SI; nPortSI++) {

      // Check (1) remote-Tx valid (2) mem ch (3) ROB occupancy
      if (this->cpRx_AR[nPortSI]->GetPair()->IsBusy() == ERESULT_TYPE_YES and
          this->cpRx_AR[nPortSI]->GetPair()->GetAx()->GetChannelNum_MMap() == i and
          this->cpROB_AR_SI[nPortSI]->IsFull() == ERESULT_TYPE_NO) {
        nCandidateList[nPortSI] = 1;

      } else {
        nCandidateList[nPortSI] = 0;
      };
    };

    int nArbResult = this->cpArb_AR[i]->Arbitration(nCandidateList);

    // Check
    if (nArbResult == -1) { // Nothing arbitrated
      continue;
    };

#ifdef DEBUG
    assert(nArbResult < this->NUM_PORT_SI);
    assert(nArbResult > -1);
#endif

    // Get Ax
    CPAxPkt cpAR = this->cpRx_AR[nArbResult]->GetPair()->GetAx();

#ifdef DEBUG
    assert(cpAR != NULL);
// cpAR->CheckPkt();
#endif

    // if (this->cpTx_AR[i]->IsBusy() == ERESULT_TYPE_YES) {

    // CACHE mode
    if (Is_CACHE_MIU_MODE == ERESULT_TYPE_YES) {
      if (this->cpTx_AR[cpAR->GetCacheCh()]->IsBusy() == ERESULT_TYPE_YES) {
        continue;
      };
    };

    // Put Rx
    this->cpRx_AR[nArbResult]->PutAx(cpAR);
    // printf("[Cycle %3ld: %s.Do_AR_fwd] (%s) put Rx_AR[%d].\n", nCycle,
    // this->cName.c_str(), cpAR->GetName().c_str(), nArbResult);
    // cpAR->Display();

    // Debug
    assert(this->GetMO_AR_SI(nArbResult) <= MAX_MO_COUNT);

    // Stat MO
    this->nAR_SI[nArbResult]++;

    // Push ROB
    UPUD upAR_new = new UUD;
    upAR_new->cpAR = Copy_CAxPkt(cpAR);
    int nMemCh = cpAR->GetChannelNum_MMap();

    // Cache mode
    if (this->Is_CACHE_MIU_MODE == ERESULT_TYPE_YES) {
      nMemCh = cpAR->GetCacheCh();
    };

    this->cpROB_AR_SI[nArbResult]->Push(upAR_new, nMemCh, 0); // Latency 0

#ifdef DEBUG
    // this->cpROB_AR_SI[nArbResult]->CheckROB();
    printf("[Cycle %3ld: %s.Do_AR_fwd] (%s) push ROB_AR_SI[%d].\n", nCycle, this->cName.c_str(),
           upAR_new->cpAR->GetName().c_str(), nArbResult);
// this->cpROB_AR_SI[nArbResult]->Display();
#endif

    // Encode ID
    int nID = upAR_new->cpAR->GetID();
    int64_t nAddr = upAR_new->cpAR->GetAddr();
    int nLen = upAR_new->cpAR->GetLen();
    nID = (nID << this->BIT_PORT_SI) + nArbResult;

    // Set pkt
    upAR_new->cpAR->SetPkt(nID, nAddr, nLen);

    // Put Tx
    // this->cpTx_AR[i]->PutAx(upAR_new->cpAR);
    assert(this->cpTx_AR[nMemCh]->IsBusy() == ERESULT_TYPE_NO);
    this->cpTx_AR[nMemCh]->PutAx(upAR_new->cpAR);
    // printf("[Cycle %3ld: %s.Do_AR_fwd] (%s) put Tx_AR[%d].\n", nCycle,
    // this->cName.c_str(), upAR_new->cpAR->GetName().c_str(), i);
    // printf("[Cycle %3ld: %s.Do_AR_fwd] (%s) put Tx_AR[%d].\n", nCycle,
    // this->cName.c_str(), upAR_new->cpAR->GetName().c_str(), nMemCh);
    // upAR_new->cpAR->Display();

    // Maintain
    Delete_UD(upAR_new, EUD_TYPE_AR);
  };

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// AW valid
// 	Buffer-less
//------------------------------------------------------
EResultType CMIU::Do_AW_fwd(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_MI; i++) {

    // Check Tx valid
    if (this->cpTx_AW[i]->IsBusy() == ERESULT_TYPE_YES) {
      continue;
    };

    // Arbiter
    int nCandidateList[this->NUM_PORT_SI];
    for (int nPortSI = 0; nPortSI < this->NUM_PORT_SI; nPortSI++) {

      // Check (1) remote-Tx valid (2) mem ch (3) ROB occupancy
      if (this->cpRx_AW[nPortSI]->GetPair()->IsBusy() == ERESULT_TYPE_YES and
          this->cpRx_AW[nPortSI]->GetPair()->GetAx()->GetChannelNum_MMap() == i and
          this->cpROB_AW_SI[nPortSI]->IsFull() == ERESULT_TYPE_NO) {
        nCandidateList[nPortSI] = 1;

      } else {
        nCandidateList[nPortSI] = 0;
      };
    };

    int nArbResult = this->cpArb_AW[i]->Arbitration(nCandidateList);

    // Check
    if (nArbResult == -1) { // Nothing arbitrated
      continue;
    };

#ifdef DEBUG
    assert(nArbResult < this->NUM_PORT_SI);
    assert(nArbResult > -1);
#endif

    // Get Ax
    CPAxPkt cpAW = this->cpRx_AW[nArbResult]->GetPair()->GetAx();

#ifdef DEBUG
    assert(cpAW != NULL);
// cpAW->CheckPkt();
#endif

    // Cache mode
    if (this->Is_CACHE_MIU_MODE == ERESULT_TYPE_YES) {
      if (this->cpTx_AW[cpAW->GetCacheCh()]->IsBusy() == ERESULT_TYPE_YES) {
        continue;
      };
    };

    // Put Rx
    this->cpRx_AW[nArbResult]->PutAx(cpAW);
    // printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) put Rx_AW[%d].\n", nCycle,
    // this->cName.c_str(), cpAW->GetName().c_str(), nArbResult);
    // cpAW->Display();

    // Debug
    assert(this->GetMO_AW_SI(nArbResult) <= MAX_MO_COUNT);

    // Stat MO
    this->nAW_SI[nArbResult]++;

    // Push ROB
    UPUD upAW_new = new UUD;
    upAW_new->cpAW = Copy_CAxPkt(cpAW);
    int nMemCh = cpAW->GetChannelNum_MMap();

    // Cache mode
    if (this->Is_CACHE_MIU_MODE == ERESULT_TYPE_YES) {
      nMemCh = cpAW->GetCacheCh(); // FIXME
    };

    this->cpROB_AW_SI[nArbResult]->Push(upAW_new, nMemCh, 0); // Latency 0

#ifdef DEBUG
    // this->cpROB_AW_SI[nArbResult]->CheckROB();
    printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) push ROB_AW_SI[%d].\n", nCycle, this->cName.c_str(),
           upAW_new->cpAW->GetName().c_str(), nArbResult);
// this->cpROB_AW_SI[nArbResult]->Display();
#endif

    // Encode ID
    int nID = upAW_new->cpAW->GetID();
    int64_t nAddr = upAW_new->cpAW->GetAddr();
    int nLen = upAW_new->cpAW->GetLen();
    nID = (nID << this->BIT_PORT_SI) + nArbResult;

    // Set pkt
    upAW_new->cpAW->SetPkt(nID, nAddr, nLen);

    // Put Tx
    assert(this->cpTx_AW[nMemCh]->IsBusy() == ERESULT_TYPE_NO);
    this->cpTx_AW[nMemCh]->PutAx(upAW_new->cpAW);
    // printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) put Tx_AW[%d].\n", nCycle,
    // this->cName.c_str(), upAW_new->cpAW->GetName().c_str(), i);
    // printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) put Tx_AW[%d].\n", nCycle,
    // this->cName.c_str(), upAW_new->cpAW->GetName().c_str(), nMemCh);
    // upAW_new->cpAW->Display();

    // Maintain
    Delete_UD(upAW_new, EUD_TYPE_AW);
  };

  //	#ifdef USE_W_CHANNEL
  //	// Generate new Ax arbitrated
  //	UPUD upAW_new = new UUD;
  //	upAW_new->cpAW = Copy_CAxPkt(cpAW);
  //
  //	// Push AW FIFO
  //	this->cpFIFO_AW->Push(upAW_new);
  //
  //	// Debug
  //	// printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) push FIFO_AW.\n", nCycle,
  // this->cName.c_str(), cpAW->GetName());
  //	// this->cpFIFO_AW->Display();
  //	// this->cpFIFO_AW->CheckFIFO();
  //
  //	// Maintain
  //	Delete_UD(upAW_new, EUD_TYPE_AW);
  //
  //	#endif

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// W valid
// 	Buffer-less
//------------------------------------------------------
EResultType CMIU::Do_W_fwd(int64_t nCycle) {

  //	// Check Tx valid
  //	if (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES) {
  //		return (ERESULT_TYPE_FAIL);
  //	};
  //
  //	// Check FIFO_AW
  //	if (this->cpFIFO_AW->IsEmpty() == ERESULT_TYPE_YES) {
  //		return (ERESULT_TYPE_SUCCESS);
  //	};
  //
  //	// Get arbitrated port
  //	int AxID = this->cpFIFO_AW->GetTop()->cpAW->GetID();
  //	int nArb = GetPortNum_SI(AxID);
  //
  //	#ifdef DEBUG
  //	assert (nArb > -1);
  //	assert (nArb < this->NUM_PORT_SI);
  //	#endif
  //
  //	// Check remote-Tx valid
  //	if (this->cpRx_W[nArb]->GetPair()->IsBusy() == ERESULT_TYPE_NO) {
  //		return (ERESULT_TYPE_SUCCESS);
  //	};
  //
  //	// Get W
  //	CPWPkt cpW = this->cpRx_W[nArb]->GetPair()->GetW();
  //
  //	#ifdef DEBUG
  //	assert (cpW != NULL);
  //	// cpW->CheckPkt();
  //	#endif
  //
  //	// Put Rx
  //	this->cpRx_W[nArb]->PutW(cpW);
  //	// printf("[Cycle %3ld: %s.Do_W_fwd] (%s) put Rx_W[%d].\n", nCycle,
  // this->cName.c_str(), cpW->GetName().c_str(), nArbResult);
  //	// cpW->Display();
  //
  //	// Encode ID
  //	int nID = cpW->GetID();
  //	nID = (nID << this->BIT_PORT_SI) + nArb;
  //
  //	// Set pkt
  //	cpW->SetID(nID);
  //
  //	// Put Tx
  //	this->cpTx_W->PutW(cpW);
  //
  //	// Debug
  //	if (cpW->IsLast() == ERESULT_TYPE_YES) {
  //		// printf("[Cycle %3ld: %s.Do_W_fwd] (%s) WLAST put Tx_W.\n",
  // nCycle, this->cName.c_str(), cpW->GetName().c_str());
  //		// cpW->Display();
  //	}
  //	else {
  //		// printf("[Cycle %3ld: %s.Do_W_fwd] (%s) put Tx_W.\n", nCycle,
  // this->cName.c_str(), cpW->GetName().c_str());
  //		// cpW->Display();
  //	};

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// AR ready
// 	Buffer-less
//------------------------------------------------------
// EResultType CMIU::Do_AR_bwd(int64_t nCycle) {
//
//	for (int i=0; i < this->NUM_PORT_MI; i++) {
//
//		// Check Tx ready
//		if (this->cpTx_AR[i]->GetAcceptResult() == ERESULT_TYPE_REJECT)
//{ 			continue;
//		};
//
//		CPAxPkt cpAR = this->cpTx_AR[i]->GetAx();
//
//		//#ifdef DEBUG
//		assert (cpAR != NULL);
//		assert (this->cpTx_AR[i]->IsBusy() == ERESULT_TYPE_YES);
//		string cARPktName = cpAR->GetName();
//		printf("[Cycle %3ld: %s.Do_AR_bwd] (%s) handshake Tx_AR[%d].\n",
// nCycle, this->cName.c_str(), cARPktName.c_str(), i);
//		// cpAR->Display();
//		//#endif
//
//		// Get arbitrated port
//		int nArbResult = this->cpArb_AR[i]->GetArbResult();
//		assert (nArbResult > -1);
//		assert (nArbResult < this->NUM_PORT_SI);
//
//		int nID = cpAR->GetID();
//		int nPortSI = GetPortNum_SI(nID);
//
//		#ifdef DEBUG
//		assert (nArbResult == nPortSI);
//		#endif
//
//		// Check Rx valid
//		if (this->cpRx_AR[nPortSI]->IsBusy() == ERESULT_TYPE_YES) {
//			// Set ready
//			this->cpRx_AR[nPortSI]->SetAcceptResult(ERESULT_TYPE_ACCEPT);
//			// Debug
//			printf("[Cycle %3ld: %s.Do_AR_bwd] (%s) handshake
// Rx_AR[%d].\n", nCycle, this->cName.c_str(), cARPktName.c_str(), nPortSI);
//
//			// MO
//			this->nMO_AR_SI[nPortSI]++;
//		};
//
//	};
//	return (ERESULT_TYPE_SUCCESS);
// };

//------------------------------------------------------
// AR ready
//------------------------------------------------------
EResultType CMIU::Do_AR_bwd_MI(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_MI; i++) {

    // Check Tx ready
    if (this->cpTx_AR[i]->GetAcceptResult() == ERESULT_TYPE_REJECT) {
      continue;
    };

#ifdef DEBUG
    CPAxPkt cpAR = this->cpTx_AR[i]->GetAx();
    assert(cpAR != NULL);
    assert(this->cpTx_AR[i]->IsBusy() == ERESULT_TYPE_YES);
    string cARPktName = cpAR->GetName();
// printf("[Cycle %3ld: %s.Do_AR_bwd_MI] (%s) handshake Tx_AR[%d].\n", nCycle,
// this->cName.c_str(), cARPktName.c_str(), i); cpAR->Display();
#endif
  };
  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// AR ready
//------------------------------------------------------
EResultType CMIU::Do_AR_bwd_SI(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_SI; i++) {

    // Check Rx valid
    if (this->cpRx_AR[i]->IsBusy() == ERESULT_TYPE_YES) {

      CPAxPkt cpAR = this->cpRx_AR[i]->GetAx();
      string cARPktName = cpAR->GetName();

      // Set ready
      this->cpRx_AR[i]->SetAcceptResult(ERESULT_TYPE_ACCEPT);

      // Debug
      // printf("[Cycle %3ld: %s.Do_AR_bwd_SI] (%s) handshake Rx_AR[%d].\n",
      // nCycle, this->cName.c_str(), cARPktName.c_str(), i);

      // MO
      this->nMO_AR_SI[i]++;
    };
  };
  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// AW ready
//------------------------------------------------------
EResultType CMIU::Do_AW_bwd_MI(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_MI; i++) {

    // Check Tx ready
    if (this->cpTx_AW[i]->GetAcceptResult() == ERESULT_TYPE_REJECT) {
      continue;
    };

#ifdef DEBUG
    CPAxPkt cpAW = this->cpTx_AW[i]->GetAx();
    assert(cpAW != NULL);
    assert(this->cpTx_AW[i]->IsBusy() == ERESULT_TYPE_YES);
    string cAWPktName = cpAW->GetName();
    printf("[Cycle %3ld: %s.Do_AW_bwd_MI] (%s) handshake Tx_AW[%d].\n", nCycle, this->cName.c_str(), cAWPktName.c_str(),
           i);
// cpAW->Display();
#endif
  };
  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// AW ready
//------------------------------------------------------
EResultType CMIU::Do_AW_bwd_SI(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_SI; i++) {

    // Check Rx valid
    if (this->cpRx_AW[i]->IsBusy() == ERESULT_TYPE_YES) {

      CPAxPkt cpAW = this->cpRx_AW[i]->GetAx();
      string cAWPktName = cpAW->GetName();

      // Set ready
      this->cpRx_AW[i]->SetAcceptResult(ERESULT_TYPE_ACCEPT);

      // Debug
      // printf("[Cycle %3ld: %s.Do_AW_bwd_SI] (%s) handshake Rx_AW[%d].\n",
      // nCycle, this->cName.c_str(), cAWPktName.c_str(), i);

      // MO
      this->nMO_AW_SI[i]++;
    };
  };
  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// AW ready
//------------------------------------------------------
// EResultType CMIU::Do_AW_bwd(int64_t nCycle) {
//
//	// Check Tx ready
//	if (this->cpTx_AW->GetAcceptResult() == ERESULT_TYPE_REJECT) {
//		return (ERESULT_TYPE_SUCCESS);
//	};
//
//	CPAxPkt cpAW = this->cpTx_AW->GetAx();
//
//	#ifdef DEBUG
//	assert (cpAW != NULL);
//	assert (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES);
//	// string cAWPktName = cpAW->GetName();
//	// printf("[Cycle %3ld: %s.Do_AW_bwd] (%s) handshake Tx_AW.\n", nCycle,
// this->cName.c_str(), cAWPktName.c_str());
//	// cpAW->Display();
//	#endif
//
//	#ifdef DEBUG
//	// Get arbitrated port
//	int nArbResult = this->cpArb_AW->GetArbResult();
//	assert (nArbResult > -1);
//	assert (nArbResult < this->NUM_PORT_SI);
//	#endif
//
//	int nID = cpAW->GetID();
//	int nPortSI = GetPortNum_SI(nID);
//
//	#ifdef DEBUG
//	assert (nArbResult == nPortSI);
//	#endif
//
//	// Check Rx valid
//	if (this->cpRx_AW[nPortSI]->IsBusy() == ERESULT_TYPE_YES) {
//		// Set ready
//		this->cpRx_AW[nPortSI]->SetAcceptResult(ERESULT_TYPE_ACCEPT);
//
//		// MO
//		if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES ) {
//			this->nMO_AW_SI++;
//		};
//	};
//
//	return (ERESULT_TYPE_SUCCESS);
// };

//------------------------------------------------------
// W ready
// 	Buffer-less
//------------------------------------------------------
EResultType CMIU::Do_W_bwd(int64_t nCycle) {

  //	// Check Tx ready
  //	if (this->cpTx_W->GetAcceptResult() == ERESULT_TYPE_REJECT) {
  //		return (ERESULT_TYPE_SUCCESS);
  //	};
  //
  //	CPWPkt cpW = this->cpTx_W->GetW();
  //
  //	#ifdef DEBUG
  //	assert (cpW != NULL);
  //	assert (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES);
  //	// printf("[Cycle %3ld: %s.Do_W_bwd] (%s) handshake Tx_W.\n", nCycle,
  // this->cName.c_str(), cpW->GetName().c_str());
  //	// cpW->Display();
  //	assert (this->cpFIFO_AW->IsEmpty() == ERESULT_TYPE_NO);
  //	#endif
  //
  //	// Get arbitrated port
  //	int AxID = this->cpFIFO_AW->GetTop()->cpAW->GetID();
  //	int nArb = GetPortNum_SI(AxID);
  //
  //	#ifdef DEBUG
  //	assert (nArb >= 0);
  //	assert (nArb < this->NUM_PORT_SI);
  //	#endif
  //
  //	// Check Rx valid
  //	if (this->cpRx_W[nArb]->IsBusy() == ERESULT_TYPE_YES) {
  //		// Set ready
  //		this->cpRx_W[nArb]->SetAcceptResult(ERESULT_TYPE_ACCEPT);
  //	};
  //
  //	// Check WLAST. Pop FIFO_AW
  //	if (this->cpTx_W->IsPass() == ERESULT_TYPE_YES and cpW->IsLast() ==
  // ERESULT_TYPE_YES) {
  //
  //		#ifdef DEBUG
  //		assert (this->cpFIFO_AW->IsEmpty() == ERESULT_TYPE_NO);
  //		#endif
  //
  //		UPUD up_new = this->cpFIFO_AW->Pop();
  //		// this->cpFIFO_AW->CheckFIFO();
  //
  //		// Maintain
  //		Delete_UD(up_new, EUD_TYPE_AW);
  //	};

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// R Valid
// 	MIU N x M
// 	1. Check ROB full
// 	2. Arbitrate
// 	3. Push ROB
//------------------------------------------------------
EResultType CMIU::Do_R_fwd_Arb(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_SI; i++) {

    // Check ROB_R
    if (this->cpROB_R_SI[i]->IsFull() == ERESULT_TYPE_YES) {
      continue;
    };

    // Arbiter
    int nCandidateList[this->NUM_PORT_MI];
    // EResultType IsRLAST[this->NUM_PORT_MI];
    for (int nPortMI = 0; nPortMI < this->NUM_PORT_MI; nPortMI++) {

      nCandidateList[nPortMI] = 0;
      // IsRLAST[nPortMI] = ERESULT_TYPE_NO;

      // Check (1) remote-Tx valid
      if (this->cpRx_R[nPortMI]->GetPair()->IsBusy() == ERESULT_TYPE_YES) {

        CPRPkt cpR = this->cpRx_R[nPortMI]->GetPair()->GetR();
        int nID = cpR->GetID();
        int nPortSI = GetPortNum_SI(nID);

        // Check (2) mem ch
        if (nPortSI == i) {
          nCandidateList[nPortMI] = 1;
          // IsRLAST[nPortMI] = cpR->IsLast();
        };
      };
    };

#ifdef R_ARBITER_TRANSACTION_LEVEL
    int nArbResult = this->cpArb_R[i]->Arbitration_RR(nCandidateList, IsRLAST); // R transaction level
#else
    int nArbResult = this->cpArb_R[i]->Arbitration(nCandidateList); // R data level
#endif

    // Check
    if (nArbResult == -1) { // Nothing arbitrated
      continue;
    };

#ifdef DEBUG
    assert(nArbResult < this->NUM_PORT_MI);
    assert(nArbResult > -1);
#endif

    // Get R
    CPRPkt cpR = this->cpRx_R[nArbResult]->GetPair()->GetR();

#ifdef DEBUG
    assert(cpR != NULL);
// cpAR->CheckPkt();
#endif

    // Get destination port
    int nID = cpR->GetID();
    int nPortSI = GetPortNum_SI(nID);

    // Debug
    assert(nPortSI == i);

    // Check ROB
    if (this->cpROB_R_SI[i]->IsFull() == ERESULT_TYPE_YES) {
      assert(0);
      return (ERESULT_TYPE_SUCCESS);
    };

    // Stat
    this->nR_SI[i]++;

    // Put Rx
    this->cpRx_R[nArbResult]->PutR(cpR);

    // Debug
    if (cpR->IsLast() == ERESULT_TYPE_YES) {
      // printf("[Cycle %3ld: %s.Do_R_fwd_MI] (%s) RID 0x%x RLAST put
      // Rx_R[%d].\n", nCycle, this->cName.c_str(), cpR->GetName().c_str(),
      // cpR->GetID(), nArbResult); cpR->Display();
    } else {
      // printf("[Cycle %3ld: %s.Do_R_fwd_MI] (%s) RID 0x%x put Rx_R[%d].\n",
      // nCycle, this->cName.c_str(), cpR->GetName().c_str(), cpR->GetID(),
      // nArbResult); cpR->Display();
    };

    // Decode ID
    UPUD upR_new = new UUD;
    upR_new->cpR = Copy_CRPkt(cpR);

    nID = nID >> this->BIT_PORT_SI;
    upR_new->cpR->SetID(nID);

    // Debug
    int nMemCh = cpR->GetMemCh();

    // Cache mode
    if (this->Is_CACHE_MIU_MODE == ERESULT_TYPE_YES) {
      nMemCh = cpR->GetCacheCh();
    };

    assert(nMemCh == nArbResult);

    // Push ROB
    this->cpROB_R_SI[i]->Push(upR_new, nMemCh, ROB_RESP_LATENCY);

#ifdef DEBUG
    printf("[Cycle %3ld: %s.Do_R_fwd_MI] (%s) push ROB_R_SI[%d].\n", nCycle, this->cName.c_str(),
           upR_new->cpR->GetName().c_str(), i);
// this->cpROB_R_SI[i]->Display();
// this->cpROB_R_SI[i]->CheckROB();
#endif

    // Maintain
    Delete_UD(upR_new, EUD_TYPE_R);
  };

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// R Valid
// 	MIU
// 	1. Check. Find popable R (per SI)
//------------------------------------------------------
EResultType CMIU::Do_R_fwd_SI(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_SI; i++) {

    // Check Tx valid
    if (this->cpTx_R[i]->IsBusy() == ERESULT_TYPE_YES) {
      continue;
    };

    // Get data
    SPLinkedRUD spR = this->cpROB_R_SI[i]->GetResp_poppable_ROB(this->cpROB_AR_SI[i]);

    // Check
    if (spR == NULL) {
      continue;
    };

    int nID = spR->nID;
    int nMemCh = spR->nMemCh;

    //// Cache mode
    // if (this->Is_CACHE_MIU_MODE == ERESULT_TYPE_YES) {
    // 	nMemCh = spR->nMemCh;
    // };

    // Stat
    this->nTotal_alloc_cycles_ROB_R_SI[i] += spR->nCycle_wait;

    if (this->nMax_alloc_cycles_ROB_R_SI[i] < spR->nCycle_wait) {
      this->nMax_alloc_cycles_ROB_R_SI[i] = spR->nCycle_wait;
    };

    // Pop data
    UPUD upR_new = this->cpROB_R_SI[i]->Pop(nID, nMemCh);

    CPRPkt cpR = upR_new->cpR;

    // Debug
    assert(nID == cpR->GetID());

    // Maintain ROB_Ax
    if (cpR->IsLast() == ERESULT_TYPE_YES) {
      // printf("[Cycle %3ld: %s.Do_R_fwd_SI] (%s) RID 0x%x RLAST popped %s.\n",
      // nCycle, this->cName.c_str(), cpR->GetName().c_str(), cpR->GetID(),
      // this->cpROB_R_SI[i]->GetName().c_str()); cpR->Display();

      // Get head Ax
      SPLinkedRUD spNode = this->cpROB_AR_SI[i]->GetReqNode_ROB(nID, nMemCh);

      // Stat
      this->nTotal_alloc_cycles_ROB_AR_SI[i] += spNode->nCycle_wait;

      if (this->nMax_alloc_cycles_ROB_AR_SI[i] < spNode->nCycle_wait) {
        this->nMax_alloc_cycles_ROB_AR_SI[i] = spNode->nCycle_wait;
      };

      // Pop ROB_Ax
      UPUD upAR_new = this->cpROB_AR_SI[i]->Pop(nID, nMemCh);

      // Maintain
      Delete_UD(upAR_new, EUD_TYPE_AR);
    } else {
      // printf("[Cycle %3ld: %s.Do_R_fwd_SI] (%s) RID 0x%x popped %s.\n",
      // nCycle, this->cName.c_str(), cpR->GetName().c_str(), cpR->GetID(),
      // this->cpROB_R_SI[i]->GetName().c_str()); cpR->Display();
    };

    // Put Tx
    CPRPkt cpR_new = Copy_CRPkt(cpR);
    // int nID = nID >> this->BIT_PORT_SI;
    // cpR_new->SetID(nID);

    this->cpTx_R[i]->PutR(cpR_new);

    // Debug
    if (cpR_new->IsLast() == ERESULT_TYPE_YES) {
      // printf("[Cycle %3ld: %s.Do_R_fwd_SI] (%s) RID 0x%x RLAST put
      // Tx_R[%d].\n", nCycle, this->cName.c_str(), cpR_new->GetName().c_str(),
      // cpR_new->GetID(), i); cpR_new->Display();
    } else {
      // printf("[Cycle %3ld: %s.Do_R_fwd_SI] (%s) RID 0x%x put Tx_R[%d].\n",
      // nCycle, this->cName.c_str(), cpR_new->GetName().c_str(),
      // cpR_new->GetID(), i); cpR_new->Display();
    };

    // Maintain
    Delete_UD(upR_new, EUD_TYPE_R);
    delete (cpR_new);
    cpR_new = NULL;
  };

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// B Valid
// 	MIU N x M
// 	1. Check ROB full
// 	2. Arbitrate
// 	3. Push ROB
//------------------------------------------------------
EResultType CMIU::Do_B_fwd_Arb(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_SI; i++) {

    // Check ROB_B
    if (this->cpROB_B_SI[i]->IsFull() == ERESULT_TYPE_YES) {
      continue;
    };

    // Arbiter
    int nCandidateList[this->NUM_PORT_MI];
    for (int nPortMI = 0; nPortMI < this->NUM_PORT_MI; nPortMI++) {

      nCandidateList[nPortMI] = 0;

      // Check (1) remote-Tx valid
      if (this->cpRx_B[nPortMI]->GetPair()->IsBusy() == ERESULT_TYPE_YES) {

        CPBPkt cpB = this->cpRx_B[nPortMI]->GetPair()->GetB();
        int nID = cpB->GetID();
        int nPortSI = GetPortNum_SI(nID);

        // Check (2) mem ch
        if (nPortSI == i) {
          nCandidateList[nPortMI] = 1;
        };
      };
    };

    int nArbResult = this->cpArb_B[i]->Arbitration(nCandidateList);

    // Check
    if (nArbResult == -1) { // Nothing arbitrated
      continue;
    };

#ifdef DEBUG
    assert(nArbResult < this->NUM_PORT_MI);
    assert(nArbResult > -1);
#endif

    // Get B
    CPBPkt cpB = this->cpRx_B[nArbResult]->GetPair()->GetB();

#ifdef DEBUG
    assert(cpB != NULL);
// cpB->CheckPkt();
#endif

    // Get destination port
    int nID = cpB->GetID();
    int nPortSI = GetPortNum_SI(nID);

    // Debug
    assert(nPortSI == i);

    // Check ROB
    if (this->cpROB_B_SI[i]->IsFull() == ERESULT_TYPE_YES) {
      assert(0);
      return (ERESULT_TYPE_SUCCESS);
    };

    // Put Rx
    this->cpRx_B[nArbResult]->PutB(cpB);

    // Debug
    // printf("[Cycle %3ld: %s.Do_B_fwd_MI] (%s) BID 0x%x put Rx_B[%d].\n",
    // nCycle, this->cName.c_str(), cpB->GetName().c_str(), cpB->GetID(),
    // nArbResult); cpB->Display();

    // Decode ID
    UPUD upB_new = new UUD;
    upB_new->cpB = Copy_CBPkt(cpB);

    nID = nID >> this->BIT_PORT_SI;
    upB_new->cpB->SetID(nID);

    // Debug
    int nMemCh = cpB->GetMemCh();

    // Cache mode
    if (this->Is_CACHE_MIU_MODE == ERESULT_TYPE_YES) {
      nMemCh = cpB->GetCacheCh(); // FIXME
    };

    if (nMemCh != nArbResult) {
      printf("\n[Cycle %ld %s Something wrong. nMemCh = %d, nArbResult=%d\n", nCycle, this->cName.c_str(), nMemCh,
             nArbResult); // FIXME
    };
    assert(nMemCh == nArbResult);

    // Push ROB
    this->cpROB_B_SI[i]->Push(upB_new, nMemCh, ROB_RESP_LATENCY);

#ifdef DEBUG
    printf("[Cycle %3ld: %s.Do_B_fwd_MI] (%s) push ROB_B_SI[%d].\n", nCycle, this->cName.c_str(),
           upB_new->cpB->GetName().c_str(), i);
// this->cpROB_B_SI[i]->Display();
// this->cpROB_B_SI[i]->CheckROB();
#endif

    // Maintain
    Delete_UD(upB_new, EUD_TYPE_B);
  };

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// B Valid
// 	MIU
// 	1. Check. Find popable B (per SI)
//------------------------------------------------------
EResultType CMIU::Do_B_fwd_SI(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_SI; i++) {

    // Check Tx valid
    if (this->cpTx_B[i]->IsBusy() == ERESULT_TYPE_YES) {
      continue;
    };

    // Get data
    SPLinkedRUD spB = this->cpROB_B_SI[i]->GetResp_poppable_ROB(this->cpROB_AW_SI[i]);

    // Check
    if (spB == NULL) {
      continue;
    };

    int nID = spB->nID;
    int nMemCh = spB->nMemCh;

    // Stat
    this->nTotal_alloc_cycles_ROB_B_SI[i] += spB->nCycle_wait;

    if (this->nMax_alloc_cycles_ROB_B_SI[i] < spB->nCycle_wait) {
      this->nMax_alloc_cycles_ROB_B_SI[i] = spB->nCycle_wait;
    };

    // Pop data
    UPUD upB_new = this->cpROB_B_SI[i]->Pop(nID, nMemCh);

    CPBPkt cpB = upB_new->cpB;

    // Debug
    assert(nID == cpB->GetID());

    // Maintain ROB_Ax
    // printf("[Cycle %3ld: %s.Do_B_fwd_SI] (%s) BID 0x%x popped %s.\n", nCycle,
    // this->cName.c_str(), cpB->GetName().c_str(), cpB->GetID(),
    // this->cpROB_B_SI[i]->GetName().c_str()); cpB->Display();

    // Get head Ax
    SPLinkedRUD spNode = this->cpROB_AW_SI[i]->GetReqNode_ROB(nID, nMemCh);

    // Stat
    this->nTotal_alloc_cycles_ROB_AW_SI[i] += spNode->nCycle_wait;

    if (this->nMax_alloc_cycles_ROB_AW_SI[i] < spNode->nCycle_wait) {
      this->nMax_alloc_cycles_ROB_AW_SI[i] = spNode->nCycle_wait;
    };

    // Pop ROB_Ax
    UPUD upAW_new = this->cpROB_AW_SI[i]->Pop(nID, nMemCh);

    // Maintain
    Delete_UD(upAW_new, EUD_TYPE_AW);

    // Put Tx
    CPBPkt cpB_new = Copy_CBPkt(cpB);
    // int nID = nID >> this->BIT_PORT_SI;
    // cpB_new->SetID(nID);

    this->cpTx_B[i]->PutB(cpB_new);

    // Stat
    this->nB_SI[i]++;

#ifdef DEBUG
    printf("[Cycle %3ld: %s.Do_B_fwd_SI] (%s) BID 0x%x put Tx_B[%d].\n", nCycle, this->cName.c_str(),
           cpB_new->GetName().c_str(), cpB_new->GetID(), i);
// cpB_new->Display();
#endif

    // Maintain
    Delete_UD(upB_new, EUD_TYPE_B);
    delete (cpB_new);
    cpB_new = NULL;
  };

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// B Valid
// 	Buffer-less
//------------------------------------------------------
// EResultType CMIU::Do_B_fwd(int64_t nCycle) {
//
//	// Check remote-Tx valid
//	CPBPkt cpB = this->cpRx_B->GetPair()->GetB();
//	if (cpB == NULL) {
//		return (ERESULT_TYPE_SUCCESS);
//	};
//	// cpB->CheckPkt();
//
//	// Get destination port
//	int nID = cpB->GetID();
//	int nPortSI = GetPortNum_SI(nID);
//
//	// Check Tx valid
//	if (this->cpTx_B[nPortSI]->IsBusy() == ERESULT_TYPE_YES) {
//	        return (ERESULT_TYPE_SUCCESS);
//	};
//
//	// Put Rx
//	this->cpRx_B->PutB(cpB);
//
//	// Debug
//	// printf("[Cycle %3ld: %s.Do_B_fwd] (%s) BID 0x%x put Rx_B.\n", nCycle,
// this->cName.c_str(), cpB->GetName().c_str(), cpB->GetID() );
//	// cpB->Display();
//
//	// Decode ID
//	CPBPkt cpB_new = Copy_CBPkt(cpB);
//	nID = nID >> this->BIT_PORT_SI;
//	cpB_new->SetID(nID);
//
//	// Put Tx
//	this->cpTx_B[nPortSI]->PutB(cpB_new);
//
//	// Stat
//	this->nB_SI[nPortSI]++;
//
//	// Debug
//	// printf("[Cycle %3ld: %s.Do_B_fwd] (%s) BID 0x%x put Tx_B[%d].\n",
// nCycle, this->cName.c_str(), cpB_new->GetName().c_str(), cpB->GetID(),
// nPortSI);
//	// cpB_new->Display();
//
//	// Maintain
//	delete (cpB_new);
//	cpB_new = NULL;
//
//	return (ERESULT_TYPE_SUCCESS);
// };

//------------------------------------------------------
// R ready
// 	Buffer-less
//------------------------------------------------------
// EResultType CMIU::Do_R_bwd(int64_t nCycle) {
//
//	for (int i= 0; i < this->NUM_PORT_MI; i++) {
//
//		// Check Rx valid
//		CPRPkt cpR = this->cpRx_R[i]->GetR();
//		if (cpR == NULL) {
//			continue;
//		};
//
//		#ifdef DEBUG
//		// cpR->CheckPkt();
//		assert (this->cpRx_R[i]->IsBusy() == ERESULT_TYPE_YES);
//		#endif
//
//		// Get destination port
//		int nID = cpR->GetID();
//		int nPortSI = GetPortNum_SI(nID);
//
//		#ifdef DEBUG
//		// assert (eAcceptResult == ERESULT_TYPE_ACCEPT);
//		#endif
//
//		// Set ready
//		this->cpRx_R[i]->SetAcceptResult(ERESULT_TYPE_ACCEPT);
//
//		// Debug
//		if (cpR->IsLast() == ERESULT_TYPE_YES) {
//			// printf("[Cycle %3ld: %s.Do_R_bwd] (%s) RLAST
// handshake Rx_R[%d].\n", nCycle, this->cName.c_str(), cpR->GetName().c_str(),
// i);
//
//			// Decrease MO
//			this->nMO_AR_SI[nPortSI]--;
//		}
//		else {
//			// printf("[Cycle %3ld: %s.Do_R_bwd] (%s) handshake
// Rx_R[%d].\n", nCycle, this->cName.c_str(), cpR->GetName().c_str(), i);
//		};
//
//		// Debug
//		// cpR->Display();
//	};
//
//	return (ERESULT_TYPE_SUCCESS);
// };

//------------------------------------------------------
// R ready
//------------------------------------------------------
EResultType CMIU::Do_R_bwd_MI(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_MI; i++) {

    // Check Rx valid
    CPRPkt cpR = this->cpRx_R[i]->GetR();
    if (cpR == NULL) {
      continue;
    };

#ifdef DEBUG
    // cpR->CheckPkt();
    assert(this->cpRx_R[i]->IsBusy() == ERESULT_TYPE_YES);
#endif

    // Get destination port
    // int nID = cpR->GetID();
    // int nPortSI = GetPortNum_SI(nID);

    // Set ready
    this->cpRx_R[i]->SetAcceptResult(ERESULT_TYPE_ACCEPT);

    // Debug
    // if (cpR->IsLast() == ERESULT_TYPE_YES) {
    //	 printf("[Cycle %3ld: %s.Do_R_bwd_MI] (%s) RLAST handshake Rx_R[%d].\n",
    // nCycle, this->cName.c_str(), cpR->GetName().c_str(), i);
    //}
    // else {
    //	 printf("[Cycle %3ld: %s.Do_R_bwd_MI] (%s) handshake Rx_R[%d].\n",
    // nCycle, this->cName.c_str(), cpR->GetName().c_str(), i);
    //};

    // Debug
    // cpR->Display();
  };

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// R ready
//------------------------------------------------------
EResultType CMIU::Do_R_bwd_SI(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_SI; i++) {

    // Check Rx valid
    CPRPkt cpR = this->cpTx_R[i]->GetR();
    if (cpR == NULL) {
      continue;
    };

    // Get destination port
    // int nID = cpR->GetID();
    // int nPortSI = GetPortNum_SI(nID);

    // Check Tx ready
    EResultType eAcceptResult = this->cpTx_R[i]->GetAcceptResult();
    if (eAcceptResult == ERESULT_TYPE_ACCEPT) {

      if (cpR->IsLast() == ERESULT_TYPE_YES) {
        // Decrease MO
        this->nMO_AR_SI[i]--;

        // printf("[Cycle %3ld: %s.Do_R_bwd_SI] (%s) RLAST handshake
        // Tx_R[%d].\n", nCycle, this->cName.c_str(), cpR->GetName().c_str(),
        // i);
      } else {
        // printf("[Cycle %3ld: %s.Do_R_bwd_SI] (%s) handshake Tx_R[%d].\n",
        // nCycle, this->cName.c_str(), cpR->GetName().c_str(), i);
      };
    };

#ifdef DEBUG
// assert (eAcceptResult == ERESULT_TYPE_ACCEPT);
#endif

    // Debug
    // cpR->Display();
  };

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// B ready
//------------------------------------------------------
EResultType CMIU::Do_B_bwd_MI(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_MI; i++) {

    // Check Rx valid
    CPBPkt cpB = this->cpRx_B[i]->GetB();
    if (cpB == NULL) {
      continue;
    };

#ifdef DEBUG
    // cpB->CheckPkt();
    assert(this->cpRx_B[i]->IsBusy() == ERESULT_TYPE_YES);
#endif

    // Get destination port
    // int nID = cpB->GetID();
    // int nPortSI = GetPortNum_SI(nID);

    // Set ready
    this->cpRx_B[i]->SetAcceptResult(ERESULT_TYPE_ACCEPT);

    // Debug
    // printf("[Cycle %3ld: %s.Do_B_bwd_MI] (%s) handshake Rx_B[%d].\n", nCycle,
    // this->cName.c_str(), cpB->GetName().c_str(), i);

    // Debug
    // cpB->Display();
  };

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// B ready
//------------------------------------------------------
EResultType CMIU::Do_B_bwd_SI(int64_t nCycle) {

  for (int i = 0; i < this->NUM_PORT_SI; i++) {

    // Check Rx valid
    CPBPkt cpB = this->cpTx_B[i]->GetB();
    if (cpB == NULL) {
      continue;
    };

    // Get destination port
    // int nID = cpB->GetID();
    // int nPortSI = GetPortNum_SI(nID);

    // Check Tx ready
    EResultType eAcceptResult = this->cpTx_B[i]->GetAcceptResult();
    if (eAcceptResult == ERESULT_TYPE_ACCEPT) {

      // Decrease MO
      this->nMO_AW_SI[i]--;

      // printf("[Cycle %3ld: %s.Do_B_bwd_SI] (%s) handshake Tx_B[%d].\n",
      // nCycle, this->cName.c_str(), cpB->GetName().c_str(), i);
    };

#ifdef DEBUG
// assert (eAcceptResult == ERESULT_TYPE_ACCEPT);
#endif

    // Debug
    // cpB->Display();
  };

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// B ready
// 	Buffer-less
//------------------------------------------------------
// EResultType CMIU::Do_B_bwd(int64_t nCycle) {
//
//	// Check Rx valid
//	CPBPkt cpB = this->cpRx_B->GetB();
//	if (cpB == NULL) {
//	        return (ERESULT_TYPE_SUCCESS);
//	};
//
//	#ifdef DEBUG
//	// cpB->CheckPkt();
//	assert (this->cpRx_B->IsBusy() == ERESULT_TYPE_YES);
//	#endif
//
//	// Get destination port
//	int nID = cpB->GetID();
//	int nPortSI = GetPortNum_SI(nID);
//
//	// Check Tx ready
//	EResultType eAcceptResult = this->cpTx_B[nPortSI]->GetAcceptResult();
//	if (eAcceptResult == ERESULT_TYPE_REJECT) {
//	        return (ERESULT_TYPE_SUCCESS);
//	};
//
//	#ifdef DEBUG
//	assert (eAcceptResult == ERESULT_TYPE_ACCEPT);
//	#endif
//
//	// Set ready
//	this->cpRx_B->SetAcceptResult(ERESULT_TYPE_ACCEPT);
//
//	// Debug
//	// printf("[Cycle %3ld: %s.Do_B_bwd] (%s) B handshake Rx_B.\n", nCycle,
// this->cName.c_str(), cpB->GetName().c_str());
//
//	// Decrease MO
//	this->nMO_AW_SI--;
//
//	// Debug
//	// cpB->Display();
//
//	return (ERESULT_TYPE_SUCCESS);
// };

// //--------------------------------------------------
// // Get R, B poppable from ROB_R_SI, B
// //--------------------------------------------------
// SPLinkedRUD CMIU::GetResp_poppable_ROB(EUDType eType) {
//
//	// Debug
//	assert (eType == EUD_TYPE_R or eType == EUD_TYPE_B);
//
//	if (eType == EUD_TYPE_R) {
//
//		SPLinkedRUD spScan = this->spRUDList_head;
//		while (spScan != NULL) {
//
//			#ifdef DEBUG
//			assert (spScan->upData != NULL);
//			assert (spScan->nLatency >= 0);
//			#endif
//
//			if (spScan->nID == nID and spScan->nMemCh = nMemCh) {
//				spScan->IsHead_Resp = ERESULT_TYPE_YES;
//				return (ERESULT_TYPE_SUCCESS);
//			};
//
//			spScan = spScan->spNext;
//		};
//
//
//	}
//	else if (eType == EUD_TYPE_B) {
//
//	}
//	else {
//		assert (0);
//	};
// };

// Set Cache interleaving mode
EResultType CMIU::Set_CACHE_MIU_MODE(EResultType IsCacheMode) {

  this->Is_CACHE_MIU_MODE = IsCacheMode;
  return (ERESULT_TYPE_SUCCESS);
};

// Debug
EResultType CMIU::CheckLink() {

  for (int i = 0; i < this->NUM_PORT_MI; i++) {

    assert(this->cpTx_AR[i] != NULL);
    assert(this->cpTx_AW[i] != NULL);
    assert(this->cpTx_W[i] != NULL);
    assert(this->cpRx_R[i] != NULL);
    assert(this->cpRx_B[i] != NULL);
    assert(this->cpTx_AR[i]->GetPair() != NULL);
    assert(this->cpTx_AW[i]->GetPair() != NULL);
    assert(this->cpTx_W[i]->GetPair() != NULL);
    assert(this->cpRx_R[i]->GetPair() != NULL);
    assert(this->cpRx_B[i]->GetPair() != NULL);
    assert(this->cpTx_AR[i]->GetTRxType() != this->cpTx_AR[i]->GetPair()->GetTRxType());
    assert(this->cpTx_AW[i]->GetTRxType() != this->cpTx_AW[i]->GetPair()->GetTRxType());
    assert(this->cpTx_W[i]->GetTRxType() != this->cpTx_W[i]->GetPair()->GetTRxType());
    assert(this->cpRx_R[i]->GetTRxType() != this->cpRx_R[i]->GetPair()->GetTRxType());
    assert(this->cpRx_B[i]->GetTRxType() != this->cpRx_B[i]->GetPair()->GetTRxType());
    assert(this->cpTx_AR[i]->GetPktType() == this->cpTx_AR[i]->GetPair()->GetPktType());
    assert(this->cpTx_AW[i]->GetPktType() == this->cpTx_AW[i]->GetPair()->GetPktType());
    assert(this->cpTx_W[i]->GetPktType() == this->cpTx_W[i]->GetPair()->GetPktType());
    assert(this->cpRx_R[i]->GetPktType() == this->cpRx_R[i]->GetPair()->GetPktType());
    assert(this->cpRx_B[i]->GetPktType() == this->cpRx_B[i]->GetPair()->GetPktType());
    assert(this->cpTx_AR[i]->GetPair()->GetPair() == this->cpTx_AR[i]);
    assert(this->cpTx_AW[i]->GetPair()->GetPair() == this->cpTx_AW[i]);
    assert(this->cpTx_W[i]->GetPair()->GetPair() == this->cpTx_W[i]);
    assert(this->cpRx_R[i]->GetPair()->GetPair() == this->cpRx_R[i]);
    assert(this->cpRx_B[i]->GetPair()->GetPair() == this->cpRx_B[i]);
  };
  return (ERESULT_TYPE_SUCCESS);
};

// Get AR MO count
int CMIU::GetMO_AR_SI(int nPort) {

#ifdef DEBUG
  assert(this->nMO_AR_SI[nPort] >= 0);
  assert(this->nMO_AR_SI[nPort] <= MAX_MO_COUNT);
#endif

  return (this->nMO_AR_SI[nPort]);
};

// Get AW MO count
int CMIU::GetMO_AW_SI(int nPort) {

#ifdef DEBUG
  assert(this->nMO_AW_SI[nPort] >= 0);
  assert(this->nMO_AW_SI[nPort] <= MAX_MO_COUNT);
#endif

  return (this->nMO_AW_SI[nPort]);
};

int CMIU::GetPortNum_SI(int nID) {

  int nPort = -1;

  if (this->BIT_PORT_SI == 0)
    nPort = nID & 0x0; // 0-bit.       1 port
  else if (this->BIT_PORT_SI == 1)
    nPort = nID & 0x1; // 1-bit.  Upto 2 ports
  else if (this->BIT_PORT_SI == 2)
    nPort = nID & 0x3; // 2-bits. Upto 4 ports
  else if (this->BIT_PORT_SI == 3)
    nPort = nID & 0x7;
  else if (this->BIT_PORT_SI == 4)
    nPort = nID & 0xF;
  else
    assert(0);

#ifdef DEBUG
  assert(nPort >= 0);
#endif

  return (nPort);
};

// Update state
EResultType CMIU::UpdateState(int64_t nCycle) {

#ifdef DEBUG

#endif

  // Port MI
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpTx_AR[i]->UpdateState();
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpTx_AW[i]->UpdateState();
  };
  // for (int i=0; i < this->NUM_PORT_MI; i++) { this->cpTx_W[i]->UpdateState();
  // };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpRx_R[i]->UpdateState();
  };
  for (int i = 0; i < this->NUM_PORT_MI; i++) {
    this->cpRx_B[i]->UpdateState();
  };

  // Port SI
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpRx_AR[i]->UpdateState();
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpRx_AW[i]->UpdateState();
  };
  // for (int i=0; i < this->NUM_PORT_SI; i++) { this->cpRx_W[i]
  // ->UpdateState(); };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpTx_R[i]->UpdateState();
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpTx_B[i]->UpdateState();
  };

  // FIFO
  this->cpFIFO_AW->UpdateState();

  // ROB SI
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpROB_AR_SI[i]->UpdateState();
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpROB_R_SI[i]->UpdateState();
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpROB_AW_SI[i]->UpdateState();
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    this->cpROB_B_SI[i]->UpdateState();
  };

#ifdef DEBUG
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nMax_alloc_cycles_ROB_AR_SI[i] >= STARVATION_CYCLE) {
      printf("[Cycle %3ld: %s.UpdateState] ROB_AR_SI SI[%d] starvation occurs "
             "(%d cycles).\n",
             nCycle, this->cName.c_str(), i, this->nMax_alloc_cycles_ROB_AR_SI[i]);
      assert(0);
    };
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nMax_alloc_cycles_ROB_R_SI[i] >= STARVATION_CYCLE) {
      printf("[Cycle %3ld: %s.UpdateState] ROB_R_SI SI[%d] starvation occurs "
             "(%d cycles).\n",
             nCycle, this->cName.c_str(), i, this->nMax_alloc_cycles_ROB_R_SI[i]);
      assert(0);
    };
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nMax_alloc_cycles_ROB_AW_SI[i] >= STARVATION_CYCLE) {
      printf("[Cycle %3ld: %s.UpdateState] ROB_AW_SI SI[%d] starvation occurs "
             "(%d cycles).\n",
             nCycle, this->cName.c_str(), i, this->nMax_alloc_cycles_ROB_AW_SI[i]);
      assert(0);
    };
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nMax_alloc_cycles_ROB_B_SI[i] >= STARVATION_CYCLE) {
      printf("[Cycle %3ld: %s.UpdateState] ROB_B_SI SI[%d] starvation occurs "
             "(%d cycles).\n",
             nCycle, this->cName.c_str(), i, this->nMax_alloc_cycles_ROB_B_SI[i]);
      assert(0);
    };
  };
#endif

#ifdef DEBUG
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    assert(this->cpROB_AR_SI[i]->GetCurNum() <= this->cpROB_AR_SI[i]->GetMaxNum());
    assert(this->cpROB_R_SI[i]->GetCurNum() <= this->cpROB_R_SI[i]->GetMaxNum());

    assert(this->nMO_AR_SI[i] <= MAX_MO_COUNT);
    assert(this->nMO_AR_SI[i] >= 0);
    assert(this->nMO_AW_SI[i] <= MAX_MO_COUNT);
    assert(this->nMO_AW_SI[i] >= 0);
  };
#endif

#ifdef STAT_DETAIL
  // Occupancy
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->cpROB_AR_SI[i]->GetCurNum() > this->nMax_Occupancy_ROB_AR_SI[i]) {

      assert(this->cpROB_AR_SI[i]->GetCurNum() == 1 + this->nMax_Occupancy_ROB_AR_SI[i]);
      this->nMax_Occupancy_ROB_AR_SI[i]++;
    };
    if (this->cpROB_R_SI[i]->GetCurNum() > this->nMax_Occupancy_ROB_R_SI[i]) {

      assert(this->cpROB_R_SI[i]->GetCurNum() == 1 + this->nMax_Occupancy_ROB_R_SI[i]);
      this->nMax_Occupancy_ROB_R_SI[i]++;
    };

    this->nTotal_Occupancy_ROB_AR_SI[i] += this->cpROB_AR_SI[i]->GetCurNum();
    this->nTotal_Occupancy_ROB_R_SI[i] += this->cpROB_R_SI[i]->GetCurNum();
  };

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->cpROB_AW_SI[i]->GetCurNum() > this->nMax_Occupancy_ROB_AW_SI[i]) {

      assert(this->cpROB_AW_SI[i]->GetCurNum() == 1 + this->nMax_Occupancy_ROB_AW_SI[i]);
      this->nMax_Occupancy_ROB_AW_SI[i]++;
    };
    if (this->cpROB_B_SI[i]->GetCurNum() > this->nMax_Occupancy_ROB_B_SI[i]) {

      assert(this->cpROB_B_SI[i]->GetCurNum() == 1 + this->nMax_Occupancy_ROB_B_SI[i]);
      this->nMax_Occupancy_ROB_B_SI[i]++;
    };

    this->nTotal_Occupancy_ROB_AW_SI[i] += this->cpROB_AW_SI[i]->GetCurNum();
    this->nTotal_Occupancy_ROB_B_SI[i] += this->cpROB_B_SI[i]->GetCurNum();
  };

#endif

  return (ERESULT_TYPE_SUCCESS);
};

// Stat
EResultType CMIU::PrintStat(int64_t nCycle, FILE *fp) {

  printf("--------------------------------------------------------\n");
  printf("\t Name : %s\n", this->cName.c_str());
  printf("--------------------------------------------------------\n");

  printf("\t Number SI ports                     : %d\n", this->NUM_PORT_SI);
  printf("\t Number MI ports                     : %d\n\n", this->NUM_PORT_MI);

  // printf("\t Max allowed AR MO                : %d\n",	 MAX_MO_COUNT);
  // printf("\t Max allowed AW MO                : %d\n\n",MAX_MO_COUNT);

  // Number
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nAR_SI[i] > 0) {
      printf("\t Number AR SI[%d]                     : %d\n", i, this->nAR_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nAW_SI[i] > 0) {
      printf("\t Number AW SI[%d]                     : %d\n", i, this->nAW_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nR_SI[i] > 0) {
      printf("\t Number R SI[%d]                      : %d\n", i, this->nR_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nB_SI[i] > 0) {
      printf("\t Number B SI[%d]                      : %d\n", i, this->nB_SI[i]);
    };
  };
  printf("\n");

  // Alloc cycles
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nAR_SI[i] > 0) {
      printf("\t Max alloc cycles ROB_AR_SI[%d]       : %d\n", i, this->nMax_alloc_cycles_ROB_AR_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nR_SI[i] > 0) {
      printf("\t Max alloc cycles ROB_R_SI[%d]        : %d\n", i, this->nMax_alloc_cycles_ROB_R_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nAR_SI[i] > 0) {
      printf("\t Avg alloc cycles ROB_AR_SI[%d]       : %1.1f\n", i,
             (float)this->nTotal_alloc_cycles_ROB_AR_SI[i] / this->nAR_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nR_SI[i] > 0) {
      printf("\t Avg alloc cycles ROB_R_SI[%d]        : %1.1f\n", i,
             (float)this->nTotal_alloc_cycles_ROB_R_SI[i] / this->nR_SI[i]);
    };
  };
  printf("\n");

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nAW_SI[i] > 0) {
      printf("\t Max alloc cycles ROB_AW_SI[%d]       : %d\n", i, this->nMax_alloc_cycles_ROB_AW_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nB_SI[i] > 0) {
      printf("\t Max alloc cycles ROB_B_SI[%d]        : %d\n", i, this->nMax_alloc_cycles_ROB_B_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nAW_SI[i] > 0) {
      printf("\t Avg alloc cycles ROB_AW_SI[%d]       : %1.1f\n", i,
             (float)this->nTotal_alloc_cycles_ROB_AW_SI[i] / this->nAW_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nB_SI[i] > 0) {
      printf("\t Avg alloc cycles ROB_B_SI[%d]        : %1.1f\n", i,
             (float)this->nTotal_alloc_cycles_ROB_B_SI[i] / this->nB_SI[i]);
    };
  };
  printf("\n");

  // Occupancy
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nAR_SI[i] > 0) {
      printf("\t Max occupancy ROB_AR_SI[%d]          : %d\n", i, this->nMax_Occupancy_ROB_AR_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nR_SI[i] > 0) {
      printf("\t Max occupancy ROB_R_SI[%d]           : %d\n", i, this->nMax_Occupancy_ROB_R_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nAR_SI[i] > 0) {
      printf("\t Avg occupancy ROB_AR_SI[%d]          : %1.1f\n", i,
             (float)this->nTotal_Occupancy_ROB_AR_SI[i] / nCycle);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nR_SI[i] > 0) {
      printf("\t Avg occupancy ROB_R_SI[%d]           : %1.1f\n", i,
             (float)this->nTotal_Occupancy_ROB_R_SI[i] / nCycle);
    };
  };
  printf("\n");

  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nAW_SI[i] > 0) {
      printf("\t Max occupancy ROB_AW_SI[%d]          : %d\n", i, this->nMax_Occupancy_ROB_AW_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nB_SI[i] > 0) {
      printf("\t Max occupancy ROB_B_SI[%d]           : %d\n", i, this->nMax_Occupancy_ROB_B_SI[i]);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nAW_SI[i] > 0) {
      printf("\t Avg occupancy ROB_AW_SI[%d]          : %1.1f\n", i,
             (float)this->nTotal_Occupancy_ROB_AW_SI[i] / nCycle);
    };
  };
  for (int i = 0; i < this->NUM_PORT_SI; i++) {
    if (this->nB_SI[i] > 0) {
      printf("\t Avg occupancy ROB_B_SI[%d]           : %1.1f\n", i,
             (float)this->nTotal_Occupancy_ROB_B_SI[i] / nCycle);
    };
  };

  return (ERESULT_TYPE_SUCCESS);
};
