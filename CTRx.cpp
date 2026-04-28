//----------------------------------------------------
// Filename	: CTRx.cpp
// Version	: 0.71
// DATE		: 18 Nov 2019
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Tx Rx definition
//----------------------------------------------------
#include <stdlib.h>
#include <assert.h>
#include <iostream>

#include "CTRx.h"


// Construct
CTRx::CTRx(string cName, ETRxType eTRxType, EPktType ePktType) {

	// Generate later when put
	this->cpAx	= NULL;
	this->cpR	= NULL;
	this->cpW	= NULL;
	this->cpB	= NULL;

	#ifdef CCI_ON
		this->cpAC = NULL;
		this->cpCR = NULL;
		this->cpCD = NULL;
	#endif

	// Initialize
	this->cName 	= cName;
	this->eTRxType	= eTRxType;
	this->ePktType	= ePktType;

	this->eState          = ESTATE_TYPE_IDLE;
	this->eState_D        = ESTATE_TYPE_IDLE;
	this->eAcceptResult   = ERESULT_TYPE_REJECT;
	this->eAcceptResult_D = ERESULT_TYPE_REJECT;
};


// Destruct
CTRx::~CTRx() {
	
	#ifdef CCI_ON
		delete (this->cpAC);
		delete (this->cpCR);
		delete (this->cpCD);
	#endif

	delete (this->cpAx); // Do we need
	delete (this->cpR);
	delete (this->cpB);
	delete (this->cpW);
	
	this->cpAx = NULL;
	this->cpR  = NULL;
	this->cpB  = NULL;
	this->cpW  = NULL;
};


// Set valid high
// Generate pkt 
EResultType CTRx::PutAx(CPAxPkt cpPkt) { 

	// Debug
	// cpPkt->CheckPkt();

	// Debug. Check state
	if (this->eState == ESTATE_TYPE_BUSY) { 
		assert (0);
		return (ERESULT_TYPE_FAIL);
	};

	// Get and generate
	CPAxPkt cpAx_new = Copy_CAxPkt(cpPkt);
	
	// Put
	this->cpAx = cpAx_new;
	this->eState = ESTATE_TYPE_BUSY;
	this->eAcceptResult = ERESULT_TYPE_UNDEFINED;

	// Debug 
	// cpAx_new->CheckPkt();
	// this->cpAx->CheckPkt();

	return (ERESULT_TYPE_SUCCESS);
};


// Set valid high
// Generate pkt
EResultType CTRx::PutR(CPRPkt cpPkt) { 

	// Debug
	// cpPkt->CheckPkt();

	// Debug. Check state
    if (this->eState == ESTATE_TYPE_BUSY) {
		assert (0);
        return (ERESULT_TYPE_FAIL);
    };

	// Get and generate
	CPRPkt cpR_new = Copy_CRPkt(cpPkt); 
        
	// Put 
        this->cpR = cpR_new;
        eState = ESTATE_TYPE_BUSY;
        eAcceptResult = ERESULT_TYPE_UNDEFINED;

	// Debug 
	// cpR_new->CheckPkt();
	// this->cpR->CheckPkt();

        return (ERESULT_TYPE_SUCCESS);
};


// Set valid high
// Generate pkt
EResultType CTRx::PutW(CPWPkt cpPkt) { 

	// Debug
	// cpPkt->CheckPkt();

	// Debug. Check state 
        if (this->eState == ESTATE_TYPE_BUSY) {
			assert (0);
            return (ERESULT_TYPE_FAIL);
        };

	// Get and generate 
	CPWPkt cpW_new;
	cpW_new  = Copy_CWPkt(cpPkt); 
        
	// Put 
        this->cpW = cpW_new;
        eState = ESTATE_TYPE_BUSY;
        eAcceptResult = ERESULT_TYPE_UNDEFINED;

	// Debug 
	// cpW_new->CheckPkt();
	// this->cpW->CheckPkt();

        return (ERESULT_TYPE_SUCCESS);
};


// Set valid high
// Generate pkt
EResultType CTRx::PutB(CPBPkt cpPkt) { 

	// Debug
	// cpPkt->CheckPkt();

	// Debug. Check state 
        if (this->eState == ESTATE_TYPE_BUSY) {
			assert (0);
            return (ERESULT_TYPE_FAIL);
        };

	// Get and generate 
	CPBPkt cpB_new;
	cpB_new = Copy_CBPkt(cpPkt); 
        
	// Put 
        this->cpB = cpB_new;
        eState = ESTATE_TYPE_BUSY;
        eAcceptResult = ERESULT_TYPE_UNDEFINED;

	// Debug 
	// cpB_new->CheckPkt();
	// this->cpB->CheckPkt();

        return (ERESULT_TYPE_SUCCESS);
};

