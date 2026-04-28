//-----------------------------------------------------------
// Filename     : CFIFO.cpp 
// Version	: 0.78
// Date         : 24 Feb 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: FIFO definition
//-----------------------------------------------------------
// Ver 0.79	: Add spPrev (only needed in queue) FIXME
//-----------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>

#include "CFIFO.h"


// Constructor
CFIFO::CFIFO(string cName, EUDType eUDType, int nMaxNum) {

	// Generate a node later when push
	this->spUDList_head = NULL;	
	this->spUDList_tail = NULL;	

	// Initialize	
        this->cName	= cName;
        this->eUDType   = eUDType;
	this->nMaxNum   = nMaxNum;
	this->nCurNum	= -1;
};


// Destructor
CFIFO::~CFIFO() {

	// Debug
	#ifndef BACKGROUND_TRAFFIC_ON 
	assert (this->GetCurNum() == 0);	// RMM, background RPTW maybe on-going
	assert (this->spUDList_head == NULL);
	assert (this->spUDList_tail == NULL);
	#endif

	// Delete all nodes
	SPLinkedUD spScan;
	SPLinkedUD spTarget;
	spScan = this->spUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		spScan   = spScan->spNext;
		Delete_UD(spTarget->upData, this->eUDType);
		spTarget->upData = NULL;
		spTarget = NULL;
		// spTarget->spNext = NULL;
	};

	// Debug
	// assert (this->spUDList_head == NULL);
	// assert (this->spUDList_tail == NULL);
};


// Initialize
EResultType CFIFO::Reset() {

	// Delete all nodes 
	SPLinkedUD spScan;
	SPLinkedUD spTarget;
	spScan = this->spUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		spScan   = spScan->spNext;
		Delete_UD(spTarget->upData, this->eUDType);
		delete (spTarget->upData);
		spTarget->upData = NULL;
	};
	
	// Initialize 
	this->spUDList_head = NULL;
	this->spUDList_tail = NULL;
	this->nCurNum   = 0;
	
	return (ERESULT_TYPE_SUCCESS);
};


// Push
EResultType CFIFO::Push(UPUD upThis, int nLatency) {

	#ifdef DEBUG
	assert (upThis != NULL);
	assert (nLatency >= 0);
	assert (this->GetCurNum() < this->GetMaxNum());	
	#endif

	// Generate node. Initialize
	UPUD upUD_new = Copy_UD(upThis, this->eUDType); // Generate new up and cp
	SPLinkedUD spLinkedUD_new = new SLinkedUD;
	spLinkedUD_new->upData = upUD_new;
	// spLinkedUD_new->spPrev = NULL;		// FIXME not used in FIFO
	spLinkedUD_new->spNext = NULL;
	spLinkedUD_new->nLatency = nLatency;

	// Push
	if (this->spUDList_head == NULL) {

		#ifdef DEBUG
		assert (this->spUDList_tail == NULL);
		#endif

		this->spUDList_head = spLinkedUD_new;
		this->spUDList_tail = spLinkedUD_new;
	} 
	else {

		#ifdef DEBUG
		assert (this->GetCurNum() > 0);	
		assert (this->spUDList_tail != NULL);
		#endif

		this->spUDList_tail->spNext = spLinkedUD_new;
		this->spUDList_tail = spLinkedUD_new;
	};

	// Increment occupancy
	this->nCurNum++;

	#ifdef DEBUG
	assert (upUD_new != NULL);
	// this->CheckFIFO();
	#endif

	return (ERESULT_TYPE_SUCCESS);
};


// Push in zero cycle 
EResultType CFIFO::Push(UPUD upThis) {

	#ifdef DEBUG
	assert (upThis != NULL);
	#endif

	// Push
	this->Push(upThis, 0);

	// Debug
	// this->CheckFIFO();

	return (ERESULT_TYPE_SUCCESS);
};


