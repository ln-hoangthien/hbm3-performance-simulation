//-----------------------------------------------------------
// FileName	: CBank.cpp
// Version	: 0.81
// Date		: 18 Nov 2019
// Contact	: JaeYoung.Hur@gmail.com
//-----------------------------------------------------------
// Description	: Memory bank FSM class definition
//-----------------------------------------------------------
// Refer to timing diagram
//-----------------------------------------------------------

//--------------------------------------------------
// nCnt_RP      PRE2ACT. Precharging. Auto state change
//              Min 0, Max tRP
//              At PRE, start increase
//              At ACT, reset 0
//--------------------------------------------------
// nCnt_RAS     ACT2PRE
//              Min 0, Max any
//              At ACT, start increase
//              At PRE, reset 0
//--------------------------------------------------
// nCnt_RCD     ACT2RD. ACT2WR. Activating. Auto state change
//              Min 0, Max tRCD-1
//              At ACT,    start increase 
//              At tRCD-1, reset 0
//--------------------------------------------------
// nCnt_RTP     RD2PRE
//              Min 0, Max any
//              At RD,  start increase 
//              At PRE, reset 0
//--------------------------------------------------
// nCnt_WR      WR2PRE
//              Min 0, Max any
//              At WR,  start increase 
//              At PRE, reset 0
//--------------------------------------------------
// nCnt_CL      RD2DATA
//              Min 0, Max tCL
//              At RD,  start increase
//              At tCL, reset 0
//--------------------------------------------------
// nCnt_WL      WR2DATA
//              Min 0, Max tWL
//              At RD,  start increase
//              At tWL, reset 0
//--------------------------------------------------
// nCnt_CCD     RD2RD. WR2WR.
//              Min 0, Max tCCD
//              At RD/WR, start increase
//              At tCCD,  reset
//-----------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <string>

#include "CBank.h"


// Construct
CBank::CBank(string cName) {

	// Initialize
	this->cName  = cName;

	this->eMemCmd = EMEM_CMD_TYPE_UNDEFINED;
	this->nBankCmd = -1;
	this->nRowCmd  = -1;

	this->eMemState = EMEM_STATE_TYPE_IDLE;
	this->nActivatedRow = -1;

	this->nCnt_RP  = -1;  				// PRE2ACT. Precharging.
	this->nCnt_RAS = -1;  				// ACT2PRE
	this->nCnt_RCD = -1;  				// ACT2RD. ACT2WR. Activating.
	this->nCnt_RTP = -1;  				// RD2PRE
	this->nCnt_WR  = -1;  				// WR2PRE
	this->nCnt_CL  = -1;  				// RD2DATA
	this->nCnt_WL  = -1;  				// WR2DATA
	this->nCnt_CCD = -1;  				// RD2RD. WR2WR.

	this->IsCmd_overlap_r  = ERESULT_TYPE_NO; 	// Back-to-back RD/WR
	this->IsBankPrepared_r = ERESULT_TYPE_NO; 	// Bank prepared (activated, not yet RD/WR) 

	this->nCnt_Data = -1;
};


// Destruct
CBank::~CBank() {
};


// Reset
EResultType CBank::Reset() {

	this->eMemCmd  = EMEM_CMD_TYPE_NOP;
	this->nBankCmd = -1;
	this->nRowCmd  = -1;

	this->eMemState     = EMEM_STATE_TYPE_IDLE;
	this->nActivatedRow = -1;

	this->nCnt_RP  = tRP; 				// PRE2ACT. Initially in cycle 1, bank state idle (or precharge finished)
	this->nCnt_RAS = 0;   				// ACT2PRE
	this->nCnt_RCD = 0;   				// ACT2RD, ACT2WR. Activating.
	this->nCnt_RTP = 0;   				// RD2PRE
	this->nCnt_WR  = 0;   				// WR2PRE
	this->nCnt_CL  = 0;   				// RD2DATA
	this->nCnt_WL  = 0;   				// WR2DATA
	this->nCnt_CCD = 0;   				// RD2RD, WR2WR

	this->IsCmd_overlap_r  = ERESULT_TYPE_NO;
	this->IsBankPrepared_r = ERESULT_TYPE_NO;

	this->nCnt_Data = 0;

	return (ERESULT_TYPE_SUCCESS);
};


// Set mem cmd
EResultType CBank::SetMemCmd(EMemCmdType eCmd) {

	this->eMemCmd = eCmd;
	return (ERESULT_TYPE_SUCCESS);
};


