//----------------------------------------------------
// Filename     : main.cpp
// DATE         : 25 Apr 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description  : main 
// MST3: Display read (standalone)
//----------------------------------------------------
//----------------------------------------------------
#include <stdlib.h>
#include <assert.h>
#include <iostream>

#include "CMST.h"
#include "CSLV.h"
#include "CMMU.h"
#include "CBUS.h"
#include "CMIU.h"
#include "CCache.h"

#include "./Buddy/CBuddy.h"	// Buddy

#define MST3_AR_VSTART 0x0000000000000000

int main() {

	int64_t nCycle = 1;
    //int SEED = clock();
    int SEED = SEED_INIT;
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
	CPMST	cpMST0	= new CMST("MST0");
	CPMST	cpMST1	= new CMST("MST1");
	CPMST	cpMST2	= new CMST("MST2");
	CPMST	cpMST3	= new CMST("MST3");

	CPMMU	cpMMU0	= new CMMU("MMU0");
	CPMMU	cpMMU1	= new CMMU("MMU1");
	CPMMU	cpMMU2	= new CMMU("MMU2");
	CPMMU	cpMMU3	= new CMMU("MMU3");

	CPBUS   cpBUS  = new CBUS("BUS", 4);

	CPCache cpCacheL3 = new CCache("CacheL3");

	CPSLV	cpSLV	= new CSLV("SLV", 0);

	//------------------------------------
	// 2. Link TRx topology
	//------------------------------------
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

  // BUS and CacheL3
  cpBUS->cpTx_AR->SetPair(cpCacheL3->cpRx_AR);
  cpBUS->cpTx_AW->SetPair(cpCacheL3->cpRx_AW);
  cpBUS->cpTx_W->SetPair(cpCacheL3->cpRx_W);
  cpCacheL3->cpTx_R->SetPair(cpBUS->cpRx_R);
  cpCacheL3->cpTx_B->SetPair(cpBUS->cpRx_B);

   // CacheL3 and SLV
   cpCacheL3->cpTx_AR->SetPair(cpSLV->cpRx_AR);
   cpCacheL3->cpTx_AW->SetPair(cpSLV->cpRx_AW);
   cpCacheL3->cpTx_W->SetPair(cpSLV->cpRx_W);
   cpSLV->cpTx_R->SetPair(cpCacheL3->cpRx_R);
   cpSLV->cpTx_B->SetPair(cpCacheL3->cpRx_B);  
  
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
  cpCacheL3->CheckLink();
  cpSLV->CheckLink();


	//----------------------------------------------------------
	// Config Buddy initial FreeList.
	//----------------------------------------------------------
	#ifdef BUDDY_ENABLE
		// cpBuddy->Set_FreeList();   //DUONGTRAN comment

		// DUONGTRAN add: Allign initialize FreeList
		//                      #block size     #number of blocks
		//cpBuddy->Set_FreeList(BLOCKSIZE_INIT, NUM_TOTAL_PAGES / BLOCKSIZE_INIT);

		//////DUONGTRAN add: Missalign initialize Memmap
		//cpBuddy->InitializeMemMap(BLOCKSIZE_INIT, NUM_TOTAL_PAGES);
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
		SPPTE* spPageTable = NULL; 
	
	
		//-------------------
		// MST3 AR
		//-------------------
		// Allocate
		NUM_REQ_PAGES = IMG_HORIZONTAL_SIZE*IMG_VERTICAL_SIZE*BYTE_PER_PIXEL/4096;
		cpBuddy->Do_Allocate(MST3_AR_VSTART >> 12, NUM_REQ_PAGES);
		
		// Get page-table
		spPageTable = cpBuddy->Get_PageTable();
		
		// Set page-table
		cpMMU3->Set_PageTable(spPageTable);
	
		#ifdef AT_ENABLE
		cpMMU3->anchorDistance = cpBuddy->anchorDistance;	//DUONGTRAN add
		#endif
	
		cpCacheL3->Set_PageTable(spPageTable);
		//cpCacheL3->START_VPN = MST3_AR_VSTART;

	#endif


	//------------------------------
	// Set total Ax NUM 
	//------------------------------
	#ifdef TILE
		int NUM = (IMG_HORIZONTAL_SIZE*IMG_VERTICAL_SIZE*BYTE_PER_PIXEL/MAX_TRANS_SIZE)*(TILE_SIZE*TILE_SIZE);
	#else
		int NUM = IMG_HORIZONTAL_SIZE*IMG_VERTICAL_SIZE*BYTE_PER_PIXEL/MAX_TRANS_SIZE;
	#endif

	// Number of transactions
	cpMST3->Set_nAR_GEN_NUM(0);
	cpMST3->Set_nAW_GEN_NUM(0);	// DUONGTRAN comment
	

	cpMST3->Set_nAR_GEN_NUM(NUM);


	//------------------------------
	// Set image scaling
	//------------------------------
	cpMST3->Set_ScalingFactor(1);


	//------------------------------
	// Set start address MST
	//------------------------------

    cpMST3->Set_nAR_START_ADDR(MST3_AR_VSTART);
	//cpMST3->Set_nAW_START_ADDR(MST3_AW_VSTART);   // DUONGTRAN comment

	////------------------------------
	//// Set start address MMU. RMM
	////------------------------------

	//cpMMU3->Set_nAR_START_ADDR(MST3_AR_VSTART);
	//cpMMU3->Set_nAW_START_ADDR(MST3_AW_VSTART);


	//------------------------------
	// Set start VPN 
	//	Buddy pag-table
	//	Assume single thread read or write FIXME
	//------------------------------

	cpMMU3->Set_START_VPN(MST3_AR_VSTART >> 12);


	//------------------------------
	// Set operation 
	//------------------------------
	// RASTER_SCAN, ROTATION, RANDOM
	//------------------------------
	#ifdef RASTER_SCAN
		cpMST3->Set_AR_Operation("RASTER_SCAN");
		cpMST3->Set_AW_Operation("RASTER_SCAN");
	#elif ROTATION
		cpMST3->Set_AR_Operation("ROTATION");
		cpMST3->Set_AW_Operation("RASTER_SCAN");
	#elif TILE
		cpMST3->Set_AR_Operation("TILE");
		cpMST3->Set_AW_Operation("TILE");
	#else
		assert(0);
	#endif

	////------------------------------
	//// Set Ax issue interval cycles 
	////------------------------------
	//cpMST0->Set_AR_ISSUE_MIN_INTERVAL(1);    // DUONGTRAN FIXME ??? Why? When reset then it become 4
	//cpMST0->Set_AW_ISSUE_MIN_INTERVAL(1);

	//cpMST3->Set_AR_ISSUE_MIN_INTERVAL(1);
	//cpMST3->Set_AW_ISSUE_MIN_INTERVAL(1);


	//------------------------------
	// Set CACHE interleaving
	//------------------------------
	//cpBUS->Set_CACHE_MIU_MODE(ERESULT_TYPE_YES);


	//------------------------------
	// Set AddrMap
	//------------------------------
	string cAR_AddrMap;
	string cAW_AddrMap;
	
	
	#if defined(AR_LIAM)
	cAR_AddrMap = "LIAM";
	#elif defined(AR_BFAM) 
	cAR_AddrMap = "BFAM";
	#elif defined(AR_BGFAM)
	cAR_AddrMap = "BGFAM";
	#elif defined(AR_TILE)
	cAR_AddrMap = "TILE";
	#elif defined(AR_OIRAM)
	cAR_AddrMap = "OIRAM";
	#elif defined(AR_MIN_K_UNION)
	cAR_AddrMap = "MIN_K_UNION";
	#elif defined(AR_FLATFISH)
	cAR_AddrMap = "FLATFISH";
	#elif defined(AR_NEAR_OPTIMAL)
		#ifdef RASTER_SCAN
			cAR_AddrMap = "NEAR_OPTIMAL_ROW_WISE";
		#elif ROTATION
			cAR_AddrMap = "NEAR_OPTIMAL_COL_WISE";
		#else
			assert(0);
		#endif
	#endif
	
	#if defined(AW_LIAM)
	cAW_AddrMap = "LIAM";
	#elif defined(AW_BFAM)
	cAW_AddrMap = "BFAM";
	#elif defined(AW_BGFAM)
	cAW_AddrMap = "BGFAM";
	#elif defined(AW_TILE)
	cAW_AddrMap = "TILE";
	#elif defined(AW_OIRAM)
	cAW_AddrMap = "OIRAM";
	#elif defined(AW_MIN_K_UNION)
	cAW_AddrMap = "MIN_K_UNION";
	#elif defined(AW_FLATFISH)
	cAW_AddrMap = "FLATFISH";
	#elif defined(AW_NEAR_OPTIMAL)
		#ifdef RASTER_SCAN
			cAW_AddrMap = "NEAR_OPTIMAL_ROW_WISE";
		#elif ROTATION
			cAW_AddrMap = "NEAR_OPTIMAL_COL_WISE";
		#else
			assert(0);
		#endif
	#endif

	// Simulate
	while (1) {
		
		//---------------------------
		// 3. Reset
		//---------------------------
		if (nCycle == 1) {
			cpMST3->Reset();
			cpMMU3->Reset();
			cpBUS->Reset();
			cpCacheL3->Reset();
			cpSLV->Reset();
		};
		//---------------------------------------------
		// 4. Start simulation
		//---------------------------------------------
		// Master geneates transactions
		#ifdef RASTER_SCAN
			cpMST3->LoadTransfer_AR(nCycle, cAR_AddrMap, "RASTER_SCAN");
			//cpMST3->LoadTransfer_AW(nCycle, cAW_AddrMap, "RASTER_SCAN");  // DUONGTRAN comment
		#elif ROTATION
			cpMST3->LoadTransfer_AR(nCycle, cAR_AddrMap, "ROTATION");
			//cpMST3->LoadTransfer_AW(nCycle, cAW_AddrMap, "ROTATION");  // DUONGTRAN comment
		#elif TILE
			cpMST3->LoadTransfer_AR(nCycle, cAR_AddrMap, "TILE");
			//cpMST3->LoadTransfer_AW(nCycle, cAW_AddrMap, "ROTATION");  // DUONGTRAN comment
		#else
			assert(0);
		#endif
		
		//---------------------------------
		// Propagate valid
		//---------------------------------
		//---------------------------------
		// AR, AW, W VALID
		//---------------------------------
		cpMST3->Do_AR_fwd(nCycle);
		//cpMST3->Do_AW_fwd(nCycle);   // DUONGTRAN comment
		// cpMST3->Do_W_fwd(nCycle);
		//--------------------------------
		#ifdef MMU_OFF	
		cpMMU3->Do_AR_fwd_MMU_OFF(nCycle);
		cpMMU3->Do_AW_fwd_MMU_OFF(nCycle); 
		#endif
		
		// DUONGTRAN add
		#ifdef MMU_ON
			#ifdef AT_ENABLE
				cpMMU3->Do_AR_fwd_SI_AT(nCycle);
				cpMMU3->Do_AR_fwd_MI(nCycle);
			#else // PCAD, TRAD
				cpMMU3->Do_AR_fwd_SI(nCycle);
				cpMMU3->Do_AR_fwd_MI(nCycle);
			#endif
		#endif
		//-------------------------------
		cpBUS->Do_AR_fwd(nCycle);
		cpBUS->Do_AW_fwd(nCycle);
		// cpBUS->Do_W_fwd(nCycle);
		//--------------------------------
		#ifdef Cache_OFF	
		cpCacheL3->Do_AR_fwd_Cache_OFF(nCycle);
		cpCacheL3->Do_AW_fwd_Cache_OFF(nCycle);
		// cpCacheL3->Do_W_fwd_Cache_OFF(nCycle);
		#endif
		#ifdef Cache_ON
		cpCacheL3->Do_AR_fwd_SI(nCycle);
		cpCacheL3->Do_AR_fwd_MI(nCycle);
		cpCacheL3->Do_AW_fwd_SI(nCycle);
		cpCacheL3->Do_AW_fwd_MI(nCycle);
		// cpCacheL3->Do_W_fwd(nCycle);
		#endif
		//-------------------------------
		#ifdef IDEAL_MEMORY
		cpSLV->Do_AR_fwd(nCycle);
		cpSLV->Do_AW_fwd(nCycle);
		// cpSLV->Do_W_fwd(nCycle);
		#endif
		#ifdef MEMORY_CONTROLLER
		cpSLV->Do_AR_fwd_MC_Frontend(nCycle);
		cpSLV->Do_AW_fwd_MC_Frontend(nCycle);
		// cpSLV->Do_W_fwd_MC_Frontend(nCycle);
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
		#ifdef Cache_OFF
		cpCacheL3->Do_R_fwd_Cache_OFF(nCycle);
		cpCacheL3->Do_B_fwd_Cache_OFF(nCycle);
		#endif
		#ifdef Cache_ON
		cpCacheL3->Do_R_fwd_MI(nCycle);
		cpCacheL3->Do_R_fwd_SI(nCycle);
		cpCacheL3->Do_B_fwd_MI(nCycle);
		cpCacheL3->Do_B_fwd_SI(nCycle);
		#endif
		//---------------------------
		cpBUS->Do_R_fwd(nCycle);
		cpBUS->Do_B_fwd(nCycle);
		//---------------------------
		#ifdef MMU_OFF
			cpMMU3->Do_R_fwd_MMU_OFF(nCycle);
			cpMMU3->Do_B_fwd_MMU_OFF(nCycle);
		#endif
		#ifdef MMU_ON
			#ifdef AT_ENABLE
				cpMMU3->Do_R_fwd_AT(nCycle);
				cpMMU3->Do_B_fwd(nCycle);
			#else // PCAD, TRAD
				cpMMU3->Do_R_fwd(nCycle);
				cpMMU3->Do_B_fwd(nCycle);
			#endif
		#endif
		//---------------------------
		cpMST3->Do_R_fwd(nCycle);
		//cpMST3->Do_B_fwd(nCycle);
		//---------------------------------
		// Propagate ready
		//---------------------------------
		//---------------------------------
		// AR, AW, W READY
		//---------------------------------
		cpSLV->Do_AR_bwd(nCycle);
		cpSLV->Do_AW_bwd(nCycle);
		// cpSLV->Do_W_bwd(nCycle);
		//-----------------------------
		#ifdef Cache_OFF
			cpCacheL3->Do_AR_bwd_Cache_OFF(nCycle);
			cpCacheL3->Do_AW_bwd_Cache_OFF(nCycle);
			// cpCacheL3->Do_W_bwd_Cache_OFF(nCycle);
		#endif
		#ifdef Cache_ON
			cpCacheL3->Do_AR_bwd(nCycle);
			cpCacheL3->Do_AW_bwd(nCycle);
			// cpCacheL3->Do_W_bwd(nCycle);
		#endif
		//-------------------------------
		cpBUS->Do_AR_bwd(nCycle);
		cpBUS->Do_AW_bwd(nCycle);
		#ifdef MMU_OFF
		cpMMU3->Do_AR_bwd_MMU_OFF(nCycle);
		cpMMU3->Do_AW_bwd_MMU_OFF(nCycle);
		// cpMMU3->Do_W_bwd_MMU_OFF(nCycle);
		#endif
		#ifdef MMU_ON
		cpMMU3->Do_AR_bwd(nCycle);
		//cpMMU3->Do_AW_bwd(nCycle);    // DUONGTRAN comment
		//cpMMU3->Do_W_bwd(nCycle);
		#endif
		
		//-----------------------------
		cpMST3->Do_AR_bwd(nCycle);
		//cpMST3->Do_AW_bwd(nCycle);  // DUONGTRAN comment
		//cpMST3->Do_W_bwd(nCycle);
		//---------------------------------
		// R, B READY
		//---------------------------------
		cpMST3->Do_R_bwd(nCycle);
		//cpMST3->Do_B_bwd(nCycle);   // DUONGTRAN comment
		//---------------------------
		#ifdef MMU_OFF
		cpMMU3->Do_R_bwd_MMU_OFF(nCycle);
		cpMMU3->Do_B_bwd_MMU_OFF(nCycle);
		#endif
		#ifdef MMU_ON
		cpMMU3->Do_R_bwd(nCycle);
		cpMMU3->Do_B_bwd(nCycle);
		#endif
		//---------------------------
		cpBUS->Do_R_bwd(nCycle);
		cpBUS->Do_B_bwd(nCycle);
		//---------------------------
		#ifdef Cache_OFF
		cpCacheL3->Do_R_bwd_Cache_OFF(nCycle);
		cpCacheL3->Do_B_bwd_Cache_OFF(nCycle);
		#endif
		#ifdef Cache_ON
		cpCacheL3->Do_R_bwd_SI(nCycle);
		cpCacheL3->Do_R_bwd_MI(nCycle);
		cpCacheL3->Do_B_bwd_SI(nCycle);
		cpCacheL3->Do_B_bwd_MI(nCycle);
		#endif
		//---------------------------
		cpSLV->Do_R_bwd(nCycle);
		cpSLV->Do_B_bwd(nCycle);
		//---------------------------------
		// 5. Update state
		//---------------------------------
		cpMST3->UpdateState(nCycle);
		cpMMU3->UpdateState(nCycle);
		cpBUS->UpdateState(nCycle);
		cpCacheL3->UpdateState(nCycle);
		cpSLV->UpdateState(nCycle);

		//---------------------------------
		// 6. Check simulation finish 
		//---------------------------------
		if ((cpMST3->IsARTransFinished() == ERESULT_TYPE_YES)
			// || (cpMST3->nARTrans == cpSLV->nAR)
		) {
			//--------------------------------------------------------------
			printf("[Cycle %3ld] Simulation is finished. \n", nCycle);
			printf("---------------------------------------------\n");
			#ifdef RASTER_SCAN
			printf("MST3: \t Display read (raster-scan) \n");
			#elif ROTATION
			printf("MST3: \t Display read (rotation) \n");
			#endif
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
			printf("\t Cache            : OFF \n");
			#endif
			#ifdef Cache_ON
			printf("\t Cache            : ON \n");
			#endif
			#ifdef MMU_OFF 
			printf("\t MMU				: OFF \n");
			#endif
			#ifdef MMU_ON
			printf("\t MMU				: ON \n");
			#endif
			printf("\t PTW				: Single fetch\n");
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
			//FILE *fp = NULL;
			// DUONGTRAN comment
			// Stat
			// cpMST0->PrintStat(nCycle, fp);
			// cpMST1->PrintStat(nCycle, fp);
			// cpMST2->PrintStat(nCycle, fp);
			// cpMST3->PrintStat(nCycle, fp);
			// cpMST4->PrintStat(nCycle, fp);
			// cpMST5->PrintStat(nCycle, fp);
			#ifdef MMU_ON
			//cpMMU0->PrintStat(nCycle, fp);
			// cpMMU1->PrintStat(nCycle, fp);
			// cpMMU2->PrintStat(nCycle, fp);
			//cpMMU3->PrintStat(nCycle, fp);
			#endif
			// Get total avg TLB hit rate and PTW
			#ifdef MMU_ON
			//int Total_nAR_SI = cpMMU0->Get_nAR_SI() + cpMMU1->Get_nAR_SI() + cpMMU2->Get_nAR_SI() + cpMMU3->Get_nAR_SI();
			int Total_nAR_SI = cpMMU3->Get_nAR_SI();
			//int Total_nAW_SI = cpMMU0->Get_nAW_SI() + cpMMU1->Get_nAW_SI() + cpMMU2->Get_nAW_SI() + cpMMU3->Get_nAW_SI();
			int Total_nAW_SI = cpMMU3->Get_nAW_SI();
			//int Total_nHit_AR_TLB = cpMMU0->Get_nHit_AR_TLB() + cpMMU1->Get_nHit_AR_TLB() + cpMMU2->Get_nHit_AR_TLB() + cpMMU3->Get_nHit_AR_TLB();
			int Total_nHit_AR_TLB = cpMMU3->Get_nHit_AR_TLB();
			//int Total_nHit_AW_TLB = cpMMU0->Get_nHit_AW_TLB() + cpMMU1->Get_nHit_AW_TLB() + cpMMU2->Get_nHit_AW_TLB() + cpMMU3->Get_nHit_AW_TLB();
			int Total_nHit_AW_TLB = cpMMU3->Get_nHit_AW_TLB();
			float Avg_TLB_hit_rate = (float) (Total_nHit_AR_TLB + Total_nHit_AW_TLB) / (Total_nAR_SI + Total_nAW_SI) ; 
			// Get TLB reach
			float Avg_TLB_reach = (float) (cpMMU3->GetTLB_reach(nCycle));
			int total_nPTW = cpMMU3->nPTW_total + cpMMU3->nAPTW_total_AT;
			printf("---------------------------------------------\n");
			printf("\t Total Avg TLB hit rate = %1.5f\n", Avg_TLB_hit_rate);
			printf("\t Total Avg TLB reach    = %1.5f\n", Avg_TLB_reach);
			printf("\t Total PTW              = %d\n", total_nPTW);
			printf("---------------------------------------------\n");
			#endif  // MMU_ON
			cpSLV->PrintStat(nCycle, nullptr);
			// DUONGTRAN comment
			//cpBUS ->PrintStat(nCycle, fp);
			//cpSLV ->PrintStat(nCycle, fp);
			#ifdef Cache_ON
			FILE *fp = NULL;
			cpCacheL3->PrintStat(nCycle, fp);
			#endif //Cache_ON
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
	cpBuddy->Do_Deallocate(MST3_AR_VSTART >> 12);
	#endif


	//---------------------
	// 8. Destruct
	//---------------------
	delete (cpMST0);
	delete (cpMST1);
	delete (cpMST2);
	delete (cpMST3);
	delete (cpMMU0);
	delete (cpMMU1);
	delete (cpMMU2);
	delete (cpMMU3);
	delete (cpBUS);
	delete (cpCacheL3);
	delete (cpSLV);

	#ifdef BUDDY_ENABLE
	delete (cpBuddy);
	#endif

	return (-1);
}

