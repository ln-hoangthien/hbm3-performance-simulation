//------------------------------------------------------------
// FileName	: CMem.h
// Version	: 0.7
// DATE 	: 3 Mar 2018
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Memory wrapper header 
//------------------------------------------------------------
#ifndef CMem_H
#define CMem_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#include "Top.h"
#include "Memory.h"
#include "CBank.h"

using namespace std;

//---------------------------
// Mem bank class
//---------------------------
typedef class CMem* CPMem;
class CMem{

public:
	// 1. Contructor and Destructor
	CMem(string cName);
	~CMem();

	// 2. Control
	EResultType	Reset();

	// Set value
	EResultType	SetMemCmdPkt(EMemCmdType eMemCmd, int nBank, int nRow);
	EResultType	SetMemCmdPkt(SPMemCmdPkt spMemCmdPkt);

	// Get value
	string		GetName();

	SPMemStatePkt	GetMemStatePkt();

	EResultType	IsRD_global_ready();						// Global CCD (RD2RD)
	EResultType	IsWR_global_ready();						// Global CCD (WR2WR)
	EResultType	IsACT_global_ready();						// Global RRD (ACT2ACT)
	// EResultType	IsFirstData_global_ready();
	// EResultType	IsData_global_busy();
	
	// Control
	EResultType	UpdateState();

	// Debug
	// EResultType	CheckCnt();
	EResultType	Display();

	// Memory
	CPBank		cpBank[BANK_NUM];

private:
	// Original info 
	string		cName;								// Mem name

	// Cmd 
	SPMemCmdPkt	spMemCmdPkt;							// cmd, addr
	
	// Mem status
	SPMemStatePkt	spMemStatePkt;							// state, ready (RD,ACT,PRE,DATA), ActivatedRow

	// Control state
	int		nCnt_CCD;							// Global. RD2RD. WR2WR	(all banks)
	int		nCnt_RRD;							// Global. ACT2ACT (all banks)	
	// int		nCnt_RD2Data;							// Cycle counter to wait for data
};

#endif

