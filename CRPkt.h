//------------------------------------------------------------
// FileName	: CRPkt.h
// Version	: 0.79
// DATE 	: 1 Nov 2021
// Contact	: JaeYoung.Hur@gmail.com
// Description	: R packet class header 
//------------------------------------------------------------
// Note
// 1. When member added, modify "Copy_RPkt" function in "UD_Bus.cpp"
//------------------------------------------------------------
#ifndef CRPKT_H
#define CRPKT_H

#include <stdio.h>
#include <string>
#include "Top.h"

using namespace std;

//-----------------------------
// R pkt
//-----------------------------
typedef struct tagSRPkt* SPRPkt;
typedef struct tagSRPkt{
	int nID;
	int nData;
	int nLast;
	#ifdef CCI_ON
	int nResp;
	#endif
}SRPkt;


//-----------------------------
// R pkt class
//-----------------------------
typedef class CRPkt* CPRPkt;
class CRPkt{

public:
	// 1. Contructor and Destructor
	CRPkt(string cName);
	CRPkt();
	~CRPkt();

	// 2. Control
	// Set value
	// EResultType	SetPkt(SPRPkt spPkt_new);
	EResultType	SetPkt(int nID, int nData, int nLast);
	EResultType	SetName(string cName);
	EResultType	SetID(int nID);
	EResultType	SetData(int nData);
	EResultType	SetLast(EResultType eResult);
	#ifdef CCI_ON
		EResultType	SetPkt(int nID, int nData, int nLast, int nResp);
		EResultType	SetResp(int nResp);
	#endif
	EResultType	SetFinalTrans(EResultType eResult);
	EResultType	SetMemCh(int nMemCh);
	EResultType	SetCacheCh(int nCacheCh);

	// Get value
	string		GetName();
	SPRPkt		GetPkt();
	int			GetID();
	int			GetData();
	EResultType	IsLast();
	EResultType	IsFinalTrans();
	int			GetMemCh();
	int			GetCacheCh();
	#ifdef CCI_ON
		int	GetResp();
	#endif

	// Debug
	EResultType	CheckPkt();
	EResultType	Display();

private:
	// Original pkt info
	SPRPkt		spPkt;

	// Control info
	string		cName;
	EResultType	eFinalTrans;
	int			nMemCh;
	int			nCacheCh;
};

#endif
