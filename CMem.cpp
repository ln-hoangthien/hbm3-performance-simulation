//-----------------------------------------------------------
// FileName	: CMem.cpp
// Version	: 0.71
// Date		: 18 Nov 2019
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Memory wrapper definition
//-----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <string>

#include "CMem.h"


// Construct
CMem::CMem(string cName) {

	// Generate Bank 
	for (int i=0; i<BANK_NUM; i++) {
		char cBankName[50];
		sprintf(cBankName, "Bank%d",i);
		this->cpBank[i] = new CBank(cBankName);
	};

	// Initialize
	this->cName  = cName;

	//this->nCnt_RD2Data  = -1;
	for (int i=0; i<BANK_NUM/BanksPerBGroup; i++) this->nCnt_CCD[i]	 = -1;		// Bank Group. Column-to-Column delay. RD2RD. WR2WR
	for (int i=0; i<BANK_NUM/BanksPerBGroup; i++) this->nCnt_RRD[i]  = -1;		// Bank Group. ACT2ACT
	for (int i=0; i<BANK_NUM/BanksPerBGroup; i++) this->nCnt_WTR[i] = -1;		// Bank Group. WR2RD

	// Generate cmd pkt
	this->spMemCmdPkt = new SMemCmdPkt;

	// Generate bank status pkt
	this->spMemStatePkt = new SMemStatePkt;
};


// Destruct
CMem::~CMem() {

	// Delete banks
	for (int i=0; i<BANK_NUM; i++) {
		delete (this->cpBank[i]);
		this->cpBank[i] = NULL;
	};

	// Delete cmd pkt
	delete (this->spMemCmdPkt);
	this->spMemCmdPkt = NULL;

	// Delete bank status pkt
	delete (this->spMemStatePkt);
	this->spMemStatePkt = NULL;
};


// Reset
EResultType CMem::Reset() {

	// Initialize banks
	for (int i=0; i<BANK_NUM; i++) {
		this->cpBank[i]->Reset();
	};

	// Initialize cnt
	for (int i=0; i<BANK_NUM/BanksPerBGroup; i++) this->nCnt_CCD[i]	= tCCD; // Global. RD2RD. WR2WR
	for (int i=0; i<BANK_NUM/BanksPerBGroup; i++) this->nCnt_RRD[i]	= tRRD; // Global. ACT2ACT diff bank
	for (int i=0; i<BANK_NUM/BanksPerBGroup; i++) this->nCnt_WTR[i]	= tWTR; // Global. WR2RD diff bank

	// mem cmd pkt
	this->spMemCmdPkt->eMemCmd = EMEM_CMD_TYPE_NOP;
	this->spMemCmdPkt->nBank   = -1;
	this->spMemCmdPkt->nRow    = -1;

	// Bank status pkt
	for (int i=0; i<BANK_NUM; i++) {
		this->spMemStatePkt->eMemState[i]         = EMEM_STATE_TYPE_IDLE;
		this->spMemStatePkt->IsRD_ready[i]        = ERESULT_TYPE_NO;
		this->spMemStatePkt->IsWR_ready[i]        = ERESULT_TYPE_NO;
		this->spMemStatePkt->IsFirstData_ready[i] = ERESULT_TYPE_NO;
		this->spMemStatePkt->IsBankPrepared[i] 	  = ERESULT_TYPE_NO;
		this->spMemStatePkt->IsACT_ready[i]       = ERESULT_TYPE_NO;
		this->spMemStatePkt->IsPRE_ready[i]       = ERESULT_TYPE_NO;
		this->spMemStatePkt->nActivatedRow[i]     = -1; 
	};

	return (ERESULT_TYPE_SUCCESS);
};


// Set mem cmd pkt in current cycle
EResultType CMem::SetMemCmdPkt(SPMemCmdPkt spMemCmdPkt) {

	EMemCmdType eCmd = spMemCmdPkt->eMemCmd;
	int nBank = spMemCmdPkt->nBank;
	int nRow  = spMemCmdPkt->nRow;

	#ifdef DEBUG
	if (nBank == -1 or nRow == -1) {
		assert (eCmd == EMEM_CMD_TYPE_NOP);
	};
	#endif

	// Set cmd pkt
	this->spMemCmdPkt->eMemCmd = eCmd;
	this->spMemCmdPkt->nBank   = nBank;
	this->spMemCmdPkt->nRow    = nRow;

	// Set cmd 
	if (eCmd == EMEM_CMD_TYPE_NOP) {
		// Set NOP for all banks
		for (int i=0; i<BANK_NUM; i++) {
			this->cpBank[i]->SetMemCmd(eCmd);
		};
	}
	else {	
		#ifdef DEBUG
		assert (nBank >= 0);
		#endif

		// Set cmd for target bank
		for (int i=0; i<BANK_NUM; i++) {
			if (i == nBank) {
				this->cpBank[i]->SetMemCmd(eCmd);
				this->cpBank[i]->SetMemAddr(nBank, nRow);
			}
			else {
				this->cpBank[i]->SetMemCmd(EMEM_CMD_TYPE_NOP);
			};
		};
	};

	return (ERESULT_TYPE_SUCCESS);
};


