//-----------------------------------------------------------
// Filename     : CQ.cpp 
// Version	: 0.8
// Date         : 18 Nov 2019
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Queue definition
//-----------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>

#include "CQ.h"

// Constructor
CQ::CQ(string cName, EUDType eUDType, int nMaxNum) {

	// Generate node later when push
	this->spMUDList_head = NULL;	
	this->spMUDList_tail = NULL;	

	// Initialize	
	this->cName   = cName;
	this->eUDType = eUDType;
	this->nMaxNum = nMaxNum;
	this->nCurNum = -1;

	this->nMaxCycleWait = -1;
};


// Destructor
CQ::~CQ() {

	// Debug
	#ifndef BACKGROUND_TRAFFIC_ON 
	assert (this->GetCurNum() == 0);	// RMM, background RPTW maybe on-going
	assert (this->spMUDList_head == NULL);
	assert (this->spMUDList_tail == NULL);
	#endif

	// Delete all nodes
	SPLinkedMUD spScan;
	SPLinkedMUD spTarget;
	spScan = this->spMUDList_head;

	while (spScan != NULL) {
		spTarget = spScan;
		spScan   = spScan->spNext;
		Delete_UD(spTarget->upData, this->eUDType);
		spTarget->upData = NULL;
		spTarget = NULL;	
		// spTarget->spPrev = NULL;		
		// spTarget->spNext = NULL;		
		// spTarget->spMemCmdPkt = NULL;		
	};

	// Debug
	// assert (this->spMUDList_head == NULL);
	// assert (this->spMUDList_tail == NULL);
};


// Initialize
EResultType CQ::Reset() {

	// Delete all nodes 
	SPLinkedMUD spScan;
	SPLinkedMUD spTarget;
	spScan = this->spMUDList_head;

	while (spScan != NULL) {
		spTarget = spScan;
		spScan   = spScan->spNext;
		Delete_UD(spTarget->upData, this->eUDType);
		delete (spTarget->upData);
		spTarget->upData = NULL;

		spTarget->spPrev = NULL;		
		spTarget->spNext = NULL;		
		spTarget->spMemCmdPkt = NULL;		
		spTarget->eUDType = EUD_TYPE_UNDEFINED;
		spTarget->nID = -1;
		spTarget->nCycle_wait = -1;
	};

	// Initialize	
	this->nCurNum = 0;
	this->nMaxCycleWait = 0;
	this->spMUDList_head = NULL;	
	this->spMUDList_tail = NULL;	
	return (ERESULT_TYPE_SUCCESS);
};


// Push
EResultType CQ::Push(UPUD upThis) {

	#ifdef DEBUG
	assert (upThis != NULL);
	#endif

	// Generate node. Initialize
	UPUD upUD_new = Copy_UD(upThis, this->eUDType);

	SPLinkedMUD spLinkedMUD_new = new SLinkedMUD;

	spLinkedMUD_new->upData  = upUD_new;
	spLinkedMUD_new->eUDType = this->eUDType;
	spLinkedMUD_new->spPrev  = NULL;
	spLinkedMUD_new->spNext  = NULL;
	spLinkedMUD_new->spMemCmdPkt = new SMemCmdPkt;
	spLinkedMUD_new->spMemCmdPkt->eMemCmd = EMEM_CMD_TYPE_NOP;
	spLinkedMUD_new->spMemCmdPkt->nBank   = -1;
	spLinkedMUD_new->spMemCmdPkt->nRow    = -1;
	spLinkedMUD_new->nCycle_wait = 0;

	if (this->eUDType == EUD_TYPE_AR) {
		spLinkedMUD_new->nID = upUD_new->cpAR->GetID();
	} 
	else if (this->eUDType == EUD_TYPE_AW) {
		spLinkedMUD_new->nID = upUD_new->cpAW->GetID();
	} 
	else {
		assert (0);
	};

	// Push
	if (this->spMUDList_head == NULL) {

		#ifdef DEBUG
		assert (this->nCurNum == 0);
		assert (this->spMUDList_tail == NULL);
		#endif

		this->spMUDList_head = spLinkedMUD_new;
		this->spMUDList_tail = spLinkedMUD_new;
	} 
	else {
		#ifdef DEBUG
		assert (this->nCurNum > 0);
		assert (this->spMUDList_tail != NULL);
		#endif

		this->spMUDList_tail->spNext = spLinkedMUD_new;
		spLinkedMUD_new->spPrev = this->spMUDList_tail;
		this->spMUDList_tail = spLinkedMUD_new;
	};

	// Increment occupancy
	this->nCurNum++;

	#ifdef DEBUG
	assert (upUD_new != NULL);
	assert (this->spMUDList_head != NULL);
	assert (this->spMUDList_tail != NULL);
	// this->CheckQ();
	#endif

	return (ERESULT_TYPE_SUCCESS);
};


