//----------------------------------------------------
// Filename	 : mainMatrix.cpp
// DATE		 : 28 Jan 2023
// Contact	: duong.uitce@gmail.com
// Description  : Matrix main (all in one)
//----------------------------------------------------
//----------------------------------------------------
#include <assert.h>
#include <iostream>
#include <stdlib.h>

#include "CMMU.h"
#include "CMST.h"
#include "CSLV.h"
// #include "CBUS.h"
#include "CCache.h"
#include "CMIU.h"

#include "./Buddy/CBuddy.h" // Buddy

#ifdef PADDING_ENABLE
#define MST0_AR_VSTART 0x0000000000000000 // Source Matrix 1
#define MST0_AR_VSTART2                                                                                                \
  (MST0_AR_VSTART +                                                                                                    \
   (IMG_HORIZONTAL_SIZE + MAX_TRANS_SIZE / ELEMENT_SIZE) * IMG_VERTICAL_SIZE * ELEMENT_SIZE) // Source Matrix 2
#define MST0_AW_VSTART                                                                                                 \
  (MST0_AR_VSTART2 + (IMG_HORIZONTAL_SIZE_B + MAX_TRANS_SIZE / ELEMENT_SIZE) * IMG_VERTICAL_SIZE_B *                   \
                         ELEMENT_SIZE) // Destination Matrix 1
#define MST0_AW_VSTART2                                                                                                \
  (MST0_AW_VSTART + (IMG_HORIZONTAL_SIZE_C + MAX_TRANS_SIZE / ELEMENT_SIZE) * IMG_VERTICAL_SIZE_C *                    \
                        ELEMENT_SIZE) // Destination Matrix 2
#define MST0_AW_VSTART3                                                                                                \
  (MST0_AW_VSTART2 + (IMG_HORIZONTAL_SIZE_D + MAX_TRANS_SIZE / ELEMENT_SIZE) * IMG_VERTICAL_SIZE_D *                   \
                         ELEMENT_SIZE) // Destination Matrix 3
#else
#define MST0_AR_VSTART 0x0000000000000000                                                         // Source Matrix 1
#define MST0_AR_VSTART2 (MST0_AR_VSTART + IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE * ELEMENT_SIZE) // Source Matrix 2
#define MST0_AW_VSTART                                                                                                 \
  (MST0_AR_VSTART2 + IMG_HORIZONTAL_SIZE_B * IMG_VERTICAL_SIZE_B * ELEMENT_SIZE) // Destination Matrix 1
#define MST0_AW_VSTART2                                                                                                \
  (MST0_AW_VSTART + IMG_HORIZONTAL_SIZE_C * IMG_VERTICAL_SIZE_C * ELEMENT_SIZE) // Destination Matrix 2
#define MST0_AW_VSTART3                                                                                                \
  (MST0_AW_VSTART2 + IMG_HORIZONTAL_SIZE_D * IMG_VERTICAL_SIZE_D * ELEMENT_SIZE) // Destination Matrix 3
#endif

