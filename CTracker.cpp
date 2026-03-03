//-----------------------------------------------------------
// Filename     : CTracker.cpp 
// Version	: 0.82
// Date         : 14 Mar 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Cache tracker priority queue definition
//-----------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>

#include "CTracker.h"


// Constructor
CTracker::CTracker(string cName, int nMaxNum) {

	// Initialize	
	this->cName   = cName;
	this->nMaxNum = nMaxNum;
	this->nCurNum = -1;

	// Generate node later when push conducted
	this->spCUDList_head = NULL;	
	this->spCUDList_tail = NULL;	
};


// Destructor
CTracker::~CTracker() {

	#ifndef BACKGROUND_TRAFFIC_ON 
	assert (this->GetCurNum() == 0);	// RMM, background RPTW maybe on-going
	assert (this->spCUDList_head == NULL);
	assert (this->spCUDList_tail == NULL);
	#endif

	// Delete all nodes
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		spScan   = spScan->spNext;
		Delete_UD(spTarget->upData, spTarget->eUDType);
		spTarget->upData = NULL;
		spTarget = NULL;
		// spTarget->spPrev = NULL;		
		// spTarget->spNext = NULL;		
	};

	// Debug
	// assert (this->spCUDList_head == NULL);
	// assert (this->spCUDList_tail == NULL);
};


// Initialize
EResultType CTracker::Reset() {

	// Delete all nodes 
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		spScan   = spScan->spNext;
		Delete_UD(spTarget->upData, spTarget->eUDType);
		delete (spTarget->upData);

		spTarget->upData = NULL;
		spTarget->spPrev = NULL;		
		spTarget->spNext = NULL;		
		spTarget->eUDType = EUD_TYPE_UNDEFINED;
		spTarget->nID = -1;
		spTarget->nCycle_wait = -1;
	};

	// Initialize	
	this->nCurNum = 0;
	this->nMaxCycleWait = 0;
	this->spCUDList_head = NULL;	
	this->spCUDList_tail = NULL;	

	return (ERESULT_TYPE_SUCCESS);
};


// Push
EResultType CTracker::Push(UPUD upThis, EUDType eUDType, ECacheStateType eState) {

	#ifdef DEBUG
	assert (upThis != NULL);
	#endif

	// Generate node. Initialize
	UPUD upUD_new = Copy_UD(upThis, eUDType);

	SPLinkedCUD spLinkedCUD_new = new SLinkedCUD;
	spLinkedCUD_new->upData  = upUD_new;
	spLinkedCUD_new->eUDType = eUDType;
	spLinkedCUD_new->eState  = eState;
	spLinkedCUD_new->nCycle_wait = 0;

	spLinkedCUD_new->spPrev  = NULL;
	spLinkedCUD_new->spNext  = NULL;

	// Set ID
	if (eUDType == EUD_TYPE_AR) {
		spLinkedCUD_new->nID = upUD_new->cpAR->GetID();
	} 
	else if (eUDType == EUD_TYPE_AW) {
		spLinkedCUD_new->nID = upUD_new->cpAW->GetID();
	} 
	else {
		assert (0);
	};

	// Push
	if (this->spCUDList_head == NULL) {

		#ifdef DEBUG
		assert (this->nCurNum == 0);
		assert (this->spCUDList_tail == NULL);
		#endif

		// Create head and tail
		this->spCUDList_head = spLinkedCUD_new;
		this->spCUDList_tail = spLinkedCUD_new;
	} 
	else {
		#ifdef DEBUG
		assert (this->nCurNum > 0);
		assert (this->spCUDList_tail != NULL);
		#endif

		// Add to tail
		this->spCUDList_tail->spNext = spLinkedCUD_new;
		spLinkedCUD_new->spPrev = this->spCUDList_tail;
		this->spCUDList_tail = spLinkedCUD_new;
	};

	// Increment occupancy
	this->nCurNum++;

	#ifdef DEBUG
	assert (upUD_new != NULL);
	// this->CheckTracker();
	#endif

	return (ERESULT_TYPE_SUCCESS);
};

