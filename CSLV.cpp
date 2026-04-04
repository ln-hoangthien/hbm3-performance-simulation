//-----------------------------------------------------------
// Filename     : CSLV.cpp 
// Version		: 0.791a
// Date         : 24 Nov 2022
// Contact		: JaeYoung.Hur@gmail.com
// Description	: Slave definition
// Modify
// --- 2023.03.15: Duong support ARMv8
//-----------------------------------------------------------
// Memory controller
//-----------------------------------------------------------
// Known issues
// 	1. MST1 keep reads same address. MST5 keep writes same address. They access same bank different row.
// 	   Even when TIME_OUT passed, MST5 keep get scheduling. MST5 keep send WR cmd.  The bank is not PREable. Even when AR_priority high, MST1 can not PRE, tWR is not 0. 
// 	   This is bug.  This case never occurs in experiment. FIXME
//-----------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <algorithm>

#include "CSLV.h"


// Constructor
CSLV::CSLV(string cName, int nMemCh) {

        // Generate links
        this->cpRx_AR = new CTRx("SLV_Rx_AR", ETRX_TYPE_RX, EPKT_TYPE_AR);
        this->cpRx_AW = new CTRx("SLV_Rx_AW", ETRX_TYPE_RX, EPKT_TYPE_AW);
        this->cpRx_W  = new CTRx("SLV_Rx_W",  ETRX_TYPE_RX, EPKT_TYPE_W);
        this->cpTx_R  = new CTRx("SLV_Tx_R",  ETRX_TYPE_TX, EPKT_TYPE_R);
        this->cpTx_B  = new CTRx("SLV_Tx_B",  ETRX_TYPE_TX, EPKT_TYPE_B);

        // Generate FIFO
        this->cpQ_AR	= new CQ("SLV_Q_AR",	   EUD_TYPE_AR, MAX_MO_COUNT);
        this->cpQ_AW	= new CQ("SLV_Q_AW",	   EUD_TYPE_AW, MAX_MO_COUNT);
        this->cpQ_W		= new CQ("SLV_Q_W",	   	   EUD_TYPE_W,  MAX_MO_COUNT*4);

        this->cpFIFO_AR	= new CFIFO("SLV_FIFO_AR", EUD_TYPE_AR, MAX_MO_COUNT);
        this->cpFIFO_AW	= new CFIFO("SLV_FIFO_AW", EUD_TYPE_AW, MAX_MO_COUNT);
        this->cpFIFO_R	= new CFIFO("SLV_FIFO_R",  EUD_TYPE_R,  MAX_MO_COUNT*4);
        this->cpFIFO_B	= new CFIFO("SLV_FIFO_B",  EUD_TYPE_B,  MAX_MO_COUNT);
	
	// Generate scheduler 
	this->cpScheduler = new CScheduler("Scheduler");

	// Generate Mem 
	this->cpMem = new CMem("Mem");

	// Generate mem cmd pkt
	this->spMemCmdPkt = new SMemCmdPkt;

	// Generate mem state pkt
	this->spMemStatePkt = new SMemStatePkt;

	// Initialize
    this->cName = cName;
	this->nMemCh = nMemCh;


	this->previous_nBank   = -1;
	for (int i = 0; i < BANK_NUM; i++) { this->prepare_Refresing[i] = false; }

	this->nAR   = -1;
	this->nAW   = -1;
	this->nR    = -1;
	this->nB    = -1;
	this->nW    = -1;
	this->nPTW  = -1;

	this->nACT_cmd = -1;
	this->nPRE_cmd = -1;
	this->nWR_cmd  = -1;
	this->nRD_cmd  = -1;
	this->nNOP_cmd = -1;

	this->nMax_Q_AR_Occupancy   = -1;
	this->nMax_Q_AW_Occupancy   = -1;
	this->nTotal_Q_AR_Occupancy = -1;
	this->nTotal_Q_AW_Occupancy = -1;

	this->nMax_Q_AR_Wait = -1;
	this->nMax_Q_AW_Wait = -1;

	this->nEmpty_Q_AR_cycles = -1;
	this->nEmpty_Q_AW_cycles = -1;
	this->nEmpty_Q_Ax_cycles = -1;

	this->nMax_Q_AR_Scheduled_Wait = -1;
	this->nMax_Q_AW_Scheduled_Wait = -1;
	this->nTotal_Q_AR_Scheduled_Wait = -1;
	this->nTotal_Q_AW_Scheduled_Wait = -1;
};


// Destructor
CSLV::~CSLV() {

	delete (this->cpRx_AR);
	delete (this->cpRx_AW);
	delete (this->cpRx_W);
	delete (this->cpTx_R);
	delete (this->cpTx_B);

	this->cpRx_AR = NULL;
	this->cpRx_AW = NULL;
	this->cpRx_W  = NULL;
	this->cpTx_R  = NULL;
	this->cpTx_B  = NULL;

	delete (this->cpQ_AR);
	delete (this->cpQ_AW);
	delete (this->cpQ_W);
	delete (this->cpFIFO_AR);
	delete (this->cpFIFO_AW);
	delete (this->cpFIFO_R);
	delete (this->cpFIFO_B);

	this->cpQ_AR	= NULL;
	this->cpQ_AW	= NULL;
	this->cpQ_W	= NULL;
	this->cpFIFO_AR = NULL;
	this->cpFIFO_AW = NULL;
	this->cpFIFO_R	= NULL;
	this->cpFIFO_B	= NULL;

	delete (this->cpScheduler);
	this->cpScheduler= NULL;

	delete (this->cpMem);
	this->cpMem = NULL;

	delete (this->spMemCmdPkt);
	this->spMemCmdPkt = NULL;

	//delete (this->spMemStatePkt);
	this->spMemStatePkt = NULL;

};


// Initialize
EResultType CSLV::Reset() {

	this->cpRx_AR->Reset();
	this->cpRx_AW->Reset();
	this->cpRx_W ->Reset();
	this->cpTx_R ->Reset();
	this->cpTx_B ->Reset();

	this->cpQ_AR->Reset();
	this->cpQ_AW->Reset();
	this->cpQ_W ->Reset();

	this->cpFIFO_AR->Reset();
	this->cpFIFO_AW->Reset();
	this->cpFIFO_R ->Reset();
	this->cpFIFO_B ->Reset();

	this->cpScheduler->Reset();

	this->cpMem->Reset();

	// Cmd to Mem
	this->spMemCmdPkt->eMemCmd = EMEM_CMD_TYPE_UNDEFINED;
	this->spMemCmdPkt->nBank   = -1; 
	this->spMemCmdPkt->nRow    = -1;

	// Mem state
	for (int i=0; i<BANK_NUM; i++) {
		this->spMemStatePkt->eMemState[i] 			= EMEM_STATE_TYPE_IDLE;
		this->spMemStatePkt->IsRD_ready[i] 			= ERESULT_TYPE_NO; 
		this->spMemStatePkt->IsWR_ready[i] 			= ERESULT_TYPE_NO;
		this->spMemStatePkt->IsPRE_ready[i] 		= ERESULT_TYPE_NO;
		this->spMemStatePkt->IsACT_ready[i] 		= ERESULT_TYPE_NO;
  		this->spMemStatePkt->IsREF_ready[i] 		= ERESULT_TYPE_NO;
  		this->spMemStatePkt->forced_PRE[i]  		= ERESULT_TYPE_NO;
  		this->spMemStatePkt->forced_REFI[i] 		= ERESULT_TYPE_NO;
  		this->spMemStatePkt->IsFirstData_ready[i]  	= ERESULT_TYPE_NO;
  		this->spMemStatePkt->IsBankPrepared[i]		= ERESULT_TYPE_NO;
  		this->spMemStatePkt->nActivatedRow[i] 		= -1;
	};
	this->spMemStatePkt->IsData_busy  				= ERESULT_TYPE_NO;


	this->previous_nBank   = -1;
	for (int i = 0; i < BANK_NUM; i++) { this->prepare_Refresing[i] = false; }

	// Stat
	this->nAR  = 0;
	this->nAW  = 0;
	this->nR   = 0;
	this->nB   = 0;
	this->nW   = 0;
	this->nPTW = 0;

	this->nACT_cmd  = 0;
	this->nPRE_cmd  = 0;
	this->nWR_cmd   = 0;
	this->nRD_cmd   = 0;
	this->nNOP_cmd  = 0;

	this->nMax_Q_AR_Occupancy   = 0;
	this->nMax_Q_AW_Occupancy   = 0;
	this->nTotal_Q_AR_Occupancy = 0;
	this->nTotal_Q_AW_Occupancy = 0;

	this->nMax_Q_AR_Wait = 0;
	this->nMax_Q_AW_Wait = 0;

	this->nEmpty_Q_AR_cycles = 0;
	this->nEmpty_Q_AW_cycles = 0;
	this->nEmpty_Q_Ax_cycles = 0;

	this->nMax_Q_AR_Scheduled_Wait   = 0;
	this->nMax_Q_AW_Scheduled_Wait   = 0;
	this->nTotal_Q_AR_Scheduled_Wait = 0;
	this->nTotal_Q_AW_Scheduled_Wait = 0;

	return (ERESULT_TYPE_SUCCESS);
};


