//-----------------------------------------------------------
// Filename     : CBUS.cpp 
// Version	: 0.79
// Date         : 18 Nov 2019
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Nx1 Bus definition
//-----------------------------------------------------------
// (1) Buffer-less bus 
//-----------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <iostream>
#include <algorithm>

#include "CBUS.h"


// Constructor
CBUS::CBUS(string cName, int NUM_PORT) {

	// Master interface
	this->cpTx_AR = new CTRx("BUS_Tx_AR", ETRX_TYPE_TX, EPKT_TYPE_AR);
	this->cpTx_AW = new CTRx("BUS_Tx_AW", ETRX_TYPE_TX, EPKT_TYPE_AW);
	this->cpRx_R  = new CTRx("BUS_Rx_R",  ETRX_TYPE_RX, EPKT_TYPE_R);
	this->cpTx_W  = new CTRx("BUS_Tx_W",  ETRX_TYPE_TX, EPKT_TYPE_W);
	this->cpRx_B  = new CTRx("BUS_Rx_B",  ETRX_TYPE_RX, EPKT_TYPE_B);

	// Slave interface
	this->cpRx_AR = new CTRx* [NUM_PORT];
	for (int i=0; i<NUM_PORT; i++) {
		char cName[50];
		sprintf(cName, "BUS_Rx%d_AR",i);
		this->cpRx_AR[i] = new CTRx(cName, ETRX_TYPE_RX, EPKT_TYPE_AR);
	};
	
	this->cpRx_AW = new CTRx* [NUM_PORT];
	for (int i=0; i<NUM_PORT; i++) {
		char cName[50];
		sprintf(cName, "BUS_Rx%d_AW",i);
		this->cpRx_AW[i] = new CTRx(cName, ETRX_TYPE_RX, EPKT_TYPE_AW);
	};

	this->cpTx_R = new CTRx* [NUM_PORT];
	for (int i=0; i<NUM_PORT; i++) {
		char cName[50];
		sprintf(cName, "BUS_Tx%d_R",i);
		this->cpTx_R[i] = new CTRx(cName, ETRX_TYPE_TX, EPKT_TYPE_R);
	};

	this->cpRx_W = new CTRx* [NUM_PORT];
	for (int i=0; i<NUM_PORT; i++) {
		char cName[50];
		sprintf(cName, "BUS_Rx%d_W",i);
		this->cpRx_W[i] = new CTRx(cName, ETRX_TYPE_RX, EPKT_TYPE_W);
	};

	this->cpTx_B = new CTRx* [NUM_PORT];
	for (int i=0; i<NUM_PORT; i++) {
		char cName[50];
		sprintf(cName, "BUS_Tx%d_B",i);
		this->cpTx_B[i] = new CTRx(cName, ETRX_TYPE_TX, EPKT_TYPE_B);
	};

	// Stat
	this->nAR_SI = new int [NUM_PORT];
	this->nAW_SI = new int [NUM_PORT];
	this->nR_SI  = new int [NUM_PORT];
	this->nB_SI  = new int [NUM_PORT];

	for (int i=0; i<NUM_PORT; i++) { this->nAR_SI[i] = -1; };
	for (int i=0; i<NUM_PORT; i++) { this->nAW_SI[i] = -1; };
	for (int i=0; i<NUM_PORT; i++) { this->nR_SI[i]  = -1; };
	for (int i=0; i<NUM_PORT; i++) { this->nB_SI[i]  = -1; };

	// Num bits port (ID encode)
	this->NUM_PORT = NUM_PORT;
	this->BIT_PORT = (int)(ceilf(log2f(NUM_PORT)));
	if(this->BIT_PORT == 0) {this->BIT_PORT = 1;}

	// FIFO
	// this->cpFIFO_AR = new CFIFO("BUS_FIFO_AR", EUD_TYPE_AR, 50);
	this->cpFIFO_AW    = new CFIFO("BUS_FIFO_AW", EUD_TYPE_AW, 50);
	// this->cpFIFO_W  = new CFIFO("BUS_FIFO_W",  EUD_TYPE_W,  50);

	#ifdef CCI_ON
		this->cpTx_AC = new CTRx* [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Tx%d_AC",i);
			this->cpTx_AC[i] = new CTRx(cName, ETRX_TYPE_TX, EPKT_TYPE_AC);
		};
		
		this->cpRx_CD = new CTRx* [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_CD",i);
			this->cpRx_CD[i] = new CTRx(cName, ETRX_TYPE_RX, EPKT_TYPE_CD);
		};

		this->cpRx_CR = new CTRx* [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_CR", i);
			this->cpRx_CR[i] = new CTRx(cName, ETRX_TYPE_RX, EPKT_TYPE_CR);
		};

		this->cpFIFO_MstAW = new CPFIFO [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_MstAW",i);
			this->cpFIFO_MstAW[i] = new CFIFO(cName, EUD_TYPE_AW, 20);
		};

		this->cpFIFO_MstAR = new CPFIFO [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_MstAR",i);
			this->cpFIFO_MstAR[i] = new CFIFO(cName, EUD_TYPE_AR, 20);
		};

		this->cpFIFO_SnoopResp = new CPFIFO [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_SnoopResp",i);
			this->cpFIFO_SnoopResp[i] = new CFIFO(cName, EUD_TYPE_CR, 20);
		};

		this->cpFIFO_SnoopData = new CFIFO("BUS_SnoopData", EUD_TYPE_W, 20);
		this->cpFIFO_Central = new CFIFO("BUS_CentralFIFO", EUD_TYPE_CENTRAL, 64);

		this->nSnoopedMaster = new int[NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			this->nSnoopedMaster[i] = 0;
		}

		this->bArb = new bool[NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			this->bArb[i] = false;
		}
	#endif

	// Arbiter
	this->cpArb_AR	= new CArb("BUS_ARBITER_AR", NUM_PORT);
	this->cpArb_AW	= new CArb("BUS_ARBITER_AW", NUM_PORT);
	
	// Initialize
	this->cName = cName;
	this->nMO_AR = -1;
	this->nMO_AW = -1;
	this->nRTrans = 0;
	this->nBTrans = 0;
	this->nR_GEN_NUM = 0;
	this->nB_GEN_NUM = 0;
};


// Destructor
CBUS::~CBUS() {

	// Debug
	assert (this->cpTx_AR != NULL);
	assert (this->cpTx_AW != NULL);
	assert (this->cpTx_W  != NULL);
	assert (this->cpRx_R  != NULL);
	assert (this->cpRx_B  != NULL);

	// MI
	delete (this->cpTx_AR);
	delete (this->cpRx_R);
	delete (this->cpTx_AW);
	delete (this->cpTx_W);
	delete (this->cpRx_B);

	// SI
	for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpRx_AR[i]); };
	for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpRx_AW[i]); };
	for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpRx_W[i]);  };
	for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpTx_R[i]);  };
	for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpTx_B[i]);  };

	delete [] this->cpRx_AR;
	delete [] this->cpRx_AW;
	delete [] this->cpRx_W;
	delete [] this->cpTx_R;
	delete [] this->cpTx_B;

	// FIFO
	delete (this->cpFIFO_AW);

	#ifdef CCI_ON
		for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpTx_AC[i]); };
		for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpRx_CD[i]); };
		for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpRx_CR[i]); };

		for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpFIFO_MstAW[i]); };
		for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpFIFO_MstAR[i]); };
		for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpFIFO_SnoopResp[i]); };

		delete [] this->cpTx_AC;
		delete [] this->cpRx_CD;
		delete [] this->cpRx_CR;

		delete [] this->cpFIFO_MstAW;
		delete [] this->cpFIFO_MstAR;
		delete [] this->cpFIFO_SnoopResp;

		delete (this->cpFIFO_SnoopData);
		delete (this->cpFIFO_Central);
	#endif
	// delete (this->cpFIFO_AR);
	// delete (this->cpFIFO_W);

	// Arb
	delete (this->cpArb_AR);
	delete (this->cpArb_AW);

	// Stat
	delete [] this->nAR_SI;
	delete [] this->nAW_SI;
	delete [] this->nR_SI;
	delete [] this->nB_SI;

	this->cpTx_AR = NULL;
	this->cpTx_AW = NULL;
	this->cpTx_W  = NULL;
	this->cpRx_R  = NULL;
	this->cpRx_B  = NULL;

	for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_AR[i] = NULL; };
	for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_AW[i] = NULL; };
	for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_W[i]  = NULL; };
	for (int i=0; i<this->NUM_PORT; i++) { this->cpTx_R[i]  = NULL; };
	for (int i=0; i<this->NUM_PORT; i++) { this->cpTx_B[i]  = NULL; };

	this->cpFIFO_AW = NULL;
	// this->cpFIFO_AR = NULL;
	// this->cpFIFO_W  = NULL;

	#ifdef CCI_ON
		for (int i=0; i<this->NUM_PORT; i++) { this->cpTx_AC[i] = NULL; };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_CD[i] = NULL; };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_CR[i] = NULL; };

		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_MstAW[i] = NULL; };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_MstAR[i] = NULL; };
		this->cpFIFO_SnoopData = NULL;
		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_SnoopResp[i] = NULL; };
		this->cpFIFO_SnoopData = NULL;
		this->cpFIFO_Central = NULL;

		delete[] this->nSnoopedMaster;
		delete[] this->bArb;
	#endif

	this->cpArb_AR = NULL;
	this->cpArb_AW = NULL;
};


// Initialize
EResultType CBUS::Reset() {

	// MI
	this->cpTx_AR->Reset();
	this->cpTx_AW->Reset();
	this->cpTx_W ->Reset();
	this->cpRx_R ->Reset();
	this->cpRx_B ->Reset();

        // SI 
	for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_AR[i]->Reset(); };
	for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_AW[i]->Reset(); };
	for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_W[i] ->Reset(); };
	for (int i=0; i<this->NUM_PORT; i++) { this->cpTx_R[i] ->Reset(); };
	for (int i=0; i<this->NUM_PORT; i++) { this->cpTx_B[i] ->Reset(); };

	for (int i=0; i<this->NUM_PORT; i++) { this->nAR_SI[i] = 0; };
	for (int i=0; i<this->NUM_PORT; i++) { this->nAW_SI[i] = 0; };
	for (int i=0; i<this->NUM_PORT; i++) { this->nR_SI[i]  = 0; };
	for (int i=0; i<this->NUM_PORT; i++) { this->nB_SI[i]  = 0; };

	// FIFO
	this->cpFIFO_AW->Reset();	
	// this->cpFIFO_AR->Reset();	
	// this->cpFIFO_W->Reset();	

	#ifdef CCI_ON
		for (int i=0; i<this->NUM_PORT; i++) { this->cpTx_AC[i] ->Reset(); };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_CD[i] ->Reset(); };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_CR[i] ->Reset(); };

		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_MstAW[i]->Reset(); };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_MstAR[i]->Reset(); };
		this->cpFIFO_SnoopData->Reset();
		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_SnoopResp[i]->Reset(); };
		this->cpFIFO_Central->Reset();

		for (int i=0; i<this->NUM_PORT; i++) {
			this->nSnoopedMaster[i] = 0;
			this->bArb[i] = false;
		}
		this->m_outstandingMemARID.clear();
		this->m_outstandingMemAWID.clear();
		this->m_outstandingMemARAddr.clear();
		this->m_outstandingMemAWAddr.clear();
	#endif

	// Arb
	this->cpArb_AR->Reset();	
	this->cpArb_AW->Reset();	

	this->nMO_AR = 0;
	this->nMO_AW = 0;

	return (ERESULT_TYPE_SUCCESS);
};