//----------------------------------------------
// Pop 
// 	Search node first matching nID, eUDType, and state
//----------------------------------------------
UPUD CTracker::Pop(int nID, EUDType eUDType, ECacheStateType eState) {

	// Debug
	//this->CheckTracker();

	// Get target node 
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget = NULL;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		if (spScan->nID == nID and 
			spScan->eUDType == eUDType and 
			spScan->eState == eState) {

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

	// Decrement occupancy
	this->nCurNum--;

	// Pop target 
	if (this->nCurNum == 0) {
		this->spCUDList_head = NULL;
		this->spCUDList_tail = NULL;
	} 
	else {
		#ifdef DEBUG
		assert (this->nCurNum >= 1);
		#endif

		if (spTarget == this->spCUDList_head) {
			this->spCUDList_head = this->spCUDList_head->spNext;
		} 
		else if (spTarget == this->spCUDList_tail) {
			this->spCUDList_tail = spTarget->spPrev;
			spTarget->spPrev->spNext = NULL;
		} 
		else {
			spTarget->spPrev->spNext = spTarget->spNext;
			spTarget->spNext->spPrev = spTarget->spPrev;
		};
	};

	// Maintain
	delete (spTarget);
	spTarget = NULL;

	#ifdef DEBUG
	assert (upTarget != NULL);
	// this->CheckTracker();
	#endif

	return (upTarget);	
};

//----------------------------------------------
// Pop 
// 	Search node first matching nID and eUDType
//----------------------------------------------
UPUD CTracker::Pop(int nID, EUDType eUDType) {

	// Debug
	//this->CheckTracker();

	// Get target node 
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget = NULL;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		if (spScan->nID == nID and spScan->eUDType == eUDType) {
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

	// Decrement occupancy
	this->nCurNum--;

	// Pop target 
	if (this->nCurNum == 0) {
		this->spCUDList_head = NULL;
		this->spCUDList_tail = NULL;
	} 
	else {
		#ifdef DEBUG
		assert (this->nCurNum >= 1);
		#endif

		if (spTarget == this->spCUDList_head) {
			this->spCUDList_head = this->spCUDList_head->spNext;
		} 
		else if (spTarget == this->spCUDList_tail) {
			this->spCUDList_tail = spTarget->spPrev;
			spTarget->spPrev->spNext = NULL;
		} 
		else {
			spTarget->spPrev->spNext = spTarget->spNext;
			spTarget->spNext->spPrev = spTarget->spPrev;
		};
	};

	// Maintain
	delete (spTarget);
	spTarget = NULL;

	#ifdef DEBUG
	assert (upTarget != NULL);
	// this->CheckTracker();
	#endif

	return (upTarget);	
};


//----------------------------------------------
// Pop 
// 	Search node spNode 
//----------------------------------------------
UPUD CTracker::Pop(SPLinkedCUD spNode) {

	// Debug
	// this->CheckTracker();

	// Get target node 
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget = NULL;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		if (spScan == spNode) {
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

	// Decrement occupancy
	this->nCurNum--;

	// Pop target 
	if (this->nCurNum == 0) {
		this->spCUDList_head = NULL;
		this->spCUDList_tail = NULL;
	} 
	else {
		#ifdef DEBUG
		assert (this->nCurNum >= 1);
		#endif

		if (spTarget == this->spCUDList_head) {
			this->spCUDList_head = this->spCUDList_head->spNext;
		} 
		else if (spTarget == this->spCUDList_tail) {
			this->spCUDList_tail = spTarget->spPrev;
			spTarget->spPrev->spNext = NULL;
		} 
		else {
			spTarget->spPrev->spNext = spTarget->spNext;
			spTarget->spNext->spPrev = spTarget->spPrev;
		};
	};

	// Maintain
	delete (spTarget);
	spTarget = NULL;

	#ifdef DEBUG
	assert (upTarget != NULL);
	// this->CheckTracker();
	#endif

	return (upTarget);	
};


// Pop head
UPUD CTracker::Pop() {

	// Debug
	// this->CheckTracker();
	
	// Get head
	SPLinkedCUD spTarget;
	spTarget = this->spCUDList_head;
	
	UPUD upTarget;
	upTarget = this->spCUDList_head->upData;
	
	// Decrement occupancy
	this->nCurNum--;
	
	// Pop head
	if (this->nCurNum == 0) {
		this->spCUDList_head = NULL;
		this->spCUDList_tail = NULL;
	} 
	else {
		this->spCUDList_head = this->spCUDList_head->spNext;
	};
	
	// Maintain
	delete (spTarget);
	spTarget = NULL;

	#ifdef DEBUG	
	assert (upTarget != NULL);
	// this->CheckTracker();
	#endif
	
	return (upTarget);
};


// Get current occupancy
int CTracker::GetCurNum() {

	return (this->nCurNum);
};


// Get size
int CTracker::GetMaxNum() {

	return (this->nMaxNum);
};


// Get max waiting cycles among enties
int CTracker::GetMaxCycleWait() {

	return (this->nMaxCycleWait);
};


// Get head upData 
UPUD CTracker::GetTop() {

	// Debug
	// this->CheckTracker();

	if (this->spCUDList_head == NULL) {
	        return (NULL);
	};
	return (this->spCUDList_head->upData);
};


// Check tracker queue empty
EResultType CTracker::IsEmpty() {

	if (this->nCurNum == 0) {
		return (ERESULT_TYPE_YES);	
	};
	return (ERESULT_TYPE_NO);
};


// Check tracker queue full 
EResultType CTracker::IsFull() {

	if (this->nCurNum == this->nMaxNum) {
		return (ERESULT_TYPE_YES);	
	};
	return (ERESULT_TYPE_NO);
};

SPLinkedCUD CTracker::GetNode(int nID, EUDType eUDType, ECacheStateType eState) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->nID == nID and spTarget->eUDType == eUDType and spTarget->eState == eState) {
			return spTarget;
		};
		spScan = spScan->spNext;
	};

	return NULL; // No node match
}