//---------------------------------------------
// Ideal memory
//---------------------------------------------
// AR valid
//---------------------------------------------
// 1. Get AR remote 
// 2. Put R immediately (into FIFO_R)
//---------------------------------------------
EResultType CSLV::Do_AR_fwd(int64_t nCycle) {

	// Check Rx busy 
	if (this->cpRx_AR->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};
	
	// Check remote-Tx valid 
	CPAxPkt cpAR = this->cpRx_AR->GetPair()->GetAx();
	if (cpAR == NULL) {
	        return (ERESULT_TYPE_SUCCESS);
	};

	// Debug	
	// cpAR->CheckPkt();

	// Put AR
	this->cpRx_AR->PutAx(cpAR);
	// cpAR->Display();

	// Get R info
	int nBurstNum = cpAR->GetLen();
	int nID = cpAR->GetID();
	int nCacheCh = cpAR->GetCacheCh();
	EResultType eFinalTrans = cpAR->IsFinalTrans();

	// Debug
	assert(this->nMemCh == cpAR->GetChannelNum_MMap());
	assert(nCacheCh != -1);

	// Push R
	for (int i=0; i<=nBurstNum; i++) {

		// Generate R
		CPRPkt cpR_new = NULL;
                char cPktName[50];

		if (i== nBurstNum) {
			sprintf(cPktName, "RLAST_R%d_for_%s", i, cpAR->GetName().c_str());
		} 
		else {
			sprintf(cPktName, "R%d_for_%s", i, cpAR->GetName().c_str());
		};

		cpR_new = new CRPkt(cPktName);
		cpR_new->SetID(nID);
		cpR_new->SetData(i);
		cpR_new->SetFinalTrans(eFinalTrans);
		cpR_new->SetMemCh(this->nMemCh);
		cpR_new->SetCacheCh(nCacheCh);

		if (i == nBurstNum) {
			cpR_new->SetLast(ERESULT_TYPE_YES);
		} 
		else {
			cpR_new->SetLast(ERESULT_TYPE_NO);
		};

		UPUD upR_new = new UUD;
		upR_new->cpR = cpR_new;
	
		// Push R
		this->cpFIFO_R->Push(upR_new, SLV_FIFO_R_LATENCY);

		#ifdef DEBUG_SLV
		// printf("[Cycle %3ld: SLV.Do_AR_fwd] (%s) push FIFO_R.\n", nCycle, upR_new->cpR->GetName().c_str());
		// this->cpFIFO_R->Display();
		// this->cpFIFO_R->CheckFIFO();
		#endif

		// Maintain
		Delete_UD(upR_new, EUD_TYPE_R); // FIXME Check upR_new deleted
	};

	// Stat
	this->nAR++;	

	ETransType eType = cpAR->GetTransType();
	if (eType == ETRANS_TYPE_FIRST_PTW  or eType == ETRANS_TYPE_SECOND_PTW  or eType == ETRANS_TYPE_THIRD_PTW  or eType == ETRANS_TYPE_FOURTH_PTW or
	    eType == ETRANS_TYPE_FIRST_PF   or eType == ETRANS_TYPE_SECOND_PF   or eType == ETRANS_TYPE_THIRD_PF   or
	    eType == ETRANS_TYPE_FIRST_RPTW or eType == ETRANS_TYPE_SECOND_RPTW or eType == ETRANS_TYPE_THIRD_RPTW or
	    eType == ETRANS_TYPE_FIRST_APTW or eType == ETRANS_TYPE_SECOND_APTW or eType == ETRANS_TYPE_THIRD_APTW or eType == ETRANS_TYPE_FOURTH_APTW) {
		this->nPTW++;
	};

	return (ERESULT_TYPE_SUCCESS);
};


