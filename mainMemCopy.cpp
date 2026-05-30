//----------------------------------------------------
// Filename     : main.cpp
// DATE         : 25 Apr 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description  : main
// MST3: Display read (standalone)
//----------------------------------------------------
//----------------------------------------------------
#include <assert.h>
#include <stdlib.h>

#include "CBUS.h"
#include "CMST.h"
#include "CSLV.h"

#define MST0_AR_VSTART 0x0000000000000000
#define MST0_AW_VSTART 0x0000000000000000

#define MST1_AR_VSTART 0x0000000000100000
#define MST1_AW_VSTART 0x0000000000100000

bool master_finish = false;

int main() {

  //===========================================
  // 0.  Configurations
  //=========================================s==
#ifdef CCI_CACHELINE
  nCACHELINE = CCI_CACHELINE;
#else
  nCACHELINE = 1;
#endif
#ifdef CCI_SNOOP_LATENCY
  SNOOP_LATENCY = CCI_SNOOP_LATENCY;
#else
  SNOOP_LATENCY = 10;
#endif
  SNOOP_MASK = 0; // Set Snoop Mask here
  MEMCOPY = true;

  //===========================================G

  int64_t nCycle = 1;
  // int SEED = clock();
  int SEED = SEED_INIT;
  printf("SEED: %d\n", SEED);
  srand(SEED);

  //------------------------------------
  // 1. Generate and initialize
  //------------------------------------
  CPMST cpMST0 = new CMST("MST0");
  CPMST cpMST1 = new CMST("MST1");
  CPBUS cpBUS = new CBUS("BUS", 2);
  CPSLV cpSLV = new CSLV("SLV", 0);

#ifdef CCI_CACHELINE
  cpMST0->Set_nCACHELINE(1);
  cpMST1->Set_nCACHELINE(CCI_CACHELINE);
#else
  cpMST0->Set_nCACHELINE(1);
  cpMST1->Set_nCACHELINE(1);
#endif

  //------------------------------------
  // 2. Link TRx topology
  //------------------------------------
  // MST0 and BUS
  cpMST0->cpTx_AR->SetPair(cpBUS->cpRx_AR[0]);
  cpMST0->cpTx_AW->SetPair(cpBUS->cpRx_AW[0]);
  cpMST0->cpTx_W->SetPair(cpBUS->cpRx_W[0]);
  cpMST0->cpTx_CR->SetPair(cpBUS->cpRx_CR[0]);
  cpMST0->cpTx_CD->SetPair(cpBUS->cpRx_CD[0]);
  cpBUS->cpTx_R[0]->SetPair(cpMST0->cpRx_R);
  cpBUS->cpTx_B[0]->SetPair(cpMST0->cpRx_B);
  cpBUS->cpTx_AC[0]->SetPair(cpMST0->cpRx_AC);

  // MST1 and BUS
  cpMST1->cpTx_AR->SetPair(cpBUS->cpRx_AR[1]);
  cpMST1->cpTx_AW->SetPair(cpBUS->cpRx_AW[1]);
  cpMST1->cpTx_W->SetPair(cpBUS->cpRx_W[1]);
  cpMST1->cpTx_CR->SetPair(cpBUS->cpRx_CR[1]);
  cpMST1->cpTx_CD->SetPair(cpBUS->cpRx_CD[1]);
  cpBUS->cpTx_R[1]->SetPair(cpMST1->cpRx_R);
  cpBUS->cpTx_B[1]->SetPair(cpMST1->cpRx_B);
  cpBUS->cpTx_AC[1]->SetPair(cpMST1->cpRx_AC);

  // BUS and SLV
  cpBUS->cpTx_AR->SetPair(cpSLV->cpRx_AR);
  cpBUS->cpTx_AW->SetPair(cpSLV->cpRx_AW);
  cpBUS->cpTx_W->SetPair(cpSLV->cpRx_W);
  cpSLV->cpTx_R->SetPair(cpBUS->cpRx_R);
  cpSLV->cpTx_B->SetPair(cpBUS->cpRx_B);

  // Debug
  cpMST0->CheckLink();
  cpMST1->CheckLink();
  cpBUS->CheckLink();
  cpSLV->CheckLink();

  //------------------------------
  // Set total Ax NUM
  //------------------------------
  int NUM = IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE * BYTE_PER_PIXEL / MAX_TRANS_SIZE;

#if defined(CCI_READ_ONLY)
  // READ-only (AR)
  cpMST0->Set_nAW_GEN_NUM(0);
  cpMST0->Set_nAR_GEN_NUM(0);
  cpMST1->Set_nAW_GEN_NUM(0);
  cpMST1->Set_nAR_GEN_NUM(ceil(NUM / cpMST1->Get_nCACHELINE()));
  cpBUS->Set_nB_GEN_NUM(0, 0);
  cpBUS->Set_nB_GEN_NUM(1, 0);
  cpBUS->Set_nR_GEN_NUM(0, 0);
  cpBUS->Set_nR_GEN_NUM(1, NUM);

#elif defined(CCI_WRITE_ONLY)
  cpMST0->Set_nAW_GEN_NUM(0);
  cpMST0->Set_nAR_GEN_NUM(0);
  cpMST1->Set_nAW_GEN_NUM(ceil(NUM / cpMST1->Get_nCACHELINE()));
  cpMST1->Set_nAR_GEN_NUM(0);
  cpBUS->Set_nB_GEN_NUM(0, 0);
  cpBUS->Set_nB_GEN_NUM(1, NUM);
  cpBUS->Set_nR_GEN_NUM(0, 0);
  cpBUS->Set_nR_GEN_NUM(1, 0);

#elif defined(CCI_WRITE_READ)
  cpMST0->Set_nAW_GEN_NUM(0);
  cpMST0->Set_nAR_GEN_NUM(ceil(NUM / cpMST0->Get_nCACHELINE()));
  cpMST1->Set_nAW_GEN_NUM(ceil(NUM / cpMST1->Get_nCACHELINE()));
  cpMST1->Set_nAR_GEN_NUM(0);
  cpBUS->Set_nB_GEN_NUM(0, 0);
  cpBUS->Set_nB_GEN_NUM(1, NUM);
  cpBUS->Set_nR_GEN_NUM(0, NUM);
  cpBUS->Set_nR_GEN_NUM(1, 0);

#elif defined(CCI_WRITE_WRITE)
  cpMST0->Set_nAW_GEN_NUM(ceil(NUM / cpMST0->Get_nCACHELINE()));
  cpMST0->Set_nAR_GEN_NUM(0);
  cpMST1->Set_nAW_GEN_NUM(ceil(NUM / cpMST1->Get_nCACHELINE()));
  cpMST1->Set_nAR_GEN_NUM(0);
  cpBUS->Set_nB_GEN_NUM(0, NUM);
  cpBUS->Set_nB_GEN_NUM(1, NUM);
  cpBUS->Set_nR_GEN_NUM(0, 0);
  cpBUS->Set_nR_GEN_NUM(1, 0);
#else
  assert(0);
#endif

  //------------------------------
  // Set image scaling
  //------------------------------
  cpMST0->Set_ScalingFactor(1);
  cpMST1->Set_ScalingFactor(1);

  //------------------------------
  // Set start address MST
  //------------------------------
  cpMST0->Set_nAR_START_ADDR(MST0_AR_VSTART);
  cpMST0->Set_nAW_START_ADDR(MST0_AW_VSTART);

  cpMST1->Set_nAR_START_ADDR(MST1_AR_VSTART);
  cpMST1->Set_nAW_START_ADDR(MST1_AW_VSTART);

  //------------------------------
  // Set operation
  //------------------------------
  // RASTER_SCAN, ROTATION, RANDOM
  //------------------------------
  cpMST0->Set_AR_Operation("RASTER_SCAN");
  cpMST0->Set_AW_Operation("RASTER_SCAN");

  cpMST1->Set_AR_Operation("RASTER_SCAN");
  cpMST1->Set_AW_Operation("RASTER_SCAN");

  // Simulate
  while (1) {
    //---------------------------
    // 3. Reset
    //---------------------------
    if (nCycle == 1) {
      cpMST0->Reset();
      cpMST1->Reset();

      cpBUS->Reset();
      cpSLV->Reset();
    };

#ifdef ISSUE_MIN_INTERVAL
    cpMST0->Set_AR_ISSUE_MIN_INTERVAL(ISSUE_MIN_INTERVAL);
    cpMST0->Set_AW_ISSUE_MIN_INTERVAL(ISSUE_MIN_INTERVAL);

    cpMST1->Set_AR_ISSUE_MIN_INTERVAL(ISSUE_MIN_INTERVAL);
    cpMST1->Set_AW_ISSUE_MIN_INTERVAL(ISSUE_MIN_INTERVAL);
#else
    cpMST0->Set_AR_ISSUE_MIN_INTERVAL(8);
    cpMST0->Set_AW_ISSUE_MIN_INTERVAL(8);

    cpMST1->Set_AR_ISSUE_MIN_INTERVAL(8);
    cpMST1->Set_AW_ISSUE_MIN_INTERVAL(8);
#endif

    //---------------------------------------------
    // 4. Start simulation
    //---------------------------------------------
    // Master geneates transactions
    cpMST0->LoadTransfer_AR(nCycle, "LIAM", "RASTER_SCAN");
    cpMST0->LoadTransfer_AW(nCycle, "LIAM", "RASTER_SCAN");

    cpMST1->LoadTransfer_AR(nCycle, "LIAM", "RASTER_SCAN");
    cpMST1->LoadTransfer_AW(nCycle, "LIAM", "RASTER_SCAN");

    //---------------------------------
    // Propagate valid
    //---------------------------------
    //---------------------------------
    // AR, AW, W VALID
    //---------------------------------
    cpMST0->Do_AR_fwd(nCycle);
    cpMST0->Do_AW_fwd(nCycle);

    cpMST1->Do_AR_fwd(nCycle);
    cpMST1->Do_AW_fwd(nCycle);
    //--------------------------------
    //-------------------------------
    cpBUS->Do_AR_fwd(nCycle);
    cpBUS->Do_AW_fwd(nCycle);

//--------------------------------
//--------------------------------
// Bus receives the AR and AW transaction and bypass it into AC ports.
#ifdef CCI_ON
    cpBUS->Do_AC_fwd(nCycle);
#endif

//--------------------------------
//--------------------------------
// Bus receives the AR and AW transaction and bypass it into AC ports.
#ifdef CCI_ON
    cpBUS->Do_CD_fwd(nCycle);
    cpBUS->Do_CR_fwd(nCycle);
#endif

// Master receive the AC transactions from BUS and forward it into CR and CD.
#ifdef CCI_ON
    cpMST0->Do_CR_fwd(nCycle);
    cpMST1->Do_CR_fwd(nCycle);

    cpMST0->Do_CD_fwd(nCycle);
    cpMST1->Do_CD_fwd(nCycle);

    cpMST0->Do_AC_fwd(nCycle);
    cpMST1->Do_AC_fwd(nCycle);
#endif

    cpBUS->Do_W_fwd(nCycle);
    cpBUS->Do_W_snoop_fwd(nCycle);

//--------------------------------
//-------------------------------
#ifdef IDEAL_MEMORY
    cpSLV->Do_AR_fwd(nCycle);
    cpSLV->Do_AW_fwd(nCycle);
    cpSLV->Do_W_fwd(nCycle);
#endif
#ifdef MEMORY_CONTROLLER
    cpSLV->Do_AR_fwd_MC_Frontend(nCycle);
    cpSLV->Do_AW_fwd_MC_Frontend(nCycle);
    cpSLV->Do_W_fwd_MC_Frontend(nCycle);
#endif
#ifdef MEMORY_CONTROLLER
    cpSLV->Do_Ax_fwd_MC_Backend_Request(nCycle);
    for (int i = 0; i < BANK_NUM; i++) {
      cpSLV->Do_AR_fwd_MC_Backend_Response(nCycle);
      cpSLV->Do_AW_fwd_MC_Backend_Response(nCycle);
    }
#endif
    //---------------------------------
    // R, B VALID
    //---------------------------------
    //---------------------------------
    cpSLV->Do_R_fwd(nCycle);
    cpSLV->Do_B_fwd(nCycle);
    //---------------------------
    //---------------------------
    cpBUS->Do_R_fwd(nCycle);
    cpBUS->Do_B_fwd(nCycle);

    //---------------------------
    //---------------------------
    cpMST0->Do_R_fwd(nCycle);
    cpMST0->Do_B_fwd(nCycle);

    cpMST1->Do_R_fwd(nCycle);
    cpMST1->Do_B_fwd(nCycle);
    //---------------------------------
    // Propagate ready
    //---------------------------------
    //---------------------------------
    // AR, AW, W READY
    //---------------------------------
    cpSLV->Do_AR_bwd(nCycle);
    cpSLV->Do_AW_bwd(nCycle);
    cpSLV->Do_W_bwd(nCycle);
    //-----------------------------
    //-------------------------------
    cpBUS->Do_AR_bwd(nCycle);
    cpBUS->Do_AW_bwd(nCycle);

    //-----------------------------
    cpMST0->Do_AR_bwd(nCycle);
    cpMST0->Do_AW_bwd(nCycle);
    cpMST0->Do_W_bwd(nCycle);

    cpMST1->Do_AR_bwd(nCycle);
    cpMST1->Do_AW_bwd(nCycle);
    cpMST1->Do_W_bwd(nCycle);

#ifdef CCI_ON
    cpMST0->Do_AC_bwd(nCycle);
    cpMST1->Do_AC_bwd(nCycle);

    cpMST0->Do_CR_bwd(nCycle);
    cpMST1->Do_CR_bwd(nCycle);

    cpMST0->Do_CD_bwd(nCycle);
    cpMST1->Do_CD_bwd(nCycle);
#endif

#ifdef CCI_ON
    cpBUS->Do_AC_bwd(nCycle);
    cpBUS->Do_CD_bwd(nCycle);
    cpBUS->Do_CR_bwd(nCycle);
#endif

    //---------------------------------
    // R, B READY
    //---------------------------------
    cpMST0->Do_R_bwd(nCycle);
    cpMST0->Do_B_bwd(nCycle);

    cpMST1->Do_R_bwd(nCycle);
    cpMST1->Do_B_bwd(nCycle);
    //---------------------------

    //---------------------------
    cpBUS->Do_R_bwd(nCycle);
    cpBUS->Do_B_bwd(nCycle);
    //---------------------------
    //---------------------------
    cpSLV->Do_R_bwd(nCycle);
    cpSLV->Do_B_bwd(nCycle);
    //---------------------------------
    // 5. Update state
    //---------------------------------
    cpMST0->UpdateState(nCycle);
    cpMST1->UpdateState(nCycle);

    cpBUS->UpdateState(nCycle);
    cpSLV->UpdateState(nCycle);

    //---------------------------------
    // 6. Check simulation finish
    //---------------------------------
    if (cpMST1->IsAllTransFinished() == ERESULT_TYPE_YES && cpMST0->IsAllTransFinished() == ERESULT_TYPE_YES &&
        !master_finish) {
      printf("[Cycle %3ld] Master is finished.\n", nCycle);
      master_finish = true;
      // break;
    };

    if (/*cpBUS->Get_nRTrans(0) == cpBUS->Get_nR_GEN_NUM(0) && cpBUS->Get_nBTrans(0) == cpBUS->Get_nB_GEN_NUM(0) &&*/
        cpBUS->Get_nRTrans(1) == cpBUS->Get_nR_GEN_NUM(1) && cpBUS->Get_nBTrans(1) == cpBUS->Get_nB_GEN_NUM(1)) {
      printf("[Cycle %3ld] Bus is finished.\n", nCycle);
      break;
    }

    nCycle++;

//--------------------------
// 7. Check termination
//--------------------------
#ifdef TERMINATE_BY_CYCLE
    if (nCycle > SIM_CYCLE) {
      printf("[Cycle %3ld] Simulation is terminated.\n", nCycle);
      break;
    };
#endif
  };

  //--------------------------------------------------------------
  printf("[Cycle %3ld] Simulation is finished. \n", nCycle);
  printf("---------------------------------------------\n");
  printf("MST1: \t Memset & BurstlyWrite \n");
  printf("---------------------------------------------\n");
  printf("\t Parameters \n");
  printf("---------------------------------------------\n");
  printf("\t GEN_NUM                  : %d\n", NUM);
  // printf("\t AW_GEN_NUM				: %d\n", AW_GEN_NUM);
  printf("\t IMG_HORIZONTAL_SIZE      : %d\n", IMG_HORIZONTAL_SIZE);
  printf("\t IMG_VERTICAL_SIZE        : %d\n", IMG_VERTICAL_SIZE);
  printf("\t MAX_MO_COUNT				: %d\n", MAX_MO_COUNT);

  printf("\t [0] R channel responses		: %d\n", cpBUS->Get_nRTrans(0));
  printf("\t [0] B channel responses		: %d\n", cpBUS->Get_nBTrans(0));
  printf("\t [1] R channel responses		: %d\n", cpBUS->Get_nRTrans(1));
  printf("\t [1] B channel responses		: %d\n", cpBUS->Get_nBTrans(1));
  printf("\t [0] Total Read Transactions	: %d\n", cpMST0->Get_nARTrans());
  printf("\t [0] Total Write Transactions	: %d\n", cpMST0->Get_nAWTrans());
  printf("\t [1] Total Read Transactions	: %d\n", cpMST1->Get_nARTrans());
  printf("\t [1] Total Write Transactions	: %d\n", cpMST1->Get_nAWTrans());

  printf("---------------------------------------------\n");

  // cpMST0->PrintStat(nCycle, nullptr); // Background transactions,  do not need to check
  cpMST1->PrintStat(nCycle, nullptr);
  cpBUS->PrintStat(nCycle, nullptr);
  cpSLV->PrintStat(nCycle, nullptr);

  //---------------------
  // 8. Destruct
  //---------------------
  delete (cpMST0);
  delete (cpMST1);
  delete (cpBUS);
  delete (cpSLV);

  return (-1);
}