#ifdef CCI_ON
EResultType CTRx::PutAC(CPACPkt cpPkt) {
	// Debug. Check state 
    if (this->eState == ESTATE_TYPE_BUSY) {
		assert (0);
        return (ERESULT_TYPE_FAIL);
    };

	// Get and generate 
	CPACPkt cpAC_new;
	cpAC_new = Copy_CACPkt(cpPkt); 
        
	// Put 
    this->cpAC = cpAC_new;
    eState = ESTATE_TYPE_BUSY;
    eAcceptResult = ERESULT_TYPE_UNDEFINED;

	// Debug 
	// cpAC_new->CheckPkt();
	// this->cpAC->CheckPkt();

    return (ERESULT_TYPE_SUCCESS);
}

EResultType CTRx::PutCD(CPCDPkt cpPkt) {
	// Debug. Check state 
    if (this->eState == ESTATE_TYPE_BUSY) {
		assert (0);
        return (ERESULT_TYPE_FAIL);
    };

	// Get and generate 
	CPCDPkt cpCD_new;
	cpCD_new = Copy_CCDPkt(cpPkt); 
        
	// Put 
    this->cpCD = cpCD_new;
    eState = ESTATE_TYPE_BUSY;
    eAcceptResult = ERESULT_TYPE_UNDEFINED;

	// Debug 
	// cpCD_new->CheckPkt();
	// this->cpCD->CheckPkt();

    return (ERESULT_TYPE_SUCCESS);
}

EResultType CTRx::PutCR(CPCRPkt cpPkt) {
	// Debug. Check state 
    if (this->eState == ESTATE_TYPE_BUSY) {
		assert (0);
        return (ERESULT_TYPE_FAIL);
    };

	// Get and generate 
	CPCRPkt cpCR_new;
	cpCR_new = Copy_CCRPkt(cpPkt); 
        
	// Put 
    this->cpCR = cpCR_new;
    eState = ESTATE_TYPE_BUSY;
    eAcceptResult = ERESULT_TYPE_UNDEFINED;

	// Debug 
	// cpCR_new->CheckPkt();
	// this->cpCR->CheckPkt();

    return (ERESULT_TYPE_SUCCESS);
}
#endif


// Set ready. Propagate
EResultType CTRx::SetAcceptResult(EResultType eResult) {

	// Set ready 
        if (this->eTRxType == ETRX_TYPE_TX) {
        	this->eAcceptResult = eResult; 
        };      

	// Set ready
	// Propagate ready to Tx 
        if (this->eTRxType == ETRX_TYPE_RX) {
        	this->eAcceptResult = eResult; 
		this->cpPair->SetAcceptResult(eResult);
        };      

        return (ERESULT_TYPE_SUCCESS);
};


// Set link
EResultType CTRx::SetPair(CPTRx cpTRx) {

	// Debug
	assert (this != NULL);
	assert (cpTRx != NULL);
	assert (this->cpPair == NULL);
	assert (this->eTRxType != cpTRx->eTRxType);
	
	// Set pair
	this->cpPair = cpTRx;
	cpTRx->cpPair = this;
	
	// Debug 
	assert (this->cpPair->cpPair == this);
	
	return (ERESULT_TYPE_SUCCESS);
};

// Get name
string CTRx::GetName() {

	// this->CheckPkt();
	return (this->cName);
};

// Get ready
EResultType CTRx::GetAcceptResult() {

	#ifdef DEBUG
	assert (this->eState == ESTATE_TYPE_BUSY or this->eState == ESTATE_TYPE_IDLE);
	#endif

	return (this->eAcceptResult); 
};


// Get pkt type
EPktType CTRx::GetPktType() {

	return (this->ePktType);
};


// Get TRx type
ETRxType CTRx::GetTRxType() {

	return (this->eTRxType);
};


// Get pair
CPTRx CTRx::GetPair() {

	return (this->cpPair);
};  


// Get Ax pkt
CPAxPkt CTRx::GetAx() {

	return (this->cpAx);
};


// Get R pkt
CPRPkt CTRx::GetR() {

	return (this->cpR);
};


// Get W pkt
CPWPkt CTRx::GetW() {

	return (this->cpW);
};


// Get B pkt
CPBPkt CTRx::GetB() {

	return (this->cpB);
};