//---------------------------------------------
// Ideal memory
//---------------------------------------------
// AW valid
//---------------------------------------------
// 1. Get AW remote
// 2. Put B immediately (into FIFO_B)
//---------------------------------------------
EResultType CSLV::Do_AW_fwd(int64_t nCycle) {

	// Check Rx busy 
	if (this->cpRx_AW->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Check remote-Tx valid 
	CPAxPkt cpAW = this->cpRx_AW->GetPair()->GetAx();
	if (cpAW == NULL) {
	        return (ERESULT_TYPE_SUCCESS);
	};

	// Debug	
	// cpAW->CheckPkt();

	// Put AW
	this->cpRx_AW->PutAx(cpAW);

	#ifdef DEBUG_SLV
	printf("[Cycle %3ld: SLV.Do_AW_fwd] (%s) put into Rx_AW.\n", nCycle, cpAW->GetName().c_str());
	// cpAW->Display();
	#endif

	#ifndef USE_W_CHANNEL
	// Get W info
	int nID = cpAW->GetID();
	int nCacheCh = cpAW->GetCacheCh();

	EResultType eFinalTrans = cpAW->IsFinalTrans();
	assert(this->nMemCh == cpAW->GetChannelNum_MMap());
	assert(nCacheCh != -1);

	// Generate B
	CPBPkt cpB_new = NULL;
        char cPktName[50];
	sprintf(cPktName, "B_for_%s", cpAW->GetName().c_str());
	cpB_new  = new CBPkt(cPktName);
	cpB_new->SetID(nID);
	cpB_new->SetFinalTrans(eFinalTrans);
	cpB_new->SetMemCh(this->nMemCh);
	cpB_new->SetCacheCh(nCacheCh);

	UPUD upB_new = new UUD;
	upB_new->cpB = cpB_new;
	
	// Push B
	this->cpFIFO_B->Push(upB_new, SLV_FIFO_B_LATENCY);

	#ifdef DEBUG_SLV
	// printf("[Cycle %3ld: SLV.Do_AW_fwd] (%s) push FIFO_B.\n", nCycle, upB_new->cpB->GetName().c_str());
	// this->cpFIFO_B->Display();
	// this->cpFIFO_B->CheckFIFO();
	#endif

	// Maintain
	Delete_UD(upB_new, EUD_TYPE_B);

  	#endif

	// Stat
	this->nAW++;	

	return (ERESULT_TYPE_SUCCESS);
};


//---------------------------------------------
// Ideal memory
//---------------------------------------------
// W valid
//---------------------------------------------
// 1. Get W remote 
//---------------------------------------------
EResultType CSLV::Do_W_fwd(int64_t nCycle) {

	// Check Rx busy 
	if (this->cpRx_W->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Check remote-Tx valid 
	CPWPkt cpW = this->cpRx_W->GetPair()->GetW();
	if (cpW == NULL) {
	        return (ERESULT_TYPE_SUCCESS);
	}

	// Debug	
	// cpW->CheckPkt();

	// Put W
	this->cpRx_W->PutW(cpW);

	// Check W last
	if (cpW->IsLast() == ERESULT_TYPE_NO) {
		return (ERESULT_TYPE_SUCCESS);
	};

	#ifdef USE_W_CHANNEL
	// Get W info
	int nID = cpW->GetID();

	// Generate B
	CPBPkt cpB_new = NULL;
        char cPktName[50];
	sprintf(cPktName, "B_for_%s", cpW->GetName().c_str());
	cpB_new  = new CBPkt(cPktName);
	cpB_new->SetID(nID);

	UPUD upB_new = new UUD;
	upB_new->cpB = cpB_new;
	
	// Push B
	this->cpFIFO_B->Push(upB_new, SLV_FIFO_B_LATENCY);

	#ifdef DEBUG_SLV
	// printf("[Cycle %3ld: SLV.Do_AW_fwd] (%s) push FIFO_B.\n", nCycle, upB_new->cpB->GetName().c_str());
	// this->cpFIFO_B->Display();
	// this->cpFIFO_B->CheckFIFO();
	#endif

	// Maintain
	Delete_UD(upB_new, EUD_TYPE_B);

	#endif

	return (ERESULT_TYPE_SUCCESS);
};


//---------------------------------------------------
// MC
//---------------------------------------------------
// AR valid
//---------------------------------------------------
// 1. Get AR remote 
// 2. Push AR into CQ
//---------------------------------------------------
EResultType CSLV::Do_AR_fwd_MC_Frontend(int64_t nCycle) {

	// Check Rx busy 
	// if (this->cpRx_AR->IsBusy() == ERESULT_TYPE_YES) {
	//	return (ERESULT_TYPE_SUCCESS);
	// };

	// Check remote-Tx valid 
	CPAxPkt cpAR = this->cpRx_AR->GetPair()->GetAx();
	if (cpAR == NULL) {
	        return (ERESULT_TYPE_SUCCESS);
	};
	
	// Debug
	// cpAR->CheckPkt();
	// this->cpQ_AR->CheckQ();
	
	// Check queue empty
	if (this->cpQ_AR->IsFull() == ERESULT_TYPE_YES) {
	        return (ERESULT_TYPE_SUCCESS);
	};

	// Put AR
	this->cpRx_AR->PutAx(cpAR);

	#ifdef DEBUG_SLV
	//printf("[Cycle %3ld: SLV.Do_AR_fwd_MC_Frontend] (%s) access to 0x%lx, put Rx_AR.\n", nCycle, cpAR->GetName().c_str(), cpAR->GetAddr());
	// cpAR->Display();
	#endif

	// Generate UD
	UPUD upAR_new = new UUD;
	upAR_new->cpAR = Copy_CAxPkt(cpAR); // FIXME FIXME Check. need copy? 

	// Push UD
	this->cpQ_AR->Push(upAR_new);

	#ifdef DEBUG_SLV
	// printf("[Cycle %3ld: SLV.Do_AR_fwd_MC] (%s) push Q_AR.\n", nCycle, cpAR->GetName().c_str());
	// this->cpQ_AR->Display();
	#endif

	// Maintain	
	Delete_UD(upAR_new, EUD_TYPE_AR);

	// Stat
	this->nAR++;

	ETransType eType = cpAR->GetTransType();
	if (eType == ETRANS_TYPE_FIRST_PTW  or eType == ETRANS_TYPE_SECOND_PTW  or eType == ETRANS_TYPE_THIRD_PTW  or eType == ETRANS_TYPE_FOURTH_PTW or
	    eType == ETRANS_TYPE_FIRST_PF   or eType == ETRANS_TYPE_SECOND_PF   or eType == ETRANS_TYPE_THIRD_PF   or
	    eType == ETRANS_TYPE_FIRST_RPTW or eType == ETRANS_TYPE_SECOND_RPTW or eType == ETRANS_TYPE_THIRD_RPTW or
	    eType == ETRANS_TYPE_FIRST_APTW or eType == ETRANS_TYPE_SECOND_APTW or eType == ETRANS_TYPE_THIRD_APTW or eType == ETRANS_TYPE_FOURTH_APTW) {
		this->nPTW++;
	};


	return (ERESULT_TYPE_SUCCESS);
};


//---------------------------------------------------
// Memory controller
//---------------------------------------------------
// Cmd response 
//---------------------------------------------------
// 1. Get response
// 2. When timing is met, push Push R (into FIFO_R)
//---------------------------------------------------
EResultType CSLV::Do_AR_fwd_MC_Backend_Response(int64_t nCycle) {

	// Check NULL 
	if (this->cpFIFO_AR->GetTop() == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	};

	#ifdef DEBUG_SLV
	assert (this->cpFIFO_AR->GetUDType() == EUD_TYPE_AR);
	assert (this->cpFIFO_AR->IsEmpty() == ERESULT_TYPE_NO);
	#endif

	// Get target bank
	int nBank = this->cpFIFO_AR->GetTop()->cpAR->GetBankNum_MMap();
	
	// Check R sent
	this->spMemStatePkt = this->cpMem->GetMemStatePkt();
	if (this->spMemStatePkt->IsFirstData_ready[nBank] == ERESULT_TYPE_NO) { // Target bank can put first data
		return (ERESULT_TYPE_FAIL);
	};

	#ifdef DEBUG_SLV
	//printf("[Cycle %3ld: SLV.Do_AR_fwd_MC_Backend_Response] R data ready bank %d, row 0x%x.\n", nCycle, nBank, this->spMemStatePkt->nActivatedRow[nBank]);
	#endif
	
	// Pop AR
	UPUD upAR_new = this->cpFIFO_AR->Pop();

	// Get Ax
	CPAxPkt cpAR = upAR_new->cpAR;

	#ifdef DEBUG_SLV	
	assert (cpAR != NULL);
	// this->cpFIFO_AR->CheckFIFO();
	#endif
	
	// Get R info
	int nBurstNum = cpAR->GetLen();
	int nID       = cpAR->GetID();
	int nCacheCh  = cpAR->GetCacheCh();
	EResultType eFinalTrans = cpAR->IsFinalTrans();
	assert(this->nMemCh == cpAR->GetChannelNum_MMap());
	assert(nCacheCh != -1);

	// Generate R
	CPRPkt cpR_new = NULL;

    for (int i=0; i<=nBurstNum; i++) {

		char cPktName[50];
		if (i == nBurstNum) {
			sprintf(cPktName, "RLAST_R%d_for_%s", i, cpAR->GetName().c_str()) ;
		} 
		else {
			sprintf(cPktName, "R%d_for_%s", i, cpAR->GetName().c_str()) ;
		};

		// Generate R
		cpR_new = new CRPkt(cPktName);
		cpR_new->SetID(nID);
		cpR_new->SetData(i);
		cpR_new->SetFinalTrans(eFinalTrans);
		cpR_new->SetMemCh(this->nMemCh);
		cpR_new->SetCacheCh(nCacheCh);
		if (i == nBurstNum) {
			cpR_new->SetLast(ERESULT_TYPE_YES);
		} 
		else {
			cpR_new->SetLast(ERESULT_TYPE_NO);
		};
		
		// Push R
		UPUD upR_new = new UUD;
		upR_new->cpR = cpR_new;
		this->cpFIFO_R->Push(upR_new, SLV_FIFO_R_LATENCY);
	
		#ifdef DEBUG_SLV	
		// upR_new->cpR->CheckPkt();
		// this->cpFIFO_R->CheckFIFO();
		// printf("[Cycle %3ld: SLV.Do_AR_fwd_MC_Backend_Response] (%s) push FIFO_R.\n", nCycle, upR_new->cpR->GetName().c_str());
		// this->cpFIFO_R->Display();
		#endif
		
		// Maintain	
		Delete_UD(upR_new, EUD_TYPE_R);
	};

	// Maintain     
	Delete_UD(upAR_new, EUD_TYPE_AR);

	return (ERESULT_TYPE_SUCCESS);
};


//---------------------------------------------------
// MC
//---------------------------------------------------
// AW valid
//---------------------------------------------------
// 1. Get AW remote 
// 2. Push AW (into Q_AW)
//---------------------------------------------------
EResultType CSLV::Do_AW_fwd_MC_Frontend(int64_t nCycle) {

	// Check Rx busy 
	// if (this->cpRx_AW->IsBusy() == ERESULT_TYPE_YES) {
	// 	return (ERESULT_TYPE_SUCCESS);
	// };

	// Check remote-Tx valid 
	CPAxPkt cpAW = this->cpRx_AW->GetPair()->GetAx();
	if (cpAW == NULL) {
	        return (ERESULT_TYPE_SUCCESS);
	};
	
	// Debug
	// cpAW->CheckPkt();
	// this->cpQ_AW->CheckQ();
	
	// Check queue empty
	if (this->cpQ_AW->IsFull() == ERESULT_TYPE_YES) {
	        return (ERESULT_TYPE_SUCCESS);
	};

	// Put AW
	this->cpRx_AW->PutAx(cpAW);

	#ifdef DEBUG_SLV
	//printf("[Cycle %3ld: SLV.Do_AW_fwd_MC_Frontend] (%s) access to 0x%lx, put Rx_AW.\n", nCycle, cpAW->GetName().c_str(), cpAW->GetAddr());
	// cpAW->Display();
	#endif

	// Generate UD
	UPUD upAW_new = new UUD;
	upAW_new->cpAW = Copy_CAxPkt(cpAW); // FIXME FIXME Check. need copy? 
	
	// Push UD
	this->cpQ_AW->Push(upAW_new);

	#ifdef DEBUG_SLV
	// printf("[Cycle %3ld: SLV.Do_AW_fwd_MC] (%s) push Q_AW.\n", nCycle, cpAW->GetName().c_str());
	// this->cpQ_AW->Display();
	#endif

	// Maintain	
	Delete_UD(upAW_new, EUD_TYPE_AW);

	// Stat
	this->nAW++;

	return (ERESULT_TYPE_SUCCESS);
};


//---------------------------------------------------
// MC
//---------------------------------------------------
// W valid
//---------------------------------------------------
// 1. Get W remote 
// 2. Push W (into Q_W)
//---------------------------------------------------
EResultType CSLV::Do_W_fwd_MC_Frontend(int64_t nCycle) {

	// Check Rx busy 
	// if (this->cpRx_W->IsBusy() == ERESULT_TYPE_YES) {
	// 	return (ERESULT_TYPE_SUCCESS);
	// };

	// Check remote-Tx valid 
	CPWPkt cpW = this->cpRx_W->GetPair()->GetW();
	if (cpW == NULL) {
	        return (ERESULT_TYPE_SUCCESS);
	};
	
	// Debug
	// cpW->CheckPkt();
	// this->cpQ_W->CheckQ();
	
	// Check queue empty
	if (this->cpQ_W->IsFull() == ERESULT_TYPE_YES) {
	        return (ERESULT_TYPE_SUCCESS);
	};

	// Put W
	this->cpRx_W->PutW(cpW);
	
	return (ERESULT_TYPE_SUCCESS);
};


//---------------------------------------------------
// MC scheduler
//---------------------------------------------------
// Ax valid
//---------------------------------------------------
// 1. Get cmd all CQ_Ax nodes
// 2. Schedule 
// 3. Send cmd to mem 
// 4. Pop CQ (immediately) 
// 5. Push FIFO_Ax (immediately)
//---------------------------------------------------
EResultType CSLV::Do_Ax_fwd_MC_Backend_Request(int64_t nCycle) {

	if (this->Handle_PRE_and_REF(nCycle) == ERESULT_TYPE_SUCCESS) {
		#ifdef DEBUG_SLV
		//printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] PRE/REF command issued.\n", nCycle);
		#endif

		return (ERESULT_TYPE_SUCCESS);
	}

	// Check queue 
	if (this->cpQ_AR->IsEmpty() == ERESULT_TYPE_YES and this->cpQ_AW->IsEmpty() == ERESULT_TYPE_YES) {
		this->SetMemCmdPkt(EMEM_CMD_TYPE_NOP, -1, -1);
		this->cpMem->SetMemCmdPkt(EMEM_CMD_TYPE_NOP, -1, -1);
		return (ERESULT_TYPE_SUCCESS);
	};

	// Set mem state
	this->spMemStatePkt = this->cpMem->GetMemStatePkt();
	this->cpQ_AR->SetMemStateCmdPkt(spMemStatePkt);
	this->cpQ_AW->SetMemStateCmdPkt(spMemStatePkt);

	// Schedule
	#ifdef BANK_HIT_FIRST
        SPLinkedMUD spScheduledMUD = this->cpScheduler->GetScheduledMUD(cpQ_AR, cpQ_AW, nCycle);
	#endif
	#ifdef FIRST_COME_FIRST_SERVE 
        SPLinkedMUD spScheduledMUD = this->cpScheduler->GetScheduledMUD_FIFO(cpQ_AR, cpQ_AW, nCycle);
	#endif

	// Get cmd
	if (spScheduledMUD != NULL) {	
		// Check forced-REF
		if (this->spMemStatePkt->forced_REFI[spScheduledMUD->spMemCmdPkt->nBank] == ERESULT_TYPE_YES) {
			this->SetMemCmdPkt(EMEM_CMD_TYPE_NOP, -1, -1);
			this->cpMem->SetMemCmdPkt(EMEM_CMD_TYPE_NOP, -1, -1);
			return (ERESULT_TYPE_SUCCESS);
		};

	} 
	else {
		this->SetMemCmdPkt(EMEM_CMD_TYPE_NOP, -1, -1);
		this->cpMem->SetMemCmdPkt(EMEM_CMD_TYPE_NOP, -1, -1);
		return (ERESULT_TYPE_SUCCESS);
	};

	// Set cmd
	this->SetMemCmdPkt(spScheduledMUD->spMemCmdPkt);
	this->cpMem->SetMemCmdPkt(spScheduledMUD->spMemCmdPkt);

    EMemCmdType eMemCmd =  this->spMemCmdPkt->eMemCmd;

	// Stat
	string cCmd = Convert_eMemCmd2string(this->spMemCmdPkt->eMemCmd);

	// Stat
	if (eMemCmd != EMEM_CMD_TYPE_NOP) {
		if (spScheduledMUD != NULL and spScheduledMUD->eUDType == EUD_TYPE_AR) {
		
			#ifdef DEBUG_SLV	
			printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] MC cmd %s (%s)(Bank %d, Row 0x%x) sent to bank %d, row 0x%x \n", nCycle, cCmd.c_str(), spScheduledMUD->upData->cpAR->GetName().c_str(), this->spMemCmdPkt->nBank, this->spMemCmdPkt->nRow, this->spMemCmdPkt->nBank, cpMem->cpBank[this->spMemCmdPkt->nBank]->GetActivatedRow());
			#endif

			// Stat
			if (eMemCmd == EMEM_CMD_TYPE_ACT) { this->nACT_cmd++; };
			if (eMemCmd == EMEM_CMD_TYPE_PRE) { this->nPRE_cmd++; };
			if (eMemCmd == EMEM_CMD_TYPE_RD)  { this->nRD_cmd++;  };
		};
		if (spScheduledMUD != NULL and spScheduledMUD->eUDType == EUD_TYPE_AW) {

			#ifdef DEBUG_SLV
			printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] MC cmd %s (%s)(Bank %d, Row 0x%x) sent to bank %d, row 0x%x \n", nCycle, cCmd.c_str(), spScheduledMUD->upData->cpAR->GetName().c_str(), this->spMemCmdPkt->nBank, this->spMemCmdPkt->nRow, this->spMemCmdPkt->nBank, cpMem->cpBank[this->spMemCmdPkt->nBank]->GetActivatedRow());
			#endif

			// Stat
			if (eMemCmd == EMEM_CMD_TYPE_ACT) { this->nACT_cmd++; };
			if (eMemCmd == EMEM_CMD_TYPE_PRE) { this->nPRE_cmd++; };
			if (eMemCmd == EMEM_CMD_TYPE_WR)  { this->nWR_cmd++;  };
		};
	};

	// Stat
	if (eMemCmd == EMEM_CMD_TYPE_NOP) {
		this->nNOP_cmd++;
	};

	// Check RD issued
	if (eMemCmd == EMEM_CMD_TYPE_RD) {

		// Stat
		if (this->nMax_Q_AR_Scheduled_Wait < spScheduledMUD->nCycle_wait) {
			this->nMax_Q_AR_Scheduled_Wait = spScheduledMUD->nCycle_wait;
		};
	
		this->previous_nBank   = this->spMemCmdPkt->nBank;
		this->nTotal_Q_AR_Scheduled_Wait += spScheduledMUD->nCycle_wait;

		#ifdef DEBUG_SLV
		// printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] %d cycles wait req Q_AR scheduled (%s) \n", nCycle, spScheduledMUD->nCycle_wait, spScheduledMUD->upData->cpAR->GetName().c_str());
		#endif

		// Pop AR
		UPUD upAR_new = this->cpQ_AR->Pop(spScheduledMUD->nID);
		
		#ifdef DEBUG_SLV
		assert (upAR_new != NULL);
		// upAR_new->cpAR->CheckPkt();
		// this->cpQ_AR->CheckQ();
		#endif
	
		#ifdef DEBUG_SLV	
		// printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] %s pop Q_AR.\n", nCycle, upAR_new->cpAR->GetName().c_str());
		// this->cpQ_AR->Display();
		#endif
		
		// Push UD (into FIFO_AR)
		this->cpFIFO_AR->Push(upAR_new, 0);
	
		#ifdef DEBUG_SLV	
		// printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] %s push FIFO_AR.\n", nCycle, upAR_new->cpAR->GetName().c_str());
		// this->cpFIFO_AR->Display();
		// this->cpFIFO_AR->CheckFIFO();
		#endif
		
		// Maintain
		Delete_UD(upAR_new, EUD_TYPE_AR);

	};

	// Check WR issued
	// if (spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_WR) {
	if (eMemCmd == EMEM_CMD_TYPE_WR) {

		// Stat
		if (this->nMax_Q_AW_Scheduled_Wait < spScheduledMUD->nCycle_wait) {
			this->nMax_Q_AW_Scheduled_Wait = spScheduledMUD->nCycle_wait;
		};

		this->previous_nBank   = this->spMemCmdPkt->nBank;
		this->nTotal_Q_AW_Scheduled_Wait += spScheduledMUD->nCycle_wait;

		#ifdef DEBUG_SLV
		// printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] %d cycles wait req Q_AW scheduled (%s) \n", nCycle, spScheduledMUD->nCycle_wait, spScheduledMUD->upData->cpAW->GetName().c_str());
		#endif

		// Pop AW
		UPUD upAW_new = this->cpQ_AW->Pop(spScheduledMUD->nID);
	
		#ifdef DEBUG_SLV	
		assert (upAW_new != NULL);
		// upAW_new->cpAW->CheckPkt();
		// this->cpQ_AW->CheckQ();
		#endif
	
		#ifdef DEBUG_SLV	
		// printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] %s pop Q_AW.\n", nCycle, upAW_new->cpAW->GetName().c_str());
		// this->cpQ_AW->Display();
		#endif
		
		// Push UD (into FIFO_AW)
		this->cpFIFO_AW->Push(upAW_new, 0);
	
		#ifdef DEBUG_SLV	
		// printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] %s push FIFO_AW.\n", nCycle, upAW_new->cpAW->GetName().c_str());
		// this->cpFIFO_AW->Display();
		// this->cpFIFO_AW->CheckFIFO();
		#endif
		
		// Maintain
		Delete_UD(upAW_new, EUD_TYPE_AW);
	};

	return (ERESULT_TYPE_SUCCESS);
};