// Get first node matching state
SPLinkedCUD CTracker::GetNode(ECacheStateType eState) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->eState == eState) {
		        return (spTarget);
		};
		spScan = spScan->spNext;
	};

	return (NULL); // No node match 
};


// Get first node matching ID
SPLinkedCUD CTracker::GetNode(int nID) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->nID == nID) {
		        return (spTarget);
		};
		spScan = spScan->spNext;
	};

	#ifdef DEBUG
	assert (0);
	#endif

	return (NULL); // No node match
};


// Get first node matching ID and EUDType 
SPLinkedCUD CTracker::GetNode(int nID, EUDType eUDType) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->nID == nID and spTarget->eUDType == eUDType) {
		        return (spTarget);
		};
		spScan = spScan->spNext;
	};

	#ifdef DEBUG
	assert (0);
	#endif

	return (NULL); // No node match
};


// Get first node matching ID and state 
SPLinkedCUD CTracker::GetNode(int nID, ECacheStateType eState) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->nID == nID and spTarget->eState == eState) {
		        return (spTarget);
		};
		spScan = spScan->spNext;
	};

	#ifdef DEBUG
	assert (0);
	#endif

	return (NULL); // No node match
};


// Get first node doing any PTW 
// SPLinkedCUD CTracker::GetNode_anyPTW() {
//	
//	SPLinkedCUD spScan;
//	SPLinkedCUD spTarget;
//	spScan = this->spCUDList_head;
//	while (spScan != NULL) {
//		spTarget = spScan;
//		if (spTarget->eState == ECACHE_STATE_TYPE_FIRST_PTW   or
//		    spTarget->eState == ECACHE_STATE_TYPE_SECOND_PTW  or
//	
//		    spTarget->eState == ECACHE_STATE_TYPE_FIRST_APTW  or
//		    spTarget->eState == ECACHE_STATE_TYPE_SECOND_APTW or 
//	
//		    spTarget->eState == ECACHE_STATE_TYPE_FIRST_RPTW  or
//		    spTarget->eState == ECACHE_STATE_TYPE_SECOND_RPTW or 
//		    spTarget->eState == ECACHE_STATE_TYPE_THIRD_RPTW ) {
//	
//		        return (spTarget);
//		};
//		spScan = spScan->spNext;
//	};
//	
//	return (NULL); // No node match 
// };


// Get first node doing PTW 
SPLinkedCUD CTracker::GetNode_PTW() {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->eState == ECACHE_STATE_TYPE_FIRST_PTW   or
		    spTarget->eState == ECACHE_STATE_TYPE_SECOND_PTW  or
		    spTarget->eState == ECACHE_STATE_TYPE_SECOND_PTW_DONE  or
		    spTarget->eState == ECACHE_STATE_TYPE_THIRD_PTW   or
		    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_PTW  or
            spTarget->eState == ECACHE_STATE_TYPE_FOURTH_PTW_DONE){   // Duong add

		        return (spTarget);
		};
		spScan = spScan->spNext;
	};

	return (NULL); // No node match 
};