// Pop node first matching ID
UPUD CQ::Pop(uint64_t nID) {

	// Debug
	// this->CheckQ();

	// Get target node 
	SPLinkedMUD spScan = NULL;
	SPLinkedMUD spTarget = NULL;
	spScan = this->spMUDList_head;

	while (spScan != NULL) {
		if (spScan->nID == nID) {
			spTarget = spScan;
			break;
		};
		spScan = spScan->spNext;
	};

	// Not matched
	if (spTarget == NULL) {
		return (NULL);
	};

	UPUD upTarget = spTarget->upData;

	#ifdef DEBUG
	assert (spTarget != NULL);
	assert (upTarget != NULL);
	#endif

	// Decrement occupancy
	this->nCurNum--;

	// Pop target 
	if (this->nCurNum == 0) {
		this->spMUDList_head = NULL;
		this->spMUDList_tail = NULL;
	} 
	else {
		#ifdef DEBUG
		assert (this->nCurNum >= 1);
		assert (this->spMUDList_head != NULL);
		assert (this->spMUDList_tail != NULL);
		#endif

		if (spTarget == this->spMUDList_head) {
			this->spMUDList_head = this->spMUDList_head->spNext;
		} 
		else if (spTarget == this->spMUDList_tail) {
			this->spMUDList_tail = spTarget->spPrev;
			spTarget->spPrev->spNext = NULL; 
		} 
		else {
			spTarget->spPrev->spNext = spTarget->spNext;
			spTarget->spNext->spPrev = spTarget->spPrev;
		};
	};

	// Maintain
	delete (spTarget->spMemCmdPkt);
	delete (spTarget);

	#ifdef DEBUG
	assert (upTarget != NULL);
	// this->CheckQ();
	#endif

	return (upTarget);	
};


// Pop head
UPUD CQ::Pop() {

	// Debug
	// this->CheckQ();
	
	// Get head
	SPLinkedMUD spTarget;
	spTarget = this->spMUDList_head;
	
	UPUD upTarget;
	upTarget = this->spMUDList_head->upData;
	
	// Decrement occupancy
	this->nCurNum--;
	
	// Pop head
	if (this->nCurNum == 0) {
		this->spMUDList_head = NULL;
		this->spMUDList_tail = NULL;
	} 
	else {
		this->spMUDList_head = this->spMUDList_head->spNext;
	};
	
	// Maintain
	delete (spTarget->spMemCmdPkt);
	delete (spTarget);

	#ifdef DEBUG	
	assert (upTarget != NULL);
	// this->CheckQ();
	#endif
	
	return (upTarget);
};


//------------------------------------------
// Set mem state pkt
//	(1) Get cmd for all nodes automatically
//	(2) Set cmd for all nodes automatically
//------------------------------------------
EResultType CQ::SetMemStateCmdPkt(SPMemStatePkt spThis) {

	#ifdef DEBUG
	assert (spThis != NULL );
	#endif

	// Set mem state
	this->spMemStatePkt = spThis;

	// Set mem cmd for all nodes 
	SPLinkedMUD spScan;
	SPLinkedMUD spTarget;
	spScan = this->spMUDList_head;

	while (spScan != NULL) {
		spTarget = spScan;
		spScan   = spScan->spNext;

		// Get target bank row
		if (this->eUDType == EUD_TYPE_AR) {
			spTarget->spMemCmdPkt->nBank = spTarget->upData->cpAR->GetBankNum_MMap();
			spTarget->spMemCmdPkt->nRow  = spTarget->upData->cpAR->GetRowNum_MMap();
		} 
		else if (this->eUDType == EUD_TYPE_AW) {
			spTarget->spMemCmdPkt->nBank = spTarget->upData->cpAW->GetBankNum_MMap();
			spTarget->spMemCmdPkt->nRow  = spTarget->upData->cpAW->GetRowNum_MMap();
		} 
		else {
			assert (0);	
		};

		spTarget->spMemCmdPkt->eMemCmd = this->GetMemCmd(spTarget);	// FIXME Check spMemCmdPkt new. Be careful memory leak.		

	};

	return (ERESULT_TYPE_SUCCESS);
};