//---------------------------------------------------
// MC
//---------------------------------------------------
// Cmd AW response 
//---------------------------------------------------
// 1. Get response
// 2. When timing is met, push Push B (into FIFO_B)
//---------------------------------------------------
EResultType CSLV::Do_AW_fwd_MC_Backend_Response(int64_t nCycle) {

	// Check FIFO_AW 
	if (this->cpFIFO_AW->GetTop() == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	};

	#ifdef DEBUG_SLV
	assert (this->cpFIFO_AW->GetUDType() == EUD_TYPE_AW);
	assert (this->cpFIFO_AW->IsEmpty() == ERESULT_TYPE_NO);
	#endif

	// Get target bank
	int nBank = this->cpFIFO_AW->GetTop()->cpAW->GetBankNum_MMap();

	// Check B sent
	this->spMemStatePkt = this->cpMem->GetMemStatePkt();
	if (this->spMemStatePkt->IsFirstData_ready[nBank] == ERESULT_TYPE_NO) {
		return (ERESULT_TYPE_FAIL);
	};

	#ifdef DEBUG_SLV
	//printf("[Cycle %3ld: SLV.Do_AW_fwd_MC_Backend_Response] W data (%s) ready bank %d, row 0x%x.\n", nCycle, this->cpFIFO_AW->GetTop()->cpAW->GetName().c_str(),  nBank, this->spMemStatePkt->nActivatedRow[nBank]);
	#endif
	
	// Pop AW
	UPUD upAW_new = this->cpFIFO_AW->Pop();

	// Get Ax
	CPAxPkt cpAW = upAW_new->cpAW;

	#ifdef DEBUG_SLV	
	assert (cpAW != NULL);
	// this->cpFIFO_AW->CheckFIFO();
	#endif
	
	// Get B info
	int nID = cpAW->GetID();
	int nCacheCh = cpAW->GetCacheCh();
	EResultType eFinalTrans = cpAW->IsFinalTrans();
	assert(this->nMemCh == cpAW->GetChannelNum_MMap());
	assert(nCacheCh != -1);

	// Generate B
	CPBPkt cpB_new = NULL;
	char cPktName[50];
	sprintf(cPktName, "B_for_%s", cpAW->GetName().c_str());
	cpB_new = new CBPkt(cPktName);
	cpB_new->SetID(nID);
	cpB_new->SetFinalTrans(eFinalTrans);
	cpB_new->SetMemCh(this->nMemCh);
	cpB_new->SetCacheCh(nCacheCh);

	// Maintain     
	Delete_UD(upAW_new, EUD_TYPE_AW);
	
	// Push B
	UPUD upB_new = new UUD;
	upB_new->cpB = cpB_new;
	this->cpFIFO_B->Push(upB_new, SLV_FIFO_B_LATENCY);

	#ifdef DEBUG_SLV	
	// upB_new->cpB->CheckPkt();
	// this->cpFIFO_B->CheckFIFO();
	// printf("[Cycle %3ld: SLV.Do_AW_fwd_MC_Backend_Response] (%s) push FIFO_B.\n", nCycle, upB_new->cpB->GetName().c_str());
	// this->cpFIFO_B->Display();
	#endif
	
	// Maintain	
	Delete_UD(upB_new, EUD_TYPE_B);

	return (ERESULT_TYPE_SUCCESS);
};