// Get first node doing PTW matching EUDType and VA
SPLinkedCUD CTracker::GetNode_Pair_PTW_VA_AT(int64_t nVA, EUDType eUDType) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;

		// Check UD type
		if (spTarget->eUDType != eUDType) {
			spScan = spScan->spNext;
			continue;
		};

		// Check VA
		if (eUDType == EUD_TYPE_AR) {	
			if (spTarget->upData->cpAR->GetVA() != nVA) {
				spScan = spScan->spNext;
				continue;
			};
		}
		else if (eUDType == EUD_TYPE_AW){
			if (spTarget->upData->cpAR->GetVA() != nVA) {
				spScan = spScan->spNext;
				continue;
			};
		}
		else {	
			assert (0);
		};

		// Check PTW
		if (spTarget->eState == ECACHE_STATE_TYPE_FIRST_PTW      or \
           spTarget->eState == ECACHE_STATE_TYPE_SECOND_PTW      or \
           spTarget->eState == ECACHE_STATE_TYPE_SECOND_PTW_DONE or \
           spTarget->eState == ECACHE_STATE_TYPE_THIRD_PTW       or \
           spTarget->eState == ECACHE_STATE_TYPE_FOURTH_PTW      or \
           spTarget->eState == ECACHE_STATE_TYPE_FOURTH_PTW_DONE or \
           spTarget->eState == ECACHE_STATE_TYPE_PTW_HIT ) {

		    // spTarget->eState == ECACHE_STATE_TYPE_FIRST_APTW  or spTarget->eState == ECACHE_STATE_TYPE_SECOND_APTW or 
		    // spTarget->eState == ECACHE_STATE_TYPE_FIRST_RPTW or spTarget->eState == ECACHE_STATE_TYPE_SECOND_RPTW or spTarget->eState == ECACHE_STATE_TYPE_THIRD_RPTW ) {

		        return (spTarget);
		};
		spScan = spScan->spNext;
	};

	return (NULL); // No node match
};


// Get first node doing APTW matching EUDType and VA
SPLinkedCUD CTracker::GetNode_Pair_APTW_VA_AT(int64_t nVA, EUDType eUDType) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;

		// Check UD type
		if (spTarget->eUDType != eUDType) {
			spScan = spScan->spNext;
			continue;
		};

		// Check VA
		if (eUDType == EUD_TYPE_AR) {	
			if (spTarget->upData->cpAR->GetVA() != nVA) {
				spScan = spScan->spNext;
				continue;
			};
		}
		else if (eUDType == EUD_TYPE_AW){
			if (spTarget->upData->cpAR->GetVA() != nVA) {
				spScan = spScan->spNext;
				continue;
			};
		}
		else {	
			assert (0);
		};

		// Check PTW
		if (spTarget->eState == ECACHE_STATE_TYPE_FIRST_APTW       or 
            spTarget->eState == ECACHE_STATE_TYPE_SECOND_APTW      or 
            spTarget->eState == ECACHE_STATE_TYPE_SECOND_APTW_DONE or 
            spTarget->eState == ECACHE_STATE_TYPE_THIRD_APTW       or 
            spTarget->eState == ECACHE_STATE_TYPE_FOURTH_APTW      or 
            spTarget->eState == ECACHE_STATE_TYPE_FOURTH_APTW_DONE) { 

		    // spTarget->eState == ECACHE_STATE_TYPE_FIRST_PTW  or spTarget->eState == ECACHE_STATE_TYPE_SECOND_PTW or 
		    // spTarget->eState == ECACHE_STATE_TYPE_FIRST_RPTW or spTarget->eState == ECACHE_STATE_TYPE_SECOND_RPTW or spTarget->eState == ECACHE_STATE_TYPE_THIRD_RPTW ) {

		        return (spTarget);
		};
		spScan = spScan->spNext;
	};

	return (NULL); // No node match
};


// Get first node related to APTW 
// 	Assume one outstanding allowed APTW FIXME
SPLinkedCUD CTracker::GetNode_APTW_AT() {
     
     SPLinkedCUD spScan;
     SPLinkedCUD spTarget;
     spScan = this->spCUDList_head;
     while (spScan != NULL) {
     	spTarget = spScan;
     	if (spTarget->eState == ECACHE_STATE_TYPE_FIRST_APTW  or
     	    spTarget->eState == ECACHE_STATE_TYPE_SECOND_APTW or 
     	    spTarget->eState == ECACHE_STATE_TYPE_SECOND_APTW_DONE or 
     	    spTarget->eState == ECACHE_STATE_TYPE_THIRD_APTW or 
     	    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_APTW or 
     	    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_APTW_DONE ) {
     
     	        return (spTarget);
     	};
     	spScan = spScan->spNext;
     };
     
     return (NULL); // No node match 
};


// Get first node matching ID and UDType
// Update new state
EResultType CTracker::SetStateNode(int nID, EUDType eUDType, ECacheStateType eNewState) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->nID == nID and spTarget->eUDType == eUDType) {
			spTarget->eState = eNewState;
		        return (ERESULT_TYPE_SUCCESS);
		};
		spScan = spScan->spNext;
	};

	#ifdef DEBUG
	assert (0);
	#endif

	return (ERESULT_TYPE_FAIL); // No node match
};