#ifndef CCI_ON
//------------------------------------------------------
// AR valid
//------------------------------------------------------
EResultType CBUS::Do_AR_fwd(int64_t nCycle) {

	if(nCycle % BUS_LATENCY != 0){
		return (ERESULT_TYPE_FAIL);
	}

	// Check Tx valid 
	if (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};

	// Check MO
	if (this->GetMO_AR() >= MAX_MO_COUNT) {
		return (ERESULT_TYPE_FAIL);
	};

	// Arbiter
	int nCandidateList[this->NUM_PORT];
	for (int nPort = 0; nPort < this->NUM_PORT; nPort++) {

		// Check remote-Tx valid 
		if (this->cpRx_AR[nPort]->GetPair()->IsBusy() == ERESULT_TYPE_YES) {
			nCandidateList[nPort] = 1;
		} 
		else {
			nCandidateList[nPort] = 0;
		};
	};

	int nArbResult = this->cpArb_AR->Arbitration(nCandidateList);

	// Check  
	if (nArbResult == -1) { 		// Nothing arbitrated
		return (ERESULT_TYPE_SUCCESS);
	};

	#ifdef DEBUG
	assert (nArbResult < this->NUM_PORT);
	assert (nArbResult > -1);
	#endif

	// Get Ax
	CPAxPkt cpAR = this->cpRx_AR[nArbResult]->GetPair()->GetAx();

	#ifdef DEBUG
	assert (cpAR != NULL);
	// cpAR->CheckPkt();
	#endif

	// Put Rx
	this->cpRx_AR[nArbResult]->PutAx(cpAR);
	#ifdef DEBUG_BUS
	printf("[Cycle %3ld: %s.Do_AR_fwd] (%s) put Rx_AR[%d].\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str(), nArbResult);
	// cpAR->Display();
	#endif

	// Stat
	this->nAR_SI[nArbResult]++;

	// Encode ID
	int     nID   = cpAR->GetID();
	int64_t nAddr = cpAR->GetAddr();
	int     nLen  = cpAR->GetLen();
	nID = (nID << this->BIT_PORT) + nArbResult;

	// Set pkt
	cpAR->SetPkt(nID, nAddr, nLen);

	// Put Tx
	this->cpTx_AR->PutAx(cpAR);
	#ifdef DEBUG_BUS
	printf("[Cycle %3ld: %s.Do_AR_fwd] (%s) put Tx_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
	// cpAR->Display();
	#endif

	return (ERESULT_TYPE_SUCCESS);	
};
#else
//---------------------------------------------------------------------------------------------------
// 	1. FIXME: Getting transactions from Masters (put into local Rx_AR ports) = assert READY.
//		1.1. If the BUS's Rx_AR is gotten, MASTERs will keep the current transactions at its ports. 
//	2. Encode the AR transactions and push it into cpFIFO_MstAR.
//	3. The copying only happens if:
//		2.1. cpFIFO_MstAR is not full.
//		2.2. The transaction is valid at the Master's ports.
//---------------------------------------------------------------------------------------------------
EResultType CBUS::Do_AR_fwd(int64_t nCycle) {

	// ---- Check MO ---
	if (this->GetMO_AR() >= MAX_MO_COUNT) {
		return (ERESULT_TYPE_FAIL);
	};

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for(int nPort = 0; nPort < this->NUM_PORT; nPort++) {
		
		// ---- 1. Checking receiving conditions ---
		// Do not accept the transactions if the FIFO is full.
		if (this->cpFIFO_MstAR[nPort]->IsFull() == ERESULT_TYPE_YES) {
			continue;
		}

		// Get Ax - Check if the port is busy.
		// Skip if MASTERs' ports do not issue transactions.
		if (this->cpRx_AR[nPort]->GetPair()->IsBusy() == ERESULT_TYPE_NO) {
			continue;
		}

		// ---- 2. BUS READY receives AR transactions ---
		// Get the transactions from MASTER ports.
		CPAxPkt cpAR = this->cpRx_AR[nPort]->GetPair()->GetAx();

		// Encoding AR transactions' ID
		int     nID    = cpAR->GetID();
		int64_t nAddr  = cpAR->GetAddr();
		int     nLen   = cpAR->GetLen();
		int     nSnoop = cpAR->GetSnoop();

		nID = (nID << this->BIT_PORT) + nPort;
		cpAR->SetPkt(nID, nAddr, nLen, nSnoop);

		#ifdef DEBUG
		assert (cpAR != NULL);
		// cpAR->CheckPkt();
		#endif

		// Put the AR transactions to local port = asserting READY.
		this->cpRx_AR[nPort]->PutAx(cpAR);

		// ---- 3. Push AR transactions to cpFIFO_MstAR ---
		// Instead of putting Rx to Tx, we put to a FIFO and maintain the FIFO to waiting snoops.
		UUD udAR;
		udAR.cpAR = cpAR;
		this->cpFIFO_MstAR[nPort]->Push(&udAR); // FIXME: Only READ

		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_AR_fwd] (%s) push to FIFO[%d].\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str(), nPort);
		#endif
	}

	return (ERESULT_TYPE_SUCCESS);	
};
#endif


//------------------------------------------------------
// AW valid
//------------------------------------------------------
#ifndef CCI_ON
EResultType CBUS::Do_AW_fwd(int64_t nCycle) {

	if(nCycle % BUS_LATENCY != 0){
		return (ERESULT_TYPE_FAIL);
	}

	// Check Tx valid 
	if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};

	// Check MO
	if (this->GetMO_AW() >= MAX_MO_COUNT) {
		return (ERESULT_TYPE_FAIL);
	};

	// Arbiter
	int nCandidateList[this->NUM_PORT];
	for (int nPort = 0; nPort < this->NUM_PORT; nPort++) {

		// Check remote-Tx valid 
		if (this->cpRx_AW[nPort]->GetPair()->IsBusy() == ERESULT_TYPE_YES) {
			nCandidateList[nPort] = 1;
		} 
		else {
			nCandidateList[nPort] = 0;
		};
	};

	int nArbResult = this->cpArb_AW->Arbitration(nCandidateList);

	// Check 
	if (nArbResult == -1) {			// Nothing arbitrated
		return (ERESULT_TYPE_SUCCESS);
	};

	#ifdef DEBUG
	assert (nArbResult < this->NUM_PORT);
	assert (nArbResult > -1);
	#endif

	// Get Ax
	CPAxPkt cpAW = this->cpRx_AW[nArbResult]->GetPair()->GetAx();

	#ifdef DEBUG
	assert (cpAW != NULL);
	// cpAW->CheckPkt();
	#endif

	// Put Rx
	this->cpRx_AW[nArbResult]->PutAx(cpAW);
	// printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) put Rx_AW[%d].\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str(), nArbResult);
	// cpAW->Display();

	// Stat
	this->nAW_SI[nArbResult]++;

	// Encode ID
	int     nID   = cpAW->GetID();
	int64_t nAddr = cpAW->GetAddr();
	int     nLen  = cpAW->GetLen();
	nID = (nID << this->BIT_PORT) + nArbResult;

	// Set pkt
	cpAW->SetPkt(nID, nAddr, nLen);
	
	// Put Tx
	this->cpTx_AW->PutAx(cpAW);
		// printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) put Tx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
	// cpAW->Display();


	#ifdef USE_W_CHANNEL
	// Generate new Ax arbitrated 
	UPUD upAW_new = new UUD;
	upAW_new->cpAW = Copy_CAxPkt(cpAW);

	// Push AW FIFO
	this->cpFIFO_AW->Push(upAW_new);

	// Debug	
	// printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) push FIFO_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName());
	// this->cpFIFO_AW->Display();
	// this->cpFIFO_AW->CheckFIFO();
	
	// Maintain     
	Delete_UD(upAW_new, EUD_TYPE_AW);

	#endif

	return (ERESULT_TYPE_SUCCESS);	
};
#else
//---------------------------------------------------------------------------------------------------
// (The flow is the same as READ transactions)
// 	1. Getting transactions from Masters (put into local Rx_AW ports) = READY.
//		1.1. If the BUS's Rx_AW is gotten, MASTERs will keep the current transactions at its ports. 
//	2. Encode the AW transactions and push it into cpFIFO_MstAW.
//	3. The copying only happens if:
//		2.1. cpFIFO_MstAW is not full.
//		2.2. The transaction is valid at the Master's ports.
//---------------------------------------------------------------------------------------------------
EResultType CBUS::Do_AW_fwd(int64_t nCycle) {
	
	// ---- Check MO ---
	if (this->GetMO_AW() >= MAX_MO_COUNT) {
		return (ERESULT_TYPE_FAIL);
	};

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for(int nPort = 0; nPort < this->NUM_PORT; nPort++) {

		// ---- 1. Checking receiving conditions ---
		// Do not accept the transactions if the FIFO is full.
		if (this->cpFIFO_MstAW[nPort]->IsFull() == ERESULT_TYPE_YES) {
			continue;
		}

		// Get Ax - Check if the port is busy.
		// Skip if MASTERs' ports do not issue transactions.
		if (this->cpRx_AW[nPort]->GetPair()->IsBusy() == ERESULT_TYPE_NO) {
			continue;
		}

		// ---- 2. BUS READY receives AW transactions ---
		// Get the transactions from MASTER ports.
		CPAxPkt cpAW = this->cpRx_AW[nPort]->GetPair()->GetAx();

		// Encoding AR transactions' ID
		int     nID    = cpAW->GetID();
		int64_t nAddr  = cpAW->GetAddr();
		int     nLen   = cpAW->GetLen();
		int     nSnoop = cpAW->GetSnoop();

		nID = (nID << this->BIT_PORT) + nPort; // FIXME: Master 3
		cpAW->SetPkt(nID, nAddr, nLen, nSnoop);

		#ifdef DEBUG
		assert (cpAW != NULL);
		// cpAW->CheckPkt();
		#endif

		// Pu the AR transactions to local port = asserting READY.
		this->cpRx_AW[nPort]->PutAx(cpAW);

		// ---- 3. Push AR transactions to cpFIFO_MstAR ---
		// Instead of putting Rx to Tx, we put to a FIFO and maintain the FIFO to waiting snoops.
		UUD udAW;
		udAW.cpAW = cpAW;
		this->cpFIFO_MstAW[nPort]->Push(&udAW); // FIXME: Only READ
	}

	return (ERESULT_TYPE_SUCCESS);	
};
#endif


