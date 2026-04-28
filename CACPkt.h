//------------------------------------------------------------
// FileName	: CACPkt.h
// Version	: 0.1
// DATE 	: 1 April 2026
// Contact	: JaeYoung.Hur@gmail.com
// DesACiption	: AC packet class header 
//------------------------------------------------------------
// Note
// 1. When member added, modify "Copy_ACPkt" function in "UD_Bus.cpp"
//------------------------------------------------------------
#ifndef CACPKT_H
#define CACPKT_H

#include <stdio.h>
#include <string>
#include "Top.h"

using namespace std;

// AC pkt
typedef struct tagSACPkt* SPACPkt;
typedef struct tagSACPkt{
	int64_t nAddr;
	int 	nSnoop;
}SACPkt;

// AC pkt class
typedef class CACPkt* CPACPkt;
class CACPkt{

public:
	// 1. Contructor and Destructor
	CACPkt(string cName);
	CACPkt();
	~CACPkt();

	// 2. Control
	// Set value
	// EResultType	SetPkt(SPACPkt spPkt_new);
	EResultType	SetName(string cName);
	EResultType	SetSnoop(int nSnoop);
	EResultType	SetAddr(int64_t nAddr);
	EResultType	SetFinalTrans(EResultType eResult);

	// Get value
	SPACPkt		GetPkt();
	string		GetName();
	int			GetSnoop();
	int64_t		GetAddr();
	EResultType	IsFinalTrans();

	// Debug
	EResultType	Display();
	EResultType	CheckPkt();

private:
	// Original pkt info
	SPACPkt		spPkt;

	// Control info
	string		cName;
	EResultType	eFinalTrans;
};

#endif