// Get first node matching ID, UDType, state
// Update new state
EResultType CTracker::SetStateNode(int nID, EUDType eUDType, ECacheStateType eOldState, ECacheStateType eNewState) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->nID == nID and spTarget->eUDType == eUDType and spTarget->eState == eOldState) {
			spTarget->eState = eNewState;
		        return (ERESULT_TYPE_SUCCESS);
		};
		spScan = spScan->spNext;
	};

	#ifdef DEBUG
	assert (0);
	#endif

	return (ERESULT_TYPE_FAIL); // No node match
};


// Check any node matching ID and UDType
EResultType CTracker::IsIDMatch(int nID, EUDType eUDType) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->nID == nID and spTarget->eUDType == eUDType) {
		        return (ERESULT_TYPE_YES);
		};
		spScan = spScan->spNext;
	};

	#ifdef DEBUG
	assert (0);
	#endif

	return (ERESULT_TYPE_NO); // No node match
};


// Check any node matching ID and UDType is doing PTW
EResultType CTracker::IsIDMatchPTW(int nID, EUDType eUDType) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->nID == nID and spTarget->eUDType == eUDType) {

			if (spTarget->eState == ECACHE_STATE_TYPE_FIRST_PTW or 
				spTarget->eState == ECACHE_STATE_TYPE_SECOND_PTW or 
				spTarget->eState == ECACHE_STATE_TYPE_THIRD_PTW or 
				spTarget->eState == ECACHE_STATE_TYPE_FOURTH_PTW) {
			        return (ERESULT_TYPE_YES);
			};
		};
		spScan = spScan->spNext;
	};

	return (ERESULT_TYPE_NO); // No node match 
};


// Check any node doing line-fill 
EResultType CTracker::IsAnyLineFill() {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;

		if (spTarget->eState == ECACHE_STATE_TYPE_FILL) {
		        return (ERESULT_TYPE_YES);
		};
		spScan = spScan->spNext;
	};

	return (ERESULT_TYPE_NO); // No node match 
};


// Check any node matching ID and UDType is doing line-fill 
EResultType CTracker::IsSameID_LineFill(int nID, EUDType eUDType) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->nID == nID and spTarget->eUDType == eUDType) {

			if (spTarget->eState == ECACHE_STATE_TYPE_FILL) {
			        return (ERESULT_TYPE_YES);
			};
		};
		spScan = spScan->spNext;
	};

	return (ERESULT_TYPE_NO); // No node match 
};


// Check any node doing PTW
EResultType CTracker::IsAnyPTW() {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;

		if (spTarget->eState == ECACHE_STATE_TYPE_FIRST_PTW   or
		    spTarget->eState == ECACHE_STATE_TYPE_SECOND_PTW  or
		    spTarget->eState == ECACHE_STATE_TYPE_SECOND_PTW_DONE  or
		    spTarget->eState == ECACHE_STATE_TYPE_THIRD_PTW   or
		    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_PTW  or
		    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_PTW_DONE  or

		    spTarget->eState == ECACHE_STATE_TYPE_FIRST_APTW  or
		    spTarget->eState == ECACHE_STATE_TYPE_SECOND_APTW or
		    spTarget->eState == ECACHE_STATE_TYPE_SECOND_APTW_DONE or
		    spTarget->eState == ECACHE_STATE_TYPE_THIRD_APTW  or
		    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_APTW or
		    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_APTW_DONE ) {

		    return (ERESULT_TYPE_YES);
		};
		spScan = spScan->spNext;
	};

	return (ERESULT_TYPE_NO); // No node match 
};


// Check any node matching UDType is doing PTW
EResultType CTracker::IsUDMatchPTW(EUDType eUDType) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		if (spTarget->eUDType == eUDType) {

			if (spTarget->eState == ECACHE_STATE_TYPE_FIRST_PTW or 
				spTarget->eState == ECACHE_STATE_TYPE_SECOND_PTW or 
				spTarget->eState == ECACHE_STATE_TYPE_THIRD_PTW or 
				spTarget->eState == ECACHE_STATE_TYPE_FOURTH_PTW) {
			        return (ERESULT_TYPE_YES);
			};
		};
		spScan = spScan->spNext;
	};

	return (ERESULT_TYPE_NO); // No node match 
};