int main() {

  int64_t nCycle = 1;

  int SEED = clock();
  // int SEED = 2029;
  printf("SEED: %d\n", SEED);
  srand(SEED);

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
  CPMIU cpMIUCache = new CMIU("MIUCache", 1, 4);

  CPCache cpCache0 = new CCache("Cache0");
  CPCache cpCache1 = new CCache("Cache1");
  CPCache cpCache2 = new CCache("Cache2");
  CPCache cpCache3 = new CCache("Cache3");

  CPMIU cpMIUMem = new CMIU("MIUMem", 4, 4);
  CPSLV cpSLV0 = new CSLV("SLV0", 0);
  CPSLV cpSLV1 = new CSLV("SLV1", 1);
  CPSLV cpSLV2 = new CSLV("SLV2", 2);
  CPSLV cpSLV3 = new CSLV("SLV3", 3);

  //------------------------------------
  // 2. Link TRx topology
  //------------------------------------
  // MST0 and MMU0
  cpMST0->cpTx_AR->SetPair(cpMMU0->cpRx_AR);
  cpMST0->cpTx_AW->SetPair(cpMMU0->cpRx_AW);
  cpMST0->cpTx_W->SetPair(cpMMU0->cpRx_W);
  cpMMU0->cpTx_R->SetPair(cpMST0->cpRx_R);
  cpMMU0->cpTx_B->SetPair(cpMST0->cpRx_B);

  // MMU0 and MIUCache
  cpMMU0->cpTx_AR->SetPair(cpMIUCache->cpRx_AR[0]);
  cpMMU0->cpTx_AW->SetPair(cpMIUCache->cpRx_AW[0]);
  cpMMU0->cpTx_W->SetPair(cpMIUCache->cpRx_W[0]);
  cpMIUCache->cpTx_R[0]->SetPair(cpMMU0->cpRx_R);
  cpMIUCache->cpTx_B[0]->SetPair(cpMMU0->cpRx_B);

  // MIUCache and Cache0
  cpMIUCache->cpTx_AR[0]->SetPair(cpCache0->cpRx_AR);
  cpMIUCache->cpTx_AW[0]->SetPair(cpCache0->cpRx_AW);
  cpMIUCache->cpTx_W[0]->SetPair(cpCache0->cpRx_W);
  cpCache0->cpTx_R->SetPair(cpMIUCache->cpRx_R[0]);
  cpCache0->cpTx_B->SetPair(cpMIUCache->cpRx_B[0]);

  // MIUCache and Cache1
  cpMIUCache->cpTx_AR[1]->SetPair(cpCache1->cpRx_AR);
  cpMIUCache->cpTx_AW[1]->SetPair(cpCache1->cpRx_AW);
  cpMIUCache->cpTx_W[1]->SetPair(cpCache1->cpRx_W);
  cpCache1->cpTx_R->SetPair(cpMIUCache->cpRx_R[1]);
  cpCache1->cpTx_B->SetPair(cpMIUCache->cpRx_B[1]);

  // MIUCache and Cache2
  cpMIUCache->cpTx_AR[2]->SetPair(cpCache2->cpRx_AR);
  cpMIUCache->cpTx_AW[2]->SetPair(cpCache2->cpRx_AW);
  cpMIUCache->cpTx_W[2]->SetPair(cpCache2->cpRx_W);
  cpCache2->cpTx_R->SetPair(cpMIUCache->cpRx_R[2]);
  cpCache2->cpTx_B->SetPair(cpMIUCache->cpRx_B[2]);

  // MIUCache and Cache3
  cpMIUCache->cpTx_AR[3]->SetPair(cpCache3->cpRx_AR);
  cpMIUCache->cpTx_AW[3]->SetPair(cpCache3->cpRx_AW);
  cpMIUCache->cpTx_W[3]->SetPair(cpCache3->cpRx_W);
  cpCache3->cpTx_R->SetPair(cpMIUCache->cpRx_R[3]);
  cpCache3->cpTx_B->SetPair(cpMIUCache->cpRx_B[3]);

  // Cache0 and MIUMem
  cpCache0->cpTx_AR->SetPair(cpMIUMem->cpRx_AR[0]);
  cpCache0->cpTx_AW->SetPair(cpMIUMem->cpRx_AW[0]);
  cpCache0->cpTx_W->SetPair(cpMIUMem->cpRx_W[0]);
  cpMIUMem->cpTx_R[0]->SetPair(cpCache0->cpRx_R);
  cpMIUMem->cpTx_B[0]->SetPair(cpCache0->cpRx_B);

  // Cache1 and MIUMem
  cpCache1->cpTx_AR->SetPair(cpMIUMem->cpRx_AR[1]);
  cpCache1->cpTx_AW->SetPair(cpMIUMem->cpRx_AW[1]);
  cpCache1->cpTx_W->SetPair(cpMIUMem->cpRx_W[1]);
  cpMIUMem->cpTx_R[1]->SetPair(cpCache1->cpRx_R);
  cpMIUMem->cpTx_B[1]->SetPair(cpCache1->cpRx_B);

  // Cache2 and MIUMem
  cpCache2->cpTx_AR->SetPair(cpMIUMem->cpRx_AR[2]);
  cpCache2->cpTx_AW->SetPair(cpMIUMem->cpRx_AW[2]);
  cpCache2->cpTx_W->SetPair(cpMIUMem->cpRx_W[2]);
  cpMIUMem->cpTx_R[2]->SetPair(cpCache2->cpRx_R);
  cpMIUMem->cpTx_B[2]->SetPair(cpCache2->cpRx_B);

  // Cache3 and MIUMem
  cpCache3->cpTx_AR->SetPair(cpMIUMem->cpRx_AR[3]);
  cpCache3->cpTx_AW->SetPair(cpMIUMem->cpRx_AW[3]);
  cpCache3->cpTx_W->SetPair(cpMIUMem->cpRx_W[3]);
  cpMIUMem->cpTx_R[3]->SetPair(cpCache3->cpRx_R);
  cpMIUMem->cpTx_B[3]->SetPair(cpCache3->cpRx_B);

  // MIUMem and SLV0
  cpMIUMem->cpTx_AR[0]->SetPair(cpSLV0->cpRx_AR);
  cpMIUMem->cpTx_AW[0]->SetPair(cpSLV0->cpRx_AW);
  cpMIUMem->cpTx_W[0]->SetPair(cpSLV0->cpRx_W);
  cpSLV0->cpTx_R->SetPair(cpMIUMem->cpRx_R[0]);
  cpSLV0->cpTx_B->SetPair(cpMIUMem->cpRx_B[0]);

  // MIUMem and SLV1
  cpMIUMem->cpTx_AR[1]->SetPair(cpSLV1->cpRx_AR);
  cpMIUMem->cpTx_AW[1]->SetPair(cpSLV1->cpRx_AW);
  cpMIUMem->cpTx_W[1]->SetPair(cpSLV1->cpRx_W);
  cpSLV1->cpTx_R->SetPair(cpMIUMem->cpRx_R[1]);
  cpSLV1->cpTx_B->SetPair(cpMIUMem->cpRx_B[1]);

  // MIUMem and SLV2
  cpMIUMem->cpTx_AR[2]->SetPair(cpSLV2->cpRx_AR);
  cpMIUMem->cpTx_AW[2]->SetPair(cpSLV2->cpRx_AW);
  cpMIUMem->cpTx_W[2]->SetPair(cpSLV2->cpRx_W);
  cpSLV2->cpTx_R->SetPair(cpMIUMem->cpRx_R[2]);
  cpSLV2->cpTx_B->SetPair(cpMIUMem->cpRx_B[2]);

  // MIUMem and SLV3
  cpMIUMem->cpTx_AR[3]->SetPair(cpSLV3->cpRx_AR);
  cpMIUMem->cpTx_AW[3]->SetPair(cpSLV3->cpRx_AW);
  cpMIUMem->cpTx_W[3]->SetPair(cpSLV3->cpRx_W);
  cpSLV3->cpTx_R->SetPair(cpMIUMem->cpRx_R[3]);
  cpSLV3->cpTx_B->SetPair(cpMIUMem->cpRx_B[3]);

  // Debug
  cpMST0->CheckLink();
  cpMMU0->CheckLink();
  cpMIUCache->CheckLink();
  cpCache0->CheckLink();
  cpCache1->CheckLink();
  cpCache2->CheckLink();
  cpCache3->CheckLink();
  cpMIUMem->CheckLink();
  cpSLV0->CheckLink();
  cpSLV1->CheckLink();
  cpSLV2->CheckLink();
  cpSLV3->CheckLink();

//----------------------------------------------------------
// Config Buddy initial FreeList.
//----------------------------------------------------------
#ifdef BUDDY_ENABLE
  // cpBuddy->Set_FreeList();   //DUONGTRAN comment

  // DUONGTRAN add: Allign initialize FreeList
  //					  #block size	 #number of blocks
  // cpBuddy->Set_FreeList(BLOCKSIZE_INIT, NUM_TOTAL_PAGES / BLOCKSIZE_INIT);

  ////DUONGTRAN add: Missalign initialize Memmap
  // cpBuddy->InitializeMemMap(BLOCKSIZE_INIT, NUM_TOTAL_PAGES);
  cpBuddy->InitializeMemMapRandom(BLOCKSIZE_INIT, NUM_TOTAL_PAGES);
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
#ifdef PADDING_ENABLE
  int inputSize = (IMG_HORIZONTAL_SIZE + MAX_TRANS_SIZE / ELEMENT_SIZE) * IMG_VERTICAL_SIZE * ELEMENT_SIZE;
#else
  int inputSize = IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE * ELEMENT_SIZE;
#endif
  NUM_REQ_PAGES = (inputSize * 3) / 4096; // 3 Matrix
  if ((inputSize * 3) % 4096 != 0)
    NUM_REQ_PAGES += 1;
  assert(NUM_TOTAL_PAGES >= NUM_REQ_PAGES);
  cpBuddy->Do_Allocate(MST0_AR_VSTART >> 12, NUM_REQ_PAGES);

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
// Number of transactions
#ifdef MATRIX_TRANSPOSE
      // cpMST0->Set_nAR_GEN_NUM(IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE);
  // cpMST0->Set_nAW_GEN_NUM(IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE);
  cpMST0->Set_nAR_GEN_NUM((int)(ceil(IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE) / 16.0));
  cpMST0->Set_nAW_GEN_NUM((int)(ceil(IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE) / 16.0));
#elif defined MATRIX_MULTIPLICATION
  if (IMG_HORIZONTAL_SIZE != IMG_VERTICAL_SIZE_B)
    assert(0);
  if (IMG_VERTICAL_SIZE != IMG_VERTICAL_SIZE_C)
    assert(0);
  if (IMG_HORIZONTAL_SIZE_C != IMG_HORIZONTAL_SIZE_B)
    assert(0);
  // cpMST0->Set_nAR_GEN_NUM((IMG_HORIZONTAL_SIZE + IMG_VERTICAL_SIZE_B) *
  // IMG_HORIZONTAL_SIZE_B * IMG_VERTICAL_SIZE);
  // cpMST0->Set_nAW_GEN_NUM(IMG_HORIZONTAL_SIZE_C * IMG_VERTICAL_SIZE_C);

  // cpMST0->Set_nAR_GEN_NUM(IMG_VERTICAL_SIZE * ((IMG_HORIZONTAL_SIZE_B *
  // (int)(ceil(IMG_HORIZONTAL_SIZE / 16.0))) + (IMG_HORIZONTAL_SIZE_B *
  // IMG_VERTICAL_SIZE_B))); cpMST0->Set_nAW_GEN_NUM(IMG_VERTICAL_SIZE_C *
  // (int)(ceil(IMG_HORIZONTAL_SIZE_C / 16.0)));

  //// Transaction
  // cpMST0->Set_nAR_GEN_NUM(IMG_VERTICAL_SIZE_C *
  // (int)(ceil(IMG_HORIZONTAL_SIZE_C / 16.0)) * (IMG_VERTICAL_SIZE_C +
  // (int)(ceil(IMG_HORIZONTAL_SIZE_C / 16.0))));
  // cpMST0->Set_nAW_GEN_NUM(IMG_VERTICAL_SIZE_C *
  // (int)(ceil(IMG_HORIZONTAL_SIZE_C / 16.0)));

  // Tile
  cpMST0->Set_nAR_GEN_NUM(IMG_VERTICAL_SIZE_C * IMG_HORIZONTAL_SIZE_C / TILE_SIZE * (int)(ceil(TILE_SIZE / 16.0f)) *
                          (1 + IMG_HORIZONTAL_SIZE_C / TILE_SIZE));
  cpMST0->Set_nAW_GEN_NUM(IMG_VERTICAL_SIZE_C * IMG_HORIZONTAL_SIZE_C / TILE_SIZE * (int)(ceil(TILE_SIZE / 16.0f)));
#elif defined MATRIX_MULTIPLICATIONKIJ
  if (IMG_HORIZONTAL_SIZE != IMG_VERTICAL_SIZE_B)
    assert(0);
  if (IMG_VERTICAL_SIZE != IMG_VERTICAL_SIZE_C)
    assert(0);
  if (IMG_HORIZONTAL_SIZE_C != IMG_HORIZONTAL_SIZE_B)
    assert(0);
  // cpMST0->Set_nAR_GEN_NUM(IMG_VERTICAL_SIZE * IMG_VERTICAL_SIZE_B * (1 + 2 *
  // IMG_HORIZONTAL_SIZE_B)); cpMST0->Set_nAW_GEN_NUM(IMG_VERTICAL_SIZE *
  // IMG_VERTICAL_SIZE_B * IMG_HORIZONTAL_SIZE_B);

  cpMST0->Set_nAR_GEN_NUM(IMG_VERTICAL_SIZE * IMG_VERTICAL_SIZE_B *
                          (1 + 2 * (int)(ceil(IMG_HORIZONTAL_SIZE_B / 16.0))));
  cpMST0->Set_nAW_GEN_NUM(IMG_VERTICAL_SIZE * IMG_VERTICAL_SIZE_B * (int)(ceil(IMG_HORIZONTAL_SIZE_B / 16.0)));
#elif defined MATRIX_CONVOLUTION
  //// 4 6 6 6 6 6 4			  Element-by-element
  //// 6 9 9 9 9 9 6
  //// 6 9 9 9 9 9 6
  //// 6 9 9 9 9 9 6
  //// 6 9 9 9 9 9 6
  //// 6 9 9 9 9 9 6
  //// 4 6 6 6 6 6 4
  // cpMST0->Set_nAR_GEN_NUM(((IMG_HORIZONTAL_SIZE - 2) * (IMG_HORIZONTAL_SIZE -
  // 2) * 9) + (12 * (IMG_HORIZONTAL_SIZE + IMG_HORIZONTAL_SIZE - 4)) + (4 *
  // 4));   // KERNEL 3x3, so need 9 load from source before 1 store to
  // destination cpMST0->Set_nAW_GEN_NUM(IMG_HORIZONTAL_SIZE *
  // IMG_HORIZONTAL_SIZE);

  // Load/Store in transaction, computation in element-by-element
  //// 2 -------- n = [H/16] -----	n >= [H/16], n min
  //// 3 -------- n = [H/16] -----	n >= [H/16], n min
  //// 3 -------- n = [H/16] -----	n >= [H/16], n min
  //// .....
  //// 3 -------- n = [H/16] -----	n >= [H/16], n min
  //// 2 -------- n = [H/16] -----	n >= [H/16], n min
  // cpMST0->Set_nAR_GEN_NUM((int)(ceil(IMG_HORIZONTAL_SIZE / 16.0) * (2 * 2 + 3
  // * (IMG_VERTICAL_SIZE - 2))));
  // cpMST0->Set_nAW_GEN_NUM((int)(ceil(IMG_HORIZONTAL_SIZE / 16.0)) *
  // IMG_VERTICAL_SIZE);

  // Load/Store and compute in transaction
  cpMST0->Set_nAR_GEN_NUM((int)(ceil(IMG_HORIZONTAL_SIZE / 16.0) * (2 * 2 + 3 * (IMG_VERTICAL_SIZE - 2))));
  cpMST0->Set_nAW_GEN_NUM((int)(ceil(IMG_HORIZONTAL_SIZE / 16.0)) * IMG_VERTICAL_SIZE);
#elif defined MATRIX_SOBEL
  // Element-by-element
  // cpMST0->Set_nAR_GEN_NUM(((((IMG_HORIZONTAL_SIZE - 2) * (IMG_HORIZONTAL_SIZE
  // - 2) * 9) + (12 * (IMG_HORIZONTAL_SIZE + IMG_HORIZONTAL_SIZE - 4)) + (4 *
  // 4)) * 2) + (IMG_HORIZONTAL_SIZE * IMG_HORIZONTAL_SIZE * 2));
  // // 2 times in convolute, 1 time add 2 matrix
  // cpMST0->Set_nAW_GEN_NUM(IMG_HORIZONTAL_SIZE * IMG_HORIZONTAL_SIZE * 3);
  // // 3 matrix destination

  // Load/Store and compute in transaction
  cpMST0->Set_nAR_GEN_NUM((int)(ceil(IMG_HORIZONTAL_SIZE / 16.0) * (2 * 2 + 3 * (IMG_VERTICAL_SIZE - 2))));
  cpMST0->Set_nAW_GEN_NUM((int)(ceil(IMG_HORIZONTAL_SIZE / 16.0)) * IMG_VERTICAL_SIZE);
#else
  assert(0);
#endif

  //------------------------------
  // Set image scaling
  //------------------------------
  // cpMST0->Set_ScalingFactor(1);

  //------------------------------
  // Set start address MST
  //------------------------------
  cpMST0->Set_nAR_START_ADDR(MST0_AR_VSTART);
  cpMST0->Set_nAR_START_ADDR2(MST0_AR_VSTART2);
  cpMST0->Set_nAW_START_ADDR(MST0_AW_VSTART);
  cpMST0->Set_nAW_START_ADDR2(MST0_AW_VSTART2);
  cpMST0->Set_nAW_START_ADDR3(MST0_AW_VSTART3);

  //------------------------------
  // Set start address MMU. RMM
  //------------------------------
  // cpMMU0->Set_nAR_START_ADDR(MST0_AR_VSTART);
  // cpMMU0->Set_nAW_START_ADDR(MST0_AW_VSTART);

  //------------------------------
  // Set start VPN
  //	Buddy pagetable
  //	Assume single thread read or write FIXME
  //------------------------------

  cpMMU0->Set_START_VPN(MST0_AR_VSTART >> 12);

  ////------------------------------
  //// Set Ax issue interval cycles
  ////------------------------------
  // cpMST0->Set_AR_ISSUE_MIN_INTERVAL(1);	// DUONGTRAN FIXME ??? Why? When
  // reset then it become 4 cpMST0->Set_AW_ISSUE_MIN_INTERVAL(1);

  // cpMST3->Set_AR_ISSUE_MIN_INTERVAL(1);
  // cpMST3->Set_AW_ISSUE_MIN_INTERVAL(1);

  //------------------------------
  // Set CACHE interleaving
  //------------------------------
  cpMIUCache->Set_CACHE_MIU_MODE(ERESULT_TYPE_YES);

  // Simulate
  while (1) {

    //---------------------------
    // 3. Reset
    //---------------------------
    if (nCycle == 1) {
      cpMST0->Reset();
      cpMMU0->Reset();
      cpMIUCache->Reset();
      cpCache0->Reset();
      cpCache1->Reset();
      cpCache2->Reset();
      cpCache3->Reset();
      cpMIUMem->Reset();
      cpSLV0->Reset();
      cpSLV1->Reset();
      cpSLV2->Reset();
      cpSLV3->Reset();
    };

//---------------------------------------------
// 4. Start simulation
//---------------------------------------------

// Master geneates transactions
#ifdef MATRIX_TRANSPOSE
    // cpMST0->LoadTransfer_MatrixTranspose(nCycle);
    cpMST0->LoadTransfer_MatrixTransposeTransaction(nCycle);
#elif defined MATRIX_MULTIPLICATION
    // cpMST0->LoadTransfer_MatrixMultiplication(nCycle);
    // cpMST0->LoadTransfer_MatrixMultiplicationTransaction(nCycle);
    cpMST0->LoadTransfer_MatrixMultiplicationTileTransaction(nCycle);
#elif defined MATRIX_MULTIPLICATIONKIJ
    cpMST0->LoadTransfer_MatrixMultiplicationKIJ(nCycle);
#elif defined MATRIX_CONVOLUTION
    // cpMST0->LoadTransfer_MatrixConvolution(nCycle);
    cpMST0->LoadTransfer_MatrixConvolutionTransaction(nCycle);
#elif defined MATRIX_SOBEL
    // cpMST0->LoadTransfer_MatrixSobel(nCycle);
    cpMST0->LoadTransfer_MatrixSobelTransaction(nCycle);
#else
    assert(0);
#endif

    //---------------------------------
    // Propagate valid
    //---------------------------------

    //---------------------------------
    // AR, AW, W VALID
    //---------------------------------
    cpMST0->Do_AR_fwd(nCycle); // DUONGTRAN comment
    cpMST0->Do_AW_fwd(nCycle);
// cpMST0->Do_W_fwd(nCycle);

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
#else // PCAD, TRAD
    cpMMU0->Do_AR_fwd_SI(nCycle);
    cpMMU0->Do_AR_fwd_MI(nCycle);
    cpMMU0->Do_AW_fwd(nCycle);
#endif
#endif

    //-------------------------------
    cpMIUCache->Do_AR_fwd(nCycle);
    cpMIUCache->Do_AW_fwd(nCycle);
// cpMIUCache->Do_W_fwd(nCycle);

//--------------------------------
#ifdef Cache_OFF
    cpCache0->Do_AR_fwd_Cache_OFF(nCycle);
    cpCache0->Do_AW_fwd_Cache_OFF(nCycle);
// cpCache0->Do_W_fwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache0->Do_AR_fwd_SI(nCycle);
    cpCache0->Do_AR_fwd_MI(nCycle);
    cpCache0->Do_AW_fwd_SI(nCycle);
    cpCache0->Do_AW_fwd_MI(nCycle);
// cpCache0->Do_W_fwd(nCycle);
#endif

//--------------------------------
#ifdef Cache_OFF
    cpCache1->Do_AR_fwd_Cache_OFF(nCycle);
    cpCache1->Do_AW_fwd_Cache_OFF(nCycle);
// cpCache1->Do_W_fwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache1->Do_AR_fwd_SI(nCycle);
    cpCache1->Do_AR_fwd_MI(nCycle);
    cpCache1->Do_AW_fwd_SI(nCycle);
    cpCache1->Do_AW_fwd_MI(nCycle);
// cpCache1->Do_W_fwd(nCycle);
#endif

//--------------------------------
#ifdef Cache_OFF
    cpCache2->Do_AR_fwd_Cache_OFF(nCycle);
    cpCache2->Do_AW_fwd_Cache_OFF(nCycle);
// cpCache2->Do_W_fwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache2->Do_AR_fwd_SI(nCycle);
    cpCache2->Do_AR_fwd_MI(nCycle);
    cpCache2->Do_AW_fwd_SI(nCycle);
    cpCache2->Do_AW_fwd_MI(nCycle);
// cpCache2->Do_W_fwd(nCycle);
#endif

#ifdef Cache_OFF
    cpCache3->Do_AR_fwd_Cache_OFF(nCycle);
    cpCache3->Do_AW_fwd_Cache_OFF(nCycle);
// cpCache3->Do_W_fwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache3->Do_AR_fwd_SI(nCycle);
    cpCache3->Do_AR_fwd_MI(nCycle);
    cpCache3->Do_AW_fwd_SI(nCycle);
    cpCache3->Do_AW_fwd_MI(nCycle);
// cpCache3->Do_W_fwd(nCycle);
#endif

    //-------------------------------
    cpMIUMem->Do_AR_fwd(nCycle);
    cpMIUMem->Do_AW_fwd(nCycle);
// cpMIUMem->Do_W_fwd(nCycle);

//-------------------------------
#ifdef IDEAL_MEMORY
    cpSLV0->Do_AR_fwd(nCycle);
    cpSLV1->Do_AR_fwd(nCycle);
    cpSLV2->Do_AR_fwd(nCycle);
    cpSLV3->Do_AR_fwd(nCycle);
#endif

#ifdef MEMORY_CONTROLLER
    cpSLV0->Do_AR_fwd_MC_Frontend(nCycle);
    cpSLV1->Do_AR_fwd_MC_Frontend(nCycle);
    cpSLV2->Do_AR_fwd_MC_Frontend(nCycle);
    cpSLV3->Do_AR_fwd_MC_Frontend(nCycle);
#endif

#ifdef IDEAL_MEMORY
    cpSLV0->Do_AW_fwd(nCycle);
    cpSLV1->Do_AW_fwd(nCycle);
    cpSLV2->Do_AW_fwd(nCycle);
    cpSLV3->Do_AW_fwd(nCycle);
// cpSLV->Do_W_fwd(nCycle);
#endif

#ifdef MEMORY_CONTROLLER
    cpSLV0->Do_AW_fwd_MC_Frontend(nCycle);
    // cpSLV0->Do_W_fwd_MC_Frontend(nCycle);

    cpSLV1->Do_AW_fwd_MC_Frontend(nCycle);
    // cpSLV1->Do_W_fwd_MC_Frontend(nCycle);

    cpSLV2->Do_AW_fwd_MC_Frontend(nCycle);
    // cpSLV2->Do_W_fwd_MC_Frontend(nCycle);

    cpSLV3->Do_AW_fwd_MC_Frontend(nCycle);
// cpSLV3->Do_W_fwd_MC_Frontend(nCycle);
#endif

#ifdef MEMORY_CONTROLLER
    cpSLV0->Do_Ax_fwd_MC_Backend_Request(nCycle);
    cpSLV1->Do_Ax_fwd_MC_Backend_Request(nCycle);
    cpSLV2->Do_Ax_fwd_MC_Backend_Request(nCycle);
    cpSLV3->Do_Ax_fwd_MC_Backend_Request(nCycle);
#endif

#ifdef MEMORY_CONTROLLER
    cpSLV0->Do_AR_fwd_MC_Backend_Response(nCycle);
    cpSLV1->Do_AR_fwd_MC_Backend_Response(nCycle);
    cpSLV2->Do_AR_fwd_MC_Backend_Response(nCycle);
    cpSLV3->Do_AR_fwd_MC_Backend_Response(nCycle);
#endif

#ifdef MEMORY_CONTROLLER
    cpSLV0->Do_AW_fwd_MC_Backend_Response(nCycle);
    cpSLV1->Do_AW_fwd_MC_Backend_Response(nCycle);
    cpSLV2->Do_AW_fwd_MC_Backend_Response(nCycle);
    cpSLV3->Do_AW_fwd_MC_Backend_Response(nCycle);
#endif

    //---------------------------------
    // R, B VALID
    //---------------------------------
    //---------------------------------
    cpSLV0->Do_R_fwd(nCycle);
    cpSLV0->Do_B_fwd(nCycle);

    cpSLV1->Do_R_fwd(nCycle);
    cpSLV1->Do_B_fwd(nCycle);

    cpSLV2->Do_R_fwd(nCycle);
    cpSLV2->Do_B_fwd(nCycle);

    cpSLV3->Do_R_fwd(nCycle);
    cpSLV3->Do_B_fwd(nCycle);

    cpMIUMem->Do_R_fwd_Arb(nCycle);
    cpMIUMem->Do_R_fwd_SI(nCycle);
    cpMIUMem->Do_B_fwd_Arb(nCycle);
    cpMIUMem->Do_B_fwd_SI(nCycle);

//---------------------------
#ifdef Cache_OFF
    cpCache0->Do_R_fwd_Cache_OFF(nCycle);
    cpCache0->Do_B_fwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache0->Do_R_fwd_MI(nCycle);
    cpCache0->Do_R_fwd_SI(nCycle);
    cpCache0->Do_B_fwd_MI(nCycle);
    cpCache0->Do_B_fwd_SI(nCycle);
#endif

//---------------------------
#ifdef Cache_OFF
    cpCache1->Do_R_fwd_Cache_OFF(nCycle);
    cpCache1->Do_B_fwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache1->Do_R_fwd_MI(nCycle);
    cpCache1->Do_R_fwd_SI(nCycle);
    cpCache1->Do_B_fwd_MI(nCycle);
    cpCache1->Do_B_fwd_SI(nCycle);
#endif

//---------------------------
#ifdef Cache_OFF
    cpCache2->Do_R_fwd_Cache_OFF(nCycle);
    cpCache2->Do_B_fwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache2->Do_R_fwd_MI(nCycle);
    cpCache2->Do_R_fwd_SI(nCycle);
    cpCache2->Do_B_fwd_MI(nCycle);
    cpCache2->Do_B_fwd_SI(nCycle);
#endif

#ifdef Cache_OFF
    cpCache3->Do_R_fwd_Cache_OFF(nCycle);
    cpCache3->Do_B_fwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache3->Do_R_fwd_MI(nCycle);
    cpCache3->Do_R_fwd_SI(nCycle);
    cpCache3->Do_B_fwd_MI(nCycle);
    cpCache3->Do_B_fwd_SI(nCycle);
#endif

    //---------------------------
    cpMIUCache->Do_R_fwd_Arb(nCycle);
    cpMIUCache->Do_R_fwd_SI(nCycle);
    cpMIUCache->Do_B_fwd_Arb(nCycle);
    cpMIUCache->Do_B_fwd_SI(nCycle);

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

    //---------------------------
    cpMST0->Do_R_fwd(nCycle);
    cpMST0->Do_B_fwd(nCycle);

    //---------------------------------
    // Propagate ready
    //---------------------------------

    //---------------------------------
    // AR, AW, W READY
    //---------------------------------
    cpSLV0->Do_AR_bwd(nCycle);
    cpSLV0->Do_AW_bwd(nCycle);
    // cpSLV0->Do_W_bwd(nCycle);

    cpSLV1->Do_AR_bwd(nCycle);
    cpSLV1->Do_AW_bwd(nCycle);
    // cpSLV1->Do_W_bwd(nCycle);

    cpSLV2->Do_AR_bwd(nCycle);
    cpSLV2->Do_AW_bwd(nCycle);
    // cpSLV2->Do_W_bwd(nCycle);

    cpSLV3->Do_AR_bwd(nCycle);
    cpSLV3->Do_AW_bwd(nCycle);
    // cpSLV3->Do_W_bwd(nCycle);

    cpMIUMem->Do_AR_bwd_MI(nCycle);
    cpMIUMem->Do_AR_bwd_SI(nCycle);
    cpMIUMem->Do_AW_bwd_MI(nCycle);
    cpMIUMem->Do_AW_bwd_SI(nCycle);
// cpMIUMem->Do_W_bwd(nCycle);

//-----------------------------
#ifdef Cache_OFF
    cpCache0->Do_AR_bwd_Cache_OFF(nCycle);
    cpCache0->Do_AW_bwd_Cache_OFF(nCycle);
// cpCache0->Do_W_bwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache0->Do_AR_bwd(nCycle);
    cpCache0->Do_AW_bwd(nCycle);
// cpCache0->Do_W_bwd(nCycle);
#endif

//-----------------------------
#ifdef Cache_OFF
    cpCache1->Do_AR_bwd_Cache_OFF(nCycle);
    cpCache1->Do_AW_bwd_Cache_OFF(nCycle);
// cpCache1->Do_W_bwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache1->Do_AR_bwd(nCycle);
    cpCache1->Do_AW_bwd(nCycle);
// cpCache1->Do_W_bwd(nCycle);
#endif

//-----------------------------
#ifdef Cache_OFF
    cpCache2->Do_AR_bwd_Cache_OFF(nCycle);
    cpCache2->Do_AW_bwd_Cache_OFF(nCycle);
// cpCache2->Do_W_bwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache2->Do_AR_bwd(nCycle);
    cpCache2->Do_AW_bwd(nCycle);
// cpCache2->Do_W_bwd(nCycle);
#endif

#ifdef Cache_OFF
    cpCache3->Do_AR_bwd_Cache_OFF(nCycle);
    cpCache3->Do_AW_bwd_Cache_OFF(nCycle);
// cpCache3->Do_W_bwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache3->Do_AR_bwd(nCycle);
    cpCache3->Do_AW_bwd(nCycle);
// cpCache3->Do_W_bwd(nCycle);
#endif

    //-------------------------------
    cpMIUCache->Do_AR_bwd_MI(nCycle);
    cpMIUCache->Do_AR_bwd_SI(nCycle);
    cpMIUCache->Do_AW_bwd_MI(nCycle);
    cpMIUCache->Do_AW_bwd_SI(nCycle);

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

    //-----------------------------
    cpMST0->Do_AR_bwd(nCycle);
    cpMST0->Do_AW_bwd(nCycle);
    // cpMST0->Do_W_bwd(nCycle);

    //---------------------------------
    // R, B READY
    //---------------------------------

    cpMST0->Do_R_bwd(nCycle);
    cpMST0->Do_B_bwd(nCycle);

//---------------------------
#ifdef MMU_OFF
    cpMMU0->Do_R_bwd_MMU_OFF(nCycle);
    cpMMU0->Do_B_bwd_MMU_OFF(nCycle);
#endif
#ifdef MMU_ON
    cpMMU0->Do_R_bwd(nCycle);
    cpMMU0->Do_B_bwd(nCycle);
#endif

    //---------------------------
    cpMIUCache->Do_R_bwd_SI(nCycle);
    cpMIUCache->Do_R_bwd_MI(nCycle);
    cpMIUCache->Do_B_bwd_SI(nCycle);
    cpMIUCache->Do_B_bwd_MI(nCycle);

//---------------------------
#ifdef Cache_OFF
    cpCache0->Do_R_bwd_Cache_OFF(nCycle);
    cpCache0->Do_B_bwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache0->Do_R_bwd_SI(nCycle);
    cpCache0->Do_R_bwd_MI(nCycle);
    cpCache0->Do_B_bwd_SI(nCycle);
    cpCache0->Do_B_bwd_MI(nCycle);
#endif

//---------------------------
#ifdef Cache_OFF
    cpCache1->Do_R_bwd_Cache_OFF(nCycle);
    cpCache1->Do_B_bwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache1->Do_R_bwd_SI(nCycle);
    cpCache1->Do_R_bwd_MI(nCycle);
    cpCache1->Do_B_bwd_SI(nCycle);
    cpCache1->Do_B_bwd_MI(nCycle);
#endif

//---------------------------
#ifdef Cache_OFF
    cpCache2->Do_R_bwd_Cache_OFF(nCycle);
    cpCache2->Do_B_bwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache2->Do_R_bwd_SI(nCycle);
    cpCache2->Do_R_bwd_MI(nCycle);
    cpCache2->Do_B_bwd_SI(nCycle);
    cpCache2->Do_B_bwd_MI(nCycle);
#endif

#ifdef Cache_OFF
    cpCache3->Do_R_bwd_Cache_OFF(nCycle);
    cpCache3->Do_B_bwd_Cache_OFF(nCycle);
#endif
#ifdef Cache_ON
    cpCache3->Do_R_bwd_SI(nCycle);
    cpCache3->Do_R_bwd_MI(nCycle);
    cpCache3->Do_B_bwd_SI(nCycle);
    cpCache3->Do_B_bwd_MI(nCycle);
#endif

    //---------------------------
    cpMIUMem->Do_R_bwd_SI(nCycle);
    cpMIUMem->Do_R_bwd_MI(nCycle);
    cpMIUMem->Do_B_bwd_SI(nCycle);
    cpMIUMem->Do_B_bwd_MI(nCycle);

    cpSLV0->Do_R_bwd(nCycle);
    cpSLV0->Do_B_bwd(nCycle);

    cpSLV1->Do_R_bwd(nCycle);
    cpSLV1->Do_B_bwd(nCycle);

    cpSLV2->Do_R_bwd(nCycle);
    cpSLV2->Do_B_bwd(nCycle);

    cpSLV3->Do_R_bwd(nCycle);
    cpSLV3->Do_B_bwd(nCycle);

    //---------------------------------
    // 5. Update state
    //---------------------------------
    cpMST0->UpdateState(nCycle);
    cpMMU0->UpdateState(nCycle);
    cpMIUCache->UpdateState(nCycle);
    cpCache0->UpdateState(nCycle);
    cpCache1->UpdateState(nCycle);
    cpCache2->UpdateState(nCycle);
    cpCache3->UpdateState(nCycle);
    cpMIUMem->UpdateState(nCycle);
    cpSLV0->UpdateState(nCycle);
    cpSLV1->UpdateState(nCycle);
    cpSLV2->UpdateState(nCycle);
    cpSLV3->UpdateState(nCycle);

    //---------------------------------
    // 6. Check simulation finish
    //---------------------------------
    if (cpMST0->IsAWTransFinished() == ERESULT_TYPE_YES and cpMST0->IsARTransFinished() == ERESULT_TYPE_YES) {

      //--------------------------------------------------------------
      printf("[Cycle %3ld] Simulation is finished. \n", nCycle);
      printf("---------------------------------------------\n");

      printf("---------------------------------------------\n");
      printf("\t Parameters \n");
      printf("---------------------------------------------\n");
      // printf("\t AR_GEN_NUM		: %d\n", AR_GEN_NUM);
      // printf("\t AW_GEN_NUM		: %d\n", AW_GEN_NUM);
      printf("\t IMG_HORIZONTAL_SIZE		: %d\n", IMG_HORIZONTAL_SIZE);
      printf("\t IMG_VERTICAL_SIZE		: %d\n", IMG_VERTICAL_SIZE);
      printf("\t MAX_MO_COUNT			: %d\n", MAX_MO_COUNT);
      // printf("\t AR_ADDR_INCREMENT		: 0x%x\n", AR_ADDR_INC);
      // printf("\t AW_ADDR_INCREMENT		: 0x%x\n", AW_ADDR_INC);

#ifdef Cache_OFF
      printf("\t Cache			: OFF \n");
#endif
#ifdef Cache_ON
      printf("\t Cache			: ON \n");
#endif

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

// #ifdef MMU_ON
//  cpMMU0->PrintStat(nCycle, fp);
//  //cpMMU1->PrintStat(nCycle, fp);
//  //cpMMU2->PrintStat(nCycle, fp);
//  //cpMMU3->PrintStat(nCycle, fp);
// #endif

// cpMIUCache->PrintStat(nCycle, fp);

// #ifdef Cache_ON
// cpCache0->PrintStat(nCycle, fp);
// cpCache1->PrintStat(nCycle, fp);
// cpCache2->PrintStat(nCycle, fp);
// cpCache3->PrintStat(nCycle, fp);
// #endif

// cpMIUMem->PrintStat(nCycle, fp);

// cpSLV0->PrintStat(nCycle, fp);
// cpSLV1->PrintStat(nCycle, fp);
// cpSLV2->PrintStat(nCycle, fp);
// cpSLV3->PrintStat(nCycle, fp);

// Get total avg TLB hit rate
#ifdef MMU_ON
      // int Total_nAR_SI = cpMMU0->Get_nAR_SI() + cpMMU1->Get_nAR_SI() +
      // cpMMU2->Get_nAR_SI() + cpMMU3->Get_nAR_SI();
      int Total_nAR_SI = cpMMU0->Get_nAR_SI();
      // int Total_nAW_SI = cpMMU0->Get_nAW_SI() + cpMMU1->Get_nAW_SI() +
      // cpMMU2->Get_nAW_SI() + cpMMU3->Get_nAW_SI();
      int Total_nAW_SI = cpMMU0->Get_nAW_SI();
      // int Total_nHit_AR_TLB = cpMMU0->Get_nHit_AR_TLB() +
      // cpMMU1->Get_nHit_AR_TLB() + cpMMU2->Get_nHit_AR_TLB() +
      // cpMMU3->Get_nHit_AR_TLB();
      int Total_nHit_AR_TLB = cpMMU0->Get_nHit_AR_TLB();
      // int Total_nHit_AW_TLB = cpMMU0->Get_nHit_AW_TLB() +
      // cpMMU1->Get_nHit_AW_TLB() + cpMMU2->Get_nHit_AW_TLB() +
      // cpMMU3->Get_nHit_AW_TLB();
      int Total_nHit_AW_TLB = cpMMU0->Get_nHit_AW_TLB();

      float Avg_TLB_hit_rate = (float)(Total_nHit_AR_TLB + Total_nHit_AW_TLB) / (Total_nAR_SI + Total_nAW_SI);

      // Get TLB reach
      float Avg_TLB_reach = (float)(cpMMU0->GetTLB_reach(nCycle));

      int total_nPTW = cpMMU0->nPTW_total + cpMMU0->nAPTW_total_AT;

      printf("---------------------------------------------\n");
      printf("\t Total Avg TLB hit rate = %1.5f\n", Avg_TLB_hit_rate);
      printf("\t Total Avg TLB reach	= %1.5f\n", Avg_TLB_reach);
      printf("\t Total PTW			  = %d\n", total_nPTW);
      printf("---------------------------------------------\n");

#endif // MMU_ON

      // DUONGTRAN comment
      // cpBUS ->PrintStat(nCycle, fp);
      // cpSLV ->PrintStat(nCycle, fp);

#ifdef FILE_OUT
      fclose(fp);
#endif

      break;
    };

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

//----------------------------------------------------------
// De-allocate Buddy
// 	FIXME Manage BlockList.
//----------------------------------------------------------
#ifdef BUDDY_ENABLE
  cpBuddy->Do_Deallocate(MST0_AR_VSTART >> 12);
#endif

  //---------------------
  // 8. Destruct
  //---------------------
  delete (cpMST0);
  delete (cpMMU0);
  delete (cpMIUCache);
  delete (cpCache0);
  delete (cpCache1);
  delete (cpCache2);
  delete (cpCache3);
  delete (cpMIUMem);
  delete (cpSLV0);
  delete (cpSLV1);
  delete (cpSLV2);
  delete (cpSLV3);

#ifdef BUDDY_ENABLE
  delete (cpBuddy);
#endif

  return (-1);
}