// AR ready
EResultType CSLV::Do_AR_bwd(int64_t nCycle) {

	// Check remote-Tx valid
	// CPAxPkt cpAR = this->cpRx_AR->GetPair()->GetAx();
	// if (cpAR == NULL) {
	//	return (ERESULT_TYPE_SUCCESS);
	// };
	// cpAR->CheckPkt();

	// Check Rx valid
	if (this->cpRx_AR->IsBusy() == ERESULT_TYPE_YES) {
		// Set ready
		this->cpRx_AR->SetAcceptResult(ERESULT_TYPE_ACCEPT);

		#ifdef DEBUG_SLV
		CPAxPkt cpAR = this->cpRx_AR->GetAx();
		string cARPktName = cpAR->GetName();
		// printf("[Cycle %3ld: SLV.Do_AR_bwd] (%s) handshake Rx_AR.\n", nCycle, cARPktName.c_str());
		// cpAR->Display();
		#endif
	};
	return (ERESULT_TYPE_SUCCESS);
};


// AW ready
EResultType CSLV::Do_AW_bwd(int64_t nCycle) {

	// Check remote-Tx valid
	// CPAxPkt cpAW = this->cpRx_AW->GetPair()->GetAx();
	// if (cpAW == NULL) {
	//	return (ERESULT_TYPE_SUCCESS);
	// };
	// cpAW->CheckPkt();

	// Check Rx valid 
	if (this->cpRx_AW->IsBusy() == ERESULT_TYPE_YES) {
		// Set ready
		this->cpRx_AW->SetAcceptResult(ERESULT_TYPE_ACCEPT);

		#ifdef DEBUG_SLV	
		CPAxPkt cpAW = this->cpRx_AW->GetAx();
		string cAWPktName = cpAW->GetName();
		//printf("[Cycle %3ld: SLV.Do_AW_bwd] (%s) handshake Rx_AW.\n", nCycle, cAWPktName.c_str());
		// cpAW->Display();
		#endif
		
		// Stat
		this->nW = this->nW + MAX_BURST_LENGTH;
	};
	return (ERESULT_TYPE_SUCCESS);
};


// W ready
EResultType CSLV::Do_W_bwd(int64_t nCycle) {

	// Get W remote 
	// CPWPkt cpW = this->cpRx_W->GetPair()->GetW();
	// if (cpW == NULL) {
	//	return (ERESULT_TYPE_SUCCESS);
	// };
	//cpW->CheckPkt();

	// Check Rx valid 
	if (this->cpRx_W->IsBusy() == ERESULT_TYPE_YES) {
		// Set ready
		this->cpRx_W->SetAcceptResult(ERESULT_TYPE_ACCEPT);

		#ifdef DEBUG_SLV	
		CPWPkt cpW = this->cpRx_W->GetW();
		string cWPktName = cpW->GetName();
		//printf("[Cycle %3ld: SLV.Do_W_bwd] (%s) handshake Rx_W.\n", nCycle, cWPktName.c_str());
		// cpW->Display();
		#endif
		
		// Stat
		nW++;
	};
	return (ERESULT_TYPE_SUCCESS);
};


// R valid
EResultType CSLV::Do_R_fwd(int64_t nCycle) {
	
	// Check Tx valid 
	if (this->cpTx_R->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};
	
	// Check FIFO
	if (this->cpFIFO_R->IsEmpty() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};
	
	// Pop
	UPUD upR_new = this->cpFIFO_R->Pop();
	if (upR_new == NULL) {
		return (ERESULT_TYPE_FAIL);
	};

	#ifdef DEBUG_SLV
	assert (upR_new != NULL);
	// this->cpFIFO_R->CheckFIFO();
	// printf("[Cycle %3ld: SLV.Do_R_fwd] (%s) pop FIFO_R.\n", nCycle, upR_new->cpR->GetName().c_str());
	// this->cpFIFO_R->Display();
	#endif

	// Get R 
	CPRPkt cpR_new = upR_new->cpR;
	// cpR_new->CheckPkt();
	
	// Put Tx
	this->cpTx_R->PutR(cpR_new);

	#ifdef DEBUG_SLV
	if (cpR_new->IsLast() == ERESULT_TYPE_YES) {
		//printf("[Cycle %3ld: SLV.Do_R_fwd] (%s) RID 0x%x RLAST put Tx_R.\n", nCycle, cpR_new->GetName().c_str(), cpR_new->GetID());
	} else {
		//printf("[Cycle %3ld: SLV.Do_R_fwd] (%s) RID 0x%x put Tx_R.\n", nCycle, cpR_new->GetName().c_str(), cpR_new->GetID());
	}
	// cpR_new->Display();
	#endif

	// Maintain
	Delete_UD(upR_new, EUD_TYPE_R); // FIXME Check upR_new deleted
	
	return (ERESULT_TYPE_SUCCESS);
};