// Set mem addr
EResultType CBank::SetMemAddr(int nBank, int nRow) {

	this->nBankCmd = nBank;
	this->nRowCmd  = nRow;
	return (ERESULT_TYPE_SUCCESS);
};


// Get Mem name
string CBank::GetName() {

	return (this->cName);
};


// Get state 
EMemStateType CBank::GetMemState() {

	return (this->eMemState);
};


// Get activated row
int CBank::GetActivatedRow() {

	return (this->nActivatedRow);
};


//--------------------------------------
// This bank can get RD cmd
//--------------------------------------
EResultType CBank::IsRD_ready() {

	// Check state	
	if (this->eMemState != EMEM_STATE_TYPE_ACTIVE) { 
		return (ERESULT_TYPE_NO);
	};

	#ifdef DEBUG
	assert (this->eMemState == EMEM_STATE_TYPE_ACTIVE);
	#endif

	// Check counter
	if (this->nCnt_CCD == 0 or 					// No on-going RD 
	    this->nCnt_CCD == tCCD) {					// On-going RD 

		#ifdef DEBUG
		assert (this->GetActivatedRow() != -1);
		#endif

		return (ERESULT_TYPE_YES);
	};

	return (ERESULT_TYPE_NO);	
};


//--------------------------------------
// This bank can get WR cmd
//--------------------------------------
EResultType CBank::IsWR_ready() {

	// Check state	
	if (this->eMemState != EMEM_STATE_TYPE_ACTIVE) { 
		return (ERESULT_TYPE_NO);
	};

	#ifdef DEBUG
	assert (this->eMemState == EMEM_STATE_TYPE_ACTIVE);
	#endif

	// Check counter
	if (this->nCnt_CCD == 0 or 					// No on-going WR
	    this->nCnt_CCD == tCCD) {					// On-going WR 

		#ifdef DEBUG
		assert (this->GetActivatedRow() != -1);
		#endif

		return (ERESULT_TYPE_YES);
	};

	return (ERESULT_TYPE_NO);	
};


//--------------------------------------
// Check data channel busy 
//	Be careful timing.
//	nCnt_Data UpdateState here and use next cycle.
//	This function should be called every cycle.
//--------------------------------------
//	nCnt_Data	Min 0, Max BURST LENGHTH
//			At IsFirstData_ready, set 2 for next cycle. 
//			At Cnt < BURSTLENGTH, increase
//			At BURSTLENGTGH, reset 0
//--------------------------------------
EResultType CBank::IsData_busy() {

	// Check data channel on-going 
	if (this->nCnt_Data > 1 and this->nCnt_Data < MAX_BURST_LENGTH) { // On going
		this->nCnt_Data ++;             			// Update for next cycle
		return (ERESULT_TYPE_YES);
	};

	// Check finish
	if (this->nCnt_Data == MAX_BURST_LENGTH) {  			// Finish in this cycle
		this->nCnt_Data = 0;					// No data in next cycle	
		return (ERESULT_TYPE_YES);
	};

	// Check 1st data
	if (this->IsFirstData_ready() == ERESULT_TYPE_YES) {  		// First data in this cycle 
		this->nCnt_Data = 2;					// 2nd data in next cycle
		return (ERESULT_TYPE_YES);
	};

	#ifdef DEBUG
	assert (this->nCnt_Data == 0);
	#endif

	return (ERESULT_TYPE_NO);
};


//---------------------------------------------
// Check this bank can put data 
//---------------------------------------------
// 	IsCmd_overlap_r		Assert 1 cycle
//				At RD/WR, set 
//				Otherwise, reset (See UpdateState)
//---------------------------------------------
EResultType CBank::IsFirstData_ready() {

	// Check counter	
	if (this->nCnt_CL == tCL) {					// RD2DATA
		return (ERESULT_TYPE_YES);	
	};

	if (this->nCnt_WL == tWL) {					// WR2DATA
		return (ERESULT_TYPE_YES);	
	};

	// Check back-to-back RD/WR
	if (this->nCnt_CL == 1 or this->nCnt_WL == 1) {			// RD2DATA. WR2DATA
		if (this->IsCmd_overlap_r == ERESULT_TYPE_YES) {

			#ifdef DEBUG
			assert (this->eMemState == EMEM_STATE_TYPE_ACTIVE);
			#endif
			
			return (ERESULT_TYPE_YES);	
		};
	};

	return (ERESULT_TYPE_NO);	
};


//---------------------------------------------
// Check this bank is prepared
//---------------------------------------------
//	At activation, set 
//	At RD/WR/PRE, reset 
//---------------------------------------------
EResultType CBank::IsBankPrepared() {
	
	if (this->IsBankPrepared_r == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_YES);	
	}
	else {
		return (ERESULT_TYPE_NO);	
	};
};


