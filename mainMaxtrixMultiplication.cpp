//----------------------------------------------------
// Filename     : mainMatrixMultiplication.cpp
// DATE         : 11 August 2022
// Contact	: duong.uitce@gmail.com
// Description  : main (all in one)
//----------------------------------------------------
#include <assert.h>
#include <iostream>
#include <stdlib.h>

#include "CBUS.h"
#include "CMMU.h"
#include "CMST.h"
#include "CSLV.h"

#include "./Buddy/CBuddy.h" // Buddy

#define MST0_AR_VSTART 0x0000000000000000
#define MST0_AW_VSTART 0x0000000000000000

int main() {

  int64_t nCycle = 1;

//----------------------------------------------------------
// 0. Construct Buddy
//----------------------------------------------------------
#ifdef BUDDY_ENABLE
  CPBuddy cpBuddy = new CBuddy("Buddy");
#endif

  //------------------------------------
  // 1. Generate and initialize
  //------------------------------------
  CPMST cpMST0 = new CMST("MST0");
  CPMMU cpMMU0 = new CMMU("MMU0");
  CPBUS cpBUS = new CBUS("BUS", 1);
  CPSLV cpSLV = new CSLV("SLV", 0);

  //------------------------------------
  // 2. Link TRx topology
  //------------------------------------
  // MST0 and MMU0
  cpMST0->cpTx_AR->SetPair(cpMMU0->cpRx_AR);
  cpMST0->cpTx_AW->SetPair(cpMMU0->cpRx_AW);
  cpMST0->cpTx_W->SetPair(cpMMU0->cpRx_W);
  cpMMU0->cpTx_R->SetPair(cpMST0->cpRx_R);
  cpMMU0->cpTx_B->SetPair(cpMST0->cpRx_B);

  // MMU0 and BUS
  cpMMU0->cpTx_AR->SetPair(cpBUS->cpRx_AR[0]);
  cpMMU0->cpTx_AW->SetPair(cpBUS->cpRx_AW[0]);
  cpMMU0->cpTx_W->SetPair(cpBUS->cpRx_W[0]);
  cpBUS->cpTx_R[0]->SetPair(cpMMU0->cpRx_R);
  cpBUS->cpTx_B[0]->SetPair(cpMMU0->cpRx_B);

  // BUS and SLV
  cpBUS->cpTx_AR->SetPair(cpSLV->cpRx_AR);
  cpBUS->cpTx_AW->SetPair(cpSLV->cpRx_AW);
  cpBUS->cpTx_W->SetPair(cpSLV->cpRx_W);
  cpSLV->cpTx_R->SetPair(cpBUS->cpRx_R);
  cpSLV->cpTx_B->SetPair(cpBUS->cpRx_B);

  // Debug
  cpMST0->CheckLink();
  cpMMU0->CheckLink();
  cpBUS->CheckLink();
  cpSLV->CheckLink();

//----------------------------------------------------------
// Config Buddy initial FreeList.
//----------------------------------------------------------
#ifdef BUDDY_ENABLE
  // cpBuddy->Set_FreeList();   //DUONGTRAN comment

  // DUONGTRAN add: Allign initialize FreeList
  //                      #block size     #number of blocks
  // cpBuddy->Set_FreeList(BLOCKSIZE_INIT, NUM_TOTAL_PAGES / BLOCKSIZE_INIT);

  //////DUONGTRAN add: Missalign initialize Memmap
  cpBuddy->InitializeMemMap(BLOCKSIZE_INIT, NUM_TOTAL_PAGES);
#ifdef RBS
  cpBuddy->Set_FreeList('R');
#elif defined PBS
  cpBuddy->Set_FreeList('P');
#elif defined BBS
  cpBuddy->Set_FreeList('B');
#else
  assert(0);
#endif
#endif

//----------------------------------------------------------
// Allocate Buddy
//----------------------------------------------------------
// DUONGTRAN add for RangeBuddy
#ifdef BUDDY_ENABLE
  int NUM_REQ_PAGES = -1;
  SPPTE *spPageTable = NULL;

  //-------------------
  // MST0 AR
  //-------------------
  // Allocate
  NUM_REQ_PAGES = IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE * BYTE_PER_PIXEL / 4096;
  assert(NUM_TOTAL_PAGES >= NUM_REQ_PAGES);
  cpBuddy->Do_Allocate(MST0_AW_VSTART >> 12, NUM_REQ_PAGES);

  // Get page-table
  spPageTable = cpBuddy->Get_PageTable();

  // Set page-table
  cpMMU0->Set_PageTable(spPageTable);

#ifdef AT_ENABLE
  cpMMU0->anchorDistance = cpBuddy->anchorDistance; // DUONGTRAN add
#endif

#endif

  //------------------------------
  // Set total Ax NUM
  //------------------------------
  int NUM = IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE * BYTE_PER_PIXEL / MAX_TRANS_SIZE;

  // Number of transactions
  cpMST0->Set_nAR_GEN_NUM(0);
  cpMST0->Set_nAW_GEN_NUM(0);

  // cpMST3->Set_nAR_GEN_NUM(0);
  // cpMST3->Set_nAW_GEN_NUM(0);	// DUONGTRAN comment

  cpMST0->Set_nAW_GEN_NUM(NUM);
  cpMST0->Set_nAR_GEN_NUM(NUM);
  // cpMST3->Set_nAR_GEN_NUM(NUM);

  //------------------------------
  // Set image scaling
  //------------------------------
  cpMST0->Set_ScalingFactor(1);
  // cpMST3->Set_ScalingFactor(1);

  //------------------------------
  // Set start address MST
  //------------------------------
  cpMST0->Set_nAR_START_ADDR(MST0_AR_VSTART); // DUONGTRAN comment
  cpMST0->Set_nAW_START_ADDR(MST0_AW_VSTART);

  //------------------------------
  // Set start address MMU. RMM
  //------------------------------
  cpMMU0->Set_nAR_START_ADDR(MST0_AR_VSTART);
  cpMMU0->Set_nAW_START_ADDR(MST0_AW_VSTART);

  //------------------------------
  // Set start VPN
  //	Buddy pag-table
  //	Assume single thread read or write FIXME
  //------------------------------

  cpMMU0->Set_START_VPN(MST0_AW_VSTART >> 12);
// cpMMU3->Set_START_VPN(MST3_AR_VSTART >> 12);

//------------------------------
// Set operation
//------------------------------
// RASTER_SCAN, ROTATION, RANDOM
//------------------------------
#ifdef RASTER_SCAN
  cpMST0->Set_AR_Operation("RASTER_SCAN");
  cpMST0->Set_AW_Operation("RASTER_SCAN");
  // cpMST3->Set_AR_Operation("RASTER_SCAN");
  // cpMST3->Set_AW_Operation("RASTER_SCAN");
#elif ROTATION
  cpMST0->Set_AR_Operation("ROTATION");
  cpMST0->Set_AW_Operation("ROTATION");
  // cpMST3->Set_AR_Operation("ROTATION");
  // cpMST3->Set_AW_Operation("ROTATION");
#else
  assert(0);
#endif

  ////------------------------------
  //// Set Ax issue interval cycles
  ////------------------------------
  // cpMST0->Set_AR_ISSUE_MIN_INTERVAL(1);    // DUONGTRAN FIXME ??? Why? When
  // reset then it become 4 cpMST0->Set_AW_ISSUE_MIN_INTERVAL(1);

  // cpMST3->Set_AR_ISSUE_MIN_INTERVAL(1);
  // cpMST3->Set_AW_ISSUE_MIN_INTERVAL(1);

  //------------------------------
  // Set AddrMap
  //------------------------------
  string cAR_AddrMap;
  string cAW_AddrMap;

#ifdef AR_LIAM
  cAR_AddrMap = "LIAM";
#elif AR_BFAM
  cAR_AddrMap = "BFAM";
#elif AR_TILE
  cAR_AddrMap = "TILE";
#endif

#ifdef AW_LIAM
  cAW_AddrMap = "LIAM";
#elif AW_BFAM
  cAW_AddrMap = "BFAM";
#elif AW_TILE
  cAW_AddrMap = "TILE";
#endif

  char const *FileName = "./test.trc";
  FILE *PIN_fp = NULL;

  PIN_fp = fopen(FileName, "r");
  if (!PIN_fp) {
    printf("Couldn't open file %s for reading.\n", FileName);
    return (ERESULT_TYPE_FAIL);
  }

  // Simulate
  while (1) {

    //---------------------------
    // 3. Reset
    //---------------------------
    if (nCycle == 1) {
      cpMST0->Reset();
      // cpMST3->Reset();
      cpMMU0->Reset();
      // cpMMU3->Reset();
      cpBUS->Reset();
      cpSLV->Reset();
    };

    //---------------------------------------------
    // 4. Start simulation
    //---------------------------------------------

    //// Master geneates transactions
    // #ifdef RASTER_SCAN
    //	//cpMST0->LoadTransfer_AR(nCycle, cAR_AddrMap, "RASTER_SCAN");
    ////DUONGTRAN comment 	cpMST0->LoadTransfer_AW(nCycle, cAW_AddrMap,
    //"RASTER_SCAN"); 	cpMST3->LoadTransfer_AR(nCycle, cAR_AddrMap,
    //"RASTER_SCAN");
    //	//cpMST3->LoadTransfer_AW(nCycle, cAW_AddrMap, "RASTER_SCAN");  //
    // DUONGTRAN comment #elif ROTATION
    //	//cpMST0->LoadTransfer_AR(nCycle, cAR_AddrMap, "ROTATION");  //DUONGTRAN
    // comment 	cpMST0->LoadTransfer_AW(nCycle, cAW_AddrMap, "ROTATION");
    //	cpMST3->LoadTransfer_AR(nCycle, cAR_AddrMap, "ROTATION");
    //	//cpMST3->LoadTransfer_AW(nCycle, cAW_AddrMap, "ROTATION");  //
    // DUONGTRAN comment #else 	assert(0); #endif

    cpMST0->LoadTransfer_Ax_PIN_Trace(nCycle, PIN_fp);

    //---------------------------------
    // Propagate valid
    //---------------------------------

    //---------------------------------
    // AR, AW, W VALID
    //---------------------------------
    cpMST0->Do_AR_fwd(nCycle); // DUONGTRAN comment
    cpMST0->Do_AW_fwd(nCycle);
// cpMST0->Do_W_fwd(nCycle);

// cpMST3->Do_AR_fwd(nCycle);
// cpMST3->Do_AW_fwd(nCycle);   // DUONGTRAN comment
//  cpMST3->Do_W_fwd(nCycle);

//--------------------------------
#ifdef MMU_OFF
    cpMMU0->Do_AR_fwd_MMU_OFF(nCycle);
    cpMMU0->Do_AW_fwd_MMU_OFF(nCycle);
// cpMMU0->Do_W_fwd_MMU_OFF(nCycle);
#endif

// DUONGTRAN add
#ifdef MMU_ON
#ifdef AT_ENABLE
    cpMMU0->Do_AR_fwd_SI_AT(nCycle);
    cpMMU0->Do_AR_fwd_MI(nCycle);
    cpMMU0->Do_AW_fwd_AT(nCycle);
    // cpMMU3->Do_AR_fwd_SI_AT(nCycle);
    // cpMMU3->Do_AR_fwd_MI(nCycle);
#else // PCAD, TRAD
    cpMMU0->Do_AR_fwd_SI(nCycle);
    cpMMU0->Do_AR_fwd_MI(nCycle);
    cpMMU0->Do_AW_fwd(nCycle);
    // cpMMU3->Do_AR_fwd_SI(nCycle);
    // cpMMU3->Do_AR_fwd_MI(nCycle);
#endif
#endif

    //-------------------------------
    cpBUS->Do_AR_fwd(nCycle);
    cpBUS->Do_AW_fwd(nCycle);
// cpBUS->Do_W_fwd(nCycle);

//-------------------------------
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

//---------------------------
#ifdef MMU_OFF
    cpMMU0->Do_R_fwd_MMU_OFF(nCycle);
    cpMMU0->Do_B_fwd_MMU_OFF(nCycle);
#endif

#ifdef MMU_ON
#ifdef AT_ENABLE
    cpMMU0->Do_R_fwd_AT(nCycle);
    cpMMU0->Do_B_fwd(nCycle);
#else // PCAD, TRAD
    cpMMU0->Do_R_fwd(nCycle);
    cpMMU0->Do_B_fwd(nCycle);
#endif
#endif

    // #ifdef MMU_OFF
    // cpMMU3->Do_R_fwd_MMU_OFF(nCycle);
    // cpMMU3->Do_B_fwd_MMU_OFF(nCycle);
    // #endif

    // #ifdef MMU_ON
    //	#ifdef AT_ENABLE
    //		cpMMU3->Do_R_fwd_AT(nCycle);
    //		cpMMU3->Do_B_fwd(nCycle);
    //	#else // PCAD, TRAD
    //		cpMMU3->Do_R_fwd(nCycle);
    //		cpMMU3->Do_B_fwd(nCycle);  //DUONGTRAN comment
    //	#endif
    // #endif

    //---------------------------
    cpMST0->Do_R_fwd(nCycle);
    cpMST0->Do_B_fwd(nCycle);

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
    cpMMU0->Do_AR_bwd_MMU_OFF(nCycle);
    cpMMU0->Do_AW_bwd_MMU_OFF(nCycle);
// cpMMU0->Do_W_bwd_MMU_OFF(nCycle);
#endif

#ifdef MMU_ON
    cpMMU0->Do_AR_bwd(nCycle);
    cpMMU0->Do_AW_bwd(nCycle);
// cpMMU0->Do_W_bwd(nCycle);
#endif

    // #ifdef MMU_OFF
    // cpMMU3->Do_AR_bwd_MMU_OFF(nCycle);
    // cpMMU3->Do_AW_bwd_MMU_OFF(nCycle);
    //// cpMMU3->Do_W_bwd_MMU_OFF(nCycle);
    // #endif

    // #ifdef MMU_ON
    // cpMMU3->Do_AR_bwd(nCycle);
    ////cpMMU3->Do_AW_bwd(nCycle);    // DUONGTRAN comment
    ////cpMMU3->Do_W_bwd(nCycle);
    // #endif

    //-----------------------------
    cpMST0->Do_AR_bwd(nCycle); // DUONGTRAN comment
    cpMST0->Do_AW_bwd(nCycle);
    // cpMST0->Do_W_bwd(nCycle);

    // cpMST3->Do_AR_bwd(nCycle);
    // cpMST3->Do_AW_bwd(nCycle);  // DUONGTRAN comment
    // cpMST3->Do_W_bwd(nCycle);

    //---------------------------------
    // R, B READY
    //---------------------------------

    cpMST0->Do_R_bwd(nCycle);
    cpMST0->Do_B_bwd(nCycle);

// cpMST3->Do_R_bwd(nCycle);
// cpMST3->Do_B_bwd(nCycle);   // DUONGTRAN comment

//---------------------------
#ifdef MMU_OFF
    cpMMU0->Do_R_bwd_MMU_OFF(nCycle);
    cpMMU0->Do_B_bwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
    cpMMU0->Do_R_bwd(nCycle);
    cpMMU0->Do_B_bwd(nCycle);
#endif

    // #ifdef MMU_OFF
    // cpMMU3->Do_R_bwd_MMU_OFF(nCycle);
    // cpMMU3->Do_B_bwd_MMU_OFF(nCycle);
    // #endif
    // #ifdef MMU_ON
    // cpMMU3->Do_R_bwd(nCycle);
    ////cpMMU3->Do_B_bwd(nCycle);    DUONGTRAN comment
    // #endif

    //---------------------------
    cpBUS->Do_R_bwd(nCycle);
    cpBUS->Do_B_bwd(nCycle);

    cpSLV->Do_R_bwd(nCycle);
    cpSLV->Do_B_bwd(nCycle);

    //---------------------------------
    // 5. Update state
    //---------------------------------
    cpMST0->UpdateState(nCycle);
    // cpMST3->UpdateState(nCycle);
    cpMMU0->UpdateState(nCycle);
    // cpMMU3->UpdateState(nCycle);
    cpBUS->UpdateState(nCycle);
    cpSLV->UpdateState(nCycle);

    //---------------------------------
    // 6. Check simulation finish
    //---------------------------------
    if (cpMST0->IsAWTransFinished() == ERESULT_TYPE_YES and cpMST0->IsARTransFinished() == ERESULT_TYPE_YES) {

      //--------------------------------------------------------------
      printf("[Cycle %3ld] Simulation is finished. \n", nCycle);
      printf("---------------------------------------------\n");
      printf("MST0: \t read and write\n");

      printf("---------------------------------------------\n");
      printf("\t Parameters \n");
      printf("---------------------------------------------\n");
      // printf("\t AR_GEN_NUM		: %d\n", AR_GEN_NUM);
      // printf("\t AW_GEN_NUM		: %d\n", AW_GEN_NUM);
      // printf("\t IMG_HORIZONTAL_SIZE		: %d\n", IMG_HORIZONTAL_SIZE);
      // printf("\t IMG_VERTICAL_SIZE		: %d\n", IMG_VERTICAL_SIZE);
      // printf("\t MAX_MO_COUNT			: %d\n", MAX_MO_COUNT);
      // printf("\t AR_ADDR_INCREMENT		: 0x%x\n", AR_ADDR_INC);
      // printf("\t AW_ADDR_INCREMENT		: 0x%x\n", AW_ADDR_INC);

#ifdef MMU_OFF
      printf("\t MMU				: OFF \n");
#endif
#ifdef MMU_ON
      printf("\t MMU				: ON \n");
#endif

#ifdef SINGLE_FETCH
      printf("\t PTW				: Single fetch\n");
#endif
#ifdef BLOCK_FETCH
      printf("\t PTW				: Block fetch\n");
#endif

#ifdef CONTIGUITY_DISABLE
      printf("\t CONTIGUITY			: DISABLE \n");
#endif

#ifdef CONTIGUITY_ENABLE
#ifdef CONTIGUITY_0_PERCENT
      printf("\t CONTIGUITY			: 0 percent \n");
#endif
#ifdef CONTIGUITY_25_PERCENT
      printf("\t CONTIGUITY			: 25 percent \n");
#endif
#ifdef CONTIGUITY_50_PERCENT
      printf("\t CONTIGUITY			: 50 percent \n");
#endif
#ifdef CONTIGUITY_75_PERCENT
      printf("\t CONTIGUITY			: 75 percent \n");
#endif
#ifdef CONTIGUITY_100_PERCENT
      printf("\t CONTIGUITY			: 100 percent \n");
#endif
#endif

#ifdef AR_ADDR_RANDOM
      printf("\t AR RANDOM ADDRESS 		: ON \n");
#endif

#ifdef AW_ADDR_RANDOM
      printf("\t AW RANDOM ADDRESS 		: ON \n");
#endif

#ifdef IDEAL_MEMORY
      printf("\t IDEAL MEMORY 		: ON \n");
#endif

#ifdef MEMORY_CONTROLLER
      printf("\t MEMORY_CONTROLLER 		: ON \n");
#endif

// printf("---------------------------------------------\n");

//--------------------------------------------------------------
// FILE out
//--------------------------------------------------------------
// FILE *fp = NULL;

// DUONGTRAN comment
// Stat
// cpMST0->PrintStat(nCycle, fp);
// cpMST1->PrintStat(nCycle, fp);
// cpMST2->PrintStat(nCycle, fp);
// cpMST3->PrintStat(nCycle, fp);
// cpMST4->PrintStat(nCycle, fp);
// cpMST5->PrintStat(nCycle, fp);
#ifdef MMU_ON
// cpMMU0->PrintStat(nCycle, fp);
//  cpMMU1->PrintStat(nCycle, fp);
//  cpMMU2->PrintStat(nCycle, fp);
// cpMMU3->PrintStat(nCycle, fp);
#endif

// Get total avg TLB hit rate
#ifdef MMU_ON
      // int Total_nAR_SI = cpMMU0->Get_nAR_SI() + cpMMU1->Get_nAR_SI() +
      // cpMMU2->Get_nAR_SI() + cpMMU3->Get_nAR_SI();
      int Total_nAR_SI = cpMMU0->Get_nAR_SI() + cpMMU3->Get_nAR_SI();
      // int Total_nAW_SI = cpMMU0->Get_nAW_SI() + cpMMU1->Get_nAW_SI() +
      // cpMMU2->Get_nAW_SI() + cpMMU3->Get_nAW_SI();
      int Total_nAW_SI = cpMMU0->Get_nAW_SI() + cpMMU3->Get_nAW_SI();
      // int Total_nHit_AR_TLB = cpMMU0->Get_nHit_AR_TLB() +
      // cpMMU1->Get_nHit_AR_TLB() + cpMMU2->Get_nHit_AR_TLB() +
      // cpMMU3->Get_nHit_AR_TLB();
      int Total_nHit_AR_TLB = cpMMU0->Get_nHit_AR_TLB() + cpMMU3->Get_nHit_AR_TLB();
      // int Total_nHit_AW_TLB = cpMMU0->Get_nHit_AW_TLB() +
      // cpMMU1->Get_nHit_AW_TLB() + cpMMU2->Get_nHit_AW_TLB() +
      // cpMMU3->Get_nHit_AW_TLB();
      int Total_nHit_AW_TLB = cpMMU0->Get_nHit_AW_TLB() + cpMMU3->Get_nHit_AW_TLB();

      float Avg_TLB_hit_rate = (float)(Total_nHit_AR_TLB + Total_nHit_AW_TLB) / (Total_nAR_SI + Total_nAW_SI);

      // Get TLB reach
      float Avg_TLB_reach = (float)(cpMMU0->GetTLB_reach(nCycle) + cpMMU3->GetTLB_reach(nCycle)) / 2;

      printf("---------------------------------------------\n");
      printf("\t Total Avg TLB hit rate = %1.5f\n", Avg_TLB_hit_rate);
      printf("\t Total Avg TLB reach    = %1.5f\n", Avg_TLB_reach);
      printf("---------------------------------------------\n");

#endif // MMU_ON

      // DUONGTRAN comment
      // cpBUS ->PrintStat(nCycle, fp);
      // cpSLV ->PrintStat(nCycle, fp);

      // PIN_trace
      fclose(PIN_fp);

      break;
    };

    nCycle++;

//--------------------------
// 7. Check termination
//--------------------------
#ifdef TERMINATE_BY_CYCLE
    if (nCycle > SIM_CYCLE) {
      printf("[Cycle %3ld] Simulation is terminated.\n", nCycle);
      // break; DUONGTRAN
      return 0;
    };
#endif
  };

//----------------------------------------------------------
// De-allocate Buddy
// 	FIXME Manage BlockList.
//----------------------------------------------------------
#ifdef BUDDY_ENABLE
  cpBuddy->Do_Deallocate(MST0_AW_VSTART >> 12);
  cpBuddy->Do_Deallocate(MST3_AR_VSTART >> 12);
#endif

  //---------------------
  // 8. Destruct
  //---------------------
  delete (cpMST0);
  // delete (cpMST1);
  // delete (cpMST2);
  delete (cpMST3);
  // delete (cpMST4);
  // delete (cpMST5);
  delete (cpMMU0);
  // delete (cpMMU1);
  // delete (cpMMU2);
  delete (cpMMU3);
  delete (cpBUS);
  delete (cpSLV);

#ifdef BUDDY_ENABLE
  delete (cpBuddy);
#endif

  return (-1);
}