// Set mem cmd pkt in current cycle
EResultType CMem::SetMemCmdPkt(EMemCmdType eCmd, int nBank, int nRow) {

	#ifdef DEBUG
	if (nBank == -1 or nRow == -1) {
		assert (eCmd == EMEM_CMD_TYPE_NOP);
	};
	#endif

	// Set cmd pkt
	this->spMemCmdPkt->eMemCmd = eCmd;
	this->spMemCmdPkt->nBank   = nBank;
	this->spMemCmdPkt->nRow    = nRow;

	if (this->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_REFpb) {

		#ifdef DEBUG
		assert(Refresh_Counter.size() <= BANK_NUM);
		#endif

		for (int i=0; i<static_cast<int>(Refresh_Counter.size()); i++) {
			assert(Refresh_Counter[i] != this->spMemCmdPkt->nBank); // There should not be more than 1 REF command in tREFI
		};

		Refresh_Counter.push_back(this->spMemCmdPkt->nBank);

		if (Refresh_Counter.size() == BANK_NUM) {
			Refresh_Counter.erase(Refresh_Counter.begin());
		}

	}

	// Set cmd
	if (nBank == -1) {

		#ifdef DEBUG
		assert (eCmd == EMEM_CMD_TYPE_NOP);
		#endif

		// Set NOP fo all banks
		for (int i=0; i<BANK_NUM; i++) {
			this->cpBank[i]->SetMemCmd(eCmd);
		};
	}
	else {	
		#ifdef DEBUG
		assert (nBank >= 0);
		#endif

		// Set cmd for target bank
		for (int i=0; i<BANK_NUM; i++) {
			if (i == nBank) {
				this->cpBank[i]->SetMemCmd(eCmd);
				this->cpBank[i]->SetMemAddr(i, nRow);
			}
			else {
				this->cpBank[i]->SetMemCmd(EMEM_CMD_TYPE_NOP);
			};
		};
	};

	return (ERESULT_TYPE_SUCCESS);
};


// Get mem name
string CMem::GetName() {
	return (this->cName);
};

int CMem::Get_tCCD (int bank, int nbank) {
	bool IsSameBankGroup = (std::floor(bank/BanksPerBGroup) == std::floor(nbank/BanksPerBGroup));
	bool IsSameSID = (bank/BanksPerSID == nbank/BanksPerSID);

	// Return the CCD timing based on the current configuration
	if (IsSameBankGroup) {
		return tCCDL; // Same bank group
	} else if (IsSameSID) {
		return tCCDS; // Same stack ID
	} else {
		return tCCDR; // Different stack ID
	}
};

int CMem::Get_tRRD (int bank, int nbank) {
	bool IsSameBankGroup = (std::floor(bank/BanksPerBGroup) == std::floor(nbank/BanksPerBGroup));

	// Return the WTR timing based on the current configuration
	if (IsSameBankGroup) {
		return tRRDL; // Same bank group
	} else {
		return tRRDS; // Different bank group
	}
};

int CMem::Get_tWTR (int bank, int nbank) {
	bool IsSameBankGroup = (std::floor(bank/BanksPerBGroup) == std::floor(nbank/BanksPerBGroup));

	// Return the WTR timing based on the current configuration
	if (IsSameBankGroup) {
		return tWTRL; // Same bank group
	} else {
		return tWTRS; // Different bank group
	}
};

// Check RD cmd ready
EResultType CMem::IsRD_global_ready(int bank) {
	for (int nbank=0; nbank<BANK_NUM; nbank++) {
		int check_bankgroup = std::floor(nbank/BanksPerBGroup);
		if (this->nCnt_CCD[check_bankgroup] < Get_tCCD(bank, nbank) or 
			this->nCnt_WTR[check_bankgroup] < Get_tWTR(bank, nbank)) {
			return (ERESULT_TYPE_NO);
		}
	};
	return (ERESULT_TYPE_YES);
};


// Check WR cmd ready
EResultType CMem::IsWR_global_ready(int bank) {
	for (int nbank=0; nbank<BANK_NUM; nbank++) {
		int check_bankgroup = std::floor(nbank/BanksPerBGroup);
		if (this->nCnt_CCD[check_bankgroup] < Get_tCCD(bank, nbank) or
			this->nCnt_RTW[check_bankgroup] < tRTW)
			return (ERESULT_TYPE_NO);
	};
	return (ERESULT_TYPE_YES);
};


// Check ACT cmd ready
EResultType CMem::IsACT_global_ready(int bank) {
	for (int nbank=0; nbank<BANK_NUM; nbank++) {
		
		int check_bankgroup = std::floor(nbank/BanksPerBGroup);

		if (this->nCnt_RRD[check_bankgroup] < Get_tRRD(bank, nbank) or
			this->nCnt_RREFD                < tRREFD)

			return (ERESULT_TYPE_NO);

	};
	return (ERESULT_TYPE_YES);
};