//--------------------------------------
// This bank can get PRE 
//--------------------------------------
EResultType CBank::IsPRE_ready() {

	// Check state
	if (this->eMemState != EMEM_STATE_TYPE_ACTIVE) { 
		return (ERESULT_TYPE_NO);
	};

	#ifdef DEBUG
	assert (this->eMemState == EMEM_STATE_TYPE_ACTIVE);
	#endif

	// Check counter	
	if (this->nCnt_RAS >= tRAS and		                	// ACT2PRE
	   (this->nCnt_RTP == 0 or this->nCnt_RTP >= tRTP) and		// RD2PRE. When RD, cnt increases. When no RD, cnt 0 
	   (this->nCnt_WR  == 0 or this->nCnt_WR  >= tWR)) {		// WR2PRE. When WR, cnt increases. When no WR, cnt 0. FIXME check back-to-back write. Other master read access same bank

		#ifdef DEBUG
		assert (this->GetActivatedRow() != -1);
		#endif

		return (ERESULT_TYPE_YES);	
	};

	return (ERESULT_TYPE_NO);
};

//--------------------------------------
// This bank can get ACT 
//--------------------------------------
EResultType CBank::IsACT_ready() {

	// Check state	
	if (this->eMemState == EMEM_STATE_TYPE_IDLE) { 

		#ifdef DEBUG
		assert (this->GetActivatedRow() == -1);
		#endif

		return (ERESULT_TYPE_YES);
	};

	return (ERESULT_TYPE_NO);	
};