// B valid
EResultType CSLV::Do_B_fwd(int64_t nCycle) {
	
	// Check Tx valid 
	if (this->cpTx_B->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};
	
	// Check FIFO
	if (this->cpFIFO_B->IsEmpty() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};
	
	// Pop
	UPUD upB_new = this->cpFIFO_B->Pop();
	if (upB_new == NULL) {
		return (ERESULT_TYPE_FAIL);
	};

	#ifdef DEBUG_SLV	
	assert (upB_new != NULL);
	// this->cpFIFO_B->CheckFIFO();
	// printf("[Cycle %3ld: SLV.Do_B_fwd] (%s) pop FIFO_B.\n", nCycle, upB_new->cpB->GetName().c_str());
	// this->cpFIFO_B->Display();
	#endif
	
	// Get B 
	CPBPkt cpB_new = upB_new->cpB;
	// cpB_new->CheckPkt();
	
	// Put Tx
	this->cpTx_B->PutB(cpB_new);

	#ifdef DEBUG_SLV
	//printf("[Cycle %3ld: SLV.Do_B_fwd] (%s) BID 0x%x put Tx_B.\n", nCycle, cpB_new->GetName().c_str(), cpB_new->GetID());
	// cpB_new->Display();
	#endif
	
	// Maintain
	Delete_UD(upB_new, EUD_TYPE_B);
	
	return (ERESULT_TYPE_SUCCESS);
};


// R ready
EResultType CSLV::Do_R_bwd(int64_t nCycle) {

	// Check Tx valid 
	CPRPkt cpR = this->cpTx_R->GetR();
	if (cpR == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	};
	// cpR->CheckPkt();
	
	// Check Tx ready
	EResultType eAcceptResult = this->cpTx_R->GetAcceptResult();
	if (eAcceptResult == ERESULT_TYPE_ACCEPT) {
		string cRPktName = cpR->GetName();
		if (cpR->IsLast() == ERESULT_TYPE_YES) {

			#ifdef DEBUG_SLV
			//printf("[Cycle %3ld: SLV.Do_R_bwd] (%s) RID 0x%x handshake Tx_R.\n", nCycle, cRPktName.c_str(), cpR->GetID());
			// cpR->Display();
			#endif
		} 
		else {
			#ifdef DEBUG_SLV
			//printf("[Cycle %3ld: SLV.Do_R_bwd] (%s) RID 0x%x handshake Tx_R.\n", nCycle, cRPktName.c_str(), cpR->GetID());
			// cpR->Display();
			#endif
		};

		// Stat
		this->nR++;
	};

	return (ERESULT_TYPE_SUCCESS);
};


// B ready
EResultType CSLV::Do_B_bwd(int64_t nCycle) {

	// Check Tx valid 
	CPBPkt cpB = this->cpTx_B->GetB();
	if (cpB == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	}
	// cpB->CheckPkt();
	
	// Check Tx ready
	EResultType eAcceptResult = this->cpTx_B->GetAcceptResult();
	if (eAcceptResult == ERESULT_TYPE_ACCEPT) {

		#ifdef DEBUG_SLV
		string cBPktName = cpB->GetName();
		//printf("[Cycle %3ld: SLV.Do_B_bwd] (%s) handshake Tx_B.\n", nCycle, cBPktName.c_str());
		// cpB->Display();
		#endif

		// Stat
		this->nB++;
	};

	return (ERESULT_TYPE_SUCCESS);
};


// Set mem cmd pkt
EResultType CSLV::SetMemCmdPkt(SPMemCmdPkt spMemCmdPkt) {

	this->spMemCmdPkt->eMemCmd = spMemCmdPkt->eMemCmd;
	this->spMemCmdPkt->nBank   = spMemCmdPkt->nBank;
	this->spMemCmdPkt->nRow    = spMemCmdPkt->nRow;
	return (ERESULT_TYPE_SUCCESS);
};


// Set mem cmd pkt
EResultType CSLV::SetMemCmdPkt(EMemCmdType eCmd, int nBank, int nRow) {

	this->spMemCmdPkt->eMemCmd = eCmd;
	this->spMemCmdPkt->nBank   = nBank;
	this->spMemCmdPkt->nRow    = nRow;
	return (ERESULT_TYPE_SUCCESS);
};


EResultType CSLV::Handle_PRE_and_REF(int64_t nCycle) {

	this->spMemStatePkt = this->cpMem->GetMemStatePkt();

	/*
	if (this->previous_nBank != -1) {
		// After Every RD/WR, check if need PRE
		if (spMemStatePkt->forced_PRE[this->previous_nBank] == ERESULT_TYPE_YES) {
				
			spMemCmdPkt->eMemCmd = EMEM_CMD_TYPE_PRE;
			spMemCmdPkt->nBank = this->previous_nBank;
			spMemCmdPkt->nRow  = -1; // PRE and REFpb: Row is don't care
			this->SetMemCmdPkt(spMemCmdPkt);
			this->cpMem->SetMemCmdPkt(spMemCmdPkt);

			#ifdef DEBUG_SLV
			string cCmd = Convert_eMemCmd2string(spMemCmdPkt->eMemCmd);
			printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] MC cmd %s (Bank %d, Row 0x%x) sent to bank %d, row 0x%x \n", nCycle, cCmd.c_str(), spMemCmdPkt->nBank, spMemCmdPkt->nRow, spMemCmdPkt->nBank, cpMem->cpBank[spMemCmdPkt->nBank]->GetActivatedRow());
			#endif

			return (ERESULT_TYPE_SUCCESS);

		}
		
		// After Issue RD/WR, check if need REF
		if ( spMemStatePkt->forced_REFI[this->previous_nBank] == ERESULT_TYPE_YES) {

			this->need_Refresing.push_back(this->previous_nBank);
			
			if (spMemStatePkt->IsPRE_ready[this->previous_nBank] == ERESULT_TYPE_YES) { // Ready for PRE
				// Issue PRE first because the 
				spMemCmdPkt->eMemCmd = EMEM_CMD_TYPE_PRE;
				spMemCmdPkt->nBank = this->previous_nBank;
				spMemCmdPkt->nRow  = 0; // PRE and REFpb: Row is don't care
				this->SetMemCmdPkt(spMemCmdPkt);
				this->cpMem->SetMemCmdPkt(spMemCmdPkt);

				#ifdef DEBUG_SLV
				string cCmd = Convert_eMemCmd2string(spMemCmdPkt->eMemCmd);
				printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] MC cmd %s (Bank %d, Row 0x%x) sent to bank %d, row 0x%x \n", nCycle, cCmd.c_str(), spMemCmdPkt->nBank, spMemCmdPkt->nRow, spMemCmdPkt->nBank, cpMem->cpBank[spMemCmdPkt->nBank]->GetActivatedRow());
				#endif
					
				this->prepare_Refresing[this->previous_nBank]=true;
				return (ERESULT_TYPE_SUCCESS);
			}
		}
	}

	// Merge two arrays

	for (auto  element : this->need_Refresing) {
        if (spMemStatePkt->forced_REFI[element] 	== ERESULT_TYPE_YES) {

			if (spMemStatePkt->IsPRE_ready[element] == ERESULT_TYPE_YES) { // Ready for PRE
				// Issue PRE first because the 
				spMemCmdPkt->eMemCmd = EMEM_CMD_TYPE_PRE;
				spMemCmdPkt->nBank 	 = element;
				spMemCmdPkt->nRow  	 = 0; // PRE and REFpb: Row is don't care
				this->SetMemCmdPkt(spMemCmdPkt);
				this->cpMem->SetMemCmdPkt(spMemCmdPkt);

				#ifdef DEBUG_SLV
				string cCmd = Convert_eMemCmd2string(spMemCmdPkt->eMemCmd);
				printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] MC cmd %s (Bank %d, Row 0x%x) sent to bank %d, row 0x%x \n", nCycle, cCmd.c_str(), spMemCmdPkt->nBank, spMemCmdPkt->nRow, spMemCmdPkt->nBank, cpMem->cpBank[spMemCmdPkt->nBank]->GetActivatedRow());
				#endif
					
				this->prepare_Refresing[element]=true;
				return (ERESULT_TYPE_SUCCESS);
			}

			if (spMemStatePkt->IsREF_ready[element] == ERESULT_TYPE_YES) {

				spMemCmdPkt->eMemCmd = EMEM_CMD_TYPE_REFpb;
				spMemCmdPkt->nBank 	 = element;
				spMemCmdPkt->nRow  	 = 0; // PRE and REFpb: Row is don't care
				this->SetMemCmdPkt(spMemCmdPkt);
				this->cpMem->SetMemCmdPkt(spMemCmdPkt);

				#ifdef DEBUG_SLV
				string cCmd = Convert_eMemCmd2string(spMemCmdPkt->eMemCmd);
				printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] MC cmd %s (Bank %d, Row 0x%x) sent to bank %d, row 0x%x \n", nCycle, cCmd.c_str(), spMemCmdPkt->nBank, spMemCmdPkt->nRow, spMemCmdPkt->nBank, cpMem->cpBank[spMemCmdPkt->nBank]->GetActivatedRow());
				#endif

				this->prepare_Refresing[element]=false;
				if (this->need_Refresing.size() > 0) { this->need_Refresing.erase(find(this->need_Refresing.begin(), this->need_Refresing.end(), element)); }
				if (this->previous_nBank == element) { this->previous_nBank = -1; }
				return (ERESULT_TYPE_SUCCESS);
			}

		}
    }
	*/

	for (int element = 0; element < BANK_NUM; element++) {
        if (this->spMemStatePkt->forced_REFI[element] 	== ERESULT_TYPE_YES) {

			if (this->spMemStatePkt->IsPRE_ready[element] == ERESULT_TYPE_YES) { // Ready for PRE
				// Issue PRE first because the 
				this->SetMemCmdPkt(EMEM_CMD_TYPE_PRE, element, 0);
				this->cpMem->SetMemCmdPkt(EMEM_CMD_TYPE_PRE, element, 0);

				#ifdef DEBUG_SLV
				string cCmd = Convert_eMemCmd2string(this->spMemCmdPkt->eMemCmd);
				printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] MC cmd %s (Bank %d, Row 0x%x) sent to bank %d, row 0x%x \n", nCycle, cCmd.c_str(), this->spMemCmdPkt->nBank, this->spMemCmdPkt->nRow, this->spMemCmdPkt->nBank, cpMem->cpBank[this->spMemCmdPkt->nBank]->GetActivatedRow());
				#endif
					
				return (ERESULT_TYPE_SUCCESS);
			}

			if (this->spMemStatePkt->IsREF_ready[element] == ERESULT_TYPE_YES) {

				this->SetMemCmdPkt(EMEM_CMD_TYPE_REFpb, element, 0);
				this->cpMem->SetMemCmdPkt(EMEM_CMD_TYPE_REFpb, element, 0);

				#ifdef DEBUG_SLV
				string cCmd = Convert_eMemCmd2string(this->spMemCmdPkt->eMemCmd);
				printf("[Cycle %3ld: SLV.Do_Ax_fwd_MC_Backend_Request] MC cmd %s (Bank %d, Row 0x%x) sent to bank %d, row 0x%x \n", nCycle, cCmd.c_str(), this->spMemCmdPkt->nBank, this->spMemCmdPkt->nRow, this->spMemCmdPkt->nBank, cpMem->cpBank[this->spMemCmdPkt->nBank]->GetActivatedRow());
				#endif

				return (ERESULT_TYPE_SUCCESS);
			}

		}
    }

	return (ERESULT_TYPE_FAIL);
}

