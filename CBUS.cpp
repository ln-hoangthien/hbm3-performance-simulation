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
			sprintf(cName, "BUS_Rx%d_CCI_AW",i);
			this->cpFIFO_MstAW[i] = new CFIFO(cName, EUD_TYPE_AW, 20);
		};

		this->cpFIFO_MstAR = new CPFIFO [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_CCI_AR",i);
			this->cpFIFO_MstAR[i] = new CFIFO(cName, EUD_TYPE_AR, 20);
		};

		this->cpFIFO_SnoopResp = new CPFIFO [NUM_PORT];
		for (int i=0; i<NUM_PORT; i++) {
			char cName[50];
			sprintf(cName, "BUS_Rx%d_CCI_CR",i);
			this->cpFIFO_SnoopResp[i] = new CFIFO(cName, EUD_TYPE_CR, 20);
		};

		this->cpFIFO_SnoopData = new CFIFO("BUS_CCI_W", EUD_TYPE_W, 20);
		this->cpFIFO_ActiveSnoopAx = new CFIFO("BUS_SnoopCRID", EUD_TYPE_AR, 64);
		this->cpFIFO_ActiveSnoopAC    = new CFIFO("BUS_SnoopAC", EUD_TYPE_AC, 64);

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
		delete (this->cpFIFO_ActiveSnoopAx);
		delete (this->cpFIFO_ActiveSnoopAC);
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
		this->cpFIFO_ActiveSnoopAx = NULL;
		this->cpFIFO_ActiveSnoopAC = NULL;

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
		this->cpFIFO_ActiveSnoopAx->Reset();
		this->cpFIFO_ActiveSnoopAC->Reset();

		for (int i=0; i<this->NUM_PORT; i++) {
			this->nSnoopedMaster[i] = 0;
			this->bArb[i] = false;
		}
		this->m_outstandingMemARID.clear();
		this->m_outstandingMemAWID.clear();
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

		// Hazard Check
		if ((nSnoop == 0b0011) || // WriteBack
			(nSnoop == 0b0010) || // WriteClean
			(nSnoop == 0b0101)    // WriteEvict
		) {
			if (!this->m_outstandingMemAWID.empty()) {
				// Potential hazard with ongoing snoop or other write
				continue; 
			}
		}

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
			if (this->bArb[initMaster]) {
				UPUD upTmp = this->cpFIFO_MstAR[initMaster]->Pop();
				if (upTmp) Delete_UD(upTmp, EUD_TYPE_AR);
			}
			else {
				UPUD upTmp = this->cpFIFO_MstAW[initMaster]->Pop();
				if (upTmp) {
					// Clean up hazard tracking
					if (upTmp->cpAW->GetSnoop() == 0b0000 || upTmp->cpAW->GetSnoop() == 0b0001) {
						auto it = std::find(this->m_outstandingMemAWID.begin(), this->m_outstandingMemAWID.end(), upTmp->cpAW->GetID());
						if (it != this->m_outstandingMemAWID.end()) {
							this->m_outstandingMemAWID.erase(it);
						}
					}
					Delete_UD(upTmp, EUD_TYPE_AW);
				}
			}
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
			//return (ERESULT_TYPE_FAIL);
			continue;
		}
		// -------------------------------------------------


		// -------------------------------------------------
		// ---- 2. Encoding the snooping transactions -----
		// -------------------------------------------------
		CPAxPkt initTrans = NULL;

		// Ignore the case that AR or AW are empty.
		if (this->bArb[initMaster]) {
			if (this->cpFIFO_MstAR[initMaster]->IsEmpty() == ERESULT_TYPE_YES) {

				if (this->cpFIFO_MstAW[initMaster]->IsEmpty() == ERESULT_TYPE_NO) {
					initTrans = this->cpFIFO_MstAW[initMaster]->GetTop()->cpAW;
					this->bArb[initMaster] = false;
				}
				else {
					continue; // All of FIFO is empty
				}

			}
			else {
				initTrans = this->cpFIFO_MstAR[initMaster]->GetTop()->cpAR;
				this->bArb[initMaster] = true;
			}
		}
		else {
			if (this->cpFIFO_MstAW[initMaster]->IsEmpty() == ERESULT_TYPE_YES) {

				if (this->cpFIFO_MstAR[initMaster]->IsEmpty() == ERESULT_TYPE_NO) {
					initTrans = this->cpFIFO_MstAR[initMaster]->GetTop()->cpAR;
					this->bArb[initMaster] = true;
				}
				else {
					continue; // All of FIFO is empty
				}

			}
			else {
				initTrans = this->cpFIFO_MstAW[initMaster]->GetTop()->cpAW;
				this->bArb[initMaster] = false; // Continue with AR until the all snoops is sent out
			}
		}

		// If there is no transaction, continue with another MASTERs.
		if (initTrans == NULL) {
			continue;
		}

		CPACPkt SnoopPkt = new CACPkt;

		// Copy initiating transactions to snooping transaction.
		SnoopPkt->SetSnoop(initTrans->GetSnoop());
		SnoopPkt->SetName(initTrans->GetName());
		SnoopPkt->SetAddr(initTrans->GetAddr());

		// READ: Encoding the SNOOP transactions.
		if (initTrans->GetDir() == ETRANS_DIR_TYPE_READ) { // Read transactions
			SnoopPkt->SetTransDirType(ETRANS_DIR_TYPE_READ);
			
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
			SnoopPkt->SetTransDirType(ETRANS_DIR_TYPE_WRITE);

			if (initTrans->GetSnoop() == 0b0000) { // WriteUnique
				SnoopPkt->SetSnoop(0b1001); // WriteUnique -> CleanInvalid
			}
			else if (initTrans->GetSnoop() == 0b0001) { // WriteLineUnique
				SnoopPkt->SetSnoop(0b1101); // WriteLineUnique -> MakeInvalid
			}
			else if ((initTrans->GetSnoop() == 0b0011) || // WriteBack
					 (initTrans->GetSnoop() == 0b0010) || // WriteClean
					 (initTrans->GetSnoop() == 0b0101)	  // WriteEvict
			) {
				// Checking the same ID outstanding.
				int currentOriginalID = initTrans->GetID();
				auto itAW = std::find(this->m_outstandingMemAWID.begin(), this->m_outstandingMemAWID.end(), currentOriginalID);
				if (itAW != this->m_outstandingMemAWID.end()) {
					delete SnoopPkt; SnoopPkt = NULL;
					return (ERESULT_TYPE_SUCCESS); // Stall
				}

				// Issuing a Write to the memory.
				// Check Tx valid 
				if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES) {
					delete SnoopPkt; SnoopPkt = NULL;
					continue; // Not doing the rest.
				};

				this->cpTx_AW->PutAx(initTrans);
				this->m_outstandingMemAWID.push_back(initTrans->GetID());

				UPUD upTmp = this->cpFIFO_MstAW[initMaster]->Pop();
				if (upTmp) Delete_UD(upTmp, EUD_TYPE_AW);

				delete SnoopPkt; SnoopPkt = NULL;
				continue; // Write to memory, do not need to snoop other master.

			} else if (initTrans->GetSnoop() == 0b100) { // Evict
				// Do nothing

				#ifdef DEBUG_BUS
					printf("[Cycle %3ld: %s.Do_AC_fwd] (%s) Drop EVICT of Master[%d].\n", nCycle, this->cName.c_str(), initTrans->GetName().c_str(), initMaster);
				#endif

				// this->cpTx_AW->PutAx(initTrans);
				UPUD upTmp = this->cpFIFO_MstAW[initMaster]->Pop();
				if (upTmp) Delete_UD(upTmp, EUD_TYPE_AW);

				delete SnoopPkt; SnoopPkt = NULL;
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
		// Check shared FIFO full
		if (this->cpFIFO_ActiveSnoopAC->IsFull() == ERESULT_TYPE_YES) {
			assert(this->cpFIFO_ActiveSnoopAx->IsFull() == ERESULT_TYPE_YES);
			delete SnoopPkt; SnoopPkt = NULL;
			continue;
		}

		// Recording the transaction ONCE if we are starting to snoop.
		//if (this->nSnoopedMaster[initMaster] == (1 << initMaster)) {
		// Recording the transaction IDs.
		UUD udAx_new;
		if (initTrans->GetDir() == ETRANS_DIR_TYPE_READ) udAx_new.cpAR = initTrans;
		else                                             udAx_new.cpAW = initTrans;
		this->cpFIFO_ActiveSnoopAx->Push(&udAx_new);
		
		// Recording the transaction types + data, for WriteUnique and WriteLineUnique.
		UUD udAC_new;
		udAC_new.cpAC = SnoopPkt;
		this->cpFIFO_ActiveSnoopAC->Push(&udAC_new);
		//}

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

				// Put the Snoop transaction to snooped MASTERs.
				this->cpTx_AC[snoopMaster]->PutAC(SnoopPkt);

				#ifdef DEBUG_BUS
					printf("[Cycle %3ld: %s.Do_AC_fwd] (%s) put Tx_AC[%d].\n", nCycle, this->cName.c_str(), SnoopPkt->GetName().c_str(), snoopMaster);
				#endif
			}
		}

		// Track ongoing WriteUnique/WriteLineUnique snoops
		if (nSnoopSelector != 0 && (initTrans->GetSnoop() == 0b0000 || initTrans->GetSnoop() == 0b0001)) {
			int nID = initTrans->GetID();
			if (std::find(this->m_outstandingMemAWID.begin(), this->m_outstandingMemAWID.end(), nID) == this->m_outstandingMemAWID.end()) {
				this->m_outstandingMemAWID.push_back(nID);
			}
		}

		// Record snooped master and maintain FIFO
		this->nSnoopedMaster[initMaster] |= nSnoopSelector;
		delete SnoopPkt; SnoopPkt = NULL;
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
		printf("[Cycle %3ld: %s.Do_CD_fwd] cpFIFO_SnoopData is full.\n", nCycle, this->cName.c_str());
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
		// Instead of putting Rx to Tx, we put to a FIFO and maintain the FIFO to waiting snoops.
		// Only capture data if there is an active snoop transaction.
		//if (this->cpSnoop_CDID->IsEmpty() == ERESULT_TYPE_YES) {
		//	#ifdef DEBUG_BUS
		//		printf("[Cycle %3ld: %s.Do_CD_fwd] Unexpected CD packet from Master[%d] without active snoop.\n", nCycle, this->cName.c_str(), i);
		//	#endif
		//	continue;
		//}

		// Encode ID
		UPUD upTop = this->cpFIFO_ActiveSnoopAx->GetTop();
		int nID = (upTop->cpAR != NULL) ? upTop->cpAR->GetID() : upTop->cpAW->GetID();
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

	// Check if snoopedMaster ready - in case no SnoopMaster is READY.
	int nReadyMaster = 0;
	int InitMaster = -1;

	// Dynamically determine the initiating master from a valid snoop response (CRID tracks the original Ax)
	if (this->cpFIFO_ActiveSnoopAx->IsEmpty() == ERESULT_TYPE_NO) {
		UPUD upTop = this->cpFIFO_ActiveSnoopAx->GetTop();
		CPAxPkt cpAxTop = (upTop->cpAR != NULL) ? upTop->cpAR : upTop->cpAW;
		InitMaster = GetPortNum(cpAxTop->GetID());
	} else {
		// Not all responses are in yet, or no active transaction.
		return (ERESULT_TYPE_SUCCESS);
	}

	assert(InitMaster >= 0);


	// ----------------------------------------------------------------------------------------------------
	// 1. Collect CR from snoop targets to FIFOs by putting the CR transactions from remote ports to local ports.
	// ----------------------------------------------------------------------------------------------------
	for (int for_snoopMaster = 0; for_snoopMaster < this->NUM_PORT; for_snoopMaster++) {

		// Skip if the SnoopMaster and InitMaster is the same.
		if (for_snoopMaster == InitMaster) { continue; }

		// ---- 1. Checking receiving conditions ---
		// Do not accept the transactions if the FIFO is full.
		if (this->cpFIFO_SnoopResp[for_snoopMaster]->IsFull() == ERESULT_TYPE_YES) { // No transaction
			continue;
		}

		// Get CR - Check if the port is busy.
		// Skip if MASTERs' ports do not have transactions.
		if (this->cpRx_CR[for_snoopMaster]->GetPair()->IsBusy() == ERESULT_TYPE_NO) { // No transaction
			continue;
		}

		// ---- 2. BUS READY receives CR transactions ---
		// Get the transactions from MASTER ports.
		CPCRPkt cpCR = this->cpRx_CR[for_snoopMaster]->GetPair()->GetCR();
		
		if (cpCR == NULL) {
			continue;
		};

		// Put the AR transactions to local port = READY.
		this->cpRx_CR[for_snoopMaster]->PutCR(cpCR);

		// ---- 3. Push AR transactions to cpFIFO_SnoopResp ---
		// Instead of putting Rx to Tx, we put to a FIFO and maintain the FIFO to waiting snoops.
		UPUD upCR_new = new UUD;
		upCR_new->cpCR = Copy_CCRPkt(cpCR);
		this->cpFIFO_SnoopResp[for_snoopMaster]->Push(upCR_new);
		Delete_UD(upCR_new, EUD_TYPE_CR);

		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) push to cpFIFO_CR[%d].\n", nCycle, this->cName.c_str(), cpCR->GetName().c_str(), for_snoopMaster);
		#endif
	}

	// Ensure all snoop targets have returned CR
	for (int for_snoopMaster = 0; for_snoopMaster < this->NUM_PORT; for_snoopMaster++) {
		if (for_snoopMaster == InitMaster) continue;
		if (this->cpFIFO_SnoopResp[for_snoopMaster]->IsEmpty() == ERESULT_TYPE_YES) {
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
	std::vector<int> nCRRespList(this->NUM_PORT); // avoid memory leak
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
	for (int for_snoopMaster = 0; for_snoopMaster < this->NUM_PORT; for_snoopMaster++) {
		
		// Skip if the SnoopMaster and InitMaster is the same.
		if (for_snoopMaster == InitMaster) {continue;}

		// Check if the current CR FIFO is available.
		if (this->cpFIFO_SnoopResp[for_snoopMaster]->IsEmpty() == ERESULT_TYPE_YES) {
			continue;
		};

		// Get the response signals and encode it.
		nCRRespList[for_snoopMaster] = this->cpFIFO_SnoopResp[for_snoopMaster]->GetTop()->cpCR->GetResp(); // Record source port in ID
		
		if ((nCRRespList[for_snoopMaster] & 0b00001) == 0b00001) { // DataTransfer
			bDataTransfer |= true;
		}
		
		if ((nCRRespList[for_snoopMaster] & 0b00100) == 0b00100) { // PassDirty
			bPassDirty |= true;
		}
		
		if ((nCRRespList[for_snoopMaster] & 0b01000) == 0b01000) { // IsShared
			bIsShared |= true;
		}
		
		if ((nCRRespList[for_snoopMaster] & 0b10000) == 0b10000) { // WasUnique
			bWasUnique |= true;
		}
	}

	// If (bDataTransfer == 0), bPassDirty have to be de-assert.
	if (!bDataTransfer) assert(!bPassDirty);

	// --------------------------------------------------
	// 3. Write to the main memory
	// --------------------------------------------------
	bool isWrite = (this->cpFIFO_ActiveSnoopAC->GetTop()->cpAC->GetDir() == ETRANS_DIR_TYPE_WRITE);
	int snoopType = this->cpFIFO_ActiveSnoopAC->GetTop()->cpAC->GetSnoop();
	
	// Check if already marked UNDEFINED (address phase done)
	UPUD upTopStatus = this->cpFIFO_ActiveSnoopAx->GetTop();
	CPAxPkt cpAxStatus = (upTopStatus->cpAR != NULL) ? upTopStatus->cpAR : upTopStatus->cpAW;
	bool isUndefined = (cpAxStatus->GetDir() == ETRANS_DIR_TYPE_UNDEFINED);

	if (((snoopType == 0b1000 || snoopType == 0b1001) && bDataTransfer) || isWrite)
	{
		if (isUndefined) return (ERESULT_TYPE_SUCCESS);

		// =========================== Issuing the Write request ===========================
		if ((this->cpFIFO_ActiveSnoopAx->IsEmpty() == ERESULT_TYPE_NO) &&
		    (this->GetMO_AW() < MAX_MO_COUNT))
		{
			// Issuing to the main memory
			CPAxPkt cpAx = NULL;

			if (this->cpFIFO_ActiveSnoopAx->GetTop()->cpAR != NULL) {
				cpAx = Copy_CAxPkt(this->cpFIFO_ActiveSnoopAx->GetTop()->cpAR);
			} else if (this->cpFIFO_ActiveSnoopAx->GetTop()->cpAW != NULL) {
				cpAx = Copy_CAxPkt(this->cpFIFO_ActiveSnoopAx->GetTop()->cpAW);
			}

			if (cpAx != NULL) {
				int     nID   = cpAx->GetID();
				int64_t nAddr = cpAx->GetAddr();
				int     nLen  = cpAx->GetLen();

				nID = (nID << this->BIT_PORT) + InitMaster;

				// Set pkt for write to memory.
				cpAx->SetPkt(nID, nAddr, nLen);

				if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES) {
					delete(cpAx); cpAx = NULL;
					return (ERESULT_TYPE_SUCCESS);
				}

				this->cpTx_AW->PutAx(cpAx);
				this->m_outstandingMemAWID.push_back(nID);

				#ifdef DEBUG_BUS
					printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) put Tx_AW. Mark top snoop UNDEFINED.\n", nCycle, this->cName.c_str(), cpAx->GetName().c_str());
				#endif

				// After issuing a write, the direction of the top component of tracking FIFO should be change to UNDEFINED.
				// This signals that the address phase is done and we are waiting for data in Do_W_snoop_fwd.
				UPUD upTop = this->cpFIFO_ActiveSnoopAx->GetTop();
				if (upTop->cpAR) upTop->cpAR->SetTransDirType(ETRANS_DIR_TYPE_UNDEFINED);
				else if (upTop->cpAW) upTop->cpAW->SetTransDirType(ETRANS_DIR_TYPE_UNDEFINED);

				delete(cpAx);
				cpAx = NULL;
			}
		}

		return (ERESULT_TYPE_SUCCESS);
	}

	// --------------------------------------------------
	// 4. Read the main memory
	// --------------------------------------------------
	if ((snoopType != 0b1000) && // CleanShared
		(snoopType != 0b1001) && // CleanInvalid
		(snoopType != 0b1101) && // MakeInvalid
		(!bIsShared && !bWasUnique && !bDataTransfer && !bPassDirty)) {

		if (isUndefined) return (ERESULT_TYPE_SUCCESS);

		// ---- Check MO ---
		if (this->GetMO_AR() >= MAX_MO_COUNT) {
			return (ERESULT_TYPE_FAIL);
		};

		// Check if there is any other masters that have already issued READ transaction to the main memory.
		if (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES) {
			return (ERESULT_TYPE_SUCCESS);
		}

		// Issuing to the main memory
		CPAxPkt cpAR = Copy_CAxPkt(this->cpFIFO_ActiveSnoopAx->GetTop()->cpAR);

		// Encode ID
		int     nID   = cpAR->GetID();
		int64_t nAddr = cpAR->GetAddr();
		int     nLen  = cpAR->GetLen();
		//nID = (nID << this->BIT_PORT) + InitMaster;

		// Set pkt
		cpAR->SetPkt(nID, nAddr, nLen);

		this->cpTx_AR->PutAx(cpAR);
		this->m_outstandingMemARID.push_back(nID);

		// Pop-out the snoop responses.
		for (int for_snoopMaster = 0; for_snoopMaster < this->NUM_PORT; for_snoopMaster++) {
			if (for_snoopMaster == InitMaster) continue;
			UPUD upTmp;
			upTmp = this->cpFIFO_SnoopResp[for_snoopMaster]->Pop();			
			if (upTmp) Delete_UD(upTmp, EUD_TYPE_CR);
		}

		// Pop shared FIFOs ONCE
		UPUD upTmp;
		upTmp = this->cpFIFO_ActiveSnoopAx->Pop();
		if (upTmp) Delete_UD(upTmp, EUD_TYPE_AR);

		upTmp = this->cpFIFO_ActiveSnoopAC->Pop();
		if (upTmp) Delete_UD(upTmp, EUD_TYPE_AC);
		
		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) put Tx_AR.\n", nCycle, this->cName.c_str(), cpAR->GetName().c_str());
		#endif

		delete(cpAR);
		cpAR = NULL;
		return (ERESULT_TYPE_SUCCESS);
	}
	
	// --------------------------------------------------
	// 5. Return to the initiating Master
	// --------------------------------------------------
	if (isUndefined) return (ERESULT_TYPE_SUCCESS);

	// Enforce ID ordering: Block if a transaction with the same ID is in flight to memory
	UPUD upTop_hazard = this->cpFIFO_ActiveSnoopAx->GetTop();

	if (upTop_hazard->cpAR == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	}

	int currentOriginalID = upTop_hazard->cpAR->GetID();

	auto itAR = std::find(this->m_outstandingMemARID.begin(), this->m_outstandingMemARID.end(), currentOriginalID);
	if (itAR != this->m_outstandingMemARID.end()) {
		return (ERESULT_TYPE_SUCCESS); // Stall
	}

	// Check Tx valid 
	if (this->cpTx_R[InitMaster]->IsBusy() == ERESULT_TYPE_YES) {
        return (ERESULT_TYPE_SUCCESS);
	};

	bool brespIsShared = bIsShared;
	bool brespPassDirty = false;
	EResultType IsLast = ERESULT_TYPE_NO; // Assume completion since data transfer is disabled.

	// Checking centralized FIFO to collect data.
	if (bDataTransfer) {
		if (this->cpFIFO_SnoopData->IsEmpty() == ERESULT_TYPE_NO) {
			if (this->cpFIFO_SnoopData->GetTop()->cpW->IsLast() == ERESULT_TYPE_YES) {
				IsLast = ERESULT_TYPE_YES;
			}
		}
	} else {
		IsLast = ERESULT_TYPE_YES;
	}

	if (!bDataTransfer) IsLast = ERESULT_TYPE_YES;
	
	//// Construct the R transactions.
	UPUD respTrans = new UUD;
	respTrans->cpR = new CRPkt;

	respTrans->cpR->SetPkt(
		InitMaster, 
		0, 			
		IsLast, 	
		brespIsShared << 3 | brespPassDirty << 2
	); 
	respTrans->cpR->SetLast(IsLast);
	respTrans->cpR->SetName(this->cpFIFO_ActiveSnoopAC->GetTop()->cpAC->GetName());

	// Put to R.
	this->cpTx_R[InitMaster]->PutR(respTrans->cpR);

	#ifdef DEBUG_BUS
		printf("[Cycle %3ld: %s.Do_CR_fwd] (%s) put cpTx_R[%d] - bDataTransfer = %d; IsLast = %s.\n", nCycle, this->cName.c_str(), respTrans->cpR->GetName().c_str(), InitMaster, bDataTransfer, Convert_eResult2string(IsLast).c_str());
	#endif

	//// Pop-out the snoop responses.
	for (int for_snoopMaster = 0; for_snoopMaster < this->NUM_PORT; for_snoopMaster++) {
		if (for_snoopMaster == InitMaster) { continue; }
		UPUD upTmp;

		if (IsLast == ERESULT_TYPE_YES) {
			upTmp = this->cpFIFO_SnoopResp[for_snoopMaster]->Pop();
			if (upTmp) Delete_UD(upTmp, EUD_TYPE_CR);
		}
	}

	//// Pop shared FIFOs ONCE
	UPUD upTmp;
	if (bDataTransfer) {
		upTmp = this->cpFIFO_SnoopData->Pop();
		if (upTmp) Delete_UD(upTmp, EUD_TYPE_W);
	}

	if (IsLast == ERESULT_TYPE_YES) {
		upTmp = this->cpFIFO_ActiveSnoopAx->Pop();
		if (upTmp) Delete_UD(upTmp, EUD_TYPE_AR);
		
		upTmp = this->cpFIFO_ActiveSnoopAC->Pop();
		if (upTmp) Delete_UD(upTmp, EUD_TYPE_AC);
	}

	//// Clean up the allocated response packet
	Delete_UD(respTrans, EUD_TYPE_R);

	return (ERESULT_TYPE_SUCCESS);
};


