//----------------------------------------------------
// Filename     : main_pin.cpp
// Version	: 0.75
// DATE         : 30 Jun 2018
// Contact	: JaeYoung.Hur@gmail.com
// Description  : main
//----------------------------------------------------
// PIN trace
//----------------------------------------------------
// MST1: read and write
//----------------------------------------------------
#include <assert.h>
#include <iostream>
#include <stdlib.h>

#include "CBUS.h"
#include "CMMU.h"
#include "CMST.h"
#include "CSLV.h"

int main() {

  int64_t nCycle = 1;

  // 1. Generate and initialize
  CPMST cpMST0 = new CMST("MST0");
  CPMST cpMST1 = new CMST("MST1");
  CPMST cpMST2 = new CMST("MST2");
  CPMST cpMST3 = new CMST("MST3");
  CPMMU cpMMU0 = new CMMU("MMU0");
  CPMMU cpMMU1 = new CMMU("MMU1");
  CPMMU cpMMU2 = new CMMU("MMU2");
  CPMMU cpMMU3 = new CMMU("MMU3");
  CPBUS cpBUS = new CBUS("BUS", 4);
  CPSLV cpSLV = new CSLV("SLV", 0);

  // 2. Link TRx topology
  // MST0 and MMU0
  cpMST0->cpTx_AR->SetPair(cpMMU0->cpRx_AR);
  cpMST0->cpTx_AW->SetPair(cpMMU0->cpRx_AW);
  cpMST0->cpTx_W->SetPair(cpMMU0->cpRx_W);
  cpMMU0->cpTx_R->SetPair(cpMST0->cpRx_R);
  cpMMU0->cpTx_B->SetPair(cpMST0->cpRx_B);

  // MST1 and MMU1
  cpMST1->cpTx_AR->SetPair(cpMMU1->cpRx_AR);
  cpMST1->cpTx_AW->SetPair(cpMMU1->cpRx_AW);
  cpMST1->cpTx_W->SetPair(cpMMU1->cpRx_W);
  cpMMU1->cpTx_R->SetPair(cpMST1->cpRx_R);
  cpMMU1->cpTx_B->SetPair(cpMST1->cpRx_B);

  // MST2 and MMU2
  cpMST2->cpTx_AR->SetPair(cpMMU2->cpRx_AR);
  cpMST2->cpTx_AW->SetPair(cpMMU2->cpRx_AW);
  cpMST2->cpTx_W->SetPair(cpMMU2->cpRx_W);
  cpMMU2->cpTx_R->SetPair(cpMST2->cpRx_R);
  cpMMU2->cpTx_B->SetPair(cpMST2->cpRx_B);

  // MST3 and MMU3
  cpMST3->cpTx_AR->SetPair(cpMMU3->cpRx_AR);
  cpMST3->cpTx_AW->SetPair(cpMMU3->cpRx_AW);
  cpMST3->cpTx_W->SetPair(cpMMU3->cpRx_W);
  cpMMU3->cpTx_R->SetPair(cpMST3->cpRx_R);
  cpMMU3->cpTx_B->SetPair(cpMST3->cpRx_B);

  // MMU0 and BUS
  cpMMU0->cpTx_AR->SetPair(cpBUS->cpRx_AR[0]);
  cpMMU0->cpTx_AW->SetPair(cpBUS->cpRx_AW[0]);
  cpMMU0->cpTx_W->SetPair(cpBUS->cpRx_W[0]);
  cpBUS->cpTx_R[0]->SetPair(cpMMU0->cpRx_R);
  cpBUS->cpTx_B[0]->SetPair(cpMMU0->cpRx_B);

  // MMU1 and BUS
  cpMMU1->cpTx_AR->SetPair(cpBUS->cpRx_AR[1]);
  cpMMU1->cpTx_AW->SetPair(cpBUS->cpRx_AW[1]);
  cpMMU1->cpTx_W->SetPair(cpBUS->cpRx_W[1]);
  cpBUS->cpTx_R[1]->SetPair(cpMMU1->cpRx_R);
  cpBUS->cpTx_B[1]->SetPair(cpMMU1->cpRx_B);

  // MMU2 and BUS
  cpMMU2->cpTx_AR->SetPair(cpBUS->cpRx_AR[2]);
  cpMMU2->cpTx_AW->SetPair(cpBUS->cpRx_AW[2]);
  cpMMU2->cpTx_W->SetPair(cpBUS->cpRx_W[2]);
  cpBUS->cpTx_R[2]->SetPair(cpMMU2->cpRx_R);
  cpBUS->cpTx_B[2]->SetPair(cpMMU2->cpRx_B);

  // MMU3 and BUS
  cpMMU3->cpTx_AR->SetPair(cpBUS->cpRx_AR[3]);
  cpMMU3->cpTx_AW->SetPair(cpBUS->cpRx_AW[3]);
  cpMMU3->cpTx_W->SetPair(cpBUS->cpRx_W[3]);
  cpBUS->cpTx_R[3]->SetPair(cpMMU3->cpRx_R);
  cpBUS->cpTx_B[3]->SetPair(cpMMU3->cpRx_B);

  // BUS and SLV
  cpBUS->cpTx_AR->SetPair(cpSLV->cpRx_AR);
  cpBUS->cpTx_AW->SetPair(cpSLV->cpRx_AW);
  cpBUS->cpTx_W->SetPair(cpSLV->cpRx_W);
  cpSLV->cpTx_R->SetPair(cpBUS->cpRx_R);
  cpSLV->cpTx_B->SetPair(cpBUS->cpRx_B);

  // Debug
  cpMST0->CheckLink();
  cpMST1->CheckLink();
  cpMST2->CheckLink();
  cpMST3->CheckLink();
  cpMMU0->CheckLink();
  cpMMU1->CheckLink();
  cpMMU2->CheckLink();
  cpMMU3->CheckLink();
  cpBUS->CheckLink();
  cpSLV->CheckLink();

  // Number of transactions
  int NUM = IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE * BYTE_PER_PIXEL / MAX_TRANS_SIZE;

  // Trans num
  cpMST0->Set_nAR_GEN_NUM(0);
  cpMST0->Set_nAW_GEN_NUM(0);

  cpMST1->Set_nAR_GEN_NUM(10); // How many AR
  cpMST1->Set_nAW_GEN_NUM(0);

  cpMST2->Set_nAR_GEN_NUM(0);
  cpMST2->Set_nAW_GEN_NUM(0);

  cpMST3->Set_nAR_GEN_NUM(0);
  cpMST3->Set_nAW_GEN_NUM(0);

  // Start addr
  cpMST0->Set_nAR_START_ADDR(0);
  cpMST0->Set_nAW_START_ADDR(0x10000000);

  cpMST1->Set_nAR_START_ADDR(0x20000800);
  cpMST1->Set_nAW_START_ADDR(0);

  cpMST2->Set_nAR_START_ADDR(0x30001000);
  cpMST2->Set_nAW_START_ADDR(0);

  cpMST3->Set_nAR_START_ADDR(0x40001800);
  cpMST3->Set_nAW_START_ADDR(0);

  // Config operation
  // RASTER_SCAN, ROTATION
  cpMST0->Set_AR_Operation("RASTER_SCAN");
  cpMST0->Set_AW_Operation("RASTER_SCAN");

  cpMST1->Set_AR_Operation("ROTATION");
  cpMST1->Set_AW_Operation("RASTER_SCAN");

  cpMST2->Set_AR_Operation("RASTER_SCAN");
  cpMST2->Set_AW_Operation("RASTER_SCAN");

  cpMST3->Set_AR_Operation("RASTER_SCAN");
  cpMST3->Set_AW_Operation("RASTER_SCAN");

  // PIN trace
  // char const *FileName = "PIN_trace_test.trc";
  // char const *FileName = "./radix.log";
  char const *FileName = "./test.trc";

  FILE *PIN_fp = NULL;
  // char cLine[BUFSIZ]; // BUFSIZ (8192) is defined in stdio.h

  PIN_fp = fopen(FileName, "r");
  if (!PIN_fp) {
    printf("Couldn't open file %s for reading.\n", FileName);
    return (ERESULT_TYPE_FAIL);
  }
  // printf("Opened file %s for reading.\n\n", FileName);

  // Simulate
  while (1) {

    // 3. Reset
    if (nCycle == 1) {
      cpMST0->Reset();
      cpMST1->Reset();
      cpMST2->Reset();
      cpMST3->Reset();
      cpMMU0->Reset();
      cpMMU1->Reset();
      cpMMU2->Reset();
      cpMMU3->Reset();
      cpBUS->Reset();
      cpSLV->Reset();

      // Master geneates transactions

      // #ifdef AR_ADDR_GEN_TEST
      // cpMST0->LoadTransfer_AR_Test(nCycle);
      // //cpMST1->LoadTransfer_AR_Test(nCycle);
      // cpMST2->LoadTransfer_AR_Test(nCycle);
      // cpMST3->LoadTransfer_AR_Test(nCycle);
      // #endif

      // #ifdef AW_ADDR_GEN_TEST
      // cpMST0->LoadTransfer_AW_Test(nCycle);
      // cpMST1->LoadTransfer_AW_Test(nCycle);
      // cpMST2->LoadTransfer_AW_Test(nCycle);
      // cpMST3->LoadTransfer_AW_Test(nCycle);
      // #endif
    };

    // 4. Start simulation

    // Master generate transaction

    // #ifdef AR_LIAM
    // cpMST0->LoadTransfer_AR(nCycle, "LIAM");
    // #endif
    // #ifdef AR_BFAM
    // cpMST0->LoadTransfer_AR(nCycle, "BFAM");
    // #endif
    // #ifdef AR_TILE
    // cpMST0->LoadTransfer_AR(nCycle, "TILE");
    // #endif

    // #ifdef AW_LIAM
    // cpMST0->LoadTransfer_AW(nCycle, "LIAM");
    // #endif
    // #ifdef AW_BFAM
    // cpMST0->LoadTransfer_AW(nCycle, "BFAM");
    // #endif
    // #ifdef AW_TILE
    // cpMST0->LoadTransfer_AW(nCycle, "TILE");
    // #endif

#ifdef AR_LIAM
// cpMST1->LoadTransfer_AR(nCycle, "LIAM");
#endif
#ifdef AR_BFAM
// cpMST1->LoadTransfer_AR(nCycle, "BFAM");
#endif
#ifdef AR_TILE
// cpMST1->LoadTransfer_AR(nCycle, "TILE");
#endif

    // #ifdef AW_LIAM
    // cpMST1->LoadTransfer_AW(nCycle, "LIAM");
    // #endif
    // #ifdef AW_BFAM
    // cpMST1->LoadTransfer_AW(nCycle, "BFAM");
    // #endif
    // #ifdef AW_TILE
    // cpMST1->LoadTransfer_AR(nCycle, "TILE");
    // #endif

    // #ifdef AR_LIAM
    // cpMST2->LoadTransfer_AR(nCycle, "LIAM");
    // #endif
    // #ifdef AR_BFAM
    // cpMST2->LoadTransfer_AR(nCycle, "BFAM");
    // #endif
    // #ifdef AR_TILE
    // cpMST2->LoadTransfer_AR(nCycle, "TILE");
    // #endif
    //
    // #ifdef AW_LIAM
    // cpMST2->LoadTransfer_AW(nCycle, "LIAM");
    // #endif
    // #ifdef AW_BFAM
    // cpMST2->LoadTransfer_AW(nCycle, "BFAM");
    // #endif
    // #ifdef AW_TILE
    // cpMST2->LoadTransfer_AW(nCycle, "TILE");
    // #endif
    //
    // #ifdef AR_LIAM
    // cpMST3->LoadTransfer_AR(nCycle, "LIAM");
    // #endif
    // #ifdef AR_BFAM
    // cpMST3->LoadTransfer_AR(nCycle, "BFAM");
    // #endif
    // #ifdef AR_TILE
    // cpMST3->LoadTransfer_AR(nCycle, "TILE");
    // #endif
    //
    // #ifdef AW_LIAM
    // cpMST3->LoadTransfer_AW(nCycle, "LIAM");
    // #endif
    // #ifdef AW_BFAM
    // cpMST3->LoadTransfer_AW(nCycle, "BFAM");
    // #endif
    // #ifdef AW_TILE
    // cpMST3->LoadTransfer_AW(nCycle, "TILE");
    // #endif

#ifdef AR_ADDR_GEN_TRACE
// cpMST1->LoadTransfer_AR_AXI_Trace(nCycle);
// cpMST1->LoadTransfer_AR_PIN_Trace(nCycle, PIN_fp);
#endif

#ifdef AW_ADDR_GEN_TRACE
// cpMST1->LoadTransfer_AW_AXI_Trace(nCycle);
// cpMST1->LoadTransfer_AW_PIN_Trace(nCycle, PIN_fp);
#endif

#ifdef Ax_ADDR_GEN_TRACE
    cpMST1->LoadTransfer_Ax_PIN_Trace(nCycle, PIN_fp);
#endif

    //---------------------------------
    // Propagate valid
    //---------------------------------

    //---------------------------------
    // AR, AW, W VALID
    //---------------------------------

    // cpMST0->Do_AR_fwd(nCycle);
    // cpMST0->Do_AW_fwd(nCycle);
    // cpMST0->Do_W_fwd(nCycle);

    cpMST1->Do_AR_fwd(nCycle);
    cpMST1->Do_AW_fwd(nCycle);
    // cpMST1->Do_W_fwd(nCycle);

    // cpMST2->Do_AR_fwd(nCycle);
    // cpMST2->Do_AW_fwd(nCycle);
    // cpMST2->Do_W_fwd(nCycle);

    // cpMST3->Do_AR_fwd(nCycle);
    // cpMST3->Do_AW_fwd(nCycle);
    // cpMST3->Do_W_fwd(nCycle);

#ifdef MMU_OFF
// cpMMU0->Do_AR_fwd_MMU_OFF(nCycle);
// cpMMU0->Do_AW_fwd_MMU_OFF(nCycle);  // AWVALID
// cpMMU0->Do_W_fwd_MMU_OFF(nCycle);   // WVALID
#endif
#ifdef MMU_ON
// cpMMU0->Do_AR_fwd_SI(nCycle);
// cpMMU0->Do_AR_fwd_MI(nCycle);
// cpMMU0->Do_AW_fwd(nCycle);
// cpMMU0->Do_W_fwd_SI(nCycle);
// cpMMU0->Do_W_fwd_MI(nCycle);
#endif

#ifdef MMU_OFF
    cpMMU1->Do_AR_fwd_MMU_OFF(nCycle);
    cpMMU1->Do_AW_fwd_MMU_OFF(nCycle);
// cpMMU1->Do_W_fwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
    cpMMU1->Do_AR_fwd_SI(nCycle);
    cpMMU1->Do_AR_fwd_MI(nCycle);
    cpMMU1->Do_AW_fwd(nCycle);
// cpMMU1->Do_W_fwd_SI(nCycle);
// cpMMU1->Do_W_fwd_MI(nCycle);
#endif

#ifdef MMU_OFF
// cpMMU2->Do_AR_fwd_MMU_OFF(nCycle);
// cpMMU2->Do_AW_fwd_MMU_OFF(nCycle);
// cpMMU2->Do_W_fwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
// cpMMU2->Do_AR_fwd_SI(nCycle);
// cpMMU2->Do_AR_fwd_MI(nCycle);
// cpMMU2->Do_AW_fwd(nCycle);
// cpMMU2->Do_W_fwd_SI(nCycle);
// cpMMU2->Do_W_fwd_MI(nCycle);
#endif

#ifdef MMU_OFF
// cpMMU3->Do_AR_fwd_MMU_OFF(nCycle);
// cpMMU3->Do_AW_fwd_MMU_OFF(nCycle);
// cpMMU3->Do_W_fwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
// cpMMU3->Do_AR_fwd_SI(nCycle);
// cpMMU3->Do_AR_fwd_MI(nCycle);
// cpMMU3->Do_AW_fwd(nCycle);
// cpMMU3->Do_W_fwd_SI(nCycle);
// cpMMU3->Do_W_fwd_MI(nCycle);
#endif

    cpBUS->Do_AR_fwd(nCycle);
    cpBUS->Do_AW_fwd(nCycle);
    // cpBUS->Do_W_fwd(nCycle);

#ifdef IDEAL_MEMORY
    cpSLV->Do_AR_fwd(nCycle);
#endif
#ifdef MEMORY_CONTROLLER
    cpSLV->Do_AR_fwd_MC_Frontend(nCycle);
    cpSLV->Do_AR_fwd_MC_Backend_Response(nCycle);
#endif

#ifdef IDEAL_MEMORY
    cpSLV->Do_AW_fwd(nCycle);
// cpSLV->Do_W_fwd(nCycle);
#endif
#ifdef MEMORY_CONTROLLER
    cpSLV->Do_AW_fwd_MC_Frontend(nCycle);
    // cpSLV->Do_W_fwd_MC_Frontend(nCycle);
    cpSLV->Do_AW_fwd_MC_Backend_Response(nCycle);
#endif
#ifdef MEMORY_CONTROLLER
    cpSLV->Do_Ax_fwd_MC_Backend_Request(nCycle);
#endif

    //---------------------------------
    // R, B VALID
    //---------------------------------

    cpSLV->Do_R_fwd(nCycle);
    cpSLV->Do_B_fwd(nCycle);

    cpBUS->Do_R_fwd(nCycle);
    cpBUS->Do_B_fwd(nCycle);

#ifdef MMU_OFF
// cpMMU0->Do_R_fwd_MMU_OFF(nCycle);
// cpMMU0->Do_B_fwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
// cpMMU0->Do_R_fwd(nCycle);
// cpMMU0->Do_B_fwd(nCycle);
#endif

#ifdef MMU_OFF
    cpMMU1->Do_R_fwd_MMU_OFF(nCycle);
    cpMMU1->Do_B_fwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
    cpMMU1->Do_R_fwd(nCycle);
    cpMMU1->Do_B_fwd(nCycle);
#endif

#ifdef MMU_OFF
// cpMMU2->Do_R_fwd_MMU_OFF(nCycle);
// cpMMU2->Do_B_fwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
// cpMMU2->Do_R_fwd(nCycle);
// cpMMU2->Do_B_fwd(nCycle);
#endif

#ifdef MMU_OFF
// cpMMU3->Do_R_fwd_MMU_OFF(nCycle);
// cpMMU3->Do_B_fwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
// cpMMU3->Do_R_fwd(nCycle);
// cpMMU3->Do_B_fwd(nCycle);
#endif

    // cpMST0->Do_R_fwd(nCycle);
    // cpMST0->Do_B_fwd(nCycle);

    cpMST1->Do_R_fwd(nCycle);
    cpMST1->Do_B_fwd(nCycle);

    // cpMST2->Do_R_fwd(nCycle);
    // cpMST2->Do_B_fwd(nCycle);

    // cpMST3->Do_R_fwd(nCycle);
    // cpMST3->Do_B_fwd(nCycle);

    //---------------------------------
    // Propagate ready
    //---------------------------------

    //---------------------------------
    // AR, AW, W READY
    //---------------------------------

    cpSLV->Do_AR_bwd(nCycle);
    cpSLV->Do_AW_bwd(nCycle);
    // cpSLV->Do_W_bwd(nCycle);

    cpBUS->Do_AR_bwd(nCycle);
    cpBUS->Do_AW_bwd(nCycle);
    // cpBUS->Do_W_bwd(nCycle);

#ifdef MMU_OFF
// cpMMU0->Do_AR_bwd_MMU_OFF(nCycle);
// cpMMU0->Do_AW_bwd_MMU_OFF(nCycle);
// cpMMU0->Do_W_bwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
// cpMMU0->Do_AR_bwd(nCycle);
// cpMMU0->Do_AW_bwd(nCycle);
// cpMMU0->Do_W_bwd(nCycle);
#endif

#ifdef MMU_OFF
    cpMMU1->Do_AR_bwd_MMU_OFF(nCycle);
    cpMMU1->Do_AW_bwd_MMU_OFF(nCycle);
// cpMMU1->Do_W_bwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
    cpMMU1->Do_AR_bwd(nCycle);
    cpMMU1->Do_AW_bwd(nCycle);
// cpMMU1->Do_W_bwd(nCycle);
#endif

#ifdef MMU_OFF
// cpMMU2->Do_AR_bwd_MMU_OFF(nCycle);
// cpMMU2->Do_AW_bwd_MMU_OFF(nCycle);
// cpMMU2->Do_W_bwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
// cpMMU2->Do_AR_bwd(nCycle);
// cpMMU2->Do_AW_bwd(nCycle);
// cpMMU2->Do_W_bwd(nCycle);
#endif

#ifdef MMU_OFF
// cpMMU3->Do_AR_bwd_MMU_OFF(nCycle);
// cpMMU3->Do_AW_bwd_MMU_OFF(nCycle);
// cpMMU3->Do_W_bwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
// cpMMU3->Do_AR_bwd(nCycle);
// cpMMU3->Do_AW_bwd(nCycle);
// cpMMU3->Do_W_bwd(nCycle);
#endif

    // cpMST0->Do_AR_bwd(nCycle);
    // cpMST0->Do_AW_bwd(nCycle);
    // cpMST0->Do_W_bwd(nCycle);

    cpMST1->Do_AR_bwd(nCycle);
    cpMST1->Do_AW_bwd(nCycle);
    // cpMST1->Do_W_bwd(nCycle);

    // cpMST2->Do_AR_bwd(nCycle);
    // cpMST2->Do_AW_bwd(nCycle);
    // cpMST2->Do_W_bwd(nCycle);

    // cpMST3->Do_AR_bwd(nCycle);
    // cpMST3->Do_AW_bwd(nCycle);
    // cpMST3->Do_W_bwd(nCycle);

    //---------------------------------
    // R, B READY
    //---------------------------------

    // cpMST0->Do_R_bwd(nCycle);
    // cpMST0->Do_B_bwd(nCycle);

    cpMST1->Do_R_bwd(nCycle);
    cpMST1->Do_B_bwd(nCycle);

    // cpMST2->Do_R_bwd(nCycle);
    // cpMST2->Do_B_bwd(nCycle);

    // cpMST3->Do_R_bwd(nCycle);
    // cpMST3->Do_B_bwd(nCycle);

#ifdef MMU_OFF
// cpMMU0->Do_R_bwd_MMU_OFF(nCycle);
// cpMMU0->Do_B_bwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
// cpMMU0->Do_R_bwd(nCycle);
// cpMMU0->Do_B_bwd(nCycle);
#endif

#ifdef MMU_OFF
    cpMMU1->Do_R_bwd_MMU_OFF(nCycle);
    cpMMU1->Do_B_bwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
    cpMMU1->Do_R_bwd(nCycle);
    cpMMU1->Do_B_bwd(nCycle);
#endif

#ifdef MMU_OFF
// cpMMU2->Do_R_bwd_MMU_OFF(nCycle);
// cpMMU2->Do_B_bwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
// cpMMU2->Do_R_bwd(nCycle);
// cpMMU2->Do_B_bwd(nCycle);
#endif

#ifdef MMU_OFF
// cpMMU3->Do_R_bwd_MMU_OFF(nCycle);
// cpMMU3->Do_B_bwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
// cpMMU3->Do_R_bwd(nCycle);
// cpMMU3->Do_B_bwd(nCycle);
#endif

    cpBUS->Do_R_bwd(nCycle);
    cpBUS->Do_B_bwd(nCycle);

    cpSLV->Do_R_bwd(nCycle);
    cpSLV->Do_B_bwd(nCycle);

    //---------------------------------
    // 5. Update state
    //---------------------------------
    // cpMST0->UpdateState(nCycle);
    cpMST1->UpdateState(nCycle);
    // cpMST2->UpdateState(nCycle);
    // cpMST3->UpdateState(nCycle);
    // cpMMU0->UpdateState(nCycle);
    cpMMU1->UpdateState(nCycle);
    // cpMMU2->UpdateState(nCycle);
    // cpMMU3->UpdateState(nCycle);
    cpBUS->UpdateState(nCycle);
    cpSLV->UpdateState(nCycle);

    // 6. Check simulation finish
    // if (cpMST0->IsAllTransFinished() == ERESULT_TYPE_YES) {
    // if (cpMST0->IsARTransFinished()  == ERESULT_TYPE_YES) {
    // if (cpMST0->IsAWTransFinished()  == ERESULT_TYPE_YES) {

    // if (cpMST1->IsAllTransFinished() == ERESULT_TYPE_YES) {
    // if (cpMST1->IsARTransFinished()  == ERESULT_TYPE_YES) {
    // if (cpMST1->IsAWTransFinished()  == ERESULT_TYPE_YES) {

    // if (cpMST0->IsAllTransFinished() == ERESULT_TYPE_YES and
    // cpMST1->IsAllTransFinished() == ERESULT_TYPE_YES) { if
    // (cpMST0->IsARTransFinished()  == ERESULT_TYPE_YES and
    // cpMST1->IsARTransFinished()  == ERESULT_TYPE_YES) { if
    // (cpMST0->IsAWTransFinished()  == ERESULT_TYPE_YES and
    // cpMST1->IsAWTransFinished()  == ERESULT_TYPE_YES) { if
    // (cpMST0->IsAWTransFinished()  == ERESULT_TYPE_YES and
    // cpMST1->IsARTransFinished()  == ERESULT_TYPE_YES) {

    // if (cpMST0->IsAWTransFinished() == ERESULT_TYPE_YES and
    // cpMST3->IsARTransFinished() == ERESULT_TYPE_YES) {

    // if (cpMST0->IsAllTransFinished() == ERESULT_TYPE_YES and
    // cpMST1->IsAllTransFinished() == ERESULT_TYPE_YES and
    // cpMST2->IsAllTransFinished() == ERESULT_TYPE_YES) { if
    // (cpMST0->IsARTransFinished()  == ERESULT_TYPE_YES and
    // cpMST1->IsARTransFinished()  == ERESULT_TYPE_YES and
    // cpMST2->IsARTransFinished()  == ERESULT_TYPE_YES) { if
    // (cpMST0->IsAWTransFinished()  == ERESULT_TYPE_YES and
    // cpMST1->IsAWTransFinished()  == ERESULT_TYPE_YES and
    // cpMST2->IsAWTransFinished()  == ERESULT_TYPE_YES) { if
    // (cpMST0->IsARTransFinished()  == ERESULT_TYPE_YES and
    // cpMST1->IsARTransFinished()  == ERESULT_TYPE_YES and
    // cpMST2->IsAWTransFinished()  == ERESULT_TYPE_YES) {

    // if (cpMST0->IsAllTransFinished() == ERESULT_TYPE_YES and
    // cpMST1->IsAllTransFinished() == ERESULT_TYPE_YES and
    // cpMST2->IsAllTransFinished() == ERESULT_TYPE_YES and
    // cpMST3->IsAllTransFinished() == ERESULT_TYPE_YES) { if
    // (cpMST0->IsARTransFinished()  == ERESULT_TYPE_YES and
    // cpMST1->IsARTransFinished()  == ERESULT_TYPE_YES and
    // cpMST2->IsARTransFinished()  == ERESULT_TYPE_YES and
    // cpMST3->IsARTransFinished()  == ERESULT_TYPE_YES) { if
    // (cpMST0->IsAWTransFinished()  == ERESULT_TYPE_YES and
    // cpMST1->IsAWTransFinished()  == ERESULT_TYPE_YES and
    // cpMST2->IsAWTransFinished()  == ERESULT_TYPE_YES and
    // cpMST3->IsAWTransFinished()  == ERESULT_TYPE_YES) {

    //--------------------------------
    // Experiment
    //--------------------------------
    // if (cpMST0->IsAWTransFinished()  == ERESULT_TYPE_YES and
    // cpMST1->IsARTransFinished()  == ERESULT_TYPE_YES) { if
    // (cpMST0->IsAWTransFinished()  == ERESULT_TYPE_YES and
    // cpMST1->IsARTransFinished()  == ERESULT_TYPE_YES and
    // cpMST2->IsARTransFinished()  == ERESULT_TYPE_YES and
    // cpMST3->IsARTransFinished()  == ERESULT_TYPE_YES) {

    // 6
    // if (cpMST1->IsARTransFinished()  == ERESULT_TYPE_YES and
    // cpMST2->IsAWTransFinished()  == ERESULT_TYPE_YES and
    // cpMST3->IsARTransFinished()  == ERESULT_TYPE_YES) {

    // 7
    if (cpMST1->IsARTransFinished() == ERESULT_TYPE_YES and cpMST1->IsAWTransFinished() == ERESULT_TYPE_YES) {

      //--------------------------------------------------------------
      printf("[Cycle %3ld] Simulation is finished. \n", nCycle);
      printf("---------------------------------------------\n");
      printf("\t PIN trace \n");
      printf("---------------------------------------------\n");
      printf("\t MST1: \t read and write) \n");
      printf("---------------------------------------------\n");
      printf("\t Parameters \n");
      printf("---------------------------------------------\n");
      // printf("\t AR_GEN_NUM: \t %d\n", AR_GEN_NUM);
      // printf("\t AW_GEN_NUM: \t %d\n", AW_GEN_NUM);
      // printf("\t IMG_HORIZONTAL_SIZE: \t %d\n", IMG_HORIZONTAL_SIZE);
      // printf("\t IMG_VERTICAL_SIZE: \t %d\n", IMG_VERTICAL_SIZE);
      // printf("\t MAX_MO_COUNT: \t %d\n", MAX_MO_COUNT);
      // printf("\t AR_ADDR_INCREMENT: \t 0x%x\n", AR_ADDR_INC);
      // printf("\t AW_ADDR_INCREMENT: \t 0x%x\n", AW_ADDR_INC);

#ifdef MMU_OFF
      printf("\t MMU OFF \n");
#endif
#ifdef MMU_ON
      printf("\t MMU ON \n");
#endif

#ifdef SINGLE_FETCH
      printf("\t PTW: \t Single fetch\n");
#endif
#ifdef BLOCK_FETCH
      printf("\t PTW: \t Block fetch\n");
#endif

#ifdef CONTIGUITY_DISABLE
      printf("\t CONTIGUITY DISABLE \n");
#endif

#ifdef CONTIGUITY_ENABLE
#ifdef CONTIGUITY_0_PERCENT
      printf("\t CONTIGUITY 0 percent \n");
#endif
#ifdef CONTIGUITY_25_PERCENT
      printf("\t CONTIGUITY 25 percent \n");
#endif
#ifdef CONTIGUITY_50_PERCENT
      printf("\t CONTIGUITY 50 percent \n");
#endif
#ifdef CONTIGUITY_75_PERCENT
      printf("\t CONTIGUITY 75 percent \n");
#endif
#ifdef CONTIGUITY_100_PERCENT
      printf("\t CONTIGUITY 100 percent \n");
#endif
#endif

#ifdef AR_ADDR_RANDOM
      printf("\t AR RANDOM ADDRESS used \n");
#endif

#ifdef AW_ADDR_RANDOM
      printf("\t AW RANDOM ADDRESS used \n");
#endif

#ifdef IDEAL_MEMORY
      printf("\t IDEAL MEMORY used \n");
#endif

#ifdef MEMORY_CONTROLLER
      printf("\t MEMORY_CONTROLLER used \n");
#endif

      // printf("---------------------------------------------\n");

      //--------------------------------------------------------------
      // FILE out
      //--------------------------------------------------------------
      FILE *fp = NULL;

#ifdef FILE_OUT

#ifdef MMU_OFF
      if (IMG_VERTICAL_SIZE == 480) {
        fp = fopen("./com7_480p_off.txt", "w+");
      };
      if (IMG_VERTICAL_SIZE == 720) {
        fp = fopen("./com7_720p_off.txt", "w+");
      };
      if (IMG_VERTICAL_SIZE == 1088) {
        fp = fopen("./com7_1080p_off.txt", "w+");
      };
#endif

#ifdef MMU_ON
      if (IMG_VERTICAL_SIZE == 480) {

#ifdef CONTIGUITY_DISABLE
#ifdef SINGLE_FETCH
        fp = fopen("./com7_480p_no_contiguity_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_480p_no_contiguity_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_ENABLE

#ifdef CONTIGUITY_0_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_480p_0percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_480p_0percent_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_25_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_480p_25percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_480p_25percent_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_50_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_480p_50percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_480p_50percent_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_75_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_480p_75percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_480p_75percent_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_100_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_480p_100percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_480p_100percent_BF.txt", "w+");
#endif
#endif

#endif
      };

      if (IMG_VERTICAL_SIZE == 720) {

#ifdef CONTIGUITY_DISABLE
#ifdef SINGLE_FETCH
        fp = fopen("./com7_720p_no_contiguity_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_720p_no_contiguity_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_ENABLE

#ifdef CONTIGUITY_0_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_720p_0percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_720p_0percent_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_25_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_720p_25percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_720p_25percent_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_50_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_720p_50percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_720p_50percent_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_75_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_720p_75percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_720p_75percent_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_100_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_720p_100percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_720p_100percent_BF.txt", "w+");
#endif
#endif

#endif
      };

      if (IMG_VERTICAL_SIZE == 1088) {

#ifdef CONTIGUITY_DISABLE
#ifdef SINGLE_FETCH
        fp = fopen("./com7_1080p_no_contiguity_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_1080p_no_contiguity_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_ENABLE

#ifdef CONTIGUITY_0_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_1080p_0percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_1080p_0percent_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_25_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_1080p_25percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_1080p_25percent_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_50_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_1080p_50percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_1080p_50percent_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_75_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_1080p_75percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_1080p_75percent_BF.txt", "w+");
#endif
#endif

#ifdef CONTIGUITY_100_PERCENT
#ifdef SINGLE_FETCH
        fp = fopen("./com7_1080p_100percent_SF.txt", "w+");
#endif
#ifdef BLOCK_FETCH
        fp = fopen("./com7_1080p_100percent_BF.txt", "w+");
#endif
#endif

#endif
      };
#endif

      fprintf(fp, "---------------------------------------------\n");
      fprintf(fp, "[Cycle %3ld] Simulation is finished.\n", nCycle);
      fprintf(fp, "---------------------------------------------\n");
      fprintf(fp, "\t Parameters \n");
      fprintf(fp, "---------------------------------------------\n");
      fprintf(fp, "\t PIN trace \n");
      fprintf(fp, "---------------------------------------------\n");
      fprintf(fp, "\t MST1: \t read and write) \n");
      fprintf(fp, "---------------------------------------------\n");
      // fprintf(fp, "\t AR_GEN_NUM: \t %d\n", AR_GEN_NUM);
      // fprintf(fp, "\t AW_GEN_NUM: \t %d\n", AW_GEN_NUM);
      // fprintf(fp, "\t IMG_HORIZONTAL_SIZE: \t %d\n", IMG_HORIZONTAL_SIZE);
      // fprintf(fp, "\t IMG_VERTICAL_SIZE: \t %d\n", IMG_VERTICAL_SIZE);
      // fprintf(fp, "\t MAX_MO_COUNT: \t %d\n", MAX_MO_COUNT);
      // fprintf(fp, "\t AR_ADDR_INCREMENT: \t 0x%x\n", AR_ADDR_INC);
      // fprintf(fp, "\t AW_ADDR_INCREMENT: \t 0x%x\n", AW_ADDR_INC);

#ifdef MMU_OFF
      fprintf(fp, "\t MMU OFF \n");
#endif
#ifdef MMU_ON
      fprintf(fp, "\t MMU ON \n");
#endif

#ifdef SINGLE_FETCH
      fprintf(fp, "\t PTW: \t Single fetch\n");
#endif
#ifdef BLOCK_FETCH
      fprintf(fp, "\t PTW: \t Block fetch\n");
#endif

#ifdef CONTIGUITY_DISABLE
      fprintf(fp, "\t CONTIGUITY DISABLE \n");
#endif

#ifdef CONTIGUITY_ENABLE
#ifdef CONTIGUITY_0_PERCENT
      fprintf(fp, "\t CONTIGUITY 0 percent \n");
#endif
#ifdef CONTIGUITY_25_PERCENT
      fprintf(fp, "\t CONTIGUITY 25 percent \n");
#endif
#ifdef CONTIGUITY_50_PERCENT
      fprintf(fp, "\t CONTIGUITY 50 percent \n");
#endif
#ifdef CONTIGUITY_75_PERCENT
      fprintf(fp, "\t CONTIGUITY 75 percent \n");
#endif
#ifdef CONTIGUITY_100_PERCENT
      fprintf(fp, "\t CONTIGUITY 100 percent \n");
#endif
#endif

#ifdef AR_ADDR_RANDOM
      fprintf(fp, "\t AR RANDOM ADDRESS used \n");
#endif
#ifdef AW_ADDR_RANDOM
      fprintf(fp, "\t AW RANDOM ADDRESS used \n");
#endif
#ifdef IDEAL_MEMORY
      fprintf(fp, "\t IDEAL MEMORY used \n");
#endif
#ifdef MEMORY_CONTROLLER
      fprintf(fp, "\t MEMORY_CONTROLLER used \n");
#endif
      // fprintf(fp, "---------------------------------------------\n");

#endif

      // Stat
      // cpMST0->PrintStat(nCycle, fp);
      cpMST1->PrintStat(nCycle, fp);
      // cpMST2->PrintStat(nCycle, fp);
      // cpMST3->PrintStat(nCycle, fp);
      // cpMMU0->PrintStat(nCycle, fp);
      cpMMU1->PrintStat(nCycle, fp);
      // cpMMU2->PrintStat(nCycle, fp);
      // cpMMU3->PrintStat(nCycle, fp);
      cpSLV->PrintStat(nCycle, fp);

#ifdef FILE_OUT
      fclose(fp);
#endif

      // PIN_trace
      fclose(PIN_fp);

      break;
    };

    nCycle++;

// 7. Check termination
#ifdef TERMINATE_BY_CYCLE
    if (nCycle > SIM_CYCLE) {
      printf("[Cycle %3ld] Simulation is terminated.\n", nCycle);
      break;
    };
#endif
  };

  // 8. Destruct
  delete (cpMST0);
  delete (cpMST1);
  delete (cpMST2);
  delete (cpMST3);
  delete (cpMMU0);
  delete (cpMMU1);
  delete (cpMMU2);
  delete (cpMMU3);
  delete (cpBUS);
  delete (cpSLV);

  return (-1);
}