// Debug
EResultType CSLV::CheckLink() {

	assert (this->cpRx_AR != NULL);
	assert (this->cpRx_AW != NULL);
	assert (this->cpRx_W  != NULL);
	assert (this->cpTx_R  != NULL);
	assert (this->cpTx_B  != NULL);
	assert (this->cpRx_AR->GetPair() != NULL);
	assert (this->cpRx_AW->GetPair() != NULL);
	assert (this->cpRx_W ->GetPair() != NULL);
	assert (this->cpTx_R ->GetPair() != NULL);
	assert (this->cpTx_B ->GetPair() != NULL);
	assert (this->cpRx_AR->GetTRxType() != this->cpRx_AR->GetPair()->GetTRxType());
	assert (this->cpRx_AW->GetTRxType() != this->cpRx_AW->GetPair()->GetTRxType());
	assert (this->cpRx_W ->GetTRxType() != this->cpRx_W ->GetPair()->GetTRxType());
	assert (this->cpTx_R ->GetTRxType() != this->cpTx_R ->GetPair()->GetTRxType());
	assert (this->cpTx_B ->GetTRxType() != this->cpTx_B ->GetPair()->GetTRxType());
	assert (this->cpRx_AR->GetPktType() == this->cpRx_AR->GetPair()->GetPktType());
	assert (this->cpRx_AW->GetPktType() == this->cpRx_AW->GetPair()->GetPktType());
	assert (this->cpRx_W ->GetPktType() == this->cpRx_W ->GetPair()->GetPktType());
	assert (this->cpTx_R ->GetPktType() == this->cpTx_R ->GetPair()->GetPktType());
	assert (this->cpTx_B ->GetPktType() == this->cpTx_B ->GetPair()->GetPktType());
	assert (this->cpRx_AR->GetPair()->GetPair() == this->cpRx_AR);
	assert (this->cpRx_AW->GetPair()->GetPair() == this->cpRx_AW);
	assert (this->cpRx_W ->GetPair()->GetPair() == this->cpRx_W);
	assert (this->cpTx_R ->GetPair()->GetPair() == this->cpTx_R);
	assert (this->cpTx_B ->GetPair()->GetPair() == this->cpTx_B);

	return (ERESULT_TYPE_SUCCESS);
};


// Update state
EResultType CSLV::UpdateState(int64_t nCycle) {

	// Update TRx
	this->cpRx_AR->UpdateState();
	this->cpRx_AW->UpdateState();
	// this->cpRx_W ->UpdateState();
	this->cpTx_R ->UpdateState();
	this->cpTx_B ->UpdateState();

	// FIFO
	this->cpFIFO_AR->UpdateState(); // FIXME need? 
	this->cpFIFO_AW->UpdateState();
	this->cpFIFO_R ->UpdateState();
	this->cpFIFO_B ->UpdateState();

	#ifdef MEMORY_CONTROLLER

	// Update bank
	this->cpMem->UpdateState();

	// Update cQ 
	this->cpQ_AR->UpdateState();
	this->cpQ_AW->UpdateState();

	#ifdef STAT_DETAIL
	// Max Q_AR occupancy
	if (this->cpQ_AR->GetCurNum() > this->nMax_Q_AR_Occupancy) {

		#ifdef DEBUG_SLV
		assert (this->cpQ_AR->GetCurNum() == 1 + this->nMax_Q_AR_Occupancy);
		#endif

		this->nMax_Q_AR_Occupancy++;
	};

	// Max Q_AW occupancy
	if (this->cpQ_AW->GetCurNum() > this->nMax_Q_AW_Occupancy) {

		#ifdef DEBUG_SLV
		assert (this->cpQ_AW->GetCurNum() == 1 + this->nMax_Q_AW_Occupancy);
		#endif

		this->nMax_Q_AW_Occupancy++;
	};
	#endif

	#ifdef STAT_DETAIL
	// Total Q_AR occupancy
	this->nTotal_Q_AR_Occupancy += this->cpQ_AR->GetCurNum();

	// Total Q_AW occupancy
	this->nTotal_Q_AW_Occupancy += this->cpQ_AW->GetCurNum();

	// Max Q_AR waiting cycle
	if (this->cpQ_AR->GetMaxCycleWait() > this->nMax_Q_AR_Wait) {
		this->nMax_Q_AR_Wait = this->cpQ_AR->GetMaxCycleWait();
	};
	
	// Max Q_AW waiting cycle
	if (this->cpQ_AW->GetMaxCycleWait() > this->nMax_Q_AW_Wait) {
		this->nMax_Q_AW_Wait = this->cpQ_AW->GetMaxCycleWait();
	};

	// Q empty cycles
	// if (this->cpQ_AR->IsEmpty() == ERESULT_TYPE_YES) {
	//	this->nEmpty_Q_AR_cycles++;
	// };

	// if (this->cpQ_AW->IsEmpty() == ERESULT_TYPE_YES) {
	//	this->nEmpty_Q_AW_cycles++;
	// };
	if (this->cpQ_AR->IsEmpty() == ERESULT_TYPE_YES and this->cpQ_AW->IsEmpty() == ERESULT_TYPE_YES) {
		this->nEmpty_Q_Ax_cycles++;
	};
	#endif

	#ifdef DEBUG_SLV
	// this->CheckSLV();
	if (this->nMax_Q_AR_Wait >= STARVATION_CYCLE) {
	// if (this->nMax_Q_AR_Scheduled_Wait >= STARVATION_CYCLE) {
		// printf("[Cycle %3ld: SLV.UpdateState] AR starvation occurs (%d cycles).\n", nCycle, this->nMax_Q_AR_Wait);
		printf("[Cycle %3ld: SLV.UpdateState] AR starvation occurs (%ld cycles).\n", nCycle, this->nMax_Q_AR_Scheduled_Wait);
		assert (0);
	}
	if (this->nMax_Q_AW_Wait >= STARVATION_CYCLE) {
	// if (this->nMax_Q_AW_Scheduled_Wait >= STARVATION_CYCLE) {
		// printf("[Cycle %3ld: SLV.UpdateState] AW starvation occurs (%d cycles).\n", nCycle, this->nMax_Q_AW_Wait);
		printf("[Cycle %3ld: SLV.UpdateState] AW starvation occurs (%ld cycles).\n", nCycle, this->nMax_Q_AW_Scheduled_Wait);
		assert (0);
	}
	// this->cpQ_AR->Display();
	#endif
	
	#endif	// MEMORY_CONTROLLER

	return (ERESULT_TYPE_SUCCESS);
};


