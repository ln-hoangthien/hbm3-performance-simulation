//-----------------------------------------------------------
// Filename     : UD_Bus.cpp 
// Version	: 0.80
// Date         : 15 Nov 2022
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Unified data definition
//-----------------------------------------------------------
// Delete_UD. Be careful.
// Example: 	Delete_UD (upAR_new, EUD_TYPE_AR); 
// 		Check (1) upAR_new, (2) upAR_new->cpAR, (3) upAR_new->cpAR->spPkt deleted correctly. We can manually delete.
//-----------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>

#include "UD_Bus.h"


int GetID(UPUD upThis, EUDType eType) {

	switch (eType) {
		case EUD_TYPE_AR: 
			return (upThis->cpAR->GetID());
		case EUD_TYPE_R: 
			return (upThis->cpR->GetID());
		case EUD_TYPE_AW: 
			return (upThis->cpAW->GetID());
		case EUD_TYPE_W: 
			return (upThis->cpW->GetID());
		case EUD_TYPE_B: 
			return (upThis->cpB->GetID());
		case EUD_TYPE_UNDEFINED: 
			return (-1);
		default: 
			break;
	};
	assert(0);
	return (-1);
};


// Maintain UD
EResultType Delete_UD(UPUD upThis, EUDType eType) {

	switch (eType) {
		case EUD_TYPE_AR: 
			delete (upThis->cpAR);	// spPkt deleted in cpAR destructor
			upThis->cpAR = NULL; 
			delete (upThis);	// Check upThis itself deleted
			upThis = NULL; 
			break;
		case EUD_TYPE_AW: 
			delete (upThis->cpAW);
			upThis->cpAW = NULL;
			delete (upThis);
			upThis = NULL;
			break;
		case EUD_TYPE_W: 
			delete (upThis->cpW);
			upThis->cpW  = NULL;
			delete (upThis); 
			upThis = NULL; 
			break;
		case EUD_TYPE_R: 
			delete (upThis->cpR);  
			upThis->cpR  = NULL; 
			delete (upThis); 
			upThis = NULL; 
			break;
		case EUD_TYPE_B: 
			delete (upThis->cpB);  
			upThis->cpB  = NULL; 
			delete (upThis); 
			upThis = NULL; 
			break;
		case EUD_TYPE_UNDEFINED: 
			assert(0);
			return(ERESULT_TYPE_FAIL);
		default: 
			break;
	};

	return (ERESULT_TYPE_SUCCESS);
};


UPUD Copy_UD(UPUD upThis, EUDType eType) {

	// Generate and initialize
	UPUD upNew = new UUD;
	upNew->cpAR = NULL;
	upNew->cpR  = NULL;
	upNew->cpAW = NULL;
	upNew->cpW  = NULL;
	upNew->cpB  = NULL;
	
	switch (eType) {
		case EUD_TYPE_AR:
			// upThis->cpAR->CheckPkt();
			upNew->cpAR = Copy_CAxPkt(upThis->cpAR);
			break;
		case EUD_TYPE_R:
			// upThis->cpR->CheckPkt();
			upNew->cpR  = Copy_CRPkt(upThis->cpR);
			break;
		case EUD_TYPE_AW: 
			// upThis->cpAW->CheckPkt();
			upNew->cpAW = Copy_CAxPkt(upThis->cpAW);       
			break;
		case EUD_TYPE_W:
			// upThis->cpW->CheckPkt();
			upNew->cpW  = Copy_CWPkt(upThis->cpW);
			break;
		case EUD_TYPE_B:
			// upThis->cpB->CheckPkt();
			upNew->cpB  = Copy_CBPkt(upThis->cpB);
			break;
		case EUD_TYPE_UNDEFINED:
			assert(0);
			return (NULL);
		#ifdef CCI_ON  // Adding channel for coherence
			case EUD_TYPE_AC:    
				upNew->cpAC  = Copy_CACPkt(upThis->cpAC);
				break;
			case EUD_TYPE_CR:    
				upNew->cpCR  = Copy_CCRPkt(upThis->cpCR);
				break;
			case EUD_TYPE_CD:    
				upNew->cpCD  = Copy_CCDPkt(upThis->cpCD);
				break;
		#endif
		default:
			break;
	};

	return (upNew);
};


