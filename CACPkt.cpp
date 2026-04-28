//-----------------------------------------------------------
// FileName	: CACPkt.cpp
// Version	: 0.79
// Date		: 26 Feb 2020
// Contact	: JaeYoung.Hur@gmail.com
// DesACiption	: B Pkt class definition
//-----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <string>

#include "CACPkt.h"

// Construct
CACPkt::CACPkt(string cName) {

	// Generate and initialize 
	this->spPkt = new SACPkt;
	this->spPkt->nAddr = -1;
	this->spPkt->nSnoop = -1;
	this->cName = cName;
};


// Construct
CACPkt::CACPkt() {

	// Generate and initialize 
	this->spPkt = new SACPkt;
	this->spPkt->nAddr = -1;
	this->spPkt->nSnoop = -1;
	this->cName = "AC_pkt";
};


// Destruct
CACPkt::~CACPkt() {

	this->CheckPkt();

	delete (this->spPkt);	
	this->spPkt = NULL;
};


// Set name
EResultType CACPkt::SetName(string cName) {

	this->cName = cName;
	return (ERESULT_TYPE_SUCCESS);
};

// Get Address
int64_t CACPkt::GetAddr() {

	// this->CheckPkt();
	return (this->spPkt->nAddr);
};

EResultType CACPkt::SetSnoop(int nSnoop) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nSnoop  = nSnoop;
	return (ERESULT_TYPE_SUCCESS);
};

// Set Ax Addr 
EResultType CACPkt::SetAddr(int64_t nAddr) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nAddr = nAddr;
	return (ERESULT_TYPE_SUCCESS);
};

// Set final trans
EResultType CACPkt::SetFinalTrans(EResultType eResult) {

	#ifdef DEBUG	
	assert (this->spPkt != NULL);
	#endif
	
	this->eFinalTrans = eResult;
	return (ERESULT_TYPE_SUCCESS);
};

// Get B pkt
SPACPkt CACPkt::GetPkt() {

	// this->CheckPkt();
	return (this->spPkt);
};


// Get B pkt name
string CACPkt::GetName() {

	// this->CheckPkt();
	return (this->cName);
};

// Get snoop
int CACPkt::GetSnoop() {
	// this->CheckPkt();
	return (this->spPkt->nSnoop);
};

// Check final trans
EResultType CACPkt::IsFinalTrans() {

	// this->CheckPkt();
	return (this->eFinalTrans);
};



// Debug
EResultType CACPkt::CheckPkt() {

	assert (this != NULL);
	assert (this->spPkt != NULL);
	
	return (ERESULT_TYPE_SUCCESS);
};


// Debug
EResultType CACPkt::Display() {

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

