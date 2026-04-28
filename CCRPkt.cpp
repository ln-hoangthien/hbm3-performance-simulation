//-----------------------------------------------------------
// FileName	: CCRPkt.cpp
// Version	: 0.79
// Date		: 26 Feb 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: B Pkt class definition
//-----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <string>

#include "CCRPkt.h"

// Construct
CCRPkt::CCRPkt(string cName) {

	// Generate and initialize 
	this->spPkt = new SCRPkt;
	this->spPkt->nResp = 0;
	this->cName = cName;
};


// Construct
CCRPkt::CCRPkt() {

	// Generate and initialize 
	this->spPkt = new SCRPkt;
	this->spPkt->nResp = 0;
	this->cName = "CR_pkt";
};


// Destruct
CCRPkt::~CCRPkt() {

	this->CheckPkt();

	delete (this->spPkt);	
	this->spPkt = NULL;
};


// Set name
EResultType CCRPkt::SetName(string cName) {

	this->cName = cName;
	return (ERESULT_TYPE_SUCCESS);
};


//// Set CR pkt 
//EResultType CCRPkt::SetPkt(SPCRPkt spPkt) {
//
//	// Debug
//	assert (this->spPkt != NULL);
//	
//	this->spPkt = spPkt;  // spPkt generated when CCRPkt generated
//	return (ERESULT_TYPE_SUCCESS);
//};

// Set CR response 
EResultType CCRPkt::SetResp(int nResp) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nResp = nResp;
	return (ERESULT_TYPE_SUCCESS);
};

// Set final trans
EResultType CCRPkt::SetFinalTrans(EResultType eResult) {

	#ifdef DEBUG	
	assert (this->spPkt != NULL);
	#endif
	
	this->eFinalTrans = eResult;
	return (ERESULT_TYPE_SUCCESS);
};

// Get B pkt
SPCRPkt CCRPkt::GetPkt() {

	// this->CheckPkt();
	return (this->spPkt);
};


// Get B pkt name
string CCRPkt::GetName() {

	// this->CheckPkt();
	return (this->cName);
};

// Get cache Response 
int CCRPkt::GetResp() {

	// this->CheckPkt();
	return (this->spPkt->nResp);
};

// Check final trans
EResultType CCRPkt::IsFinalTrans() {

	// this->CheckPkt();
	return (this->eFinalTrans);
};



// Debug
EResultType CCRPkt::CheckPkt() {

	assert (this != NULL);
	assert (this->spPkt != NULL);
	
	return (ERESULT_TYPE_SUCCESS);
};


// Debug
EResultType CCRPkt::Display() {

	// this->CheckPkt();

	string cFinalTrans = Convert_eResult2string(this->eFinalTrans);

	// printf("---------------------------------------------\n");
	// printf("\t B pkt display\n");
	printf("---------------------------------------------\n");
	printf("\t Name		: \t %s\n",   this->cName.c_str());
	printf("\t Response	: \t 0x%x\n", this->spPkt->nResp);
	printf("\t FinalTrans	: \t %s\n",   cFinalTrans.c_str());
	printf("---------------------------------------------\n");

	return (ERESULT_TYPE_SUCCESS);
};