//------------------------------------------------------
// W valid
//------------------------------------------------------
EResultType CBUS::Do_W_fwd(int64_t nCycle) {

	if(nCycle % BUS_LATENCY != 0){
		return (ERESULT_TYPE_FAIL);
	}

	// Check Tx valid
	if (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};

	#ifdef CCI_ON
	this->Do_W_snoop_fwd(nCycle);
	if (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};
	#endif

	// Check FIFO_AW
	if (this->cpFIFO_AW->IsEmpty() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Get arbitrated port
	// int nArb = this->cpFIFO_AW->GetTop()->cpAW->GetID() % this->NUM_PORT; // Only when power-of-2 ports FIXME
	int AxID = this->cpFIFO_AW->GetTop()->cpAW->GetID();
	int nArb = GetPortNum(AxID);

	#ifdef DEBUG	
	assert (nArb > -1);
	assert (nArb < this->NUM_PORT);
	#endif

	// Check remote-Tx valid
	if (this->cpRx_W[nArb]->GetPair()->IsBusy() == ERESULT_TYPE_NO) {
		return (ERESULT_TYPE_SUCCESS);	
	};

	// Get W
	CPWPkt cpW = this->cpRx_W[nArb]->GetPair()->GetW();
	
	#ifdef DEBUG
	assert (cpW != NULL);
	// cpW->CheckPkt();
	#endif

	// Put Rx 
	this->cpRx_W[nArb]->PutW(cpW);
	// printf("[Cycle %3ld: %s.Do_W_fwd] (%s) put Rx_W[%d].\n", nCycle, this->cName.c_str(), cpW->GetName().c_str(), nArbResult);
	// cpW->Display();

	// Encode ID
	int nID = cpW->GetID();
	nID = (nID << this->BIT_PORT) + nArb;

	// Set pkt
	cpW->SetID(nID);

	// Put Tx 
	this->cpTx_W->PutW(cpW);

	// Debug
	if (cpW->IsLast() == ERESULT_TYPE_YES) {	
		printf("[Cycle %3ld: %s.Do_W_fwd] (%s) WLAST put Tx_W.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
		// cpW->Display();
	} 
	else {
		printf("[Cycle %3ld: %s.Do_W_fwd] (%s) put Tx_W.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
		// cpW->Display();
	};

	return (ERESULT_TYPE_SUCCESS);	
};

#ifndef CCI_ON
//------------------------------------------------------
// AR ready
//------------------------------------------------------
EResultType CBUS::Do_AR_bwd(int64_t nCycle) {

	//if(nCycle % BUS_LATENCY != 0){
	//	return (ERESULT_TYPE_FAIL);
	//}

	// Check Tx ready
	if (this->cpTx_AR->GetAcceptResult() == ERESULT_TYPE_REJECT) {
		return (ERESULT_TYPE_SUCCESS);
	};

	CPAxPkt cpAR = this->cpTx_AR->GetAx();

	#ifdef DEBUG_BUS
	assert (cpAR != NULL);
	assert (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES);
	string cARPktName = cpAR->GetName();
	printf("[Cycle %3ld: %s.Do_AR_bwd] (%s) handshake Tx_AR.\n", nCycle, this->cName.c_str(), cARPktName.c_str());
	// cpAR->Display();
	#endif

	// Get arbitrated port FIXME Need? No
	int nArbResult = this->cpArb_AR->GetArbResult();
	assert (nArbResult > -1);
	assert (nArbResult < this->NUM_PORT);

	// Debug
	// int nPort = (cpAR->GetID()) % this->NUM_PORT; // Only when this->NUM_PORT power-of-2. FIXME
	// assert (nArbResult == nPort);

	// Check Rx valid 
	// if (this->cpRx_AR[nPort]->IsBusy() == ERESULT_TYPE_YES) {
	// 	// Set ready	
	// 	this->cpRx_AR[nPort]->SetAcceptResult(ERESULT_TYPE_ACCEPT);
	// 
	// 	// MO
	// 	if (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES ) {
	// 		this->Increase_MO_AR();
	// 	};
	// }; 

	int nID = cpAR->GetID();
	int nPort = GetPortNum(nID); 

	#ifdef DEBUG // FIXME Need? No
	assert (nArbResult == nPort);
	#endif

	// Check Rx valid 
	if (this->cpRx_AR[nPort]->IsBusy() == ERESULT_TYPE_YES) {
		// Set ready	
		this->cpRx_AR[nPort]->SetAcceptResult(ERESULT_TYPE_ACCEPT);
		#ifdef DEBUG_BUS
		printf("[Cycle %3ld: %s.Do_AR_bwd] (%s) handshake Rx_AR[%d].\n", nCycle, this->cName.c_str(), (this->cpRx_AR[nPort]->GetAx()->GetName()).c_str(), nPort);
		#endif
		// MO
		if (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES ) {
			this->Increase_MO_AR();
		};
	}; 

	return (ERESULT_TYPE_SUCCESS);
};
#else
//---------------------------------------------------------------------------------------------------
// AR ready
//	1. Set the acceptance results. Indicating the BUS is ready to receive AR following transactions from MASTER.
//	3. The AR READY only happens if:
//		2.1. cpFIFO_MstAR is not full (have slots).
//		2.2. Already receive a transaction. Reset the AcceptResult for the next transactions.
//---------------------------------------------------------------------------------------------------
EResultType CBUS::Do_AR_bwd(int64_t nCycle) {

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for (int nPort = 0; nPort < this->NUM_PORT; nPort++) {
		
		// ---- 1. Checking receiving conditions ---
		// Do not accept the transactions if the FIFO is full (after pushing the current transactions).
		//if (this->cpFIFO_MstAR[nPort]->IsFull() == ERESULT_TYPE_YES) { continue; }

		// ---- 2. Reset the AcceptResult = Set Ready to receive the next transactions ---
		if (this->cpRx_AR[nPort]->IsBusy() == ERESULT_TYPE_YES) {
			// Set ready
			this->cpRx_AR[nPort]->SetAcceptResult(ERESULT_TYPE_ACCEPT);

			#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_AR_bwd] (%s) handshake Rx_AR[%d].\n", nCycle, this->cName.c_str(), (this->cpRx_AR[nPort]->GetAx()->GetName()).c_str(), nPort);
			#endif
		};
	};

	// ---- Increasing the outstandings if BUS successfully puts transactions into Tx_AR port ---
	if (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES ) {
		this->Increase_MO_AR();
	};

	return (ERESULT_TYPE_SUCCESS);
};
#endif

//------------------------------------------------------
// AW ready
//------------------------------------------------------
#ifndef CCI_ON
EResultType CBUS::Do_AW_bwd(int64_t nCycle) {

	//if(nCycle % BUS_LATENCY != 0){
	//	return (ERESULT_TYPE_FAIL);
	//}

	// Check Tx ready
	if (this->cpTx_AW->GetAcceptResult() == ERESULT_TYPE_REJECT) {
		return (ERESULT_TYPE_SUCCESS);
	};

	CPAxPkt cpAW = this->cpTx_AW->GetAx();

	#ifdef DEBUG_BUS
	assert (cpAW != NULL);
	assert (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES);
	printf("[Cycle %3ld: %s.Do_AW_bwd] (%s) handshake Tx_AW.\n", nCycle, this->cName.c_str(), (cpAW->GetName()).c_str());
	// cpAW->Display();
	#endif

	#ifdef DEBUG
	// Get arbitrated port  FIXME Need? No
	int nArbResult = this->cpArb_AW->GetArbResult();
	assert (nArbResult > -1);
	assert (nArbResult < this->NUM_PORT);
	#endif

	// Debug
	// int nPort = (cpAW->GetID()) % this->NUM_PORT; // Only when powe-of-2
	// assert (nArbResult == nPort);

	// // Check Rx valid 
	// if (this->cpRx_AW[nPort]->IsBusy() == ERESULT_TYPE_YES) {
	// 	// Set ready
	// 	this->cpRx_AW[nPort]->SetAcceptResult(ERESULT_TYPE_ACCEPT);
	// 
	// 	// MO
	// 	if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES ) {
	// 		this->Increase_MO_AW();
	// 	};
	// }; 

	int nID = cpAW->GetID();
	int nPort = GetPortNum(nID); 

	#ifdef DEBUG // FIXME Need? No
	assert (nArbResult == nPort);
	#endif

	// Check Rx valid 
	if (this->cpRx_AW[nPort]->IsBusy() == ERESULT_TYPE_YES) {
		// Set ready
		this->cpRx_AW[nPort]->SetAcceptResult(ERESULT_TYPE_ACCEPT);
		#ifdef DEBUG_BUS
		printf("[Cycle %3ld: %s.Do_AW_bwd] (%s) handshake Rx_AW[%d].\n", nCycle, this->cName.c_str(), (this->cpRx_AW[nPort]->GetAx()->GetName()).c_str(), nPort);
		#endif

		// MO
		if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES ) {
			this->Increase_MO_AW();
		};
	}; 

	return (ERESULT_TYPE_SUCCESS);
};
#else
//---------------------------------------------------------------------------------------------------
// AW ready
// (The flow is the same as READ transactions)
//	1. Set the acceptance results. Indicating the BUS is ready to receive AW following transactions from MASTER.
//	3. The AW READY only happens if:
//		2.1. cpFIFO_MstAW is not full (have slots).
//		2.2. Already receive a transaction. Reset the AcceptResult for the next transactions.
//---------------------------------------------------------------------------------------------------
EResultType CBUS::Do_AW_bwd(int64_t nCycle) {

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for (int nPort = 0; nPort < this->NUM_PORT; nPort++) {

		// ---- 1. Checking receiving conditions ---
		// Do not accept the transactions if the FIFO is full (after pushing the current transactions).
		// if (this->cpFIFO_MstAW[nPort]->IsFull() == ERESULT_TYPE_YES) { continue; }

		// ---- 2. Reset the AcceptResult = Set Ready to receive the next transactions ---
		if (this->cpRx_AW[nPort]->IsBusy() == ERESULT_TYPE_YES) {
			// Set ready	
			this->cpRx_AW[nPort]->SetAcceptResult(ERESULT_TYPE_ACCEPT);

			#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_AW_bwd] (%s) handshake Rx_AW[%d].\n", nCycle, this->cName.c_str(), (this->cpRx_AW[nPort]->GetAx()->GetName()).c_str(), nPort);
			#endif
		};
	};

	// ---- Increasing the outstandings if BUS successfully puts transactions into Tx_AW port ---
	if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES ) {
		this->Increase_MO_AW();
	};

	return (ERESULT_TYPE_SUCCESS);
};
#endif

#ifdef CCI_ON
//-------------------------------------------------------
// AC valid
//	1. Scanning Ready-to-Snooped MASTERs.
//	2. Encoding the snooping transactions.
//	3. Issuing snooping to other MASTERs.
//-------------------------------------------------------
EResultType CBUS::Do_AC_fwd(int64_t nCycle) {

	// ----------------------------------------------------------------------------------
	// ---- Part 1: Snoop Issuance (Triggered after Central FIFO confirmation) ----------
	// ----------------------------------------------------------------------------------
	if (this->cpFIFO_Central->IsEmpty() == ERESULT_TYPE_NO) {

		// Get the top transaction in central FIFO (the one currently being snooped)
		UPUD upTop = this->cpFIFO_Central->GetTop();
		CPCentral centralEntry = upTop->cpCentral;
		CPAxPkt initTrans = centralEntry->cpAx;

		if (centralEntry->nCounter >= centralEntry->nLength) {
			// No need to issue snoops if all beats have been snooped.
			// This can happen if the transaction is already completed but still in the central FIFO for data issuing.
			return (ERESULT_TYPE_SUCCESS);
		}

		// Get the master ID from the transaction ID encoding
		int initMaster = GetPortNum(initTrans->GetID());
		assert (initMaster >= 0);

		// Only proceed with snoop issuance if there are still beats to be snooped (nCounter < nLength).
		// This ensures we handle multi-beat transactions correctly and do not issue snoops for completed transactions.
		if (centralEntry->nCounter < centralEntry->nLength) {

			// 1. Encoding the snooping transactions
			if (centralEntry->cpAC == NULL) {
				CPACPkt SnoopPkt = new CACPkt;
				SnoopPkt->SetSnoop(initTrans->GetSnoop());
				SnoopPkt->SetName(initTrans->GetName());
				SnoopPkt->SetAddr(initTrans->GetAddr());

				// 1.1. READ transactions: Snoop encoding is mostly 1-to-1 mapping, except for some special cases like WriteBack/WriteClean which can be treated as ReadUnique for snooping purposes.
				if (initTrans->GetDir() == ETRANS_DIR_TYPE_READ) {

					SnoopPkt->SetTransDirType(ETRANS_DIR_TYPE_READ);

					if (initTrans->GetSnoop() == 0b0000) SnoopPkt->SetSnoop(0b0000);       // ReadOnce           -> ReadOnce
					else if (initTrans->GetSnoop() == 0b0001) SnoopPkt->SetSnoop(0b0001);  // ReadClean          -> ReadClean
					else if (initTrans->GetSnoop() == 0b0010) SnoopPkt->SetSnoop(0b0010);  // ReadNotSharedDirty -> ReadNotSharedDirty
					else if (initTrans->GetSnoop() == 0b0011) SnoopPkt->SetSnoop(0b0011);  // ReadShared         -> ReadShared
					else if (initTrans->GetSnoop() == 0b0111) SnoopPkt->SetSnoop(0b0111);  // ReadUnique         -> ReadUnique
					else if (initTrans->GetSnoop() == 0b1011) SnoopPkt->SetSnoop(0b1001);  // CleanUnique        -> CleanInvalid
					else if (initTrans->GetSnoop() == 0b1100) SnoopPkt->SetSnoop(0b1101);  // MakeUnique         -> MakeInvalid
					else if (initTrans->GetSnoop() == 0b1000) SnoopPkt->SetSnoop(0b1000);  // CleanShared        -> CleanShared
					else if (initTrans->GetSnoop() == 0b1001) SnoopPkt->SetSnoop(0b1001);  // CleanInvalid       -> CleanInvalid
					else if (initTrans->GetSnoop() == 0b1101) SnoopPkt->SetSnoop(0b1101);  // MakeInvalid        -> MakeInvalid
					else assert(false);
				
				// 1.2. WRITE transactions: More complex encoding based on the snoop type. For example, WriteLineUnique may require a MakeInvalidate if snooped by multiple targets,
				} else {

					SnoopPkt->SetTransDirType(ETRANS_DIR_TYPE_WRITE);

					if (initTrans->GetSnoop() == 0b0000) 		SnoopPkt->SetSnoop(0b1001); // WriteUnique		-> CleanInvalid
					else if (initTrans->GetSnoop() == 0b0001) 	SnoopPkt->SetSnoop(0b1101); // WriteLineUnique	-> MakeInvalid
					else 										assert(false);

				}
				// Store the encoded snoop packet in the central entry for issuance to targets.
				// For multi-beat transactions, we will reuse this packet for each beat.
				centralEntry->cpAC = SnoopPkt;
			}

			// 2. Issuing to target masters
			int allTargetsMask = (1 << this->NUM_PORT) - 1;
			allTargetsMask &= ~(1 << initMaster);
			allTargetsMask &= ~SNOOP_MASK;

			for (int snoopMaster = 0; snoopMaster < this->NUM_PORT; snoopMaster++) {

				if (snoopMaster == initMaster) continue;
				if ((SNOOP_MASK & (1 << snoopMaster)) != 0) continue;

				// Check if every master whether snoop or not.
				if ((centralEntry->nSnoopMask & (1 << snoopMaster)) == 0) {

					if (this->cpTx_AC[snoopMaster]->IsBusy() == ERESULT_TYPE_NO) {

						if (centralEntry->cpAC->GetDir() == ETRANS_DIR_TYPE_UNDEFINED) break;

						this->cpTx_AC[snoopMaster]->PutAC(centralEntry->cpAC);

						centralEntry->nSnoopMask |= (1 << snoopMaster);

						#ifdef DEBUG_BUS
							printf("[Cycle %3ld: %s.Do_AC_fwd] (%s) put Tx_AC[%d] = %x.\n", nCycle, this->cName.c_str(), centralEntry->cpAC->GetName().c_str(), snoopMaster, centralEntry->nSnoopMask);
						#endif
					} 
				}
			}

			// 3. After attempting to issue snoops to all targets, check if all targets have been snooped (nSnoopMask matches allTargetsMask).
			// If so, we can update the counter and potentially issue the next beat for multi-beat transactions.
			if ((centralEntry->nSnoopMask & allTargetsMask) == allTargetsMask) { // All master is snoopped.

				centralEntry->cpAC->SetTransDirType(ETRANS_DIR_TYPE_UNDEFINED);
				
				// If there are more beats to be snooped for this transaction, we reset the snoop mask to start the next round of snooping for the next beat.
				centralEntry->nSnoopMask = 0; // Reset for next beat issuance
			}
		}
	}

	// ----------------------------------------------------------------------------------
	// ---- Part 2: Arbitration & Decoupled Movement to Central FIFO (Pipeline Stage) ----
	// ----------------------------------------------------------------------------------
	// Dependency Check: Subsequent transactions must stall until the current burst 
	// in cpFIFO_Central finishes. We enforce this by only allowing ONE active 
	// transaction in cpFIFO_Central at a time for the coherence phase.
	for (int i = 0; i < this->NUM_PORT; i++) {
		int initMaster = i; // simple scan
		CPAxPkt initTrans = NULL;
			
		// Arbitration between AR and AW per master port
		if (this->bArb[initMaster]) {
			if (this->cpFIFO_MstAR[initMaster]->IsEmpty() == ERESULT_TYPE_NO) {
				initTrans = this->cpFIFO_MstAR[initMaster]->GetTop()->cpAR;
			} else if (this->cpFIFO_MstAW[initMaster]->IsEmpty() == ERESULT_TYPE_NO) {
				initTrans = this->cpFIFO_MstAW[initMaster]->GetTop()->cpAW;
				this->bArb[initMaster] = false;
			}
		} else {
			if (this->cpFIFO_MstAW[initMaster]->IsEmpty() == ERESULT_TYPE_NO) {
				initTrans = this->cpFIFO_MstAW[initMaster]->GetTop()->cpAW;
			} else if (this->cpFIFO_MstAR[initMaster]->IsEmpty() == ERESULT_TYPE_NO) {
				initTrans = this->cpFIFO_MstAR[initMaster]->GetTop()->cpAR;
				this->bArb[initMaster] = true;
			}
		}

		if (initTrans != NULL) {
			// Identify if snoop is required for this transaction
			bool bSnoopReq = true;
			if (initTrans->GetDir() == ETRANS_DIR_TYPE_WRITE) {
				int snoop = initTrans->GetSnoop();
				// WriteBack (3), WriteClean (2), WriteEvict (5), Evict (4) do not require snooping
				if (snoop == 0b0011 || snoop == 0b0010 || snoop == 0b0101 || snoop == 0b0100) {
					bSnoopReq = false;
				}
			}

			// Only move to central FIFO if it's a snoop transaction. Snoop-less transactions can be issued directly to memory.
			if (bSnoopReq) {

				if (this->cpFIFO_Central->IsFull() == ERESULT_TYPE_YES) {
					continue; // Stall until central FIFO has space
				}

				// Confirm in cpFIFO_Central before any snoop activity occurs
				UPUD upCentral = new UUD;
				upCentral->cpCentral = new SCentral;
				upCentral->cpCentral->cpAx = Copy_CAxPkt(initTrans);
				upCentral->cpCentral->cpAC = NULL; // Not issued yet
				upCentral->cpCentral->nCounter = 0;
				upCentral->cpCentral->nDataCounter = 0;
				upCentral->cpCentral->nLength = ceil((initTrans->GetLen() + 1) / 4.0);
				upCentral->cpCentral->nSnoopMask = 0;

				this->cpFIFO_Central->Push(upCentral);
				Delete_UD(upCentral, EUD_TYPE_CENTRAL);

				// Snoop-less transaction (e.g. WriteBack): Issue directly to memory
				// Only push the outstanding memory write ID for snoop transactions.
				// Do not need to check for potential hazards here since all of writes requested transfer to the memory.
				if (initTrans->GetDir() == ETRANS_DIR_TYPE_WRITE) {
					this->m_outstandingMemAWID.push_back(initTrans->GetID());
					this->m_outstandingMemAWAddr.push_back(initTrans->GetAddr());
				}

				#ifdef DEBUG_BUS
					string curName = initTrans->GetName();
					printf("[Cycle %3ld: %s.Do_AC_fwd] (%s) Moved to Central FIFO from Master[%d].\n", 
						nCycle, this->cName.c_str(), curName.c_str(), initMaster);
				#endif
				
			// (WriteBack + WriteClean + WriteEvict + Evict + WriteLineUnique) transactions do not require snooping and can be issued directly to memory without going through the central FIFO.
			// Snoop-less transactions can be issued directly to memory, but must check for potential hazards with ongoing snoops or other writes.
			// For simplicity, we stall if there are any outstanding memory writes (AW) to avoid complex hazard detection logic in this example.
			} else {
				// WriteBack (3), WriteClean (2), WriteEvict (5), Evict (4) do not require snooping
				// Snoop-less transaction (e.g. WriteBack): Issue directly to memory
				if (initTrans->GetDir() == ETRANS_DIR_TYPE_WRITE) {
					
					if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES) { continue; } // Stall until memory interface is ready
					if (this->GetMO_AW() >= MAX_MO_COUNT) { continue; } // Stall until MO allows new transaction
					
					// If there are outstanding memory writes, we need to check for potential hazards cause by WriteUnique.
					int nID = initTrans->GetID();
					int64_t nAddr = initTrans->GetAddr();
					auto it = std::find(this->m_outstandingMemAWID.begin(), this->m_outstandingMemAWID.end(), nID);
					if (it != this->m_outstandingMemAWID.end()) { // Found an outstanding memory write with the same ID.
						continue; // Stall if this transaction is already outstanding to memory (should not happen for new transactions, but added as a safety check)
					}
					
					// Write-to-Write guarantee
					auto itAddr = std::find(this->m_outstandingMemAWAddr.begin(), this->m_outstandingMemAWAddr.end(), nAddr);
					if (itAddr != this->m_outstandingMemAWAddr.end()) { 
						continue; // Stall if address hazard detected
					}

					// Read-to-Write guarantee
					auto itAddrR = std::find(this->m_outstandingMemARAddr.begin(), this->m_outstandingMemARAddr.end(), nAddr);
					if (itAddrR != this->m_outstandingMemARAddr.end()) { 
						continue; // Stall if address hazard detected
					}

					this->cpTx_AW->PutAx(initTrans);
				}

				#ifdef DEBUG_BUS
					string curName = initTrans->GetName();
					printf("[Cycle %3ld: %s.Do_AC_fwd] (%s) Snoop-less transaction issued to memory from Master[%d].\n", 
						nCycle, this->cName.c_str(), curName.c_str(), initMaster);
				#endif
			}

			// ----------------------------------------------------------------------------------
			// -------------------- Part 3: Fast-Snoop Response Optimization --------------------
			// ----------------------------------------------------------------------------------
			// Fast-Snoop Response Optimization: For certain snoop-less transactions that are known to be WriteBack/WriteClean/WriteEvict,
			// we can directly generate a snoop response to the initiator without waiting for memory response.

			// Get the master ID from the transaction ID encoding
			int initMaster = GetPortNum(initTrans->GetID());

			// Currently, all write can be optimized as fast-snoop since they do not require snooping. We can add more conditions here if needed.
			// int snoop = initTrans->GetSnoop();
			bool isFastSnoop = (initTrans->GetDir() == ETRANS_DIR_TYPE_WRITE);

			// Doing-the fast-snoop response.
			if (isFastSnoop || (!bSnoopReq)) {

				// Stall until the initiator's B channel is ready to accept the response
				if (this->cpTx_B[initMaster]->IsBusy() == ERESULT_TYPE_YES) { continue; }

				// Handling the early response for snoop-less transactions.
				// We can directly send a response back to the initiator without waiting for snoops or memory response.
				UPUD respTrans = new UUD;
				respTrans->cpB = new CBPkt;
				respTrans->cpB->SetID(initTrans->GetID());
				respTrans->cpB->SetName(initTrans->GetName());
				respTrans->cpB->SetFinalTrans(initTrans->IsFinalTrans());
				this->cpTx_B[initMaster]->PutB(respTrans->cpB);

				#ifdef DEBUG_BUS
					printf("[Cycle %3ld: %s.Do_AC_fwd] Early Response: Acknowledging Initiating Master[%d] for (%s).\n", nCycle, this->cName.c_str(), initMaster, initTrans->GetName().c_str());
				#endif
				delete respTrans;
			}
			
			// Pop from master staging FIFO after successful movement/issuance
			if (initTrans->GetDir() == ETRANS_DIR_TYPE_READ) {
				UPUD upPop = this->cpFIFO_MstAR[initMaster]->Pop();
				if (upPop) Delete_UD(upPop, EUD_TYPE_AR);
				this->bArb[initMaster] = false; // Toggle for next
			} else {
				UPUD upPop = this->cpFIFO_MstAW[initMaster]->Pop();
				if (upPop) Delete_UD(upPop, EUD_TYPE_AW);
				this->bArb[initMaster] = true; // Toggle for next
			}
				
			break; // Move one per cycle to enforce serialization
		}
	}

	return (ERESULT_TYPE_SUCCESS);
}