// Get mem cmd pkt
EMemCmdType CQ::GetMemCmd(SPLinkedMUD spThis) {
     
	// Generate and initialize 
	EMemCmdType eMemCmd = EMEM_CMD_TYPE_NOP;
	
	// Check delay to get cmd 
	// Use this latency as register 
	if (spThis->nCycle_wait < Q_MIN_WAITING_CYCLE) {
		// Set cmd
		eMemCmd = EMEM_CMD_TYPE_NOP;	
		return (eMemCmd);
	};	
	
	
	// Get target bank row
	int nBank = -1;
	int nRow  = -1;
	if (this->eUDType == EUD_TYPE_AR) {
		nBank = spThis->upData->cpAR->GetBankNum_MMap();
		nRow  = spThis->upData->cpAR->GetRowNum_MMap();
	} 
	else if (this->eUDType == EUD_TYPE_AW) {
		nBank = spThis->upData->cpAW->GetBankNum_MMap();
		nRow  = spThis->upData->cpAW->GetRowNum_MMap();
	} 
	else {
		assert (0);	
	};
	
	// Check cmd ready
	EResultType eIsACT_ready = this->spMemStatePkt->IsACT_ready[nBank];
	EResultType eIsPRE_ready = this->spMemStatePkt->IsPRE_ready[nBank];
	EResultType eIsRD_ready  = this->spMemStatePkt->IsRD_ready[nBank];
	EResultType eIsWR_ready  = this->spMemStatePkt->IsWR_ready[nBank];

	// Check RD issueable
	if (eIsRD_ready == ERESULT_TYPE_YES and this->eUDType == EUD_TYPE_AR) {
		// Check bank hit
		if (nRow == this->spMemStatePkt->nActivatedRow[nBank]) {
			// Set cmd
			eMemCmd = EMEM_CMD_TYPE_RD;
			return (eMemCmd);
		};
	};
	
	// Check WR issueable
	if (eIsWR_ready == ERESULT_TYPE_YES and this->eUDType == EUD_TYPE_AW) {
		// Check bank hit
		if (nRow == this->spMemStatePkt->nActivatedRow[nBank]) {
			// Set cmd
			eMemCmd = EMEM_CMD_TYPE_WR;
			return (eMemCmd);
		};
	};
	
	// Check PRE issueable
	if (eIsPRE_ready == ERESULT_TYPE_YES) {
		// Check bank miss
		if (nRow != this->spMemStatePkt->nActivatedRow[nBank]) {
			// Set cmd
			eMemCmd = EMEM_CMD_TYPE_PRE;	
			return (eMemCmd);
		};
	};
	
	// Check ACT issueable
	if (eIsACT_ready == ERESULT_TYPE_YES) {
		// Set cmd
		eMemCmd = EMEM_CMD_TYPE_ACT;	
		return (eMemCmd);
	};

	// Check NOP issued
	// assert (spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_NOP);
	// assert (spMemCmdPkt->nBank != -1);
	// assert (spMemCmdPkt->nRow  != -1);
	
	return (eMemCmd);
};

// Get mem state pkt
SPMemStatePkt CQ::GetMemStatePkt() {

	return (this->spMemStatePkt);
};


// Get waiting cycles 
int CQ::GetCycleWait(UPUD upThis) {

	SPLinkedMUD spScan;
	SPLinkedMUD spTarget;
	spScan = this->spMUDList_head;

	while (spScan != NULL) {
		spTarget = spScan;
		spScan   = spScan->spNext;
	
		if (spTarget->upData == upThis) {

			#ifdef DEBUG
			assert (spTarget->nCycle_wait >= 0);
			#endif

			return (spTarget->nCycle_wait);
		};
	};

	// Debug
	assert (0);
	return (-1);
};


// Get UD type
EUDType CQ::GetUDType() {

	return (this->eUDType);
};


// Get current occupancy
int CQ::GetCurNum() {

	return (this->nCurNum);
};


// Get size
int CQ::GetMaxNum() {

	return (this->nMaxNum);
};