//--------------------------------------
// Update state
//--------------------------------------
// 	Update in the order of State, IsCmd_overlap_r, IsBankPrepared_r, Counter
//--------------------------------------
//	nCnt_RP   PRE2ACT           auto state Precharging
//	nCnt_RAS  ACT2PRE           min
//	nCnt_RCD  ACT2RD. ACT2WR.   auto state Activating 
//	nCnt_RTP  RD2PRE            min
//	nCnt_WR   WR2PRE            min
//	nCnt_CL   RD2DATA           min (related to Reading) 
//	nCnt_WL   WR2DATA           min (related to Writing state) 
//	nCnt_CCD  RD2RD. WR2WR.     min
//--------------------------------------
EResultType CBank::UpdateState() {

	#ifdef DEBUG
	// this->CheckCnt();
	this->CheckCmdReady();
	#endif

	//------------------------------------
	// State, counter
	//------------------------------------
	if (this->eMemState == EMEM_STATE_TYPE_IDLE) {

		#ifdef DEBUG
		assert (this->eMemCmd == EMEM_CMD_TYPE_ACT or this->eMemCmd == EMEM_CMD_TYPE_NOP);
		#endif

		if (this->eMemCmd == EMEM_CMD_TYPE_ACT) {

			// Update state
			this->eMemState = EMEM_STATE_TYPE_ACTIVATING;

			#ifdef DEBUG
			assert (this->nActivatedRow == -1);

			assert (this->nCnt_RCD  == 0);					// (Start increase) ACT2RD. ACT2WR. Activating.
			assert (this->nCnt_RAS  == 0);					// (Start increase) ACT2PRE
			assert (this->nCnt_CL   == 0);  				// RD2DATA
			assert (this->nCnt_WL   == 0);  				// WR2DATA
			assert (this->nCnt_CCD  == 0);  				// RD2RD. WR2WR.
			assert (this->nCnt_RTP  == 0);  				// RD2PRE
			assert (this->nCnt_WR   == 0);  				// WR2PRE
			assert (this->nCnt_RP   == tRP);      				// (Reset) PRE2ACT. Precharging.
			#endif
		} 
		else if (this->eMemCmd == EMEM_CMD_TYPE_NOP) { 

			// Update state
			this->eMemState = EMEM_STATE_TYPE_IDLE;

			#ifdef DEBUG
			assert (this->nActivatedRow == -1);

			assert (this->nCnt_RCD == 0);   				// ACT2RD. ACT2WR. Activating.
			assert (this->nCnt_RAS == 0);   				// ACT2PRE
			assert (this->nCnt_CL  == 0);   				// RD2DATA
			assert (this->nCnt_WL  == 0);   				// WR2DATA
			assert (this->nCnt_CCD == 0);   				// RD2RD. WR2WR.
			assert (this->nCnt_RTP == 0);   				// RD2PRE
			assert (this->nCnt_WR  == 0);   				// WR2PRE
			assert (this->nCnt_RP  == tRP); 				// (Initial) PRE2ACT. Precharging.
			#endif
		} 
		else {
			assert (0);
		};
	} 

	else if (this->eMemState == EMEM_STATE_TYPE_ACTIVATING) {

		#ifdef DEBUG
		assert (this->eMemCmd == EMEM_CMD_TYPE_NOP);
		#endif

		if (this->nCnt_RCD < tRCD-1) {						// (Activating) ACT2RD. ACT2WR

			// Keep state
			this->eMemState = EMEM_STATE_TYPE_ACTIVATING;

			#ifdef DEBUG
			assert (this->nActivatedRow == -1);

			assert (this->nCnt_RCD > 0 or this->nCnt_RCD < tRCD);		// ACT2RD.  ACT2WR. Activating.
			assert (this->nCnt_RAS > 0 or this->nCnt_RAS < tRAS);		// ACT2PRE. 
			assert (this->nCnt_CL  == 0);					// RD2DATA
			assert (this->nCnt_WL  == 0);					// WR2DATA
			assert (this->nCnt_CCD == 0);					// RD2RD. WR2WR.
			assert (this->nCnt_RTP == 0);					// RD2PRE
			assert (this->nCnt_WR  == 0);					// WR2PRE
			assert (this->nCnt_RP  == 0);					// PRE2ACT. Precharging.
			#endif
		} 
		else if (this->nCnt_RCD == tRCD-1) { 					// This cycle last cycle activating.

			// Update state
			this->eMemState = EMEM_STATE_TYPE_ACTIVE;
			this->nActivatedRow  = this->nRowCmd;

			#ifdef DEBUG
		 	assert (this->nCnt_RCD > 0 or this->nCnt_RCD < tRCD);		// ACT2RD.  ACT2WR. Activating.
		 	assert (this->nCnt_RAS > 0 or this->nCnt_RAS < tRAS);		// ACT2PRE. 
			assert (this->nCnt_CL  == 0);  					// RD2DATA
			assert (this->nCnt_WL  == 0);  					// WR2DATA
			assert (this->nCnt_CCD == 0);  					// RD2RD. WR2WR.
			assert (this->nCnt_RTP == 0);  					// RD2PRE
			assert (this->nCnt_WR  == 0);  					// WR2PRE
			assert (this->nCnt_RP  == 0);  					// PRE2ACT. Precharging.
			#endif
		} 
		else {
			assert (0);
		};
	} 

	else if (this->eMemState == EMEM_STATE_TYPE_ACTIVE) {

		#ifdef DEBUG
		assert (this->eMemCmd == EMEM_CMD_TYPE_RD  or 
			this->eMemCmd == EMEM_CMD_TYPE_WR  or 
			this->eMemCmd == EMEM_CMD_TYPE_PRE or 
			this->eMemCmd == EMEM_CMD_TYPE_NOP);
		#endif

		if (this->eMemCmd == EMEM_CMD_TYPE_RD) {

			// Update state
			this->eMemState = EMEM_STATE_TYPE_ACTIVE;

			#ifdef DEBUG
			assert (this->nActivatedRow == this->nRowCmd);

			assert (this->nCnt_RCD == 0);    				// ACT2RD. ACT2WR. Activating.
			assert (this->nCnt_RAS >= tRCD);				// ACT2PRE
			assert (this->nCnt_CL  == 0 or this->nCnt_CL  <= tCL);		// RD2DATA
			assert (this->nCnt_WL  == 0 or this->nCnt_WL  <= tWL);		// WR2DATA
			assert (this->nCnt_CCD == 0 or this->nCnt_CCD <= tCCD);		// RD2RD. WR2WR.
			// assert (this->nCnt_RTP == 0 or this->nCnt_RTP >= tRCD);	// RD2PRE
			// assert (this->nCnt_WR  == 0 or this->nCnt_WR  >= tRCD);	// WR2PRE  
			assert (this->nCnt_RP  == 0);					// PRE2ACT. Precharging.
			#endif
		} 
		else if (this->eMemCmd == EMEM_CMD_TYPE_WR) {

			// Update state
			this->eMemState = EMEM_STATE_TYPE_ACTIVE;

			#ifdef DEBUG
			assert (this->nActivatedRow == this->nRowCmd);

			assert (this->nCnt_RCD == 0);					// ACT2RD. ACT2WR. Activating.
			assert (this->nCnt_RAS >= tRCD);				// ACT2PRE
			assert (this->nCnt_CL  == 0 or this->nCnt_CL  <= tCL);		// RD2DATA
			assert (this->nCnt_WL  == 0 or this->nCnt_WL  <= tWL);		// WR2DATA
			assert (this->nCnt_CCD == 0 or this->nCnt_CCD <= tCCD);		// RD2RD. WR2WR.
			// assert (this->nCnt_RTP == 0 or this->nCnt_RTP >= tRCD);	// RD2PRE
			// assert (this->nCnt_WR  == 0 or this->nCnt_WR  >= tRCD);	// WR2PRE  
			assert (this->nCnt_RP == 0);					// PRE2ACT. Precharging.
			#endif
		} 
		else if (this->eMemCmd == EMEM_CMD_TYPE_PRE) {

			// Updtae state
			this->eMemState     = EMEM_STATE_TYPE_PRECHARGING;
			this->nActivatedRow = -1;

			#ifdef DEBUG
			assert (this->nCnt_RAS >= tRAS);

			assert (this->nCnt_RCD == 0);					// ACT2RD. ACT2WR. Activating.
			assert (this->nCnt_RAS >= tRAS);				// ACT2PRE
			assert (this->nCnt_CL  == 0 or this->nCnt_CL  <= tCL);		// RD2DATA
			assert (this->nCnt_WL  == 0 or this->nCnt_WL  <= tWL);		// WR2DATA
			assert (this->nCnt_CCD == 0 or this->nCnt_CCD <= tCCD);		// RD2RD. WR2WR.
			// assert (this->nCnt_RTP == 0 or this->nCnt_RTP >= tRCD);	// RD2PRE
			// assert (this->nCnt_WR  == 0 or this->nCnt_WR  >= tRCD);	// WR2PRE  
			assert (this->nCnt_RP == 0);					// PRE2ACT. Precharging.
			#endif

		} 
		else if (this->eMemCmd == EMEM_CMD_TYPE_NOP) {

			// Keep state
			this->eMemState     = EMEM_STATE_TYPE_ACTIVE;

			#ifdef DEBUG
			// assert (this->nActivatedRow == this->nRowCmd);

			assert (this->nCnt_RCD == 0);					// ACT2RD. ACT2WR. Activating.
			assert (this->nCnt_RAS >= tRCD);				// ACT2PRE
			assert (this->nCnt_CL  == 0 or this->nCnt_CL  <= tCL);		// RD2DATA
			assert (this->nCnt_WL  == 0 or this->nCnt_WL  <= tWL);		// WR2DATA
			assert (this->nCnt_CCD == 0 or this->nCnt_CCD <= tCCD);		// RD2RD. WR2WR.
			// assert (this->nCnt_RTP == 0 or this->nCnt_RTP >= tRCD);	// RD2PRE
			// assert (this->nCnt_WR  == 0 or this->nCnt_WR  >= tRCD);	// WR2PRE  
			assert (this->nCnt_RP  == 0);					// PRE2ACT. Precharging.
			#endif
		} 
		else {
			assert (0);
		};
	} 

	else if (this->eMemState == EMEM_STATE_TYPE_PRECHARGING) {

		#ifdef DEBUG
		assert (this->eMemCmd == EMEM_CMD_TYPE_NOP);
		#endif

		if (this->nCnt_RP < tRP-1) {						// PRE2ACT. Precharge on going
		
		        this->eMemState = EMEM_STATE_TYPE_PRECHARGING;

			#ifdef DEBUG
		        assert (this->nActivatedRow == -1);

			assert (this->nCnt_RCD == 0);					// ACT2RD. ACT2WR. Activating. 
			assert (this->nCnt_RAS == 0);					// ACT2PRE
			// assert (this->nCnt_CL  == 0);				// RD2DATA. FIXME irrevalent
			// assert (this->nCnt_WL  == 0);				// WR2DATA. FIXME irrevalent
			assert (this->nCnt_CCD == 0);					// RD2RD. WR2WR.
			assert (this->nCnt_RTP == 0);					// RD2PRE 
			assert (this->nCnt_WR  == 0);					// WR2PRE 
			assert (this->nCnt_RP > 0);					// PRE2ACT. Precharging
			#endif
		} 
		else if (this->nCnt_RP == tRP-1) {					// This cycle last cycle precharge

			this->eMemState     = EMEM_STATE_TYPE_IDLE;

			#ifdef DEBUG
			assert (this->nActivatedRow == -1);

			assert (this->nCnt_RCD == 0);					// ACT2RD. ACT2WR. Activating.
			assert (this->nCnt_RAS == 0);					// ACT2PRE
			assert (this->nCnt_CL  == 0);					// RD2DATA. FIXME irrevalent
			assert (this->nCnt_WL  == 0);					// WR2DATA. FIXME irrevalent
			assert (this->nCnt_CCD == 0);					// RD2RD. WR2WR.
			assert (this->nCnt_RTP == 0);					// RD2PRE
			assert (this->nCnt_WR  == 0);					// WR2PRE
			assert (this->nCnt_RP > 0);					// PRE2ACT. Precharging
			#endif
		} 
		else {
			assert (0);
		};
	} 


	//--------------------
	// IsCmd_overlap_r
	//--------------------
	// 	Register. Need this to get IsFirstData_ready
	//--------------------
	//	Back-to-back RD/WR
	//	Assert 1 cycle
	//	Initially, 0
	//	At RD/WR, set 
	//	Otherwise, reset (See UpdateState)
	//--------------------
	// Check back-to-back
	if (this->eMemCmd == EMEM_CMD_TYPE_RD and this->nCnt_CL == tCL-1) {
		this->IsCmd_overlap_r = ERESULT_TYPE_YES;
	}
	else if (this->eMemCmd == EMEM_CMD_TYPE_WR and this->nCnt_WL == tWL-1) {
		this->IsCmd_overlap_r = ERESULT_TYPE_YES;
	}
	else {
		this->IsCmd_overlap_r = ERESULT_TYPE_NO;
	};


	//--------------------
	// IsBankPrepared_r
	//--------------------
	// 	Register. 
	// 	Need this to indicate bank prepared (activated) and not yet RD/WR
	//--------------------
	// 	Note: Update this before counter
	//--------------------
	//	Initially, 0
	//	At nCnt_RCD == tRCD-1, set 
	//	At PRE, reset
	//	At RD/WR, reset 
	//--------------------
	if (this->IsBankPrepared_r == ERESULT_TYPE_NO and this->nCnt_RCD == tRCD-1) {	// Now activated. Not yet RD/WR
		this->IsBankPrepared_r = ERESULT_TYPE_YES;
	}
	else if (this->eMemCmd == EMEM_CMD_TYPE_RD) {
		this->IsBankPrepared_r = ERESULT_TYPE_NO;
	}
	else if (this->eMemCmd == EMEM_CMD_TYPE_WR) {
		this->IsBankPrepared_r = ERESULT_TYPE_NO;
	}
	else if (this->eMemCmd == EMEM_CMD_TYPE_PRE) {
		this->IsBankPrepared_r = ERESULT_TYPE_NO;
	};


	//--------------------------------------------------
	// Counter
	// 	Counter value is dependent on cmd and counter value
	//--------------------------------------------------
	
	//--------------------------------------------------
	// RCD (ACT2RD. ACT2WR) Activating. Auto state change
	//--------------------------------------------------
	//	Initially, 0
	//	Min 0, Max tRCD-1
	//	At ACT,    start increase 
	//	At tRCD-1, reset 0
	//--------------------------------------------------
	if (this->eMemCmd == EMEM_CMD_TYPE_ACT) {
		this->nCnt_RCD = 1;					// Start increase	
	}
	else if (this->nCnt_RCD == tRCD-1) {				// Activating finish 
		this->nCnt_RCD = 0;					// Reset 
	}
	else if (this->nCnt_RCD >= 1 and this->nCnt_RCD < tRCD-1) {	// Activating on-going 
		this->nCnt_RCD ++;					// Increase
	};


	//--------------------------------------------------
	// RP (PRE2ACT) Precharging. Auto state change
	//--------------------------------------------------
	//	Initially, tRP
	//	Min 0, Max tRP
	//	At PRE, start increase
	//	At ACT, reset 0
	//--------------------------------------------------
	if (this->eMemCmd == EMEM_CMD_TYPE_PRE) {
		this->nCnt_RP = 1;					// Start increase	
	}
	else if (this->eMemCmd == EMEM_CMD_TYPE_ACT) {
		this->nCnt_RP = 0;					// Reset 
	}
	else if (this->nCnt_RP >= 1 and this->nCnt_RP < tRP) {		// Precharging on-going 
		this->nCnt_RP ++;					// Increase
	};


	//--------------------------------------------------
	// RAS (ACT2PRE)
	//--------------------------------------------------
	//	Initially, 0
	//	Min 0, Max any
	//	At ACT, start increase
	//	At PRE, reset 0
	//--------------------------------------------------
	if (this->eMemCmd == EMEM_CMD_TYPE_ACT) {
		this->nCnt_RAS = 1;					/// Start increase	
	}
	else if (this->eMemCmd == EMEM_CMD_TYPE_PRE) {
		this->nCnt_RAS = 0;					/// Reset 
	}
	else if (this->nCnt_RAS >= 1) {
		this->nCnt_RAS ++;					/// Increase
	};


	//--------------------------------------------------
	// RTP (RD2PRE)
	//--------------------------------------------------
	//	Initially, 0
	//	Min 0, Max any
	//	At RD,  start increase 
	//	At PRE, reset 0
	//--------------------------------------------------
	if (this->eMemCmd == EMEM_CMD_TYPE_RD) {
		this->nCnt_RTP = 1;					// Start increase	
	}
	else if (this->eMemCmd == EMEM_CMD_TYPE_PRE) {
		this->nCnt_RTP = 0;					// Reset	
	}
	else if (this->nCnt_RTP >= 1) {					// RD issued
		this->nCnt_RTP ++;					// Increase
	};


	//--------------------------------------------------
	// WR (WR2PRE)
	//--------------------------------------------------
	//	Initially, 0
	//	Min 0, Max any
	//	At WR,  start increase 
	//	At PRE, reset 0
	//--------------------------------------------------
	if (this->eMemCmd == EMEM_CMD_TYPE_WR) {
		this->nCnt_WR = 1;					// Start increase	
	} 
	else if (this->eMemCmd == EMEM_CMD_TYPE_PRE) {
		this->nCnt_WR = 0;					// Reset	
	}
	else if (this->nCnt_WR >= 1) {					// WR issued
		this->nCnt_WR ++;					// Increase	
	};


	//--------------------------------------------------
	// CL (RD2DATA) 
	//--------------------------------------------------
	//	Initially, 0
	//	Min 0, Max tCL
	//	At RD, start increase
	//	At tCL, reset 0
	//--------------------------------------------------
	if (this->eMemCmd == EMEM_CMD_TYPE_RD) {
		this->nCnt_CL = 1;					// Start increase	
	}
	else if (this->nCnt_CL == tCL) {				// RD finished
		this->nCnt_CL = 0;					// Reset 
	}
	else if (this->nCnt_CL >= 1 and this->nCnt_CL < tCL) {		// RD issued
		this->nCnt_CL ++;					// Increase
	};


	//--------------------------------------------------
	// WL (WR2DATA) 
	//--------------------------------------------------
	//	Initially, 0
	//	Min 0, Max tWL
	//	At WR, start increase
	//	At tWL, reset 0
	//--------------------------------------------------
	if (this->eMemCmd == EMEM_CMD_TYPE_WR) {
		this->nCnt_WL = 1;					// Start increase	
	}
	else if (this->nCnt_WL == tWL) {				// WR finished
		this->nCnt_WL = 0;					// Reset 
	}
	else if (this->nCnt_WL >= 1 and this->nCnt_WL < tWL) {		// WR issued
		this->nCnt_WL ++;					// Increase
	};


	//--------------------------------------------------
	// CCD (RD2RD, WR2WR) 
	//--------------------------------------------------
	//	Initially, 0
	//	Min 0, Max tCCD
	//	At RD/WR, start increase
	//	At tCCD,  reset 0 
	//--------------------------------------------------
	if (this->eMemCmd == EMEM_CMD_TYPE_RD) {
		this->nCnt_CCD = 1;					// Start increase	
	}
	else if (this->eMemCmd == EMEM_CMD_TYPE_WR) {
		this->nCnt_CCD = 1;					// Start increase	
	}
	else if (this->nCnt_CCD == tCCD) {				// RD/WR finished
		this->nCnt_CCD = 0;					// Increase
	}
	else if (this->nCnt_CCD >= 1 and this->nCnt_CCD < tCCD) {	// RD/WR issued
		this->nCnt_CCD ++;					// Increase
	};

	return (ERESULT_TYPE_SUCCESS);
};