// Generate new pointer
// Copy all member values one-by-one
CPAxPkt Copy_CAxPkt(CPAxPkt cpThis) {

	ETransDirType eDir = cpThis->GetDir();

	CPAxPkt cpAx_new = new CAxPkt(eDir);	// New instance. Note: spPkt generated when CAxPkt generated

	// *(cpAx_new) = *(cpThis);		// This is memory-leak bug. cpThis->spPkt also copied
	// SPAxPkt spAx_new = new SAxPkt;	// New instance. Be careful delete spAx_new.
	// spAx_new->nID    = cpThis->GetID();
	// spAx_new->nAddr  = cpThis->GetAddr();
	// spAx_new->nLen   = cpThis->GetLen();
	// cpAx_new->SetPkt(spAx_new);

	// Get all member values 
	string	    cName       = cpThis->GetName();
	ETransType  eTransType  = cpThis->GetTransType();
	string	    cSrcName    = cpThis->GetSrcName();
	EResultType eFinalTrans = cpThis->IsFinalTrans();
	int	    	nTransNum   = cpThis->GetTransNum();
	int64_t	    nVA         = cpThis->GetVA();
	int	    	nTileNum    = cpThis->GetTileNum();
	// int	    nMemCh      = cpThis->GetMemCh();
	// int	    nCacheCh    = cpThis->GetCacheCh();

	int     nID   = cpThis->GetID();
	int64_t nAddr = cpThis->GetAddr();
	int     nLen  = cpThis->GetLen();

	// Set all member values
	cpAx_new->SetName(cName);
	cpAx_new->SetTransDirType(eDir);
	cpAx_new->SetTransType(eTransType);
	cpAx_new->SetSrcName(cSrcName);
	cpAx_new->SetFinalTrans(eFinalTrans);
	cpAx_new->SetTransNum(nTransNum);
	cpAx_new->SetVA(nVA);
	cpAx_new->SetTileNum(nTileNum);
	// cpAx_new->SetMemCh(nMemCh);
	// cpAx_new->SetCacheCh(nCacheCh);

	#ifdef CCI_ON
	int nSnoop = cpThis->GetSnoop();
	cpAx_new->SetPkt(nID, nAddr, nLen, nSnoop);
	#else
	cpAx_new->SetPkt(nID, nAddr, nLen);
	#endif
	return (cpAx_new);
};


// Generate new pointer
// Copy member values
CPRPkt Copy_CRPkt(CPRPkt cpThis) {

	CPRPkt cpR_new = new CRPkt;	// New instance

	// *(cpR_new) = *(cpThis);	// Be careful. Memory leak.
	//
	// SPRPkt spR_new = new SRPkt;	// New instance.
	// spR_new->nID	= cpThis->GetID();
	// spR_new->nData	= cpThis->GetData();
	// if (cpThis->IsLast() == ERESULT_TYPE_YES) { 
	//      spR_new->nLast = 1;
	// } 
	// else if (cpThis->IsLast() == ERESULT_TYPE_NO) {
	//      spR_new->nLast = 0;
	// } 
	// else {
	//      assert(0);
	// };
	// 
	// cpR_new->SetPkt(spR_new);

	// Get all members 
	string	     cName       = cpThis->GetName(); 
	EResultType  eFinalTrans = cpThis->IsFinalTrans();
	int          nMemCh      = cpThis->GetMemCh();
	int          nCacheCh    = cpThis->GetCacheCh();

	int nID   = cpThis->GetID();
	int nData = cpThis->GetData();
	int nLast = -1;
	if (cpThis->IsLast() == ERESULT_TYPE_YES) { 
	     nLast = 1;
	} 
	else if (cpThis->IsLast() == ERESULT_TYPE_NO) {
	     nLast = 0;
	} 
	else {
	     assert(0);
	};

	// Set members
	cpR_new->SetName(cName);
	cpR_new->SetFinalTrans(eFinalTrans);
	cpR_new->SetMemCh(nMemCh);
	cpR_new->SetCacheCh(nCacheCh);

	cpR_new->SetPkt(nID, nData, nLast);

	return (cpR_new);
};


// Generate new pointer
// Copy member values
CPWPkt Copy_CWPkt(CPWPkt cpThis) {
	
	CPWPkt cpW_new = new CWPkt;	// New instance

	// *(cpW_new) = *(cpThis);	// Be careful. Memory leak
	//
	// SPWPkt spW_new = new SWPkt;	// New instance
	// spW_new->nID    = cpThis->GetID();
	// spW_new->nData  = cpThis->GetData();
	// if (cpThis->IsLast() == ERESULT_TYPE_YES) {
	// 	spW_new->nLast = 1;
	// }
	// else if (cpThis->IsLast() == ERESULT_TYPE_NO) {
	// 	spW_new->nLast = 0;
	// }
	// else {
	// 	assert(0);
	// };
	// 
	// cpW_new->SetPkt(spW_new);

	// Get all members 
	string     cName      = cpThis->GetName(); 
	ETransType eTransType = cpThis->GetTransType();
	string	   cSrcName   = cpThis->GetSrcName();
	
	int nID   = cpThis->GetID();
	int nData = cpThis->GetData();
	int nLast = -1;
	if (cpThis->IsLast() == ERESULT_TYPE_YES) {
		nLast = 1;
	}
	else if (cpThis->IsLast() == ERESULT_TYPE_NO) {
		nLast = 0;
	}
	else {
		assert(0);
	};

	// Set members
	cpW_new->SetName(cName);
	cpW_new->SetTransType(eTransType);
	cpW_new->SetSrcName(cSrcName);

	cpW_new->SetPkt(nID, nData, nLast);

	return (cpW_new);
};


