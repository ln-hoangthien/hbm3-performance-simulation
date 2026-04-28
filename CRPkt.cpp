//-----------------------------------------------------------
// FileName	: CRPkt.cpp
// Version	: 0.80
// Date		: 1 Nov 2021
// Contact	: JaeYoung.Hur@gmail.com
// Description	: R Pkt class definition
//-----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <string>

#include "CRPkt.h"

// Construct
CRPkt::CRPkt(string cName) {

	// Generate. Initialize 
        this->spPkt = new SRPkt;
        this->spPkt->nID   = -1;
        this->spPkt->nData = -1;
        this->spPkt->nLast = -1;
		#ifdef CCI_ON
			this->spPkt->nResp = 0;
		#endif

	this->cName = cName;
	this->eFinalTrans = ERESULT_TYPE_NO;
	this->nMemCh = -1;
	this->nCacheCh = -1;
};


// Construct
CRPkt::CRPkt() {

	// Generate. Initialize 
        this->spPkt = new SRPkt;
        this->spPkt->nID   = -1;
        this->spPkt->nData = -1;
        this->spPkt->nLast = -1;
		#ifdef CCI_ON
			this->spPkt->nResp = 0;
		#endif

	this->cName = "R_pkt";
	this->eFinalTrans = ERESULT_TYPE_NO;
	this->nMemCh = -1;
	this->nCacheCh = -1;
};


// Destruct
CRPkt::~CRPkt() {

	this->CheckPkt();

	delete (this->spPkt);	
	this->spPkt = NULL;
};


//// Set R pkt 
//EResultType CRPkt::SetPkt(SPRPkt spPkt_new) {
//
//	// Debug
//	assert (spPkt_new != NULL);
//	
//	this->spPkt = spPkt_new;  // spPkt_new generated outside
//	return (ERESULT_TYPE_SUCCESS);
//};

EResultType CRPkt::SetPkt(int nID, int nData, int nLast) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nID   = nID;
	this->spPkt->nData = nData;
	this->spPkt->nLast = nLast;
	return (ERESULT_TYPE_SUCCESS);
};

#ifdef CCI_ON
// Set R pkt
EResultType CRPkt::SetPkt(int nID, int nData, int nLast, int nResp) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nID   = nID;
	this->spPkt->nData = nData;
	this->spPkt->nLast = nLast;
	this->spPkt->nResp = nResp;

	return (ERESULT_TYPE_SUCCESS);
};
#endif

// Set name
EResultType CRPkt::SetName(string cName) {

	this->cName = cName;
	return (ERESULT_TYPE_SUCCESS);
};


// Set R id 
EResultType CRPkt::SetID(int nID) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nID = nID;
	return (ERESULT_TYPE_SUCCESS);
};


// Set R data 
EResultType CRPkt::SetData(int nData) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nData = nData;
	return (ERESULT_TYPE_SUCCESS);
};


// Set R last
EResultType CRPkt::SetLast(EResultType eLast) {

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

#ifdef CCI_ON
// Set R response
EResultType CRPkt::SetResp(int nResp) { 

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nResp = nResp;
	return (ERESULT_TYPE_SUCCESS);
};
#endif

// Set final trans 
EResultType CRPkt::SetFinalTrans(EResultType eResult) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif
	
	this->eFinalTrans = eResult;	
	return (ERESULT_TYPE_SUCCESS);
};


// Set mem ch num 
EResultType CRPkt::SetMemCh(int nMemCh) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif
	
	this->nMemCh = nMemCh;	
	return (ERESULT_TYPE_SUCCESS);
};


// Set cache ch num 
EResultType CRPkt::SetCacheCh(int nCacheCh) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif
	
	this->nCacheCh = nCacheCh;	
	return (ERESULT_TYPE_SUCCESS);
};


// Get R pkt name
string CRPkt::GetName() {

	// this->CheckPkt();
	return (this->cName);
};


// Get R pkt
SPRPkt CRPkt::GetPkt() {

	// this->CheckPkt();
	return (this->spPkt);
};


// Get ID
int CRPkt::GetID() {

	// this->CheckPkt();
	return (this->spPkt->nID);
};


// Get data 
int CRPkt::GetData() {

	// this->CheckPkt();
	return (this->spPkt->nData);
};


// Check R last
EResultType CRPkt::IsLast() {

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
EResultType CRPkt::IsFinalTrans() {

	// this->CheckPkt();
	return (this->eFinalTrans);
};


// Get mem ch num 
int CRPkt::GetMemCh() {

	// this->CheckPkt();
	return (this->nMemCh);
};


// Get cache ch num 
int CRPkt::GetCacheCh() {

	// this->CheckPkt();
	return (this->nCacheCh);
};

#ifdef CCI_ON
// Get cache Response 
int CRPkt::GetResp() {

	// this->CheckPkt();
	return (this->spPkt->nResp);
};
#endif


EResultType CRPkt::CheckPkt() {

	// Debug
	assert (this != NULL);
	assert (this->spPkt != NULL);
	assert (this->spPkt->nLast == 1 or this->spPkt->nLast == 0);
	assert (this->spPkt->nID   >= 0);
	assert (this->spPkt->nID   <  1000);
	assert (this->spPkt->nData >= 0);
	// assert (this->spPkt->nData <  16);
	
	return (ERESULT_TYPE_SUCCESS);
};


// Debug
EResultType CRPkt::Display() {

	// this->CheckPkt();

	string cFinalTrans = Convert_eResult2string(this->eFinalTrans);

	// printf("---------------------------------------------\n");
	// printf("\t R pkt display\n");
	printf("---------------------------------------------\n");
	printf("\t Name		: \t %s\n",   this->cName.c_str());
	printf("\t ID		: \t 0x%x\n", this->spPkt->nID);
	printf("\t Data		: \t 0x%x\n", this->spPkt->nData);
	printf("\t FinalTrans	: \t %s\n",   cFinalTrans.c_str());
	printf("\t MemCh		: \t %d\n",   this->nMemCh);
	printf("\t CacheCh	: \t %d\n",   this->nCacheCh);
	printf("\t Last		: \t 0x%x\n", this->spPkt->nLast);
	printf("---------------------------------------------\n");

	return (ERESULT_TYPE_SUCCESS);
};