EResultType CBUS::Do_AC_bwd(int64_t nCycle) {

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for (int nPort=0; nPort < this->NUM_PORT; nPort++) {
		// Check Tx valid 
		CPACPkt cpAC = this->cpTx_AC[nPort]->GetAC();

		if (cpAC == NULL) {
			continue;
		};

		// Check Tx ready
		EResultType eAcceptResult = this->cpTx_AC[nPort]->GetAcceptResult();

		if (eAcceptResult == ERESULT_TYPE_ACCEPT) {

			#ifdef DEBUG_MST
			string cACPktName = cpAC->GetName();
			printf("[Cycle %3ld: %s.Do_AC_bwd] %s handshake cpTx_AC[%d].\n", nCycle, this->cName.c_str(), cACPktName.c_str(), nPort);
			// cpAR->Display();
			#endif
		};
	}

	return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// CD ready
// 	1. Getting transactions from Masters (put into local cpRx_CD ports) = READY.
//		1.1. If the BUS's cpRx_CD is gotten, MASTERs will keep the current transactions at its ports. 
//	2. Encode the CD transactions and push it into cpFIFO_SnoopData.
//	3. The copying only happens if:
//		2.1. cpFIFO_SnoopData is not full.
//		2.2. The transaction is valid at the Master's ports.
//------------------------------------------------------
EResultType CBUS::Do_CD_fwd(int64_t nCycle) {

	bool fcaptured = false;

	// ---- 1. Checking receiving conditions ---
	// Do not accept the transactions if the FIFO is full.
	if (this->cpFIFO_SnoopData->IsFull() == ERESULT_TYPE_YES) { // No transaction
		return (ERESULT_TYPE_FAIL);
	}

	for (int i=0; i < this->NUM_PORT; i++) {

		// Get CD - Check if the port is busy.
		// Skip if MASTERs' ports do not have transactions.
		if (this->cpRx_CD[i]->GetPair()->IsBusy() == ERESULT_TYPE_NO) { // No transaction
			continue;
		}

		// ---- 2. BUS READY receives CD transactions ---
		// Get the transactions from MASTER ports.
		CPCDPkt cpCD = this->cpRx_CD[i]->GetPair()->GetCD();
		
		if (cpCD == NULL) {
			continue;
		};

		// Put the CD transactions to local port = READY.
		this->cpRx_CD[i]->PutCD(cpCD);

		// ---- 3. Push CD transactions to cpFIFO_SnoopData ---
		// Encode ID
		if (this->cpFIFO_Central->IsEmpty() == ERESULT_TYPE_YES) {
			#ifdef DEBUG_BUS
				printf("[Cycle %3ld: %s.Do_CD_fwd] Unexpected CD packet from Master[%d] without active snoop.\n", nCycle, this->cName.c_str(), i);
			#endif
			continue;
		}
		UPUD upTop_central = this->cpFIFO_Central->GetTop();
		CPAxPkt cpAxTop_cd = upTop_central->cpCentral->cpAx;
		int nID = cpAxTop_cd->GetID();
		int nData = cpCD->GetData();
		int nLast = (cpCD->IsLast() == ERESULT_TYPE_YES) ? 1 : 0;

		// Only get the first data matches, every data should be the same.
		// Other data is dropped.
		if (!fcaptured) {
			// Set pkt
			UPUD upW_new = new UUD;
			upW_new->cpW = new CWPkt; 
			upW_new->cpW->SetPkt(nID, nData, nLast);
			upW_new->cpW->SetName(cpCD->GetName());

			this->cpFIFO_SnoopData->Push(upW_new);
			Delete_UD(upW_new, EUD_TYPE_W);
			fcaptured = true;
			
			#ifdef DEBUG_BUS
				printf("[Cycle %3ld: %s.Do_CD_fwd] (%s) captured Rx_CD[%d].\n", nCycle, this->cName.c_str(), cpCD->GetName().c_str(), i);
			#endif
		}
	}
	return (ERESULT_TYPE_SUCCESS);
};

//---------------------------------------------------------------------------------------------------
// CD ready
//	Handles the backward (READY) part of the CD channel handshake.
//	If the bus has received data from a master port (i.e., cpRx_CD is busy),
//	it signals acceptance (asserts READY) back to that master.
//---------------------------------------------------------------------------------------------------
EResultType CBUS::Do_CD_bwd(int64_t nCycle) {

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for (int i=0; i < this->NUM_PORT; i++) {
		// If a packet is present in the receive port, signal acceptance.
		if (this->cpRx_CD[i]->IsBusy() == ERESULT_TYPE_YES) {
			this->cpRx_CD[i]->SetAcceptResult(ERESULT_TYPE_ACCEPT);

			#ifdef DEBUG_BUS
			CPCDPkt cpCD = this->cpRx_CD[i]->GetCD();
			printf("[Cycle %3ld: %s.Do_CD_bwd] (%s) handshake cpRx_CD[%d].\n", nCycle, this->cName.c_str(), cpCD->GetName().c_str(), i);
			#endif
		}
	}

	return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------------
// CR ready
//	1. Collect CR from snoop targets to FIFOs.
// 	2. Ensure all snoop targets have returned CR.
// 	3. Write to the main memory (For transactions required WB).
//	4. Read the main memory (For transactions require READ because no MASTERs hold the requrested cacheline).
//	5. Return to the initiating Master (Do not need to access the main memory).
//------------------------------------------------------
EResultType CBUS::Do_CR_fwd(int64_t nCycle) {

	if (this->cpFIFO_Central->IsEmpty() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	}

	if (this->cpFIFO_Central->GetTop()->cpCentral->cpAC == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	}

	UPUD upCentral = this->cpFIFO_Central->GetTop();
	CPCentral centralEntry = upCentral->cpCentral;
	CPAxPkt cpAxTop = centralEntry->cpAx;
	int initMaster = GetPortNum(cpAxTop->GetID());

	// Decoding the data transfer and snoop type from the original transaction to determine.
	int snoopType = centralEntry->cpAC ? centralEntry->cpAC->GetSnoop() : -1;
	bool isWrite = (cpAxTop->GetDir() == ETRANS_DIR_TYPE_WRITE);
	bool isRead = ((snoopType == 0b1000 || snoopType == 0b1001 || snoopType == 0b1011) && (cpAxTop->GetDir() == ETRANS_DIR_TYPE_READ));
	bool isWriteLineUnique = (isWrite && snoopType == 0b1101);
	bool isUndefined = (centralEntry->cpAC->GetDir() == ETRANS_DIR_TYPE_UNDEFINED);

	// ----------------------------------------------------------------------------------
	// ---- Part 2: Collect CR from snoop targets ---------------------------------------
	// ----------------------------------------------------------------------------------
	for (int for_snoopMaster = 0; for_snoopMaster < this->NUM_PORT; for_snoopMaster++) {
		
		// Do not collect CR from the initiating master or masked-out snoop master.
		if (for_snoopMaster == initMaster) continue;
		if ((SNOOP_MASK & (1 << for_snoopMaster)) != 0) continue; // Mask out snoop master

		// Do not collect CR if the snoop response FIFO is full or the CR is not ready at the target master port.
		if (this->cpFIFO_SnoopResp[for_snoopMaster]->IsFull() == ERESULT_TYPE_YES) continue;
		if (this->cpRx_CR[for_snoopMaster]->GetPair()->IsBusy() == ERESULT_TYPE_NO) continue;

		// Collect the CR from the target master port and push it into the corresponding snoop response FIFO for later processing.
		CPCRPkt cpCR = this->cpRx_CR[for_snoopMaster]->GetPair()->GetCR();
		if (cpCR == NULL) continue;

		this->cpRx_CR[for_snoopMaster]->PutCR(cpCR);
		UPUD upCR_new = new UUD;
		upCR_new->cpCR = Copy_CCRPkt(cpCR);
		this->cpFIFO_SnoopResp[for_snoopMaster]->Push(upCR_new);
		Delete_UD(upCR_new, EUD_TYPE_CR);

		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) CR from Master[%d].\n", 
				nCycle, this->cName.c_str(), cpCR->GetName().c_str(), for_snoopMaster);
		#endif
	}

	bool bIsShared = false, bWasUnique = false, bPassDirty = false, bDataTransfer = false;
	if (!isWriteLineUnique) { // isWriteLineUnique: Do not need to check the responses.
		// ----------------------------------------------------------------------------------
		// ---- Part 2.1: Check if all Snoop Responses --------------------------------------
		// ----------------------------------------------------------------------------------
		// Ensure all snoop targets have returned CR
		int nReadyMaster = 0;
		int nExpectedMasters = 0;
		for (int for_snoopMaster = 0; for_snoopMaster < this->NUM_PORT; for_snoopMaster++) {

			if (for_snoopMaster == initMaster) continue;
			if ((SNOOP_MASK & (1 << for_snoopMaster)) != 0) continue; // Mask out snoop master

			nExpectedMasters++;

			if (this->cpFIFO_SnoopResp[for_snoopMaster]->IsEmpty() == ERESULT_TYPE_YES) continue;

			nReadyMaster++;
		}

		if (nReadyMaster < nExpectedMasters) {
			return (ERESULT_TYPE_SUCCESS);
		}

		// ----------------------------------------------------------------------------------
		// ---- Part 3: Process Snoop Results and Memory Interactions -----------------------
		// ----------------------------------------------------------------------------------

		// Decoding the snoop responses to determine the next steps for the transaction.
		for (int i = 0; i < this->NUM_PORT; i++) {

			if (i == initMaster) continue;
			if ((SNOOP_MASK & (1 << i)) != 0) continue; // Mask out snoop master

			int resp = this->cpFIFO_SnoopResp[i]->GetTop()->cpCR->GetResp();
			if (resp & 0b00001) bDataTransfer = true;
			if (resp & 0b00100) bPassDirty = true;
			if (resp & 0b01000) bIsShared = true;
			if (resp & 0b10000) bWasUnique = true;
		}
	}

	// 3.1. Write to main memory (CleanShared/Invalid with data OR any Write)
	if ((isRead && bDataTransfer) || isWrite) {

		// printf("[Cycle %3ld: %s.Do_CR_fwd] isWriteLineUnique = %d; isUndefined = %d; GetMO_AW = %d/%d.\n", nCycle, this->cName.c_str(), isWriteLineUnique, isUndefined, this->GetMO_AW(), MAX_MO_COUNT);
		// printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) nCounter: %d/%d and nDataCounter: %d/%d. Pop central FIFO.\n", nCycle, this->cName.c_str(), this->cpFIFO_Central->GetTop()->cpCentral->cpAx->GetName().c_str(), centralEntry->nCounter, centralEntry->nLength, centralEntry->nDataCounter, centralEntry->nLength);

		if (isWriteLineUnique) assert (!bDataTransfer); // WriteLineUnique should not have data transfer because it is a MakeInvalid.
		if (!isUndefined) {return (ERESULT_TYPE_SUCCESS);}

		if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES) return (ERESULT_TYPE_SUCCESS);
		if (this->GetMO_AW() >= MAX_MO_COUNT) return (ERESULT_TYPE_SUCCESS);

		CPAxPkt cpAx = Copy_CAxPkt(cpAxTop);
		this->cpTx_AW->PutAx(cpAx);
		centralEntry->cpAC->SetTransDirType(ETRANS_DIR_TYPE_WRITE);

		if (isRead) { 
			this->m_outstandingMemARID.push_back(cpAx->GetID()); 
			this->m_outstandingMemARAddr.push_back(cpAx->GetAddr()); 
		}

		// For WriteLineUnique with multiple beats, we only want to count it as 1 issuance for the whole burst, so we check the snoop type and direction to determine how to update the counter.
		centralEntry->nCounter += 1;
		centralEntry->cpAx->SetAddr(centralEntry->cpAx->GetAddr() +  (MAX_TRANS_SIZE/BYTE_PER_PIXEL*BYTE_PER_PIXEL)); // Assuming 64B beat size, update the address for the next beat
		centralEntry->cpAC->SetAddr(centralEntry->cpAC->GetAddr() +  (MAX_TRANS_SIZE/BYTE_PER_PIXEL*BYTE_PER_PIXEL)); 

		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) put Tx_AW to %lx. Mark top snoop UNDEFINED.\n", nCycle, this->cName.c_str(), cpAx->GetName().c_str(), cpAx->GetAddr());
			//if (isWriteLineUnique) 	printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) put Tx_AW. Mark top snoop UNDEFINED.\n", nCycle, this->cName.c_str(), cpAx->GetName().c_str());
			//else  					printf("[Cycle %3ld: %s.Do_CR_fwd] (%s/%s) put Tx_AW. Mark top snoop UNDEFINED.\n", nCycle, this->cName.c_str(), cpAx->GetName().c_str(), this->cpFIFO_SnoopResp[0]->GetTop()->cpCR->GetName().c_str());
		#endif

		return (ERESULT_TYPE_SUCCESS);
	}

	// 3.2. Read the main memory (if no master holds the line)
	if ((snoopType != 0b1000) && (snoopType != 0b1001) && (snoopType != 0b1101) &&
		(!bIsShared && !bWasUnique && !bDataTransfer && !bPassDirty) && !isWrite) {
		if (this->GetMO_AR() < MAX_MO_COUNT && this->cpTx_AR->IsBusy() == ERESULT_TYPE_NO) {

			// Write-to-Read guarantee
			auto itAddrW = std::find(this->m_outstandingMemAWAddr.begin(), this->m_outstandingMemAWAddr.end(), centralEntry->cpAx->GetAddr());
			if (itAddrW != this->m_outstandingMemAWAddr.end()) {
				return (ERESULT_TYPE_SUCCESS); // Stall
			}

			// Read-to-Read guarantee - Do not need
			//auto itAddrR = std::find(this->m_outstandingMemARAddr.begin(), this->m_outstandingMemARAddr.end(), centralEntry->cpAx->GetAddr());
			//if (itAddrR != this->m_outstandingMemARAddr.end()) {
			//	return (ERESULT_TYPE_SUCCESS); // Stall
			//}


			CPAxPkt cpAR = Copy_CAxPkt(cpAxTop);
			this->cpTx_AR->PutAx(cpAR);
			this->m_outstandingMemARID.push_back(cpAR->GetID());
			this->m_outstandingMemARAddr.push_back(cpAR->GetAddr());

			centralEntry->cpAC->SetTransDirType(ETRANS_DIR_TYPE_UNDEFINED);
			
			// For WriteLineUnique with multiple beats, we only want to count it as 1 issuance for the whole burst, so we check the snoop type and direction to determine how to update the counter.
			centralEntry->nCounter += 1;
			centralEntry->cpAx->SetAddr(centralEntry->cpAx->GetAddr() +  (MAX_TRANS_SIZE/BYTE_PER_PIXEL*BYTE_PER_PIXEL) ); // Assuming 64B beat size, update the address for the next beat
			centralEntry->cpAC->SetAddr(centralEntry->cpAC->GetAddr() +  (MAX_TRANS_SIZE/BYTE_PER_PIXEL*BYTE_PER_PIXEL)); 
			
			// Pop-out the data
			for (int i=0; i<this->NUM_PORT; i++) {

				if (i == initMaster) continue;
				if ((SNOOP_MASK & (1 << i)) != 0) continue; // Mask out snoop master

				UPUD upTmp = this->cpFIFO_SnoopResp[i]->Pop();
				if (upTmp) Delete_UD(upTmp, EUD_TYPE_CR);
			}
			UPUD upPopCentral = this->cpFIFO_Central->Pop();
			if (upPopCentral) Delete_UD(upPopCentral, EUD_TYPE_CENTRAL);

			// Print data.
			#ifdef DEBUG_BUS
				printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) put Tx_AR to %lx.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str(), cpAR->GetAddr());
			#endif

			return (ERESULT_TYPE_SUCCESS);

		}

		return (ERESULT_TYPE_SUCCESS);
	}

	// ----------------------------------------------------------------------------------
	// ---- Part 4: Return Response to Initiating Master --------------------------------
	// ----------------------------------------------------------------------------------

	// Enforce ID ordering: Block if a transaction with the same ID is in flight to memory
	UPUD upTop_hazard = this->cpFIFO_Central->GetTop();
	int currentOriginalID = upTop_hazard->cpCentral->cpAx->GetID();
	auto itAR = std::find(this->m_outstandingMemARID.begin(), this->m_outstandingMemARID.end(), currentOriginalID);
	if (itAR != this->m_outstandingMemARID.end()) {
		return (ERESULT_TYPE_SUCCESS); // Stall
	}

	// Issue the response back to the initiating master if the R channel is ready.
	// The response encoding can be based on the snoop results and transaction type.
	if (this->cpTx_R[initMaster]->IsBusy() == ERESULT_TYPE_NO) {

		// Determine if this is the last beat
		bool isLastBeat = (!bDataTransfer || (this->cpFIFO_SnoopData->IsEmpty() == ERESULT_TYPE_NO && this->cpFIFO_SnoopData->GetTop()->cpW->IsLast() == ERESULT_TYPE_YES));
		EResultType IsLast = isLastBeat ? ERESULT_TYPE_YES : ERESULT_TYPE_NO;
		
		UPUD respTrans = new UUD;
		respTrans->cpR = new CRPkt;
		respTrans->cpR->SetPkt(initMaster, 0, IsLast, (bIsShared << 3) | (bPassDirty << 2));
		respTrans->cpR->SetLast(IsLast);
		respTrans->cpR->SetName(cpAxTop->GetName());
		respTrans->cpR->SetFinalTrans(cpAxTop->IsFinalTrans());
		this->cpTx_R[initMaster]->PutR(respTrans->cpR);

		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) put Tx_R.\n", nCycle, this->cName.c_str(), respTrans->cpR->GetName().c_str());
		#endif

		if (bDataTransfer && this->cpFIFO_SnoopData->IsEmpty() == ERESULT_TYPE_NO) {
			UPUD upW = this->cpFIFO_SnoopData->Pop();
			if (upW) Delete_UD(upW, EUD_TYPE_W);
		}

		// For WriteLineUnique with multiple beats, we only want to count it as 1 issuance for the whole burst, so we check the snoop type and direction to determine how to update the counter.
		centralEntry->nCounter += 1;
		centralEntry->cpAx->SetAddr(centralEntry->cpAx->GetAddr() +  (MAX_TRANS_SIZE/BYTE_PER_PIXEL*BYTE_PER_PIXEL)); // Assuming 64B beat size, update the address for the next beat
		centralEntry->cpAC->SetAddr(centralEntry->cpAC->GetAddr() +  (MAX_TRANS_SIZE/BYTE_PER_PIXEL*BYTE_PER_PIXEL)); 

		// Pop when transaction is finished (last beat) AND all snoops issued
		//if (IsLast == ERESULT_TYPE_YES && centralEntry->nCounter >= centralEntry->nLength && centralEntry->nDataCounter >= centralEntry->nLength) {
		if (IsLast == ERESULT_TYPE_YES && centralEntry->nCounter >= centralEntry->nLength) {
			for (int i=0; i<this->NUM_PORT; i++) {

				if (i == initMaster) continue;
				if ((SNOOP_MASK & (1 << i)) != 0) continue; // Mask out snoop master

				UPUD upTmp = this->cpFIFO_SnoopResp[i]->Pop();
				if (upTmp) Delete_UD(upTmp, EUD_TYPE_CR);
			}
			UPUD upPopCentral = this->cpFIFO_Central->Pop();
			if (upPopCentral) Delete_UD(upPopCentral, EUD_TYPE_CENTRAL);
		}
		delete respTrans;
	}

	return (ERESULT_TYPE_SUCCESS);
}