//-------------------------------
// Check timing
// 	Use this before UpdateState
//-------------------------------
EResultType CBank::CheckCmdReady() {

	if (this->eMemState == EMEM_STATE_TYPE_IDLE) {
	
		assert (this->IsACT_ready() == ERESULT_TYPE_YES);
		assert (this->IsPRE_ready() == ERESULT_TYPE_NO);
		assert (this->IsRD_ready()  == ERESULT_TYPE_NO);
		assert (this->IsWR_ready()  == ERESULT_TYPE_NO);
	} 
	else if (this->eMemState == EMEM_STATE_TYPE_ACTIVATING) {

		assert (this->IsACT_ready() == ERESULT_TYPE_NO);
		assert (this->IsPRE_ready() == ERESULT_TYPE_NO);
		assert (this->IsRD_ready()  == ERESULT_TYPE_NO);
		assert (this->IsWR_ready()  == ERESULT_TYPE_NO);
	} 
	else if (this->eMemState == EMEM_STATE_TYPE_ACTIVE) {

		assert (this->IsACT_ready() == ERESULT_TYPE_NO);
	} 
	else if (this->eMemState == EMEM_STATE_TYPE_PRECHARGING) {

		assert (this->IsACT_ready() == ERESULT_TYPE_NO);
		assert (this->IsPRE_ready() == ERESULT_TYPE_NO);
		assert (this->IsRD_ready()  == ERESULT_TYPE_NO);
		assert (this->IsWR_ready()  == ERESULT_TYPE_NO);
	} 
	else {
		assert (0);
	};

	return (ERESULT_TYPE_SUCCESS);
};