// Check any node matching address
// EResultType CTracker::IsAddrMatch(int64_t nAddr) {		// Be careful FIXME Should be VPN. 
//	
//	SPLinkedCUD spScan;
//	SPLinkedCUD spTarget;
//	spScan = this->spCUDList_head;
//	while (spScan != NULL) {
//		spTarget = spScan;
//	
//		if (spTarget->eUDType == EUD_TYPE_AR) {
//			if (spTarget->upData->cpAR->GetAddr() == nAddr) {
//		        	return (ERESULT_TYPE_YES);
//			};
//		} 
//		else if (spTarget->eUDType == EUD_TYPE_AW) {
//			if (spTarget->upData->cpAW->GetAddr() == nAddr) {
//		        	return (ERESULT_TYPE_YES);
//			};
//		} 
//		else {
//			assert (0);
//		};
//		spScan = spScan->spNext;
//	};
//	
//	return (ERESULT_TYPE_NO); // No node match
// };


// Check any node doing PTW matching VPN 
// 	Page size 4kB
EResultType CTracker::IsAnyPTW_Match_VPN(int64_t nVA) {

	int64_t nVPN = nVA >> BIT_4K_PAGE;

	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;

		if (spTarget->eUDType == EUD_TYPE_AR) {
			// Check any PTW
			if (spTarget->eState == ECACHE_STATE_TYPE_FIRST_PTW   or
			    spTarget->eState == ECACHE_STATE_TYPE_SECOND_PTW  or
			    spTarget->eState == ECACHE_STATE_TYPE_THIRD_PTW   or
			    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_PTW  or
			    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_PTW_DONE  or

			    spTarget->eState == ECACHE_STATE_TYPE_FIRST_PF    or
			    spTarget->eState == ECACHE_STATE_TYPE_SECOND_PF   or 

			    spTarget->eState == ECACHE_STATE_TYPE_FIRST_APTW  or
			    spTarget->eState == ECACHE_STATE_TYPE_SECOND_APTW or 
			    spTarget->eState == ECACHE_STATE_TYPE_THIRD_APTW  or 
			    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_APTW or 
			    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_APTW_DONE or 

			    spTarget->eState == ECACHE_STATE_TYPE_FIRST_RPTW  or
			    spTarget->eState == ECACHE_STATE_TYPE_SECOND_RPTW or 
			    spTarget->eState == ECACHE_STATE_TYPE_THIRD_RPTW ) {
		
				// Check VPN match	
				// int64_t nTargetVPN = spTarget->upData->cpAR->GetAddr() >> BIT_4K_PAGE;	// FIXME Can ba PA
				int64_t nTargetVPN = spTarget->upData->cpAR->GetVA() >> BIT_4K_PAGE;		// Be careful

				int64_t nTargetVPN_start = nTargetVPN - (nTargetVPN % NUM_PTE_PTW);		// Be careful
				int64_t nTargetVPN_end   = nTargetVPN_start + NUM_PTE_PTW - 1;

				if (nVPN >= nTargetVPN_start and nVPN <= nTargetVPN_end) {			// Be careful
					return (ERESULT_TYPE_YES);
				};
			};
		} 
		else if (spTarget->eUDType == EUD_TYPE_AW) {
			// Check any PTW
			if (spTarget->eState == ECACHE_STATE_TYPE_FIRST_PTW   or
			    spTarget->eState == ECACHE_STATE_TYPE_SECOND_PTW  or
			    spTarget->eState == ECACHE_STATE_TYPE_THIRD_PTW   or
			    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_PTW  or
			    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_PTW_DONE  or

			    spTarget->eState == ECACHE_STATE_TYPE_FIRST_PF    or
			    spTarget->eState == ECACHE_STATE_TYPE_SECOND_PF   or 

			    spTarget->eState == ECACHE_STATE_TYPE_FIRST_APTW  or
			    spTarget->eState == ECACHE_STATE_TYPE_SECOND_APTW or 
			    spTarget->eState == ECACHE_STATE_TYPE_THIRD_APTW  or 
			    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_APTW or 
			    spTarget->eState == ECACHE_STATE_TYPE_FOURTH_APTW_DONE or 

			    spTarget->eState == ECACHE_STATE_TYPE_FIRST_RPTW  or
			    spTarget->eState == ECACHE_STATE_TYPE_SECOND_RPTW or 
			    spTarget->eState == ECACHE_STATE_TYPE_THIRD_RPTW ) {
		
				// Check VPN match	
				// int64_t nTargetVPN = spTarget->upData->cpAW->GetAddr() >> BIT_4K_PAGE;	// FIXME Can be PA
				int64_t nTargetVPN = spTarget->upData->cpAW->GetVA() >> BIT_4K_PAGE;		// Be careful

				int64_t nTargetVPN_start = nTargetVPN - (nTargetVPN % NUM_PTE_PTW);		// Be careful
				int64_t nTargetVPN_end   = nTargetVPN_start + NUM_PTE_PTW - 1;

				if (nVPN >= nTargetVPN_start and nVPN <= nTargetVPN_end) {			// Be careful
					return (ERESULT_TYPE_YES);
				};
			};
		} 
		else {
			assert (0);
		};
		spScan = spScan->spNext;
	};

	return (ERESULT_TYPE_NO); // No node match
};