// Get max waiting cycles among enties
int CQ::GetMaxCycleWait() {

	return (this->nMaxCycleWait);
};


// Get head upData 
UPUD CQ::GetTop() {

	// Debug
	// this->CheckQ();

	if (this->spMUDList_head == NULL) {

		#ifdef DEBUG
		assert (this->spMUDList_tail == NULL);
		#endif

	        return (NULL);
	};

	#ifdef DEBUG
	assert (this->spMUDList_head->upData != NULL);
	#endif

	return (this->spMUDList_head->upData);
};


// Check Q empty
EResultType CQ::IsEmpty() {

	if (this->nCurNum == 0) {

		#ifdef DEBUG
		assert (this->spMUDList_head == NULL);
		assert (this->spMUDList_tail == NULL);
		#endif

		return (ERESULT_TYPE_YES);	
	};

	#ifdef DEBUG
	assert (this->spMUDList_head != NULL);
	assert (this->spMUDList_tail != NULL);
	#endif

	return (ERESULT_TYPE_NO);
};


// Check Q full 
EResultType CQ::IsFull() {

	if (this->nCurNum == this->nMaxNum) {

		#ifdef DEBUG
		assert (this->spMUDList_head != NULL);
		assert (this->spMUDList_tail != NULL);
		#endif 

		return (ERESULT_TYPE_YES);	
	};
	return (ERESULT_TYPE_NO);
};


// Get first node matching ID
SPLinkedMUD CQ::GetIDHeadNode(uint64_t nID) {
	
	SPLinkedMUD spScan;
	SPLinkedMUD spTarget;
	spScan = this->spMUDList_head;

	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->nID == nID) {
		        return (spTarget);
		};
		spScan = spScan->spNext;
	};

	return (NULL); // No node matching ID
};


// Check any ID-head node bank hit 
EResultType CQ::IsThereIDHeadBankHit(int nTargetBank) {

	#ifdef DEBUG
	assert (nTargetBank >= 0);
	#endif

	SPLinkedMUD spScan;
	spScan = this->spMUDList_head;

	while (spScan != NULL) {

		// Check ID head
		if (spScan != this->GetIDHeadNode(spScan->nID)) {
			spScan = spScan->spNext;
			continue;
		};

		int nScanBank = -1;
		int nScanRow  = -1;
	
		// Scan	
		if (this->eUDType == EUD_TYPE_AR) {	
			nScanBank = spScan->upData->cpAR->GetBankNum_MMap();
			nScanRow  = spScan->upData->cpAR->GetRowNum_MMap();
		}
		else if (this->eUDType == EUD_TYPE_AW) {	
			nScanBank = spScan->upData->cpAW->GetBankNum_MMap();
			nScanRow  = spScan->upData->cpAW->GetRowNum_MMap();
		}
		else {
			assert (0);
		};

		// Check target bank number
		if (nScanBank != nTargetBank) { 
			spScan = spScan->spNext;
			continue;
		};

		#ifdef DEBUG
		assert (nScanBank == nTargetBank);
		#endif

		// Check bank hit
		int nActivatedRow = this->GetMemStatePkt()->nActivatedRow[nTargetBank]; // Mem state in current cycle
		if (nScanRow == nActivatedRow) {
			return (ERESULT_TYPE_YES);	
		};

		spScan = spScan->spNext;
	};

	return (ERESULT_TYPE_NO);	
};


// Check this is first node (in req Q) to access target bank 
EResultType CQ::IsThisFirstInBank(SPLinkedMUD spThis) {

	#ifdef DEBUG
	assert (spThis != NULL);
	assert (this->eUDType == spThis->eUDType);
	#endif

	int nTargetBank = -1;

	if (this->eUDType == EUD_TYPE_AR) {
		nTargetBank = spThis->upData->cpAR->GetBankNum_MMap();
	}
	else if (this->eUDType == EUD_TYPE_AW) {
		nTargetBank = spThis->upData->cpAW->GetBankNum_MMap();
	}
	else {
		assert (0);
	};

	#ifdef DEBUG
	assert (nTargetBank != -1);
	#endif

	SPLinkedMUD spScan = this->spMUDList_head;

	while (spScan != NULL) {

		int nScanBank = -1;
	
		// Scan	
		if (this->eUDType == EUD_TYPE_AR) {	
			nScanBank = spScan->upData->cpAR->GetBankNum_MMap();
		}
		else if (this->eUDType == EUD_TYPE_AW) {	
			nScanBank = spScan->upData->cpAW->GetBankNum_MMap();
		}
		else {
			assert (0);
		};

		// Check target bank number
		if (nScanBank != nTargetBank) { 
			spScan = spScan->spNext;
			continue;
		};

		#ifdef DEBUG
		assert (nScanBank == nTargetBank);
		#endif

		// Check same 
		if (spScan == spThis) {
			return (ERESULT_TYPE_YES);	
		}
		else {
			return (ERESULT_TYPE_NO);	
		};

		spScan = spScan->spNext;
	};

	return (ERESULT_TYPE_NO);	
};