//-------------------------------------
// Check timing at this cycle
// 	Use this before UpdateState
// 	FIXME Check bug
//-------------------------------------
EResultType CBank::CheckCnt() {

	assert (this->nCnt_RCD >= 0 and this->nCnt_RCD <  tRCD);	// ACT2RD. ACT2WR. Activating.
	assert (this->nCnt_CL  >= 0 and this->nCnt_CL  <= tCL);		// RD2DATA
	assert (this->nCnt_WL  >= 0 and this->nCnt_WL  <= tWL);		// WR2DATA
	assert (this->nCnt_CCD >= 0 and this->nCnt_CCD <= tCCD);	// RD2RD. WR2WR.
	assert (this->nCnt_RTP >= 0);					// RD2PRE. Max any
	assert (this->nCnt_WR  >= 0);					// WR2PRE. Max any
	assert (this->nCnt_RP  >= 0 and this->nCnt_RP  <= tRP);		// PRE2ACT. Precharging
	assert (this->nCnt_RAS >= 0);					// ACT2PRE. Max any

	if (this->eMemState == EMEM_STATE_TYPE_IDLE) {

		assert (this->nCnt_RCD == 0);				// ACT2RD. ACT2WR. Activating.
		assert (this->nCnt_CL  == 0);				// RD2DATA
		assert (this->nCnt_WL  == 0);				// WR2DATA
		assert (this->nCnt_CCD == 0);				// RD2RD. WR2WR.
		assert (this->nCnt_RTP == 0);				// RD2PRE
		assert (this->nCnt_WR  == 0);				// WR2PRE
		assert (this->nCnt_RP  == tRP);				// PRE2ACT. Precharging
		assert (this->nCnt_RAS == 0);				// ACT2PRE
		assert (this->nActivatedRow == -1);
	} 
	else if (this->eMemState == EMEM_STATE_TYPE_ACTIVATING) {

		assert (this->nCnt_RCD > 0 and this->nCnt_RCD < tRCD);	// ACT2RD. ACT2WR. Activating.
		assert (this->nCnt_CL  == 0);				// RD2DATA
		assert (this->nCnt_WL  == 0);				// WR2DATA
		assert (this->nCnt_CCD == 0);				// RD2RD. WR2WR.
		assert (this->nCnt_RTP == 0);				// RD2PRE
		assert (this->nCnt_WR  == 0);				// WR2PRE
		assert (this->nCnt_RP  == 0);				// PRE2ACT. Precharging
		assert (this->nCnt_RAS > 0 and this->nCnt_RAS < tRAS);	// ACT2PRE
		assert (this->nActivatedRow == -1);
	} 
	else if (this->eMemState == EMEM_STATE_TYPE_ACTIVE) {

		assert (this->nCnt_RCD == 0);				// ACT2RD. ACT2WR. Activating.
		assert (this->nActivatedRow >= 0);
	} 
	else if (this->eMemState == EMEM_STATE_TYPE_PRECHARGING) {

		assert (this->nCnt_RCD == 0);				// ACT2RD. ACT2WR. Activating.
		// assert (this->nCnt_CL  == 0);			// RD2DATA. FIXME irrelavnat
		// assert (this->nCnt_WL  == 0);			// WR2DATA. FIXME irrelavnat
		// assert (this->nCnt_CCD == 0);			// RD2RD. WR2WR.
		assert (this->nCnt_RTP == 0);				// RD2PRE
		assert (this->nCnt_WR  == 0);				// WR2PRE
		assert (this->nCnt_RP  > 0 and this->nCnt_RP < tRP);	// PRE2ACT. Precharging
		assert (this->nCnt_RAS == 0);				// ACT2PRE
		assert (this->nActivatedRow == -1);
	} 
	else {
		assert (0);
	};

	return (ERESULT_TYPE_SUCCESS);
};


// Debug
EResultType CBank::Display() {

	return (ERESULT_TYPE_SUCCESS);
};