// Check any node matching cache-line 
EResultType CTracker::IsAddrMatch_Cache(int64_t nAddr) {
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	int64_t nAddrTarget = -1;
	while (spScan != NULL) {
		spTarget = spScan;

		if (spTarget->eUDType == EUD_TYPE_AR) {
			nAddrTarget = spTarget->upData->cpAR->GetAddr();
			if ((nAddrTarget >> CACHE_BIT_LINE) == (nAddr >> CACHE_BIT_LINE)) {
		        	return (ERESULT_TYPE_YES);
			};
		} 
		else if (spTarget->eUDType == EUD_TYPE_AW) {
			nAddrTarget = spTarget->upData->cpAW->GetAddr();
			if ((nAddrTarget >> CACHE_BIT_LINE) == (nAddr >> CACHE_BIT_LINE)) {
		        	return (ERESULT_TYPE_YES);
			};
		} 
		else {
			assert (0);
		};
		spScan = spScan->spNext;
	};

	return (ERESULT_TYPE_NO); // No node match
};


// Check any node doing RPTW, matching address. RMM
EResultType CTracker::IsRPTW_sameVPN_Match_RMM(int64_t nAddr) {		// VPN aligned
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;

		if (spTarget->eState == ECACHE_STATE_TYPE_FIRST_RPTW or spTarget->eState == ECACHE_STATE_TYPE_SECOND_RPTW or spTarget->eState == ECACHE_STATE_TYPE_THIRD_RPTW) {
			if (spTarget->upData->cpAR->GetAddr() == nAddr) {

				#ifdef DEBUG
				assert (spTarget->eUDType == EUD_TYPE_AR);
				#endif

				return (ERESULT_TYPE_YES);
			};
		};
		spScan = spScan->spNext;
	};

	return (ERESULT_TYPE_NO); // No node match
};


// Debug
EResultType CTracker::CheckTracker() {

	// Debug
	assert (this->nCurNum >= 0);
	assert (this->nCurNum <= this->nMaxNum);
	
	// Check NULL 
	if (this->nCurNum == 0) {        
		assert (this->spCUDList_head == NULL);
		assert (this->spCUDList_tail == NULL);
		// assert (this->spCUDList_head->spPrev == NULL);
		// assert (this->spCUDList_head->spNext == NULL);
		// assert (this->spCUDList_tail->spPrev == NULL);
		// assert (this->spCUDList_tail->spNext == NULL);
		assert (this->IsEmpty() == ERESULT_TYPE_YES);
		assert (this->GetCurNum() == 0); 
	} 
	else {
		assert (this->spCUDList_head != NULL);
		assert (this->spCUDList_tail != NULL);
		// assert (this->spCUDList_head->spPrev == NULL);
		// assert (this->spCUDList_tail->spNext == NULL);
	};
	
	// Check occupancy 
	int nCurOccupancy = 0;
	SPLinkedCUD spScan;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
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
EResultType CTracker::UpdateState() {

	// Debug
	// this->CheckTracker();

	#ifdef STAT_DETAIL 
	// Update entry waiting cycle since allocation 
	int nMaxCycleWait = -1;
	SPLinkedCUD spScan;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		// Debug
		assert (spScan->nCycle_wait >= 0);
		
		spScan->nCycle_wait++;

		// Get max waiting cycles
		if (nMaxCycleWait < spScan->nCycle_wait) {
			nMaxCycleWait = spScan->nCycle_wait;
		};
		
		spScan = spScan->spNext;
	};
	
	this->nMaxCycleWait = nMaxCycleWait;
	#endif
	
	return (ERESULT_TYPE_SUCCESS);
};


