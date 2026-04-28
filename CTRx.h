//---------------------------------------------------
// Filename	: CTRx.h
// Version	: 0.7
// DATE		: 28 Feb 2018
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Tx Rx header
//---------------------------------------------------
#ifndef CTRX_H
#define CTRX_H

#include <string>
#include <stdio.h>
#include "UD_Bus.h"

using namespace std;

//--------------------------------
// Tx Rx class
//--------------------------------
typedef class CTRx* CPTRx;
class CTRx{

public:
	// 1. Contructor and Destructor
	CTRx(string cName, ETRxType eTRxType, EPktType ePktType);
	~CTRx();

	// 2. Control
	// Set value
	EResultType 	PutAx(CPAxPkt cpPkt);
	EResultType 	PutR(CPRPkt cpPkt);
	EResultType 	PutW(CPWPkt cpPkt);
	EResultType 	PutB(CPBPkt cpPkt);
	#ifdef CCI_ON
		EResultType 	PutAC(CPACPkt cpPkt);
		EResultType 	PutCD(CPCDPkt cpPkt);
		EResultType 	PutCR(CPCRPkt cpPkt);
	#endif
	EResultType 	SetAcceptResult(EResultType eResult);
	EResultType 	SetPair(CPTRx cpTRx);

	// Get value
	string			GetName();
	EResultType 	GetAcceptResult();
	EPktType 		GetPktType();
	ETRxType 		GetTRxType();
	CPTRx			GetPair();
	CPAxPkt			GetAx();
	CPRPkt			GetR();
	CPWPkt			GetW();
	CPBPkt			GetB();
	#ifdef CCI_ON
		CPACPkt		GetAC();
		CPCDPkt		GetCD();
		CPCRPkt		GetCR();
	#endif

	EResultType	IsPass();
	EResultType	IsIdle();
	EResultType	IsBusy();
	EResultType IsFirstValid();

	// Control
	EResultType 	Reset();
	EResultType 	UpdateState();
	EResultType 	FlushPkt();

	// Debug
	EResultType 	Display();

private:
	// Packet to transmit 
	CPAxPkt 	cpAx;
	CPRPkt 		cpR;
	CPWPkt 		cpW;
	CPBPkt 		cpB;
	#ifdef CCI_ON
		CPACPkt		cpAC;
		CPCDPkt		cpCD;
		CPCRPkt		cpCR;
	#endif

	// Control info
	string		cName;
	ETRxType	eTRxType;
	EPktType	ePktType;
	EStateType	eState;
	EStateType	eState_D;
	EResultType	eAcceptResult;
	EResultType	eAcceptResult_D;

	// Pair TRx
	CPTRx		cpPair;
};

#endif