EResultType CBUS::Do_CR_bwd(int64_t nCycle) {

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for (int i=0; i < this->NUM_PORT; i++) {
		// If a packet is present in the receive port, signal acceptance.
		if (this->cpRx_CR[i]->IsBusy() == ERESULT_TYPE_YES) {
			this->cpRx_CR[i]->SetAcceptResult(ERESULT_TYPE_ACCEPT);

			#ifdef DEBUG_BUS
			CPCRPkt cpCR = this->cpRx_CR[i]->GetCR();
			printf("[Cycle %3ld: %s.Do_CR_bwd] (%s) handshake cpRx_CR[%d].\n", nCycle, this->cName.c_str(), cpCR->GetName().c_str(), i);
			#endif
		}
	}

	return (ERESULT_TYPE_SUCCESS);
};

//-------------------------------------------------------
// W ready
//	1. Issuing the W data.
//-------------------------------------------------------
EResultType CBUS::Do_W_snoop_fwd(int64_t nCycle) {

	// The Tx_W from MASTER often disable since we assumpt the data followed the commands. 
	// Thus, when there are no data transfer from snooped master, we just pop the FIFO and assump the data followed the request. 
	// In case the snooped master would like to return data, we can simulate the spacing between requests and data.
	// Check if active transaction marked as UNDEFINED (waiting for data phase)

	// --------------------------------------------------
	// 0. Decode the initMaster address.
	// --------------------------------------------------
	int initMaster = -1;
	//bool isUndefined = false;
	bool isWrite = false;

	CPCentral centralEntry = NULL;

	if (this->cpFIFO_Central->IsEmpty() == ERESULT_TYPE_NO) {

		// No active transaction or the active transaction does not have AC packet (should not happen but just in case).
		if (this->cpFIFO_Central->GetTop()->cpCentral->cpAC == NULL) return (ERESULT_TYPE_SUCCESS);

		UPUD upTop_central = this->cpFIFO_Central->GetTop();
		centralEntry = upTop_central->cpCentral;
		CPAxPkt cpAxTop = centralEntry->cpAx;

		// Only Write + CleanUnique + CleanShared + CleanInvalid are able to write to main memory.
		int snoopType = centralEntry->cpAC ? centralEntry->cpAC->GetSnoop() : -1;

		isWrite 			= ((snoopType == 0b1000 || snoopType == 0b1001 || snoopType == 0b1011) &&
		                 	   (cpAxTop->GetDir() == ETRANS_DIR_TYPE_READ)) ||
							  (cpAxTop->GetDir() == ETRANS_DIR_TYPE_WRITE);

		//isUndefined 		= (centralEntry->cpAC->GetDir() == ETRANS_DIR_TYPE_UNDEFINED);

		if 	(!isWrite) {
			return (ERESULT_TYPE_SUCCESS); // Isn't the WRITE
		}

		initMaster = GetPortNum(cpAxTop->GetID());
	} else {
		return (ERESULT_TYPE_SUCCESS);
	}

	assert(initMaster >= 0);

	// isUndefined indicate that the Tx_AW has been issued but waiting for data phase.
	// In this case, we should not check the snoop response and just wait for the data to come in.
	// if (isWrite && isUndefined) {return (ERESULT_TYPE_SUCCESS);}

	// --------------------------------------------------
	// 1. Check snoop responses.
	// --------------------------------------------------
	// Ensure all snoop targets have returned CR
	int nReadyMaster = 0;
	int nExpectedMasters = 0;
	for (int for_snoopMaster = 0; for_snoopMaster < this->NUM_PORT; for_snoopMaster++) {

		if (for_snoopMaster == initMaster) continue;
		if ((SNOOP_MASK & (1 << for_snoopMaster)) != 0) continue; // Mask out snoop master

		nExpectedMasters++;

		if (this->cpFIFO_SnoopResp[for_snoopMaster]->IsEmpty() == ERESULT_TYPE_YES) continue;

		nReadyMaster++;
	}

	if (nReadyMaster < nExpectedMasters) {
		return (ERESULT_TYPE_SUCCESS);
	}

	// --------------------------------------------------
	// 2. Check if data transfer is needed based on the snoop responses and wait for data if needed.
	// --------------------------------------------------
	// Since we verified all responses were in,
	// we check bDataTransfer from the stored slave responses.
	bool bDataTransfer = false;
	for (int i = 0; i < this->NUM_PORT; i++) {
		if (i == initMaster) continue;
		if (this->cpFIFO_SnoopResp[i]->IsEmpty() == ERESULT_TYPE_NO) {
			if ((this->cpFIFO_SnoopResp[i]->GetTop()->cpCR->GetResp() & 0b00001) == 0b00001) { // DataTransfer
				bDataTransfer = true;
				break;
			}
		}
	}

	if (bDataTransfer) {
		if (this->cpFIFO_SnoopData->IsEmpty() == ERESULT_TYPE_YES) {
			return (ERESULT_TYPE_SUCCESS); // Wait for data
		}
	} else if (centralEntry->cpAx->GetDir() != ETRANS_DIR_TYPE_WRITE) {
		return (ERESULT_TYPE_SUCCESS); // No transfer data
	}

	// --------------------------------------------------
	// 3. Write data to the main memory
	// --------------------------------------------------
	if (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL); 
	};

	if (centralEntry->nDataCounter >= centralEntry->nCounter) return (ERESULT_TYPE_SUCCESS);

	// Issuing the data and pop the FIFOs when data transfer is expected.
	// If no data transfer is expected, just pop the FIFOs to finish the transaction (Just assume there is no data from snooped masters).
	if (bDataTransfer) {

		CPWPkt cpW = Copy_CWPkt(this->cpFIFO_SnoopData->GetTop()->cpW);

		// Set pkt for write to memory.
		this->cpTx_W->PutW(cpW);
		// Pop-out the snoop response data.
		UPUD upW_tmp = this->cpFIFO_SnoopData->Pop();
		if (upW_tmp) Delete_UD(upW_tmp, EUD_TYPE_W);

		if (cpW->IsLast() == ERESULT_TYPE_YES) {
			centralEntry->nDataCounter += 1;
			// Data transfer complete, pop tracking FIFOs
			for (int i = 0; i < this->NUM_PORT; i++) {

				if (i == initMaster) continue;
				if ((SNOOP_MASK & (1 << i)) != 0) continue;

				// Pop the snoop response for this beat.
				// For multi-beat transactions, we will pop after each beat is transferred.
				UPUD upTmp = this->cpFIFO_SnoopResp[i]->Pop();
				if (upTmp) Delete_UD(upTmp, EUD_TYPE_CR);
			}
			
			// Pop central FIFO if all data beats have been transferred.
			// For multi-beat transactions, we will pop after the last beat is transferred (counter >= length).
			if ((centralEntry->nCounter >= centralEntry->nLength) && (centralEntry->nDataCounter >= centralEntry->nLength)) {
			//if (centralEntry->nCounter >= centralEntry->nLength) {
				UPUD upTmpCentral = this->cpFIFO_Central->Pop();
				if (upTmpCentral) {
					int nID = upTmpCentral->cpCentral->cpAx->GetID();
					auto it = std::find(this->m_outstandingMemAWID.begin(), this->m_outstandingMemAWID.end(), nID);
					if (it != this->m_outstandingMemAWID.end()) {
						int index = std::distance(this->m_outstandingMemAWID.begin(), it);
						this->m_outstandingMemAWID.erase(it);
						this->m_outstandingMemAWAddr.erase(this->m_outstandingMemAWAddr.begin() + index);
					}
					Delete_UD(upTmpCentral, EUD_TYPE_CENTRAL);
				}
			}

			#ifdef DEBUG_BUS
				printf("[Cycle %3ld: %s.Do_W_snoop_fwd] (%s) WLAST put Tx_W. Pop tracking.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
			#endif

		} else {

			#ifdef DEBUG_BUS
				printf("[Cycle %3ld: %s.Do_W_snoop_fwd] (%s) put Tx_W.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
			#endif

		}

		return (ERESULT_TYPE_SUCCESS);

	} else {
		// No data transfer expected (Address-only write phase completed)
		centralEntry->nDataCounter += 1;
		for (int i = 0; i < this->NUM_PORT; i++) {

			if (i == initMaster) continue;
			if ((SNOOP_MASK & (1 << i)) != 0) continue;

			// Pop the snoop response for this beat.
			// For multi-beat transactions, we will pop after each beat is transferred.
			UPUD upTmp = this->cpFIFO_SnoopResp[i]->Pop();
			if (upTmp) Delete_UD(upTmp, EUD_TYPE_CR);

		}

		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_W_snoop_fwd] (%s) WLAST put Tx_W. Pop tracking.\n", nCycle, this->cName.c_str(), this->cpFIFO_Central->GetTop()->cpCentral->cpAx->GetName().c_str());
		#endif

		// Pop central FIFO if all data beats have been transferred.
		// For multi-beat transactions, we will pop after the last beat is transferred (counter >= length).
		if ((centralEntry->nCounter >= centralEntry->nLength) && (centralEntry->nDataCounter >= centralEntry->nLength)) {
		//if (centralEntry->nCounter >= centralEntry->nLength) {
			UPUD upTmpCentral = this->cpFIFO_Central->Pop();
			if (upTmpCentral) {
				int nID = upTmpCentral->cpCentral->cpAx->GetID();
				auto it = std::find(this->m_outstandingMemAWID.begin(), this->m_outstandingMemAWID.end(), nID);
				if (it != this->m_outstandingMemAWID.end()) {
					int index = std::distance(this->m_outstandingMemAWID.begin(), it);
					this->m_outstandingMemAWID.erase(it);
					this->m_outstandingMemAWAddr.erase(this->m_outstandingMemAWAddr.begin() + index);
				}
				Delete_UD(upTmpCentral, EUD_TYPE_CENTRAL);
			}
		}
		
		return (ERESULT_TYPE_SUCCESS);

	}

	return (ERESULT_TYPE_SUCCESS);
};
#endif