string Convert_eCacheState2string(ECacheStateType eType) {

        switch(eType) {
                case ECACHE_STATE_TYPE_HIT:
                        return("ECACHE_STATE_TYPE_HIT");
                        break;
                case ECACHE_STATE_TYPE_PASS:
                        return("ECACHE_STATE_TYPE_PASS");
                        break;
                case ECACHE_STATE_TYPE_STALL:
                        return("ECACHE_STATE_TYPE_STALL");
                        break; 

                case ECACHE_STATE_TYPE_FILL:
                        return("ECACHE_STATE_TYPE_FILL");
                        break;
                case ECACHE_STATE_TYPE_EVICT:
                        return("ECACHE_STATE_TYPE_EVICT");
                        break;

                case ECACHE_STATE_TYPE_FIRST_PTW:
                        return("ECACHE_STATE_TYPE_FIRST_PTW");
                        break;
                case ECACHE_STATE_TYPE_SECOND_PTW:
                        return("ECACHE_STATE_TYPE_SECOND_PTW");
                        break;
                case ECACHE_STATE_TYPE_SECOND_PTW_DONE:
                        return("ECACHE_STATE_TYPE_SECOND_PTW_DONE");
                        break;
                case ECACHE_STATE_TYPE_THIRD_PTW:
                        return("ECACHE_STATE_TYPE_THIRD_PTW");
                        break;
                case ECACHE_STATE_TYPE_FOURTH_PTW:
                        return("ECACHE_STATE_TYPE_FOURTH_PTW");
                        break;
                case ECACHE_STATE_TYPE_FOURTH_PTW_DONE:
                        return("ECACHE_STATE_TYPE_FOURTH_PTW_DONE");
                        break;

                case ECACHE_STATE_TYPE_PTW_HIT:
                        return("ECACHE_STATE_TYPE_PTW_HIT");
                        break;

                case ECACHE_STATE_TYPE_FIRST_PF:
                        return("ECACHE_STATE_TYPE_FIRST_PF");
                        break;
                case ECACHE_STATE_TYPE_SECOND_PF:
                        return("ECACHE_STATE_TYPE_SECOND_PF");
                        break;
                case ECACHE_STATE_TYPE_THIRD_PF:
                        return("ECACHE_STATE_TYPE_THIRD_PF");
                        break;

                case ECACHE_STATE_TYPE_FIRST_APTW:
                        return("ECACHE_STATE_TYPE_FIRST_APTW");
                        break;
                case ECACHE_STATE_TYPE_SECOND_APTW:
                        return("ECACHE_STATE_TYPE_SECOND_APTW");
                        break;
                case ECACHE_STATE_TYPE_SECOND_APTW_DONE:
                        return("ECACHE_STATE_TYPE_SECOND_APTW_DONE");
                        break;
                case ECACHE_STATE_TYPE_THIRD_APTW:
                        return("ECACHE_STATE_TYPE_THIRD_APTW");
                        break;
                case ECACHE_STATE_TYPE_FOURTH_APTW:
                        return("ECACHE_STATE_TYPE_FOURTH_APTW");
                        break;
                case ECACHE_STATE_TYPE_FOURTH_APTW_DONE:
                        return("ECACHE_STATE_TYPE_FOURTH_APTW_DONE");
                        break;

                case ECACHE_STATE_TYPE_FIRST_RPTW:
                        return("ECACHE_STATE_TYPE_FIRST_RPTW");
                        break;
                case ECACHE_STATE_TYPE_SECOND_RPTW:
                        return("ECACHE_STATE_TYPE_SECOND_RPTW");
                        break;
                case ECACHE_STATE_TYPE_THIRD_RPTW:
                        return("ECACHE_STATE_TYPE_THIRD_RPTW");
                        break;

                case ECACHE_STATE_TYPE_UNDEFINED:
                        return("ECACHE_STATE_TYPE_UNDEFINED");

                default:
                        break;
        };
        return (NULL);
};


// Debug
EResultType CTracker::Display() {

	string cState;
	
	printf("---------------------------------------------\n");
	printf("\t Tracker queue display\n");
	printf("---------------------------------------------\n");
	printf("\t Name         : \t %s\n", this->cName.c_str());
	printf("\t nCurNum      : \t %d\n", this->nCurNum);
	printf("\t nMaxNum      : \t %d\n", this->nMaxNum);
	printf("\t nMaxCycleWait: \t %d\n", this->nMaxCycleWait);
	// printf("---------------------------------------------\n");
	
	SPLinkedCUD spScan;
	SPLinkedCUD spTarget;
	spScan = this->spCUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		spScan   = spScan->spNext;
		Display_UD(spTarget->upData, spTarget->eUDType);	
		printf("\t Waiting cycle : \t %d\n", spTarget->nCycle_wait);
		cState = Convert_eCacheState2string(spTarget->eState);
		printf("\t State         : \t %s\n", cState.c_str());
		printf("---------------------------------------------\n");
	};
	
	return (ERESULT_TYPE_SUCCESS);
};

