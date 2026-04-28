//-----------------------------------------------------------
// FileName	: CCDPkt.cpp
// Version	: 0.79
// Date		: 26 Feb 2020
// Contact	: JaeYoung.Hur@gmail.com
// DesCDiption	: B Pkt class definition
//-----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <string>

#include "CCDPkt.h"

// Construct
CCDPkt::CCDPkt(string cName) {

	// Generate and initialize 
	this->spPkt = new SCDPkt;
	this->spPkt->nData = 0;
	this->spPkt->nLast = 0;
	this->cName = cName;
};


// Construct
CCDPkt::CCDPkt() {

	// Generate and initialize 
	this->spPkt = new SCDPkt;
	this->spPkt->nData = 0;
	this->spPkt->nLast = 0;
	this->cName = "CD_pkt";
};


// Destruct
CCDPkt::~CCDPkt() {

	this->CheckPkt();

	delete (this->spPkt);	
	this->spPkt = NULL;
};


// Set name
EResultType CCDPkt::SetName(string cName) {

	this->cName = cName;
	return (ERESULT_TYPE_SUCCESS);
};


//// Set CD pkt 
//EResultType CCDPkt::SetPkt(SPCDPkt spPkt) {
//
//	// Debug
//	assert (this->spPkt != NULL);
//	
//	this->spPkt = spPkt;  // spPkt generated when CCDPkt generated
//	return (ERESULT_TYPE_SUCCESS);
//};

// Set R data 
EResultType CCDPkt::SetData(int nData) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nData = nData;
	return (ERESULT_TYPE_SUCCESS);
};


// Set R last
EResultType CCDPkt::SetLast(EResultType eLast) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif
	
	if (eLast == ERESULT_TYPE_YES) {
	        this->spPkt->nLast = 1;
	} 
	else if (eLast == ERESULT_TYPE_NO) {
	        this->spPkt->nLast = 0;
	} 
	else {
	        assert (0);
	};
	return (ERESULT_TYPE_SUCCESS);
};

// Set final trans
EResultType CCDPkt::SetFinalTrans(EResultType eResult) {

	#ifdef DEBUG	
	assert (this->spPkt != NULL);
	#endif
	
	this->eFinalTrans = eResult;
	return (ERESULT_TYPE_SUCCESS);
};

// Get B pkt
SPCDPkt CCDPkt::GetPkt() {

	// this->CheckPkt();
	return (this->spPkt);
};


// Get B pkt name
string CCDPkt::GetName() {

	// this->CheckPkt();
	return (this->cName);
};

// Get data 
int CCDPkt::GetData() {

	// this->CheckPkt();
	return (this->spPkt->nData);
};


// Check R last
EResultType CCDPkt::IsLast() {

	// this->CheckPkt();

	if (this->spPkt->nLast == 1) {
		return (ERESULT_TYPE_YES);
	} 
	else if (this->spPkt->nLast == 0) {
		return (ERESULT_TYPE_NO);
	} 
	else {
		assert (0);
	};

	return (ERESULT_TYPE_NO);
};

// Check final trans
EResultType CCDPkt::IsFinalTrans() {

	// this->CheckPkt();
	return (this->eFinalTrans);
};

// Debug
EResultType CCDPkt::CheckPkt() {

	assert (this != NULL);
	assert (this->spPkt != NULL);
	
	return (ERESULT_TYPE_SUCCESS);
};


// Debug
EResultType CCDPkt::Display() {

	// this->CheckPkt();

	string cFinalTrans = Convert_eResult2string(this->eFinalTrans);

	// printf("---------------------------------------------\n");
	// printf("\t B pkt display\n");
	printf("---------------------------------------------\n");
	printf("\t Name		: \t %s\n",   this->cName.c_str());
	printf("\t FinalTrans	: \t %s\n",   cFinalTrans.c_str());
	printf("---------------------------------------------\n");

	return (ERESULT_TYPE_SUCCESS);
};