//---------------------------------------------------------------------------------------------------
// CR ready
//	Handles the backward (READY) part of the CR channel handshake.
//	If the bus has received a snoop response from a master port (i.e., cpRx_CR is busy),
//	it signals acceptance (asserts READY) back to that master.
//---------------------------------------------------------------------------------------------------
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

EResultType CBUS::Do_W_snoop_fwd(int64_t nCycle) {

	// The Tx_W from MASTER often disable since we assumpt the data followed the commands. 
	// Thus, when there are no data transfer from snooped master, we just pop the FIFO and assump the data followed the request. 
	// In case the snooped master would like to return data, we can simulate the spacing between requests and data.
	// Check if active transaction marked as UNDEFINED (waiting for data phase)
	int InitMaster = -1;

	if (this->cpFIFO_ActiveSnoopAx->IsEmpty() == ERESULT_TYPE_NO) {
		UPUD upTop = this->cpFIFO_ActiveSnoopAx->GetTop();
		CPAxPkt cpAxTop = (upTop->cpAR != NULL) ? upTop->cpAR : upTop->cpAW;

		// Only process if Do_CR_fwd has already issued the address phase (marked by UNDEFINED direction)
		if (cpAxTop->GetDir() != ETRANS_DIR_TYPE_UNDEFINED) {
			return (ERESULT_TYPE_SUCCESS);
		}

		InitMaster = GetPortNum(cpAxTop->GetID());
	} else {
		return (ERESULT_TYPE_SUCCESS);
	}

	assert(InitMaster >= 0);

	// Since Do_CR_fwd already verified all responses were in before marking UNDEFINED,
	// we check bDataTransfer from the stored slave responses.
	bool bDataTransfer = false;
	for (int i = 0; i < this->NUM_PORT; i++) {
		if (i == InitMaster) continue;
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
	}

	// --------------------------------------------------
	// 3. Write data to the main memory
	// --------------------------------------------------
	if (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL); 
	};

	if (bDataTransfer) {
		CPWPkt cpW = Copy_CWPkt(this->cpFIFO_SnoopData->GetTop()->cpW);

		// Set pkt for write to memory.
		this->cpTx_W->PutW(cpW);
		// Pop-out the snoop response data.
		UPUD upW_tmp = this->cpFIFO_SnoopData->Pop();
		if (upW_tmp) Delete_UD(upW_tmp, EUD_TYPE_W);

		if (cpW->IsLast() == ERESULT_TYPE_YES) {
			// Data transfer complete, pop tracking FIFOs
			for (int i = 0; i < this->NUM_PORT; i++) {
				if (i == InitMaster) continue;
				UPUD upTmp = this->cpFIFO_SnoopResp[i]->Pop();
				if (upTmp) Delete_UD(upTmp, EUD_TYPE_CR);
			}

			UPUD upTmp;
			upTmp = this->cpFIFO_ActiveSnoopAC->Pop();
			if (upTmp) Delete_UD(upTmp, EUD_TYPE_AC);
			
			upTmp = this->cpFIFO_ActiveSnoopAx->Pop();
			if (upTmp) Delete_UD(upTmp, EUD_TYPE_AR);

			#ifdef DEBUG_BUS
				printf("[Cycle %3ld: %s.Do_W_snoop_fwd] (%s) WLAST put Tx_W. Pop tracking.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
			#endif
		} else {
			#ifdef DEBUG_BUS
				printf("[Cycle %3ld: %s.Do_W_snoop_fwd] (%s) put Tx_W.\n", nCycle, this->cName.c_str(), cpW->GetName().c_str());
			#endif
		}
		delete(cpW);
		cpW = NULL;

		return (ERESULT_TYPE_SUCCESS);
	} else {
		// No data transfer expected (Address-only write phase completed)
		for (int i = 0; i < this->NUM_PORT; i++) {
			if (i == InitMaster) continue;
			UPUD upTmp = this->cpFIFO_SnoopResp[i]->Pop();
			if (upTmp) Delete_UD(upTmp, EUD_TYPE_CR);
		}

		UPUD upTmp;
		upTmp = this->cpFIFO_ActiveSnoopAC->Pop();
		if (upTmp) Delete_UD(upTmp, EUD_TYPE_AC);
				
		upTmp = this->cpFIFO_ActiveSnoopAx->Pop();
		if (upTmp) Delete_UD(upTmp, EUD_TYPE_AR);

		#ifdef DEBUG_BUS
			printf("[Cycle %3ld: %s.Do_W_snoop_fwd] Finished address-only write phase marked UNDEFINED.\n", nCycle, this->cName.c_str());
		#endif

		return (ERESULT_TYPE_SUCCESS);
	}
}
;
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

		#ifdef CCI_ON
		auto it = std::find(this->m_outstandingMemARID.begin(), this->m_outstandingMemARID.end(), cpR->GetID());
		if (it != this->m_outstandingMemARID.end()) {
			this->m_outstandingMemARID.erase(it);
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
	
	#ifdef CCI_ON
	auto it = std::find(this->m_outstandingMemAWID.begin(), this->m_outstandingMemAWID.end(), nID);
	if (it != this->m_outstandingMemAWID.end()) {
		this->m_outstandingMemAWID.erase(it);
	}
	#endif
	
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