//------------------------------------------------------
// W ready
//------------------------------------------------------
EResultType CBUS::Do_W_bwd(int64_t nCycle) {

	//if(nCycle % BUS_LATENCY != 0){
	//	return (ERESULT_TYPE_FAIL);
	//}

	// Check Tx ready
	if (this->cpTx_W->GetAcceptResult() == ERESULT_TYPE_REJECT) {
		return (ERESULT_TYPE_SUCCESS);
	};

	CPWPkt cpW = this->cpTx_W->GetW();

	#ifdef DEBUG_BUS
	assert (cpW != NULL);
	assert (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES);
	 printf("[Cycle %3ld: %s.Do_W_bwd] (%s) handshake Tx_W.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
	// cpW->Display();
	assert (this->cpFIFO_AW->IsEmpty() == ERESULT_TYPE_NO);
	#endif

	// Get arbitrated port
	// int nArb = this->cpFIFO_AW->GetTop()->cpAW->GetID() % this->NUM_PORT; // Only when power-of-2 ports
	int AxID = this->cpFIFO_AW->GetTop()->cpAW->GetID();
	int nArb = GetPortNum(AxID);

	#ifdef DEBUG
	assert (nArb >= 0);
	assert (nArb < this->NUM_PORT);
	#endif

	// Check Rx valid 
	if (this->cpRx_W[nArb]->IsBusy() == ERESULT_TYPE_YES) {
		// Set ready
		this->cpRx_W[nArb]->SetAcceptResult(ERESULT_TYPE_ACCEPT);
	}; 

	// Check WLAST. Pop FIFO_AW
	if (this->cpTx_W->IsPass() == ERESULT_TYPE_YES and cpW->IsLast() == ERESULT_TYPE_YES) {

		#ifdef DEBUG
		assert (this->cpFIFO_AW->IsEmpty() == ERESULT_TYPE_NO);
		#endif

		UPUD up_new = this->cpFIFO_AW->Pop();
		// this->cpFIFO_AW->CheckFIFO();

		// Maintain	
		Delete_UD(up_new, EUD_TYPE_AW);
	};

	return (ERESULT_TYPE_SUCCESS);
};


//------------------------------------------------------
// R Valid
//------------------------------------------------------
EResultType CBUS::Do_R_fwd(int64_t nCycle) {
	
	if(nCycle % BUS_LATENCY != 0){
		return (ERESULT_TYPE_FAIL);
	}

    // Check remote-Tx valid 
    CPRPkt cpR = this->cpRx_R->GetPair()->GetR();
    if (cpR == NULL) {
        return (ERESULT_TYPE_SUCCESS);
    };
	// cpR->CheckPkt();

	// Get destination port
	int nID = cpR->GetID();
	// int nPort = nID % this->NUM_PORT; // Only when power-of-2 FIXME
	int nPort = GetPortNum(nID);

	// Check Tx valid
	if (this->cpTx_R[nPort]->IsBusy() == ERESULT_TYPE_YES) {
         return (ERESULT_TYPE_SUCCESS);
	};

	// Put Rx
	this->cpRx_R->PutR(cpR);

	// Debug
	#ifdef DEBUG_BUS
	if (cpR->IsLast() == ERESULT_TYPE_YES) {	
		printf("[Cycle %3ld: %s.Do_R_fwd] (%s) RID 0x%x RLAST put Rx_R.\n", nCycle, this->cName.c_str(), cpR->GetName().c_str(), cpR->GetID());
		// cpR->Display();
	} 
	else {
		printf("[Cycle %3ld: %s.Do_R_fwd] (%s) RID 0x%x put Rx_R.\n", nCycle, this->cName.c_str(), cpR->GetName().c_str(), cpR->GetID());
		// cpR->Display();
	};
	#endif

	// Decode ID
	CPRPkt cpR_new = Copy_CRPkt(cpR);
	nID = nID >> this->BIT_PORT;
	cpR_new->SetID(nID);
	
	// Put Tx 
	this->cpTx_R[nPort]->PutR(cpR_new);

	// Stat
	this->nR_SI[nPort]++;
	if (cpR_new->IsLast() == ERESULT_TYPE_YES) this->nRTrans++;

	// Debug
	#ifdef DEBUG_BUS
	if (cpR_new->IsLast() == ERESULT_TYPE_YES) {	
		 printf("[Cycle %3ld: %s.Do_R_fwd] (%s) RID 0x%x RLAST put Tx_R[%d].\n", nCycle, this->cName.c_str(), cpR_new->GetName().c_str(), cpR_new->GetID(), nPort);
		 // cpR_new->Display();
	} 
	else {
		 printf("[Cycle %3ld: %s.Do_R_fwd] (%s) RID 0x%x put Tx_R[%d].\n", nCycle, this->cName.c_str(), cpR_new->GetName().c_str(), cpR_new->GetID(), nPort);
		 // cpR_new->Display();
	};
	#endif

	// Maintain
	delete (cpR_new);
	cpR_new = NULL;

    return (ERESULT_TYPE_SUCCESS);
};


//------------------------------------------------------
// B Valid
//------------------------------------------------------
#ifndef CCI_ON
EResultType CBUS::Do_B_fwd(int64_t nCycle) {

	if(nCycle % BUS_LATENCY != 0){
		return (ERESULT_TYPE_FAIL);
	}

	// Check remote-Tx valid
	CPBPkt cpB = this->cpRx_B->GetPair()->GetB();
	if (cpB == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	};
	// cpB->CheckPkt();
	
	// Get destination port
	int nID = cpB->GetID();
	// int nPort = nID % this->NUM_PORT;	 // Only when power-of-2 ports
	int nPort = GetPortNum(nID);

	// Check Tx valid 
	if (this->cpTx_B[nPort]->IsBusy() == ERESULT_TYPE_YES) {
        return (ERESULT_TYPE_SUCCESS);
	};

	// Put Rx
	this->cpRx_B->PutB(cpB);

	// Debug
	// printf("[Cycle %3ld: %s.Do_B_fwd] (%s) BID 0x%x put Rx_B.\n", nCycle, this->cName.c_str(), cpB->GetName().c_str(), cpB->GetID() );
	// cpB->Display();

	// Decode ID
	CPBPkt cpB_new = Copy_CBPkt(cpB);
	nID = nID >> this->BIT_PORT;
	cpB_new->SetID(nID);	

	// Put Tx 
	this->cpTx_B[nPort]->PutB(cpB_new);

	// Stat
	this->nB_SI[nPort]++;

	// Debug
	// printf("[Cycle %3ld: %s.Do_B_fwd] (%s) BID 0x%x put Tx_B[%d] - IsFinalTrans = %s.\n", nCycle, this->cName.c_str(), cpB_new->GetName().c_str(), cpB->GetID(), nPort, Convert_eResult2string(cpB_new->IsFinalTrans()).c_str());
	// cpB_new->Display();

	// Maintain
	delete (cpB_new);
	cpB_new = NULL;
		
	return (ERESULT_TYPE_SUCCESS);
};
#else
EResultType CBUS::Do_B_fwd(int64_t nCycle) {

	// Because we have already do the fast-response.
	// Drop all incoming B responses since CCI does not use the B channel for write responses.
	if(nCycle % BUS_LATENCY != 0){
		return (ERESULT_TYPE_FAIL);
	}

	// Check remote-Tx valid
	CPBPkt cpB = this->cpRx_B->GetPair()->GetB();
	
	if (cpB == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Put Rx
	this->cpRx_B->PutB(cpB);

	#ifdef DEBUG_BUS
		printf("[Cycle %3ld: %s.Do_B_fwd] (%s) BID 0x%x put Rx_B.\n", nCycle, this->cName.c_str(), cpB->GetName().c_str(), cpB->GetID() );
	#endif

	
	int nID = cpB->GetID();
	auto it = std::find(this->m_outstandingMemARID.begin(), this->m_outstandingMemARID.end(), nID);

	// Existed in outstanding AR,
	// means this B is for a read transaction that requires data return. We need to forward it to the master.
	if (it != this->m_outstandingMemARID.end()) {

		int nPort = GetPortNum(nID);
		if (this->cpTx_R[nPort]->IsBusy() == ERESULT_TYPE_YES) {
			return (ERESULT_TYPE_SUCCESS); // Stall
		}

		CPBPkt cpB_new = Copy_CBPkt(cpB);
		CPRPkt cpR_new = new CRPkt;

		// For CCI, RRESP and RLAST are not used since the master can identify the transaction completion by the ID and the fast response.
		cpR_new->SetPkt(nID, 0, 1, 0);
		cpR_new->SetName(cpB->GetName());
		this->cpTx_R[nPort]->PutR(cpR_new);
		this->nR_SI[nPort]++;
		this->nRTrans++;

		int index = std::distance(this->m_outstandingMemARID.begin(), it);
		this->m_outstandingMemARID.erase(it);
		this->m_outstandingMemARAddr.erase(this->m_outstandingMemARAddr.begin() + index);

		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_B_fwd] (%s) BID 0x%x put Tx_R.\n", nCycle, this->cName.c_str(), cpB->GetName().c_str(), cpB->GetID() );
		#endif

		delete (cpB_new);
		return (ERESULT_TYPE_SUCCESS);

	} else {
		this->nBTrans++;
	}

	return (ERESULT_TYPE_SUCCESS);
}
#endif


//------------------------------------------------------
// R ready
//------------------------------------------------------
EResultType CBUS::Do_R_bwd(int64_t nCycle) {

	//if(nCycle % BUS_LATENCY != 0){
	//	return (ERESULT_TYPE_FAIL);
	//}

	// Check Rx valid 
	CPRPkt cpR = this->cpRx_R->GetR();
	if (cpR == NULL) {
	        return (ERESULT_TYPE_SUCCESS);
	};

	#ifdef DEBUG
	// cpR->CheckPkt();
	assert (this->cpRx_R->IsBusy() == ERESULT_TYPE_YES);
	#endif
	
	// Get destination port
	int nID = cpR->GetID();
	// int nPort = nID % this->NUM_PORT; // Only when power-of-2 ports
	int nPort = GetPortNum(nID);
	
	// Check Tx ready FIXME FIXME
	EResultType eAcceptResult = this->cpTx_R[nPort]->GetAcceptResult();
	if (eAcceptResult == ERESULT_TYPE_REJECT) {
	    return (ERESULT_TYPE_SUCCESS);
	};

	#ifdef DEBUG_BUS
	assert (eAcceptResult == ERESULT_TYPE_ACCEPT);
	printf("[Cycle %3ld: %s.Do_R_bwd] (%s) handshake Tx_R[%d].\n", nCycle, this->cName.c_str(), this->cpTx_R[nPort]->GetR()->GetName().c_str(), nPort);
	#endif

	// Set ready 
	this->cpRx_R->SetAcceptResult(ERESULT_TYPE_ACCEPT);

	// Debug
	if (cpR->IsLast() == ERESULT_TYPE_YES) {
		#ifdef DEBUG_BUS
		printf("[Cycle %3ld: %s.Do_R_bwd] (%s) RLAST handshake Rx_R.\n", nCycle, this->cName.c_str(), cpR->GetName().c_str());
		#endif
	
		// Decrease MO
		this->Decrease_MO_AR();	

		#ifdef CCI_ON
		auto it = std::find(this->m_outstandingMemARID.begin(), this->m_outstandingMemARID.end(), cpR->GetID());
		if (it != this->m_outstandingMemARID.end()) {
			int index = std::distance(this->m_outstandingMemARID.begin(), it);
			this->m_outstandingMemARID.erase(it);
			this->m_outstandingMemARAddr.erase(this->m_outstandingMemARAddr.begin() + index);
		}
		#endif
	} 
	#ifdef DEBUG_BUS
	else {
		printf("[Cycle %3ld: %s.Do_R_bwd] (%s) handshake Rx_R.\n", nCycle, this->cName.c_str(), cpR->GetName().c_str());
	};
	#endif
	
	// Debug
	// cpR->Display();
	
	return (ERESULT_TYPE_SUCCESS);
};


//------------------------------------------------------
// B ready
//------------------------------------------------------
#ifndef CCI_ON
EResultType CBUS::Do_B_bwd(int64_t nCycle) {
	
	//if(nCycle % BUS_LATENCY != 0){
	//	return (ERESULT_TYPE_FAIL);
	//}

	// Check Rx valid 
	CPBPkt cpB = this->cpRx_B->GetB();

	if (cpB == NULL) {
	        return (ERESULT_TYPE_SUCCESS);
	};

	#ifdef DEBUG
	// cpB->CheckPkt();
	assert (this->cpRx_B->IsBusy() == ERESULT_TYPE_YES);
	#endif
	
	// Get destination port
	int nID = cpB->GetID();
	// int nPort = nID % this->NUM_PORT; // Only when power-of-2 ports
	int nPort = GetPortNum(nID);
	
	// Check Tx ready FIXME FIXME When SLV->Do_R_bwd and BUS->Do_R_bwd reverse, function issue
	EResultType eAcceptResult = this->cpTx_B[nPort]->GetAcceptResult();
	if (eAcceptResult == ERESULT_TYPE_REJECT) {
	        return (ERESULT_TYPE_SUCCESS);
	};

	#ifdef DEBUG
	assert (eAcceptResult == ERESULT_TYPE_ACCEPT);
	#endif

	// Set ready 
	this->cpRx_B->SetAcceptResult(ERESULT_TYPE_ACCEPT);

	// Debug
	#ifdef DEBUG_BUS
	 printf("[Cycle %3ld: %s.Do_B_bwd] (%s) B handshake Rx_B.\n", nCycle, this->cName.c_str(), cpB->GetName().c_str());
	#endif
	
	// Decrease MO
	this->Decrease_MO_AW();	
		
	// Debug
	// cpB->Display();

    return (ERESULT_TYPE_SUCCESS);
};
#else
EResultType CBUS::Do_B_bwd(int64_t nCycle) {
	
	//if(nCycle % BUS_LATENCY != 0){
	//	return (ERESULT_TYPE_FAIL);
	//}

	// Check Rx valid 
	CPBPkt cpB = this->cpRx_B->GetB();

	if (cpB == NULL) {
	    return (ERESULT_TYPE_SUCCESS);
	};

	#ifdef DEBUG
	// cpB->CheckPkt();
	assert (this->cpRx_B->IsBusy() == ERESULT_TYPE_YES);
	#endif

	// Set ready 
	this->cpRx_B->SetAcceptResult(ERESULT_TYPE_ACCEPT);

	// Debug
	#ifdef DEBUG_BUS
	 printf("[Cycle %3ld: %s.Do_B_bwd] (%s) B handshake Rx_B.\n", nCycle, this->cName.c_str(), cpB->GetName().c_str());
	#endif
	
	// Decrease MO
	this->Decrease_MO_AW();	
		
    return (ERESULT_TYPE_SUCCESS);
};
#endif


// Debug
EResultType CBUS::CheckLink() {

	assert (this->cpTx_AR != NULL);
	assert (this->cpTx_AW != NULL);
	assert (this->cpTx_W  != NULL);
	assert (this->cpRx_R  != NULL);
	assert (this->cpRx_B  != NULL);

	#ifdef CCI_ON
	for (int i=0; i<this->NUM_PORT; i++) {
		assert (this->cpTx_AC[i] != NULL);
		assert (this->cpRx_CR[i] != NULL);
		assert (this->cpRx_CD[i] != NULL);
	}
	#endif

	assert (this->cpTx_AR->GetPair() != NULL);
	assert (this->cpTx_AW->GetPair() != NULL);
	assert (this->cpTx_W->GetPair()  != NULL);
	assert (this->cpRx_R ->GetPair() != NULL);
	assert (this->cpRx_B ->GetPair() != NULL);

	#ifdef CCI_ON
	for (int i=0; i<this->NUM_PORT; i++) {
		assert (this->cpTx_AC[i]->GetPair() != NULL);
		assert (this->cpRx_CR[i]->GetPair() != NULL);
		assert (this->cpRx_CD[i]->GetPair() != NULL);
	}
	#endif

	assert (this->cpTx_AR->GetTRxType() != this->cpTx_AR->GetPair()->GetTRxType());
	assert (this->cpTx_AW->GetTRxType() != this->cpTx_AW->GetPair()->GetTRxType());
	assert (this->cpTx_W ->GetTRxType() != this->cpTx_W ->GetPair()->GetTRxType());
	assert (this->cpRx_R ->GetTRxType() != this->cpRx_R ->GetPair()->GetTRxType());
	assert (this->cpRx_B ->GetTRxType() != this->cpRx_B ->GetPair()->GetTRxType());

	#ifdef CCI_ON
	for (int i=0; i<this->NUM_PORT; i++) {
		assert (this->cpTx_AC[i]->GetTRxType() != this->cpTx_AC[i] ->GetPair()->GetTRxType());
		assert (this->cpRx_CR[i]->GetTRxType() != this->cpRx_CR[i] ->GetPair()->GetTRxType());
		assert (this->cpRx_CD[i]->GetTRxType() != this->cpRx_CD[i] ->GetPair()->GetTRxType());
	}
	#endif

	assert (this->cpTx_AR->GetPktType() == this->cpTx_AR->GetPair()->GetPktType());
	assert (this->cpTx_AW->GetPktType() == this->cpTx_AW->GetPair()->GetPktType());
	assert (this->cpTx_W ->GetPktType() == this->cpTx_W ->GetPair()->GetPktType());
	assert (this->cpRx_R ->GetPktType() == this->cpRx_R ->GetPair()->GetPktType());
	assert (this->cpRx_B ->GetPktType() == this->cpRx_B ->GetPair()->GetPktType());

	#ifdef CCI_ON
	for (int i=0; i<this->NUM_PORT; i++) {
		assert (this->cpTx_AC[i]->GetPktType() == this->cpTx_AC[i] ->GetPair()->GetPktType());
		assert (this->cpRx_CR[i]->GetPktType() == this->cpRx_CR[i] ->GetPair()->GetPktType());
		assert (this->cpRx_CD[i]->GetPktType() == this->cpRx_CD[i] ->GetPair()->GetPktType());
	}
	#endif
	
	assert (this->cpTx_AR->GetPair()->GetPair()== this->cpTx_AR);
	assert (this->cpTx_AW->GetPair()->GetPair()== this->cpTx_AW);
	assert (this->cpTx_W ->GetPair()->GetPair()== this->cpTx_W);
	assert (this->cpRx_R ->GetPair()->GetPair()== this->cpRx_R);
	assert (this->cpRx_B ->GetPair()->GetPair()== this->cpRx_B);

	#ifdef CCI_ON
	for (int i=0; i<this->NUM_PORT; i++) {
		assert (this->cpTx_AC[i]->GetPair()->GetPair() == this->cpTx_AC[i]);
		assert (this->cpRx_CR[i]->GetPair()->GetPair() == this->cpRx_CR[i]);
		assert (this->cpRx_CD[i]->GetPair()->GetPair() == this->cpRx_CD[i]);
	}
	#endif

    return (ERESULT_TYPE_SUCCESS);
};


// Increase AR MO count 
EResultType CBUS::Increase_MO_AR() {  

	this->nMO_AR++;

	#ifdef DEBUG
	assert (this->nMO_AR >= 0);
	assert (this->nMO_AR <= MAX_MO_COUNT);
	#endif

        return (ERESULT_TYPE_SUCCESS);
};


// Decrease AR MO count 
EResultType CBUS::Decrease_MO_AR() {  

	this->nMO_AR--;

	#ifdef DEBUG
	assert (this->nMO_AR >= 0);
	assert (this->nMO_AR <= MAX_MO_COUNT);
	#endif

        return (ERESULT_TYPE_SUCCESS);
};


// Increase AW MO count 
EResultType CBUS::Increase_MO_AW() {  

	this->nMO_AW++;

	#ifdef DEBUG
	assert (this->nMO_AW >= 0);
	assert (this->nMO_AW <= MAX_MO_COUNT);
	#endif

        return (ERESULT_TYPE_SUCCESS);
};


// Decrease AW MO count 
EResultType CBUS::Decrease_MO_AW() { 

	this->nMO_AW--;

	#ifdef DEBUG
	assert (this->nMO_AW >= 0);
	assert (this->nMO_AW <= MAX_MO_COUNT);
	#endif

        return (ERESULT_TYPE_SUCCESS);
};


// Get AR MO count
int CBUS::GetMO_AR() {

	#ifdef DEBUG
	assert (this->nMO_AR >= 0);
	assert (this->nMO_AR <= MAX_MO_COUNT);
	#endif

	return (this->nMO_AR);
};


// Get AW MO count
int CBUS::GetMO_AW() {

	#ifdef DEBUG
	assert (this->nMO_AW >= 0);
	assert (this->nMO_AW <= MAX_MO_COUNT);
	#endif

	return (this->nMO_AW);
};


int CBUS::GetPortNum(int nID) {

	int nPort = -1;

	if      (this->BIT_PORT == 1) nPort = nID & 0x1;  	// 1-bit.  Upto 2 ports
	else if (this->BIT_PORT == 2) nPort = nID & 0x3; 	// 2-bits. Upto 4 ports
	else if (this->BIT_PORT == 3) nPort = nID & 0x7;
	else if (this->BIT_PORT == 4) nPort = nID & 0xF;
	else assert(0);

	#ifdef DEBUG
	assert (nPort >= 0);	
	#endif
	
	return (nPort);
};


EResultType CBUS::Set_nR_GEN_NUM(int nNum) {
	this->nR_GEN_NUM = nNum;
	return (ERESULT_TYPE_SUCCESS);
};


EResultType CBUS::Set_nB_GEN_NUM(int nNum) {
	this->nB_GEN_NUM = nNum;
	return (ERESULT_TYPE_SUCCESS);
};


int CBUS::Get_nRTrans() {
	return (this->nRTrans);
};


int CBUS::Get_nBTrans() {
	return (this->nBTrans);
};


int CBUS::Get_nR_GEN_NUM() {
	return (this->nR_GEN_NUM);
};


int CBUS::Get_nB_GEN_NUM() {
	return (this->nB_GEN_NUM);
};


// Update state
EResultType CBUS::UpdateState(int64_t nCycle) {  

	// MI 
    this->cpTx_AR->UpdateState();
	this->cpTx_AW->UpdateState();
	this->cpTx_W ->UpdateState();
    this->cpRx_R ->UpdateState();
    this->cpRx_B ->UpdateState();

	#ifdef CCI_ON

		for (int i=0; i < this->NUM_PORT; i++) { this->cpTx_AC[i]->UpdateState(); }
		for (int i=0; i < this->NUM_PORT; i++) { this->cpRx_CR[i]->UpdateState(); }
		for (int i=0; i < this->NUM_PORT; i++) { this->cpRx_CD[i]->UpdateState(); }
	
	#endif

	// SI 
	for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_AR[i]->UpdateState(); };
	for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_AW[i]->UpdateState(); };
	// for (int i=0; i<this->NUM_PORT; i++) { this->cpRx_W[i] ->UpdateState(); };
	for (int i=0; i<this->NUM_PORT; i++) { this->cpTx_R[i] ->UpdateState(); };
	for (int i=0; i<this->NUM_PORT; i++) { this->cpTx_B[i] ->UpdateState(); };

	// FIFO
	this->cpFIFO_AW->UpdateState();

        return (ERESULT_TYPE_SUCCESS);
}; 


