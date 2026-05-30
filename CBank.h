//------------------------------------------------------------
// FileName	: CBank.h
// Version	: 0.8
// DATE 	: 9 May 2019
// Contact	: JaeYoung.Hur@gmail.com
//------------------------------------------------------------
// Description	: Memory bank FSM class header 
//------------------------------------------------------------
#ifndef CBank_H
#define CBank_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#include "Top.h"
#include "Memory.h"


using namespace std;


// Mem bank class
typedef class CBank* CPBank;
class CBank{

public:
	// 1. Contructor and Destructor
	CBank(string cName);
	~CBank();

	// 2. Control
	// Control
	EResultType	Reset();

	// Set value
	EResultType	SetMemCmd(EMemCmdType eMemCmd);
	EResultType	SetMemAddr(int nBankCmd, int nRowCmd);

public:
	// Get value
	string		GetName();
	EMemStateType	GetMemState();	
	int		GetActivatedRow();

	EResultType	IsRD_ready();		// This bank can get RD cmd.
	EResultType	IsWR_ready();
	EResultType	IsPRE_ready();
	EResultType	IsACT_ready();
	EResultType	IsFirstData_ready();	// This bank can put first data.
	EResultType	IsBankPrepared();	// This bank is activated. 
	
	// Control
	EResultType	UpdateState();

	// Debug
	EResultType	CheckCmdReady();
	EResultType	CheckCnt();
	EResultType	Display();

	// Stat
	EResultType	IsData_busy();

private:
	// Original
	string		cName;			// Bank name

	// Control cmd
	EMemCmdType	eMemCmd;		// Set by MC 
	int		nBankCmd;
	int		nRowCmd;

	// Control state
	EMemStateType	eMemState;
	int		nActivatedRow;		// Active row number

	int		nCnt_RP; 		// PRE2ACT.        Per-bank. Auto (Precharging)
	int		nCnt_RAS;		// ACT2PRE.        Per-bank (wait PRE).
	int		nCnt_RCD;		// ACT2RD. ACT2WR. Per-bank. Auto (Activating). Register

	int		nCnt_RTP;		// RD2PRE.         Per-bank (wait PRE)
	int		nCnt_WR; 		// WR2PRE.         Per-bank (wait PRE)

	int		nCnt_CL; 		// RD2DATA.        Per-bank (wait data)  Also global
	int		nCnt_WL; 		// WR2DATA.        Per-bank (wait data)  Also global

	int		nCnt_CCD;		// RD2RD. WR2WR.   Per-bank (wait RD/WR) Also global

	EResultType	IsCmd_overlap_r;	// Doing service for two RD or WR cmds. Register
	EResultType	IsBankPrepared_r;	// Bank prepared (PRE, ACT). Not yet RD/WR 

	// Stat
	int		nCnt_Data;		// Data counter for transaction under service		
};

#endif