// Generate new pointer
// Copy member values
CPBPkt Copy_CBPkt(CPBPkt cpThis) {
	
	CPBPkt cpB_new = new CBPkt;	// New instance

	// *(cpB_new) = *(cpThis);	// Be careful. Memory leak.
	// 
	// SPBPkt spB_new = new SBPkt;	// New instance
	// spB_new->nID    = cpThis->GetID();
	// cpB_new->SetPkt(spB_new);

	// Get all members 
	string      cName       = cpThis->GetName(); 
	int         nID         = cpThis->GetID();
	EResultType eFinalTrans = cpThis->IsFinalTrans();
	int         nMemCh      = cpThis->GetMemCh();
	int         nCacheCh    = cpThis->GetCacheCh();

	// Set members
	cpB_new->SetName(cName);
	cpB_new->SetPkt(nID);
	cpB_new->SetFinalTrans(eFinalTrans);
	cpB_new->SetMemCh(nMemCh);
	cpB_new->SetCacheCh(nCacheCh);

	return (cpB_new);
};


// Debug
EResultType Display_UD(UPUD upThis, EUDType eType) {

	switch (eType) {
		case EUD_TYPE_AR:
			upThis->cpAR->Display();
			break;
		case EUD_TYPE_R:
			upThis->cpR->Display();
			break;
		case EUD_TYPE_AW: 
			upThis->cpAW->Display(); 
			break;
		case EUD_TYPE_W:
			upThis->cpW->Display();
			break;
		case EUD_TYPE_B:
			upThis->cpB->Display();
			break;
		case EUD_TYPE_UNDEFINED:
			assert(0);
		default:
			break;
	};
	
	return (ERESULT_TYPE_SUCCESS);
};

#ifdef CCI_ON
// Generate new pointer
// Copy all member values one-by-one
CPACPkt Copy_CACPkt(CPACPkt cpThis) {


	CPACPkt cpCR_new = new CACPkt();	// New instance. Note: spPkt generated when CCRPkt generated

	// Get all member values 
	string	    cName       = cpThis->GetName();
	EResultType eFinalTrans = cpThis->IsFinalTrans();
	int 		Snoop		= cpThis->GetSnoop();

	// Set all member values
	cpCR_new->SetName(cName);
	cpCR_new->SetFinalTrans(eFinalTrans);
	cpCR_new->SetSnoop(Snoop);

	return (cpCR_new);
};

// Generate new pointer
// Copy all member values one-by-one
CPCDPkt Copy_CCDPkt(CPCDPkt cpThis) {


	CPCDPkt cpCR_new = new CCDPkt();	// New instance. Note: spPkt generated when CCRPkt generated

	// Get all member values 
	string	    cName       = cpThis->GetName();
	EResultType eFinalTrans = cpThis->IsFinalTrans();

	// Set all member values
	cpCR_new->SetName(cName);
	cpCR_new->SetFinalTrans(eFinalTrans);

	return (cpCR_new);
};


// Generate new pointer
// Copy all member values one-by-one
CPCRPkt Copy_CCRPkt(CPCRPkt cpThis) {


	CPCRPkt cpCR_new = new CCRPkt();	// New instance. Note: spPkt generated when CCRPkt generated
	
	// Get all member values 
	string	    cName       = cpThis->GetName();
	EResultType eFinalTrans = cpThis->IsFinalTrans();
	int     	nResp   	= cpThis->GetResp();

	// Set all member values
	cpCR_new->SetName(cName);
	cpCR_new->SetFinalTrans(eFinalTrans);
	cpCR_new->SetResp(nResp);

	return (cpCR_new);
};
#endif

// Debug
// EResultType CheckUD(UPUD upThis, EUDType eType) {
//
//	switch (eType) {
//	        case EUD_TYPE_AR:
//			upThis->cpAR->CheckPkt();
//			break;
//	        case EUD_TYPE_R:
//			upThis->cpR->CheckPkt();
//			break;
//	        case EUD_TYPE_AW:
//			upThis->cpAW->CheckPkt();
//			break;
//	        case EUD_TYPE_W:
//			upThis->cpW->CheckPkt();
//			break;
//	        case EUD_TYPE_B:
//			upThis->cpB->CheckPkt();
//			break;
//	        case EUD_TYPE_UNDEFINED:
//			assert(0);
//	        default:
//			break;
//	};
//	
//	return (ERESULT_TYPE_SUCCESS);
// };