// Pop
UPUD CFIFO::Pop() {

	// Debug
	// this->CheckFIFO();

	// Get head
	SPLinkedUD spTarget = this->spUDList_head;

	UPUD upTarget = this->spUDList_head->upData;

	// Check latency
	if (spTarget->nLatency > 0) { // Pop only when entry latency 0
		return (NULL);
	};

	#ifdef DEBUG
	assert (spTarget->nLatency == 0);
	#endif

	// Decrement occupancy
	this->nCurNum--;

	// Pop head
	if (this->nCurNum == 0) {
		this->spUDList_head = NULL;
		this->spUDList_tail = NULL;
	} 
	else {
		this->spUDList_head = this->spUDList_head->spNext;
	};

	// Maintain
	delete (spTarget);
	spTarget = NULL;

	#ifdef DEBUG
	assert (upTarget != NULL);
	// this->CheckFIFO();
	#endif

	return (upTarget); // Delete outside	
};


// Get UD type
EUDType CFIFO::GetUDType() {

	return (this->eUDType);
};


// Get current occupancy
int CFIFO::GetCurNum() {

	return (this->nCurNum);
};


// Get size
int CFIFO::GetMaxNum() {

	return (this->nMaxNum);
};


// Get head upData 
UPUD CFIFO::GetTop() {

	// Debug
	// this->CheckFIFO();

	if (this->spUDList_head == NULL) {

		#ifdef DEBUG
		assert (this->GetCurNum() == 0);
		#endif

		return (NULL);
	};

	return (this->spUDList_head->upData);
};


// Check FIFO empty
EResultType CFIFO::IsEmpty() {

	if (this->nCurNum == 0) {

		#ifdef DEBUG
		assert (this->spUDList_head == NULL);
		assert (this->spUDList_tail == NULL);
		#endif

		return (ERESULT_TYPE_YES);	
	};

	#ifdef DEBUG
	assert (this->spUDList_head != NULL);
	assert (this->spUDList_tail != NULL);
	#endif

	return (ERESULT_TYPE_NO);
};


// Check FIFO full 
EResultType CFIFO::IsFull() {

	if (this->nCurNum == this->nMaxNum) {
		return (ERESULT_TYPE_YES);	
	};

	return (ERESULT_TYPE_NO);
};


// Update state 
EResultType CFIFO::UpdateState() {

	// Debug
	// this->CheckFIFO();
	
	// Update entry wating time since allocation 
	SPLinkedUD spScan = this->spUDList_head;
	while (spScan != NULL) {

		#ifdef DEBUG
		assert (spScan->upData != NULL);
		assert (spScan->nLatency >= 0);
		#endif

		if (spScan->nLatency > 0) {
			spScan->nLatency--;
		};
		
		spScan = spScan->spNext;
	};
	
	return (ERESULT_TYPE_SUCCESS);
};


// Debug
EResultType CFIFO::CheckFIFO() {

	// Debug
	assert (this->nCurNum >= 0);
	assert (this->nCurNum <= this->nMaxNum);
	
	// Check NULL 
	if (this->nCurNum == 0) {        
		assert (this->spUDList_head == NULL);
		assert (this->spUDList_tail == NULL);
		assert (this->IsEmpty() == ERESULT_TYPE_YES);
		assert (this->GetCurNum() == 0); 
	} 
	else {
		assert (this->spUDList_head != NULL);
		assert (this->spUDList_tail != NULL);
		assert (this->IsEmpty() == ERESULT_TYPE_NO);
		assert (this->GetCurNum() > 0); 
	};
	
	// Check occupancy 
	int nCurOccupancy = 0;
	SPLinkedUD spScan = this->spUDList_head;
	while (spScan != NULL) {
		spScan = spScan->spNext;
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


// Debug
EResultType CFIFO::Display() {

	string cUDType = Convert_eUDType2string(this->eUDType);
	
	printf("---------------------------------------------\n");
	printf("\t FIFO display\n");
	printf("---------------------------------------------\n");
	printf("\t Name    : \t %s\n", this->cName.c_str());
	printf("\t eUDType : \t %s\n", cUDType.c_str());
	printf("\t nCurNum : \t %d\n", this->nCurNum);
	printf("\t nMaxNum : \t %d\n", this->nMaxNum);
	printf("---------------------------------------------\n");
	
	SPLinkedUD spScan;
	SPLinkedUD spTarget;
	spScan = this->spUDList_head;
	while (spScan != NULL) {
		spTarget = spScan;
		spScan   = spScan->spNext;
		Display_UD(spTarget->upData, this->eUDType);	
		printf("\t nLatency: \t %d\n", spTarget->nLatency);
	};
	
	return (ERESULT_TYPE_SUCCESS);
};