// Check ACT cmd ready
EResultType CMem::IsREF_global_ready(int bank) {

	if (this->nCnt_RREFD < tRREFD) {
		
			return (ERESULT_TYPE_NO);

	};
	return (ERESULT_TYPE_YES);
};


// Get mem status info
SPMemStatePkt CMem::GetMemStatePkt() {

	// Bank status pkt
	for (int i=0; i<BANK_NUM; i++) {
		// Obtain local info
		this->spMemStatePkt->eMemState[i]          	= this->cpBank[i]->GetMemState();
		this->spMemStatePkt->IsRD_ready[i]         	= this->cpBank[i]->IsRD_ready();
		this->spMemStatePkt->IsWR_ready[i]         	= this->cpBank[i]->IsWR_ready();
		this->spMemStatePkt->IsPRE_ready[i]        	= this->cpBank[i]->IsPRE_ready();
		this->spMemStatePkt->IsACT_ready[i]        	= this->cpBank[i]->IsACT_ready();
		this->spMemStatePkt->IsREF_ready[i]      	= this->cpBank[i]->IsREF_ready();
		this->spMemStatePkt->forced_PRE[i]         	= this->cpBank[i]->forced_PRE();
		this->spMemStatePkt->forced_REFI[i]         = this->cpBank[i]->forced_REFI();
		this->spMemStatePkt->IsFirstData_ready[i]	= this->cpBank[i]->IsFirstData_ready();
		this->spMemStatePkt->IsBankPrepared[i]     	= this->cpBank[i]->IsBankPrepared();
		this->spMemStatePkt->nActivatedRow[i]      	= this->cpBank[i]->GetActivatedRow();
		
		// Obtain global info and override 
		if (this->IsRD_global_ready(i) == ERESULT_TYPE_NO) {
			this->spMemStatePkt->IsRD_ready[i] = ERESULT_TYPE_NO;
		};
		if (this->IsWR_global_ready(i) == ERESULT_TYPE_NO) {
			this->spMemStatePkt->IsWR_ready[i] = ERESULT_TYPE_NO;
		};
		if (this->IsACT_global_ready(i) == ERESULT_TYPE_NO) {
			this->spMemStatePkt->IsACT_ready[i] = ERESULT_TYPE_NO;
		};
		if (this->IsREF_global_ready(i) == ERESULT_TYPE_NO) {
			this->spMemStatePkt->IsREF_ready[i] = ERESULT_TYPE_NO;
		};

		// Check data channel busy 
		if (this->cpBank[i]->IsData_busy() == ERESULT_TYPE_YES) {
			this->spMemStatePkt->IsData_busy = ERESULT_TYPE_YES;
		}
		else {
			this->spMemStatePkt->IsData_busy = ERESULT_TYPE_NO;
		};
	};
	return (this->spMemStatePkt);
};


// Check data available
// EResultType CMem::IsFirstData_global_ready() {
//	
//	if (this->nCnt_RD2Data == tRD2DATA) {
//		return (ERESULT_TYPE_YES);	
//	};
//	return (ERESULT_TYPE_NO);	
// };


// Update state
// cycle counter
EResultType CMem::UpdateState() {

	int nBankGroup = static_cast<int>(std::floor(this->spMemCmdPkt->nBank/BanksPerBGroup));
	// Update banks
	for (int i=0; i<BANK_NUM; i++) {
		this->cpBank[i]->UpdateState();
	};

	// Update global counter CCD
	// Assumption: CCD covers RD2RD, RD2WR, WR2RD, WR2WR 
	if (this->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_RD or this->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_WR) {
		this->nCnt_CCD[nBankGroup] = 1;  // Global. RD2RD. WR2WR
	}
	else {
		for (int i=0; i<BANK_NUM/BanksPerBGroup; i++) {
			this->nCnt_CCD[i] ++; // obliviously increase
		}
	};

	if (this->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_WR) {
		this->nCnt_WTR[nBankGroup] = 1;  // Global. RD2RD. WR2WR
	}
	else {
		for (int i=0; i<BANK_NUM/BanksPerBGroup; i++) {
			this->nCnt_WTR[i] ++; // obliviously increase
		}
	};

	if (this->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_RD) {
		this->nCnt_RTW[nBankGroup] = 1;  // Global. RD2RD. WR2WR
	}
	else {
		for (int i=0; i<BANK_NUM/BanksPerBGroup; i++) {
			this->nCnt_RTW[i] ++; // obliviously increase
		}
	};

	// Update global counter RRD
	if (this->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_ACT) {
		this->nCnt_RRD[nBankGroup] = 1;  // Global. ACT2ACT diff bank
	}
	else {
		for (int i=0; i<BANK_NUM/BanksPerBGroup; i++) this->nCnt_RRD[i] ++; // obliviously increase
	};

	// Update global counter RRD
	if (this->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_REFpb) {
		this->nCnt_RREFD = 1;  // Global. ACT2ACT diff bank
	}
	else {
		this->nCnt_RREFD ++; // obliviously increase
	};

	return (ERESULT_TYPE_SUCCESS);
};


// Debug
EResultType CMem::Display() { 

	return (ERESULT_TYPE_SUCCESS);
};

