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
#include <iostream>

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

		this->cpFIFO_CCI_AW = new CPFIFO [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_CCI_AW",i);
			this->cpFIFO_CCI_AW[i] = new CFIFO(cName, EUD_TYPE_AW, 20);
		};

		this->cpFIFO_CCI_AR = new CPFIFO [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_CCI_AR",i);
			this->cpFIFO_CCI_AR[i] = new CFIFO(cName, EUD_TYPE_AR, 20);
		};

		this->cpFIFO_CCI_CR = new CPFIFO [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_CCI_CR",i);
			this->cpFIFO_CCI_CR[i] = new CFIFO(cName, EUD_TYPE_CR, 20);
		};

		this->cpFIFO_CCI_CD = new CPFIFO [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_CCI_CD",i);
			this->cpFIFO_CCI_CD[i] = new CFIFO(cName, EUD_TYPE_CD, 20);
		};

		this->cpSnoopID = new CPFIFO [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_SnoopID",i);
			this->cpSnoopID[i] = new CFIFO(cName, EUD_TYPE_AR, 20); // EUD_TYPE_AX
		};

		this->cpSnoopAC = new CPFIFO [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_SnoopAC",i);
			this->cpSnoopAC[i] = new CFIFO(cName, EUD_TYPE_AC, 20); // EUD_TYPE_AX
		};

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

	// FIFO
	delete (this->cpFIFO_AW);

	#ifdef CCI_ON
		for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpFIFO_CCI_AW[i]); };
		for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpFIFO_CCI_AR[i]); };
		for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpFIFO_CCI_CD[i]); };
		for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpFIFO_CCI_CR[i]); };
		for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpSnoopID[i]); };
		for (int i=0; i<this->NUM_PORT; i++) { delete (this->cpSnoopAC[i]); };
	#endif
	// delete (this->cpFIFO_AR);
	// delete (this->cpFIFO_W);

	// Arb
	delete (this->cpArb_AR);
	delete (this->cpArb_AW);

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

		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_CCI_AW[i] = NULL; };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_CCI_AR[i] = NULL; };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_CCI_CD[i] = NULL; };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_CCI_CR[i] = NULL; };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpSnoopID[i] = NULL; };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpSnoopAC[i] = NULL; };

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

		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_CCI_AW[i]->Reset(); };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_CCI_AR[i]->Reset(); };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_CCI_CD[i]->Reset(); };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpFIFO_CCI_CR[i]->Reset(); };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpSnoopID[i]->Reset(); };
		for (int i=0; i<this->NUM_PORT; i++) { this->cpSnoopAC[i]->Reset(); };

		for (int i=0; i<this->NUM_PORT; i++) {
			this->nSnoopedMaster[i] = 0;
			this->bArb[i] = false;
		}
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
//	2. Encode the AR transactions and push it into cpFIFO_CCI_AR.
//	3. The copying only happens if:
//		2.1. cpFIFO_CCI_AR is not full.
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
		if (this->cpFIFO_CCI_AR[nPort]->IsFull() == ERESULT_TYPE_YES) {
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

		nID = (nID << this->BIT_PORT) + 3; // FIXME: Master 3
		cpAR->SetPkt(nID, nAddr, nLen, nSnoop);

		#ifdef DEBUG
		assert (cpAR != NULL);
		// cpAR->CheckPkt();
		#endif

		// Put the AR transactions to local port = asserting READY.
		this->cpRx_AR[nPort]->PutAx(cpAR);

		// ---- 3. Push AR transactions to cpFIFO_CCI_AR ---
		// Instead of putting Rx to Tx, we put to a FIFO and maintain the FIFO to waiting snoops.
		UPUD upAR_new = new UUD;
		upAR_new->cpAR = Copy_CAxPkt(cpAR);
		this->cpFIFO_CCI_AR[nPort]->Push(upAR_new); // FIXME: Only READ

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
//	2. Encode the AW transactions and push it into cpFIFO_CCI_AW.
//	3. The copying only happens if:
//		2.1. cpFIFO_CCI_AW is not full.
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
		if (this->cpFIFO_CCI_AW[nPort]->IsFull() == ERESULT_TYPE_YES) {
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

		nID = (nID << this->BIT_PORT) + 3; // FIXME: Master 3
		cpAW->SetPkt(nID, nAddr, nLen, nSnoop);

		#ifdef DEBUG
		assert (cpAW != NULL);
		// cpAW->CheckPkt();
		#endif

		// Pu the AR transactions to local port = asserting READY.
		this->cpRx_AW[nPort]->PutAx(cpAW);

		// ---- 3. Push AR transactions to cpFIFO_CCI_AR ---
		// Instead of putting Rx to Tx, we put to a FIFO and maintain the FIFO to waiting snoops.
		UPUD upAW_new = new UUD;
		upAW_new->cpAW = Copy_CAxPkt(cpAW);
		this->cpFIFO_CCI_AW[nPort]->Push(upAW_new); // FIXME: Only READ
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
		// printf("[Cycle %3ld: %s.Do_W_fwd] (%s) WLAST put Tx_W.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
		// cpW->Display();
	} 
	else {
		// printf("[Cycle %3ld: %s.Do_W_fwd] (%s) put Tx_W.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
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
//		2.1. cpFIFO_CCI_AR is not full (have slots).
//		2.2. Already receive a transaction. Reset the AcceptResult for the next transactions.
//---------------------------------------------------------------------------------------------------
EResultType CBUS::Do_AR_bwd(int64_t nCycle) {

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for (int nPort = 0; nPort < this->NUM_PORT; nPort++) {
		
		// ---- 1. Checking receiving conditions ---
		// Do not accept the transactions if the FIFO is full (after pushing the current transactions).
		//if (this->cpFIFO_CCI_AR[nPort]->IsFull() == ERESULT_TYPE_YES) { continue; }

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
//		2.1. cpFIFO_CCI_AW is not full (have slots).
//		2.2. Already receive a transaction. Reset the AcceptResult for the next transactions.
//---------------------------------------------------------------------------------------------------
EResultType CBUS::Do_AW_bwd(int64_t nCycle) {

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for (int nPort = 0; nPort < this->NUM_PORT; nPort++) {

		// ---- 1. Checking receiving conditions ---
		// Do not accept the transactions if the FIFO is full (after pushing the current transactions).
		// if (this->cpFIFO_CCI_AW[nPort]->IsFull() == ERESULT_TYPE_YES) { continue; }

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
	
	// Checking every MASTERs' ports.
	for (int initMaster = 0; initMaster < this->NUM_PORT; initMaster++) {

		// -----------------------------------------------
		// ---- 1. Scanning Ready-to-Snooped MASTERs -----
		// -----------------------------------------------
		// Do not check the current Initiating Master.
		this->nSnoopedMaster[initMaster] |= (1 << initMaster);

		// a. All snooped targets have accepted the snoop, pop the transaction and reset the snooped master record for next AR/AW.
		for (int snoopMaster = 0; snoopMaster < this->NUM_PORT; snoopMaster++) {
			
			// Do not check the current Initiating Master.
			if (snoopMaster == initMaster) {
				continue;
			}

			// If there is any master that have not snooped yet, continue snooping process.
			if ((this->nSnoopedMaster[initMaster] & (1 << snoopMaster)) == 0) {
				break;
			}

			this->nSnoopedMaster[initMaster] = 0;

			// Pop the AR and AW if snooping finished.
			if (this->bArb[initMaster]) { this->cpFIFO_CCI_AR[initMaster]->Pop(); }
			else { this->cpFIFO_CCI_AW[initMaster]->Pop(); }
			this->bArb[initMaster] = !this->bArb[initMaster]; // Round-robin
		};

		// b. Scaning through all of masters to check if there are any master ready for a snoop.
		// Check AC valid
		int nReadyMaster = 0;
		for (int snoopMaster = 0; snoopMaster <this->NUM_PORT; snoopMaster++) {

			// Do not snoop the current Initiating Master.
			if (snoopMaster == initMaster) {
				continue;
			}

			// If the remote port of MASTERs ready for a snoop -> Set the Selector to 1.
			if (this->cpTx_AC[snoopMaster]->GetPair()->IsBusy() == ERESULT_TYPE_NO) {
				nReadyMaster |= (1 << snoopMaster);
			};
		};

		// c. Collect the selectors to issuing snoops.
		int nSnoopSelector = nReadyMaster & (~this->nSnoopedMaster[initMaster]) & (~SNOOP_MASK);

		// d. Do not do anything if there is no master ready for a snoop.
		if (nSnoopSelector == 0) {
			return (ERESULT_TYPE_FAIL);
		}
		// -------------------------------------------------


		// -------------------------------------------------
		// ---- 2. Encoding the snooping transactions -----
		// -------------------------------------------------
		CPAxPkt initTrans = new CAxPkt("", ETRANS_DIR_TYPE_UNDEFINED);
		CPACPkt SnoopPkt = new CACPkt;

		// Ignore the case that AR or AW are empty.
		if (this->bArb[initMaster]) {
			if (this->cpFIFO_CCI_AR[initMaster]->IsEmpty() == ERESULT_TYPE_YES) {

				if (this->cpFIFO_CCI_AW[initMaster]->IsEmpty() == ERESULT_TYPE_NO) {
					initTrans = this->cpFIFO_CCI_AW[initMaster]->GetTop()->cpAW;
					this->bArb[initMaster] = false;
				}
				else {
					continue; // All of FIFO is empty
				}

			}
			else {
				initTrans = this->cpFIFO_CCI_AR[initMaster]->GetTop()->cpAR;
				this->bArb[initMaster] = true;
			}
		}
		else {
			if (this->cpFIFO_CCI_AW[initMaster]->IsEmpty() == ERESULT_TYPE_YES) {

				if (this->cpFIFO_CCI_AR[initMaster]->IsEmpty() == ERESULT_TYPE_NO) {
					initTrans = this->cpFIFO_CCI_AR[initMaster]->GetTop()->cpAR;
					this->bArb[initMaster] = true;
				}
				else {
					continue; // All of FIFO is empty
				}

			}
			else {
				initTrans = this->cpFIFO_CCI_AW[initMaster]->GetTop()->cpAW;
				this->bArb[initMaster] = false; // Continue with AR until the all snoops is sent out
			}
		}

		// If there is no transaction, continue with another MASTERs.
		if (initTrans == NULL) {
			continue;
		}

		// Copy initiating transactions to snooping transaction.
		SnoopPkt->SetSnoop(initTrans->GetSnoop());
		SnoopPkt->SetName(initTrans->GetName());
		SnoopPkt->SetAddr(initTrans->GetAddr());

		// READ: Encoding the SNOOP transactions.
		if (initTrans->GetDir() == ETRANS_DIR_TYPE_READ) { // Read transactions
			if (initTrans->GetSnoop() == 0b0000) { // ReadOnce
				SnoopPkt->SetSnoop(0b0000); // ReadOnce -> ReadOnce
			}
			else if (initTrans->GetSnoop() == 0b0001) { // ReadShared
				SnoopPkt->SetSnoop(0b0001); // ReadShared -> ReadShared
			}
			else if (initTrans->GetSnoop() == 0b0010) { // ReadClean
				SnoopPkt->SetSnoop(0b0010); // ReadClean -> ReadClean
			}
			else if (initTrans->GetSnoop() == 0b0011) { // ReadNotSharedDirty
				SnoopPkt->SetSnoop(0b0011); // ReadNotSharedDirty -> ReadNotSharedDirty
			}
			else if (initTrans->GetSnoop() == 0b0111) { // ReadUnique
				SnoopPkt->SetSnoop(0b0111); // ReadUnique -> ReadUnique
			}
			else if (initTrans->GetSnoop() == 0b1011) { // CleanUnique
				SnoopPkt->SetSnoop(0b1001); // CleanUnique -> CleanInvalid
			}
			else if (initTrans->GetSnoop() == 0b1100) { // MakeUnique
				SnoopPkt->SetSnoop(0b1101); // MakeUnique -> MakeInvalid
			}
			else if (initTrans->GetSnoop() == 0b1000) { // CleanShared
				SnoopPkt->SetSnoop(0b1000); // CleanShared -> CleanShared
			}
			else if (initTrans->GetSnoop() == 0b1001) { // CleanInvalid
				SnoopPkt->SetSnoop(0b1001); // CleanInvalid -> CleanInvalid
			}
			else if (initTrans->GetSnoop() == 0b1101) { // MakeInvalid
				SnoopPkt->SetSnoop(0b1101); // MakeInvalid -> MakeInvalid
			}
			else {
				// No supported: ReadNoSnoop, Barrier, DVM, etc.
				assert (false);
			}
		}
		// WRITE: Encoding the SNOOP transactions.
		else if (initTrans->GetDir() == ETRANS_DIR_TYPE_WRITE) { // Write transactions
			if (initTrans->GetSnoop() == 0b0000) { // WriteUnique
				SnoopPkt->SetSnoop(0b1001); // WriteUnique -> CleanInvalid
				// Snoop other masters.
			}
			else if (initTrans->GetSnoop() == 0b0001) { // WriteLineUnique
				SnoopPkt->SetSnoop(0b1101); // WriteLineUnique -> MakeInvalid
				// Snoop other masters.
			}
			else if ((initTrans->GetSnoop() == 0b0011) || // WriteBack
					 (initTrans->GetSnoop() == 0b0010) || // WriteClean
					 (initTrans->GetSnoop() == 0b0101)	  // WriteEvict
			) {
				// Issuing a Write to the memory.
				
				// Check Tx valid 
				if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES) {
					continue; // Not doing the rest.
				};

				this->cpTx_AW->PutAx(initTrans);
				this->cpFIFO_CCI_AW[initMaster]->Pop();

				continue; // Write to memory, do not need to snoop other master.
			} else if (initTrans->GetSnoop() == 0b100) { // Evict
				// Do nothing

				// this->cpTx_AW->PutAx(initTrans);
				this->cpFIFO_CCI_AW[initMaster]->Pop();

				continue; // Not doing the rest.
				// Update the External Snoop Filter if it is, otherwise, drop this transaction.
				// Do not need to snoop other masters.
			}  else {
				// No supported: WriteNoSnoop, Barrier, DVM, etc.
				assert (false);
			}
		} else {
			assert (false); // Undefined transactions.
		}
		// ------------------------------------


		// ------------------------------------
		// ---- 3. Snooping other Masters -----
		// ------------------------------------
		// Snooping the other master.
		for (int snoopMaster = 0; snoopMaster < this->NUM_PORT; snoopMaster++) {
			
			if (snoopMaster == initMaster) {
				continue;
			}

			if ((nSnoopSelector & (1 << snoopMaster)) != 0) {

				// If AC is busy, reset the selecting bit.
				if (this->cpTx_AC[snoopMaster]->IsBusy() == ERESULT_TYPE_YES) {
					nSnoopSelector &= ~(1 << snoopMaster);
					continue;
				}

				// If the FIFO is full (BUS do not ready to receive transactions), reset the selecting bit.
				if ((this->cpSnoopID[snoopMaster]->IsFull() == ERESULT_TYPE_YES) && 
					(this->cpSnoopAC[snoopMaster]->IsFull() == ERESULT_TYPE_YES)) {
					nSnoopSelector &= ~(1 << snoopMaster);
					continue;
				}

				// Software Hack: Depending on transaction types, selecting the correct types.
				UPUD upAW_new = new UUD;
				if (initTrans->GetDir() == ETRANS_DIR_TYPE_READ) {
					upAW_new->cpAR = Copy_CAxPkt(initTrans);
				} else if (initTrans->GetDir() == ETRANS_DIR_TYPE_WRITE) {
					upAW_new->cpAW = Copy_CAxPkt(initTrans);
				} else {
					assert (false); // Undefined transactions.
				}
				
				// Put the Snoop transaction to snooped MASTERs.
				this->cpTx_AC[snoopMaster]->PutAC(SnoopPkt);
				// Recording the transaction IDs.
				this->cpSnoopID[snoopMaster]->Push(upAW_new);

				upAW_new = new UUD;
				upAW_new->cpAC = Copy_CACPkt(SnoopPkt);
				// Recording the transaction types + data, for WriteUnique and WriteLineUnique.
				this->cpSnoopAC[snoopMaster]->Push(upAW_new);

				#ifdef DEBUG_BUS
					printf("[Cycle %3ld: %s.Do_AC_fwd] (%s) put Tx_AC[%d].\n", nCycle, this->cName.c_str(), SnoopPkt->GetName().c_str(), snoopMaster);
				#endif
			}
		}

		// Record snooped master and maintain FIFO
		this->nSnoopedMaster[initMaster] |= nSnoopSelector;
	}

	return (ERESULT_TYPE_SUCCESS);	
};


//------------------------------------------------------
// AC ready
//------------------------------------------------------
EResultType CBUS::Do_AC_bwd(int64_t nCycle) {

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for (int nPort=0; nPort < this->NUM_PORT; nPort++) {
		// Check Tx valid 
		CPACPkt cpAC = this->cpTx_AC[nPort]->GetAC();

		if (cpAC == NULL) {
			return (ERESULT_TYPE_SUCCESS);
		};

		// Check Tx ready
		EResultType eAcceptResult = this->cpTx_AC[nPort]->GetAcceptResult();

		if (eAcceptResult == ERESULT_TYPE_ACCEPT) {

			#ifdef DEBUG_MST
			string cARPktName = cpAC->GetName();
			printf("[Cycle %3ld: %s.Do_AR_bwd] %s handshake cpTx_AC[%d].\n", nCycle, this->cName.c_str(), cARPktName.c_str(), nPort);
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
//	2. Encode the CD transactions and push it into cpFIFO_CCI_CD.
//	3. The copying only happens if:
//		2.1. cpFIFO_CCI_CD is not full.
//		2.2. The transaction is valid at the Master's ports.
//------------------------------------------------------
EResultType CBUS::Do_CD_fwd(int64_t nCycle) {
	for (int i=0; i < this->NUM_PORT; i++) {

		// ---- 1. Checking receiving conditions ---
		// Do not accept the transactions if the FIFO is full.
		if (this->cpFIFO_CCI_CD[i]->IsFull() == ERESULT_TYPE_YES) { // No transaction
			continue;
		}

		// Get CD - Check if the port is busy.
		// Skip if MASTERs' ports do not have transactions.
		if (this->cpRx_CR[i]->GetPair()->IsBusy() == ERESULT_TYPE_NO) { // No transaction
			continue;
		}

		// ---- 2. BUS READY receives AR transactions ---
		// Get the transactions from MASTER ports.
		CPCDPkt cpCD = this->cpRx_CD[i]->GetPair()->GetCD();	
		
		if (cpCD == NULL) {
			continue;
		};
		
		// Put the AR transactions to local port = READY.
		this->cpRx_CD[i]->PutCD(cpCD);

		// ---- 3. Push AR transactions to cpFIFO_CCI_CD ---
		// Instead of putting Rx to Tx, we put to a FIFO and maintain the FIFO to waiting snoops.
		UPUD upCD_new = new UUD;
		upCD_new->cpCD = Copy_CCDPkt(cpCD);
		this->cpFIFO_CCI_CD[i]->Push(upCD_new);

		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_CD_fwd] (%s) put Rx_CD[%d].\n", nCycle, this->cName.c_str(), cpCD->GetName().c_str(), i);
		#endif
	}
	return (ERESULT_TYPE_SUCCESS);
};

//---------------------------------------------------------------------------------------------------
// CD valid
//	1. Set the acceptance results. Indicating the BUS is ready to receive CD following transactions from MASTER.
//	3. The CD READY only happens if:
//		2.2. Already receive a transaction. Reset the AcceptResult for the next transactions.
//---------------------------------------------------------------------------------------------------
EResultType CBUS::Do_CD_bwd(int64_t nCycle) {

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for (int i=0; i < this->NUM_PORT; i++) {

		// ---- 1. Checking receiving conditions ---
		// Do not accept the transactions if the FIFO is full (Do not need).


		// ---- 2. Reset the AcceptResult = Set Ready to receive the next transactions ---
		if (this->cpRx_CD[i]->IsBusy() == ERESULT_TYPE_YES) { // FIXME
			// Set ready	
			this->cpRx_CD[i]->SetAcceptResult(ERESULT_TYPE_ACCEPT); // FIXME

			#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_CD_bwd] (%s) handshake cpRx_CD[%d].\n", nCycle, this->cName.c_str(), (this->cpRx_CD[i]->GetCD()->GetName()).c_str(), i);
			#endif
		}
	}

	return (ERESULT_TYPE_SUCCESS);
};

int CBUS::Find_the_snoopMaster(int64_t nCycle) {
	int snoopMaster = -1;
	bool fflag = false;

	for (int i=0; i < this->NUM_PORT; i++) {
	
		if (this->cpSnoopID[i]->GetTop() == NULL) {
			continue;
		}

		for (int j=0; j < this->NUM_PORT; j++) {

			if (i == j) { continue; }

			if (this->cpSnoopID[j]->GetTop() == NULL) {
				continue;
			}

			if ( GetPortNum(this->cpSnoopID[i]->GetTop()->cpAR->GetID()) != GetPortNum(this->cpSnoopID[j]->GetTop()->cpAR->GetID())) {
				fflag = false;
				break;
			}

			fflag = true;
		}

		if (fflag) snoopMaster = i;
	}
	if (!fflag) snoopMaster = 0; // Default value, no snoop response.
	return snoopMaster;
}

//------------------------------------------------------
// CR ready
//	1. Collect CR from snoop targets to FIFOs.
// 	2. Ensure all snoop targets have returned CR.
// 	3. Write to the main memory (For transactions required WB).
//	4. Read the main memory (For transactions require READ because no MASTERs hold the requrested cacheline).
//	5. Return to the initiating Master (Do not need to access the main memory).
//------------------------------------------------------
EResultType CBUS::Do_CR_fwd(int64_t nCycle) {

	UPUD respTrans = new UUD;
	respTrans->cpR = new CRPkt;

	// Find out snoopedMasters. Because all master data + address is the same, only required get one random snooped Master.
	int snoopMaster = Find_the_snoopMaster(nCycle);

	// Check if snoopedMaster ready - in case no SnoopMaster is READY.
	int nReadyMaster = 0;
	if (this->cpSnoopID[snoopMaster]->IsEmpty() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};
	
	// Get the initiating Master address.
	int  InitMaster = GetPortNum(this->cpSnoopID[snoopMaster]->GetTop()->cpAR->GetID()); // FIXME: Need to update the "3".

	// ----------------------------------------------------------------------------------------------------
	// 1. Collect CR from snoop targets to FIFOs by putting the CR transactions from remote ports to local ports.
	// ----------------------------------------------------------------------------------------------------
	for (int snoopMaster = 0; snoopMaster < this->NUM_PORT; snoopMaster++) {

		// Skip if the SnoopMaster and InitMaster is the same.
		if (snoopMaster == InitMaster) { continue; }

		// ---- 1. Checking receiving conditions ---
		// Do not accept the transactions if the FIFO is full.
		if (this->cpFIFO_CCI_CR[snoopMaster]->IsFull() == ERESULT_TYPE_YES) { // No transaction
			continue;
		}

		// Get CR - Check if the port is busy.
		// Skip if MASTERs' ports do not have transactions.
		if (this->cpRx_CR[snoopMaster]->GetPair()->IsBusy() == ERESULT_TYPE_NO) { // No transaction
			continue;
		}

		// ---- 2. BUS READY receives CR transactions ---
		// Get the transactions from MASTER ports.
		CPCRPkt cpCR = this->cpRx_CR[snoopMaster]->GetPair()->GetCR();
		
		if (cpCR == NULL) {
			continue;
		};

		// Put the AR transactions to local port = READY.
		this->cpRx_CR[snoopMaster]->PutCR(cpCR);

		// ---- 3. Push AR transactions to cpFIFO_CCI_CR ---
		// Instead of putting Rx to Tx, we put to a FIFO and maintain the FIFO to waiting snoops.
		UPUD upCR_new = new UUD;
		upCR_new->cpCR = Copy_CCRPkt(cpCR);
		this->cpFIFO_CCI_CR[snoopMaster]->Push(upCR_new);

		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_AR_fwd] (%s) push to cpFIFO_CR[%d].\n", nCycle, this->cName.c_str(), cpCR->GetName().c_str(), snoopMaster);
		#endif
	}

	// Ensure all snoop targets have returned CR
	for (int snoopMaster = 0; snoopMaster < this->NUM_PORT; snoopMaster++) {
		if (this->cpFIFO_CCI_CR[snoopMaster]->IsEmpty() == ERESULT_TYPE_YES) {
			continue;
		};
		nReadyMaster++;
	}

	// Process CR when all snoop targets have returned CR.
	if (nReadyMaster < (NUM_PORT - 1)) {
		return (ERESULT_TYPE_SUCCESS);
	}

	// --------------------------------------------------
	// 2. Checking if all snoops is returns.
	// --------------------------------------------------
	int* nCRRespList = new int[this->NUM_PORT];
	/*===================================================
	CRRESP[0]	= DataTransfer
	CRRESP[1]	= Error // Not supported in this version
	CRRESP[2]	= PassDirty
	CRRESP[3]	= IsShared
	CRRESP[4]	= WasUnique
	======================================================*/
	bool bIsShared 		= false;
	bool bWasUnique 	= false;
	bool bPassDirty 	= false;
	bool bDataTransfer	= false;

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for (int snoopMaster = 0; snoopMaster < this->NUM_PORT; snoopMaster++) {
		
		// Skip if the SnoopMaster and InitMaster is the same.
		if (snoopMaster == InitMaster) {continue;}

		// Check if the current CR FIFO is available.
		if (this->cpFIFO_CCI_CR[snoopMaster]->IsEmpty() == ERESULT_TYPE_YES) {
			continue;
		};

		// Get the response signals and encode it.
		nCRRespList[snoopMaster] = this->cpFIFO_CCI_CR[snoopMaster]->GetTop()->cpCR->GetResp(); // Record source port in ID
		
		if ((nCRRespList[snoopMaster] & 0b00001) == 0b00001) { // DataTransfer
			bDataTransfer |= true;
		}
		
		if ((nCRRespList[snoopMaster] & 0b00100) == 0b00100) { // PassDirty
			bPassDirty |= true;
		}
		
		if ((nCRRespList[snoopMaster] & 0b01000) == 0b01000) { // IsShared
			bIsShared |= true;
		}
		
		if ((nCRRespList[snoopMaster] & 0b10000) == 0b10000) { // WasUnique
			bWasUnique |= true;
		}
		if (nCRRespList[snoopMaster] == 0b00000) { // NoSnoop or ReadClean or CleanShared, etc.
			// No supported: NoSnoop, ReadClean, CleanShared, etc.
		}
	}

	// --------------------------------------------------
	// 3. Write to the main memory
	// --------------------------------------------------
	if ((this->cpSnoopAC[snoopMaster]->GetTop()->cpAC->GetSnoop() == 0b1000) || // CleanShared
		(this->cpSnoopAC[snoopMaster]->GetTop()->cpAC->GetSnoop() == 0b1001) || // CleanInvalid
		(this->cpSnoopAC[snoopMaster]->GetTop()->cpAC->GetSnoop() == 0b1101)	// MakeInvalid
	){
		// ---- Check MO ---
		if (this->GetMO_AW() >= MAX_MO_COUNT) {
			return (ERESULT_TYPE_FAIL);
		};

		// Data have not returned yet.
		CPCDPkt cpCD = new CCDPkt;

		// Checking if the CD responses are received + Assertion.
		if (bDataTransfer) {

			// Checking every MASTERs' ports.
			for (int snoopMaster = 0; snoopMaster < this->NUM_PORT; snoopMaster++) {
				if (snoopMaster == InitMaster) {
					continue;
				}

				// Assmupt that only one master response data because only one master hold the dirty data.
				// FIXME: Should uncover this assertion when the MASTER is complete.
				// assert((cpCD != NULL) &&
				// 	   (this->cpRx_CD[snoopMaster]->GetPair()->GetCD() == NULL)); // Only one dirty

				if (cpCD == NULL) { cpCD = this->cpRx_CD[snoopMaster]->GetPair()->GetCD(); }
			}

			if (cpCD == NULL) { // The data have not returned yet.
				return (ERESULT_TYPE_SUCCESS);
			}
			// PassDirty must be asserted because only Diry data is allowed to transfer data for Clean* transactions.
			if ((this->cpSnoopAC[snoopMaster]->GetTop()->cpAC->GetSnoop() == 0b1000) || // CleanShared
				(this->cpSnoopAC[snoopMaster]->GetTop()->cpAC->GetSnoop() == 0b1001)) { // CleanInvalid
				assert(bPassDirty);
			}
			// This environment do not allow to PassDirty for non-Clear* transactions.
			else {
				assert(!bPassDirty);
			}
		}

		// Merge written data.
		// The environment is the performance environment, the merge data can be skipped.

		// Issuing to the main memory
		CPAxPkt cpAW = new CAxPkt("", ETRANS_DIR_TYPE_UNDEFINED);

		if (this->cpSnoopID[snoopMaster]->GetTop()->cpAR != NULL) {
			cpAW = this->cpSnoopID[snoopMaster]->GetTop()->cpAR;
		} else if (this->cpSnoopID[snoopMaster]->GetTop()->cpAW != NULL) {
			cpAW = this->cpSnoopID[snoopMaster]->GetTop()->cpAW;
		}

		int     nID   = cpAW->GetID();
		int64_t nAddr = cpAW->GetAddr();
		int     nLen  = cpAW->GetLen();
		nID = (nID << this->BIT_PORT) + InitMaster;

		// Set pkt for write to memory.
		cpAW->SetPkt(nID, nAddr, nLen);

		this->cpTx_AW->PutAx(cpAW);

		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) put Tx_AW.\n", nCycle, this->cName.c_str(), cpAW->GetName().c_str());
			// cpAR->Display();
		#endif

		// Pop-out the snoop response.
		for (int snoopMaster = 0; snoopMaster < this->NUM_PORT; snoopMaster++) {
			if (snoopMaster == InitMaster) continue;
			if (!bDataTransfer) {
				this->cpFIFO_CCI_CR[snoopMaster]->Pop();
				this->cpSnoopID[snoopMaster]->Pop();
				this->cpSnoopAC[snoopMaster]->Pop();
			} else {
				this->cpFIFO_CCI_CD[snoopMaster]->Pop();
				if (cpCD->IsLast()) {
					this->cpFIFO_CCI_CR[snoopMaster]->Pop();
					this->cpSnoopID[snoopMaster]->Pop();
					this->cpSnoopAC[snoopMaster]->Pop();
				}
			}
		}

		return (ERESULT_TYPE_SUCCESS);
	}

	// --------------------------------------------------
	// 4. Read the main memory
	// --------------------------------------------------
	if (!bIsShared && !bWasUnique && !bDataTransfer && !bPassDirty) {
		// In this environemnt, the Master should  never PassDirty.

		// ---- Check MO ---
		if (this->GetMO_AR() >= MAX_MO_COUNT) {
			return (ERESULT_TYPE_FAIL);
		};

		// Check if there is any other masters that have already issued READ transaction to the main memory.
		if (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES) {
			return (ERESULT_TYPE_SUCCESS);
		}

		// Issuing to the main memory
		CPAxPkt cpAR = this->cpSnoopID[snoopMaster]->GetTop()->cpAR; // FIXME

		// Encode ID
		int     nID   = cpAR->GetID();
		int64_t nAddr = cpAR->GetAddr();
		int     nLen  = cpAR->GetLen();
		nID = (nID << this->BIT_PORT) + InitMaster;

		// Set pkt
		cpAR->SetPkt(nID, nAddr, nLen);

		this->cpTx_AR->PutAx(cpAR);

		// this->Increase_MO_AR(); // FIXME: Only tracking memory access.
		// Pop-out the snoop response.
		for (int snoopMaster = 0; snoopMaster < this->NUM_PORT; snoopMaster++) {
			if (snoopMaster == InitMaster) continue;
			this->cpFIFO_CCI_CR[snoopMaster]->Pop();			
			this->cpSnoopID[snoopMaster]->Pop();
			this->cpSnoopAC[snoopMaster]->Pop();
		}

		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) put Tx_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
			// cpAR->Display();
		#endif

		return (ERESULT_TYPE_SUCCESS);
	}
	
	// --------------------------------------------------
	// 5. Return to the initiating Master
	// --------------------------------------------------
	// Check Tx valid 
	if (this->cpTx_R[InitMaster]->IsBusy() == ERESULT_TYPE_YES) {
        return (ERESULT_TYPE_SUCCESS);
	};

	bool brespIsShared = bIsShared;
	bool brespPassDirty = false; // We don't model the dirty state in this version, so PassDirty is treated as no hit.
	
	bool IsLast = false;
	bool DataReturned = false;

	// Checking all of snooped MASTERs to collect data.
	for (int snoopMaster = 0; snoopMaster < this->NUM_PORT; snoopMaster++) {

		// Skip if the SnoopMaster and InitMaster is the same.
		if (snoopMaster == InitMaster) { continue; }

		if (!bDataTransfer) { continue; }

		if (this->cpFIFO_CCI_CD[snoopMaster]->IsEmpty() == ERESULT_TYPE_YES) {
			continue;
		}

		// Collecting IsLast + DataReturned
		IsLast |= this->cpFIFO_CCI_CD[snoopMaster]->GetTop()->cpCD->IsLast();
		DataReturned = true;
	}

	// If any data returned.
	if (!DataReturned) { return (ERESULT_TYPE_SUCCESS); }

	// Construct the R transactions.
	respTrans->cpR->SetPkt(
		InitMaster, // RID carries the source port for response routing
		0, 			// Data is not modeled in this version
		IsLast, 	// FIXME: Assump the length is only 1
		brespIsShared << 3 | brespPassDirty << 2
	); // Response only contains IsShared information in this version

	// Put to R.
	this->cpTx_R[InitMaster]->PutR(respTrans->cpR);

	#ifdef DEBUG_BUS
		printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) put cpTx_R[%d].\n", nCycle, this->cName.c_str(), respTrans->cpR->GetName().c_str(), InitMaster);
		// cpAR->Display();
	#endif

	// Checking every MASTERs' ports.
	for (int snoopMaster = 0; snoopMaster < this->NUM_PORT; snoopMaster++) {

		// Skip if the SnoopMaster and InitMaster is the same.
		if (snoopMaster == InitMaster) { continue; }

		// Pop-out the snoop response.
		if (!bDataTransfer) {
			this->cpFIFO_CCI_CR[snoopMaster]->Pop();
			this->cpSnoopID[snoopMaster]->Pop();
			this->cpSnoopAC[snoopMaster]->Pop();
		} else {
			this->cpFIFO_CCI_CD[snoopMaster]->Pop();
			if (IsLast) {
				this->cpFIFO_CCI_CR[snoopMaster]->Pop();
				this->cpSnoopID[snoopMaster]->Pop();
				this->cpSnoopAC[snoopMaster]->Pop();
			}
		}
	}

	return (ERESULT_TYPE_SUCCESS);
};

//---------------------------------------------------------------------------------------------------
// CR Valid
//	1. Set the acceptance results. Indicating the BUS is ready to receive CR following transactions from MASTER.
//	3. The CR READY only happens if:
//		2.2. Already receive a transaction. Reset the AcceptResult for the next transactions.
//---------------------------------------------------------------------------------------------------
EResultType CBUS::Do_CR_bwd(int64_t nCycle) {

	// ---- Receives Transactions ---
	// Checking every MASTERs' ports.
	for (int i=0; i < this->NUM_PORT; i++) {
		// ---- 1. Checking receiving conditions ---
		// Do not accept the transactions if the FIFO is full (Do not need).
		//if (this->cpFIFO_CCI_CR[i]->IsFull() == ERESULT_TYPE_YES) { // No transaction
		//	continue;
		//}

		// ---- 2. Reset the AcceptResult = Set Ready to receive the next transactions ---
		if (this->cpRx_CR[i]->IsBusy() == ERESULT_TYPE_YES) { // FIXME
			// Set ready	
			this->cpRx_CR[i]->SetAcceptResult(ERESULT_TYPE_ACCEPT); // FIXME

			#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_CR_bwd] (%s) handshake cpRx_CR[%d].\n", nCycle, this->cName.c_str(), (this->cpRx_CR[i]->GetCR()->GetName()).c_str(), i);
			#endif
		}
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


// Update state
EResultType CBUS::UpdateState(int64_t nCycle) {  

	// MI 
    this->cpTx_AR->UpdateState();
	this->cpTx_AW->UpdateState();
	// this->cpTx_W ->UpdateState();
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