#ifdef CCI_ON
// Get AC pkt
CPACPkt CTRx::GetAC() {
	return (this->cpAC);
}

// Get CD pkt
CPCDPkt CTRx::GetCD() {
	return (this->cpCD);
}

// Get CR pkt
CPCRPkt CTRx::GetCR() {
	return (this->cpCR);
}
#endif

// Check valid/ready handshaked
EResultType CTRx::IsPass() {

    if (this->eState == ESTATE_TYPE_BUSY) {
        if (this->eAcceptResult == ERESULT_TYPE_ACCEPT) {
        	return (ERESULT_TYPE_YES);
		};
	};
        return (ERESULT_TYPE_NO);
};


// Check valid low
EResultType CTRx::IsIdle() { 

        if (this->eState == ESTATE_TYPE_IDLE) {
        	return (ERESULT_TYPE_YES);
	};
        return (ERESULT_TYPE_NO);
};


// Check valid high 
EResultType CTRx::IsBusy() { 

        if (this->eState == ESTATE_TYPE_BUSY) {
        	return (ERESULT_TYPE_YES);
	};
        return (ERESULT_TYPE_NO);
};


// Check first valid
// This function is used in arbiter
EResultType CTRx::IsFirstValid() {

	if ( ( (this->eState_D == ESTATE_TYPE_IDLE) ||
	    (this->eAcceptResult_D == ERESULT_TYPE_ACCEPT) ) &&
	     (  this->eState == ESTATE_TYPE_IDLE) ) {
	
		return (ERESULT_TYPE_YES);
	} 
	else {
		return (ERESULT_TYPE_NO);
	};
	
	return (ERESULT_TYPE_NO);
};


// Delete and initialize
EResultType CTRx::Reset() {

	// Maintain trans 
	this->FlushPkt();

	// Generate later when Put is conducted
	this->cpAx	= NULL;
	this->cpR	= NULL;
	this->cpW	= NULL;
	this->cpB	= NULL;

	// Initialize
	eState          = ESTATE_TYPE_IDLE;
	eState_D        = ESTATE_TYPE_IDLE;
	eAcceptResult   = ERESULT_TYPE_REJECT;
	eAcceptResult_D = ERESULT_TYPE_REJECT;
	
	return (ERESULT_TYPE_SUCCESS);
};


// Set state (at the end of cycle)
EResultType CTRx::UpdateState() {

	// // Remember previous state
	// this->eState_D        = this->eState;
	// this->eAcceptResult_D = this->eAcceptResult;
	// 
	// // Debug
	// assert (this->eState		== ESTATE_TYPE_IDLE    or this->eState          == ESTATE_TYPE_BUSY);
	// assert (this->eState_D	== ESTATE_TYPE_IDLE    or this->eState_D        == ESTATE_TYPE_BUSY);
	// assert (this->eAcceptResult	== ERESULT_TYPE_ACCEPT or this->eAcceptResult   == ERESULT_TYPE_REJECT or this->eAcceptResult   == ERESULT_TYPE_UNDEFINED);
	// assert (this->eAcceptResult_D== ERESULT_TYPE_ACCEPT or this->eAcceptResult_D == ERESULT_TYPE_REJECT or this->eAcceptResult_D == ERESULT_TYPE_UNDEFINED);
	// assert (this->eState		!= ESTATE_TYPE_UNDEFINED);
	// assert (this->eState_D	!= ESTATE_TYPE_UNDEFINED);
	
	// Update handshaked state
	if (this->IsPass() == ERESULT_TYPE_YES) {
		this->eState = ESTATE_TYPE_IDLE;
		this->FlushPkt();
	};

	// For next cycle, set ready low initially
	// Every cycle, set ready appropriately
	this->eAcceptResult = ERESULT_TYPE_REJECT;
	
	return (ERESULT_TYPE_SUCCESS);
};


// Delete
EResultType CTRx::FlushPkt() {

	delete (this->cpAx);
	delete (this->cpR);
	delete (this->cpW);
	delete (this->cpB);
	
	this->cpAx = NULL;
	this->cpR  = NULL;
	this->cpW  = NULL;
	this->cpB  = NULL;

	#ifdef CCI_ON
		delete (this->cpAC);
		delete (this->cpCR);
		delete (this->cpCD);
		
		this->cpAC = NULL;
		this->cpCR = NULL;
		this->cpCD = NULL;
	#endif
	
	return (ERESULT_TYPE_SUCCESS);
};


// Debug
EResultType CTRx::Display() {

	return (ERESULT_TYPE_SUCCESS);
};