// Debug
EResultType CQ::CheckQ() {

	// Debug
	assert (this->nCurNum >= 0);
	assert (this->nCurNum <= this->nMaxNum);
	
	// Check NULL 
	if (this->nCurNum == 0) {        
		assert (this->spMUDList_head == NULL);
		assert (this->spMUDList_tail == NULL);
		// assert (this->spMUDList_head->spPrev == NULL);
		// assert (this->spMUDList_head->spNext == NULL);
		// assert (this->spMUDList_tail->spPrev == NULL);
		// assert (this->spMUDList_tail->spNext == NULL);
		assert (this->IsEmpty() == ERESULT_TYPE_YES);
		assert (this->GetCurNum() == 0); 
	} 
	else {
		assert (this->spMUDList_head != NULL);
		assert (this->spMUDList_tail != NULL);
		// assert (this->spMUDList_head->spPrev == NULL);
		// assert (this->spMUDList_tail->spNext == NULL);
	};
	
	// Check occupancy 
	int nCurOccupancy = 0;
	SPLinkedMUD spScan = this->spMUDList_head;
	while (spScan != NULL) {
		// Debug
		assert (spScan->nCycle_wait >= 0); 	

		spScan   = spScan->spNext;
		nCurOccupancy++;
	};
	assert (nCurOccupancy == this->nCurNum);

	// Check full	
	if (nCurOccupancy == this->nMaxNum) {
		assert (this->IsFull() == ERESULT_TYPE_YES);
	} 
	else {
		assert (this->IsFull() == ERESULT_TYPE_NO);
	};
	
	return (ERESULT_TYPE_SUCCESS);
};


// UpdateState 
EResultType CQ::UpdateState() {

	// Debug
	// this->CheckQ();

	// Update entry waiting cycle since allocation 
	uint64_t nMaxCycleWait = -1;
	SPLinkedMUD spScan = this->spMUDList_head;
	while (spScan != NULL) {

		#ifdef DEBUG
		if (spScan->nCycle_wait < 0) {
			printf("Error: CQ::UpdateState() - nCycle_wait < 0\n");
			Display();
			assert (0);
		};
		assert (spScan->nCycle_wait >= 0); 	
		#endif

		spScan->nCycle_wait++;		// Used in scheduler			

		// Get max waiting cycles
		if (nMaxCycleWait < spScan->nCycle_wait) {
			nMaxCycleWait = spScan->nCycle_wait;
		};

		spScan = spScan->spNext;
	};

	this->nMaxCycleWait = nMaxCycleWait;

	return (ERESULT_TYPE_SUCCESS);
};


// Debug
EResultType CQ::Display() {

	string cUDType = Convert_eUDType2string(this->eUDType);

	printf("---------------------------------------------\n");
	printf("\t Q display\n");
	printf("---------------------------------------------\n");
	printf("\t Name         : \t %s\n", this->cName.c_str());
	printf("\t eUDType      : \t %s\n", cUDType.c_str());
	printf("\t nCurNum      : \t %d\n", this->nCurNum);
	printf("\t nMaxNum      : \t %d\n", this->nMaxNum);
	printf("\t nMaxCycleWait: \t %d\n", this->nMaxCycleWait);
	printf("---------------------------------------------\n");
	
	SPLinkedMUD spScan = NULL;
	SPLinkedMUD spTarget = NULL;
	spScan = this->spMUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		spScan   = spScan->spNext;
		Display_UD(spTarget->upData, this->eUDType);	
		printf("\t Waiting cycle : \t %ld\n", spTarget->nCycle_wait);
	};
	
	return (ERESULT_TYPE_SUCCESS);
};

