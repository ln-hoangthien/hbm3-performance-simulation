//------------------------------------------------------------
// FileName	: CCDPkt.h
// Version	: 0.1
// DATE 	: 1 April 2026
// Contact	: JaeYoung.Hur@gmail.com
// DesCDiption	: CD packet class header 
//------------------------------------------------------------
// Note
// 1. When member added, modify "Copy_CDPkt" function in "UD_Bus.cpp"
//------------------------------------------------------------
#ifndef CCDPKT_H
#define CCDPKT_H

#include <stdio.h>
#include <string>
#include "Top.h"

using namespace std;

// CD pkt
typedef struct tagSCDPkt* SPCDPkt;
typedef struct tagSCDPkt{
	int nData;
	int nLast;
}SCDPkt;

// CD pkt class
typedef class CCDPkt* CPCDPkt;
class CCDPkt{

public:
	// 1. Contructor and Destructor
	CCDPkt(string cName);
	CCDPkt();
	~CCDPkt();

	// 2. Control
	// Set value
	// EResultType	SetPkt(SPCDPkt spPkt_new);
	EResultType	SetName(string cName);
	EResultType	SetPkt(int nData, int nLast);
	EResultType	SetData(int nData);
	EResultType	SetLast(EResultType eResult);
	EResultType	SetFinalTrans(EResultType eResult);

	// Get value
	SPCDPkt		GetPkt();
	string		GetName();
	int			GetData();
	EResultType	IsLast();
	EResultType	IsFinalTrans();

	// Debug
	EResultType	Display();
	EResultType	CheckPkt();

private:
	// Original pkt info
	SPCDPkt		spPkt;

	// Control info
	string		cName;
	EResultType	eFinalTrans;
};

#endif