// Debug
EResultType CSLV::CheckSLV() {

	return (ERESULT_TYPE_SUCCESS);
};


// Stat 
EResultType CSLV::PrintStat(int64_t nCycle, FILE *fp) {

	// Debug
	//this->CheckSLV();
	
	printf("--------------------------------------------------------\n");
	printf("\t Name: %s\n", this->cName.c_str());
	printf("--------------------------------------------------------\n");
	printf("\t Number AR                                    : %d\n", 	this->nAR);
	printf("\t Number AW                                    : %d\n",	this->nAW);
	printf("\t Number R                                     : %d\n",	this->nR);
	printf("\t Number B                                     : %d\n",	this->nB);
	printf("\t Number PTW                                   : %d\n\n",	this->nPTW);
	printf("\t Percentage PTW                               : %1.3f\n",     (float)(this->nPTW) / (this->nAR + this->nAW));

	printf("\t Direction change cycle                       : %d\n",	CYCLE_COUNT_DIR_CHANGE);

	printf("\t Utilization R channel                        : %1.2f\n",	(float)(this->nR)/nCycle);
	printf("\t Utilization W channel                        : %1.2f\n\n",	(float)(this->nW)/nCycle);

	printf("\t Number ACT cmd                               : %d\n",	this->nACT_cmd);
	printf("\t Number PRE cmd                               : %d\n",	this->nPRE_cmd);
	printf("\t Number RD  cmd                               : %d\n",	this->nRD_cmd);
	printf("\t Number WR  cmd                               : %d\n",	this->nWR_cmd);
	printf("\t Number of NOP cmd for AR                     : %d\n",	this->nNOP_cmd);
	// printf("\t Bank hit rate                             : %1.2f\n\n",	(float)(this->nRD_cmd + this->nWR_cmd)/(this->nACT_cmd + this->nPRE_cmd + this->nRD_cmd + this->nWR_cmd));
	printf("\t Bank hit rate                                : %1.2f\n\n",	(float)(this->nRD_cmd + this->nWR_cmd)/(this->nPRE_cmd + this->nRD_cmd + this->nWR_cmd));

	#ifdef STAT_DETAIL
	// printf("\t Empty req Q AR cycles                     : %d\n",	this->nEmpty_Q_AR_cycles);
	// printf("\t Empty req Q AW cycles                     : %d\n",	this->nEmpty_Q_AW_cycles);
	printf("\t Empty req Q Ax cycles                        : %d\n\n",	this->nEmpty_Q_Ax_cycles);

	printf("\t Max req Q AR Occupancy                       : %d\n",	this->nMax_Q_AR_Occupancy);
	printf("\t Avg req Q AR Occupancy                       : %1.2f\n",	(float)(this->nTotal_Q_AR_Occupancy)/nCycle);
	printf("\t Max req Q AW Occupancy                       : %d\n",	this->nMax_Q_AW_Occupancy);
	printf("\t Avg req Q AW Occupancy                       : %1.2f\n\n",	(float)(this->nTotal_Q_AW_Occupancy)/nCycle);

	printf("\t Max req Q all AR waiting cycles              : %d\n",	this->nMax_Q_AR_Wait);
	printf("\t Max req Q all AW waiting cycles              : %d\n",	this->nMax_Q_AW_Wait);
	printf("\t Max req Q scheduled AR waiting cycles        : %ld\n",	this->nMax_Q_AR_Scheduled_Wait);
	printf("\t Max req Q scheduled AW waiting cycles        : %ld\n",	this->nMax_Q_AW_Scheduled_Wait);
	printf("\t Avg req Q scheduled AR waiting cycles        : %1.2f\n",	(float)(this->nTotal_Q_AR_Scheduled_Wait)/this->nAR);
	printf("\t Avg req Q scheduled AW waiting cycles        : %1.2f\n",	(float)(this->nTotal_Q_AW_Scheduled_Wait)/this->nAW);
	printf("\t Avg req Q scheduled Ax waiting cycles        : %1.2f\n",	(float)(this->nTotal_Q_AR_Scheduled_Wait + this->nTotal_Q_AW_Scheduled_Wait)/(this->nAR + this->nAW));
	#endif

	this->cpQ_AR->Display();
	this->cpQ_AW->Display();

	printf("--------------------------------------------------------\n");

	//------------------------------------------------------------------
	// FILE out
	//------------------------------------------------------------------
	#ifdef FILE_OUT
	fprintf(fp, "--------------------------------------------------------\n");
	fprintf(fp, "\t Name : %s\n", 	this->cName.c_str());
	fprintf(fp, "--------------------------------------------------------\n");
	fprintf(fp, "\t Number AR                               : %d\n", 	this->nAR);
	fprintf(fp, "\t Number AW                               : %d\n\n", 	this->nAW);

	fprintf(fp, "\t Direction change cycle                  : %d\n", 	CYCLE_COUNT_DIR_CHANGE);

	fprintf(fp, "\t Utilization of R channel                : %1.2f\n",   	(float)(this->nR)/nCycle);
	fprintf(fp, "\t Utilization of W channel                : %1.2f\n\n", 	(float)(this->nW)/nCycle);

	fprintf(fp, "\t Number ACT cmd                          : %d\n",	this->nACT_cmd);
	fprintf(fp, "\t Number PRE cmd                          : %d\n",	this->nPRE_cmd);
	fprintf(fp, "\t Number RD  cmd                          : %d\n",	this->nRD_cmd);
	fprintf(fp, "\t Number WR  cmd                          : %d\n",	this->nWR_cmd);
	fprintf(fp, "\t Number NOP cmd                          : %d\n",	this->nNOP_cmd);
	fprintf(fp, "\t Bank hit rate                           : %1.2f\n\n",	(float)(this->nRD_cmd + this->nWR_cmd)/(this->nACT_cmd + this->nPRE_cmd + this->nRD_cmd + this->nWR_cmd));

	// fprintf(fp, "\t Empty req Q AR cycles                : %d\n",	this->nEmpty_Q_AR_cycles);
	// fprintf(fp, "\t Empty req Q AW cycles                : %d\n",	this->nEmpty_Q_AW_cycles);
	fprintf(fp, "\t Empty req Q Ax cycles                   : %d\n\n",	this->nEmpty_Q_Ax_cycles);

	fprintf(fp, "\t Max req Q AR Occupancy                  : %d\n",	this->nMax_Q_AR_Occupancy);
	fprintf(fp, "\t Avg req Q AR Occupancy                  : %1.2f\n",	(float)(this->nTotal_Q_AR_Occupancy)/nCycle);
	fprintf(fp, "\t Max req Q AW Occupancy                  : %d\n",	this->nMax_Q_AW_Occupancy);
	fprintf(fp, "\t Avg req Q AW Occupancy                  : %1.2f\n\n",	(float)(this->nTotal_Q_AW_Occupancy)/nCycle);

	// fprintf(fp, "\t Max req Q all AR waiting cycles      : %d\n",	this->nMax_Q_AR_Wait);
	// fprintf(fp, "\t Max req Q all AW waiting cycles      : %d\n",	this->nMax_Q_AW_Wait);
	fprintf(fp, "\t Max req Q scheduled AR waiting cycles   : %d\n",	this->nMax_Q_AR_Scheduled_Wait);
	fprintf(fp, "\t Max req Q scheduled AW waiting cycles   : %d\n",	this->nMax_Q_AW_Scheduled_Wait);
	fprintf(fp, "\t Avg req Q scheduled AR waiting cycles   : %1.2f\n",	(float)(this->nTotal_Q_AR_Scheduled_Wait)/nAR);
	fprintf(fp, "\t Avg req Q scheduled AW waiting cycles   : %1.2f\n\n",	(float)(this->nTotal_Q_AW_Scheduled_Wait)/nAW);

	fprintf(fp, "--------------------------------------------------------\n");
	#endif

	return (ERESULT_TYPE_SUCCESS);
};