// Stat 
EResultType CBUS::PrintStat(int64_t nCycle, FILE *fp) {

	printf("--------------------------------------------------------\n");
	printf("\t Name : %s\n", this->cName.c_str());
	printf("--------------------------------------------------------\n");

	printf("\t Number of SI ports         : %d\n\n",this->NUM_PORT);

	for (int i=0; i<this->NUM_PORT; i++) {
		printf("\t Number of AR SI[%d]         : %d\n",	i, this->nAR_SI[i]);
	};
	printf("\n");
	for (int i=0; i<this->NUM_PORT; i++) {
		printf("\t Number of AW SI[%d]         : %d\n",	i, this->nAW_SI[i]);
	};
	printf("\n");
	for (int i=0; i<this->NUM_PORT; i++) {
		printf("\t Number of R SI[%d]          : %d\n",	i, this->nR_SI[i]);
	};
	printf("\n");
	for (int i=0; i<this->NUM_PORT; i++) {
		printf("\t Number of B SI[%d]          : %d\n",	i, this->nB_SI[i]);
	};
	printf("\n");
	
	
	// printf("\t Max allowed AR MO                : %d\n",	 MAX_MO_COUNT);
	// printf("\t Max allowed AW MO                : %d\n\n",MAX_MO_COUNT);
	
        return (ERESULT_TYPE_SUCCESS);
};
