//------------------------------------------------------------
// FileName	: CCRPkt.h
// Version	: 0.1
// DATE 	: 1 April 2026
// Contact	: JaeYoung.Hur@gmail.com
// Description	: CR packet class header 
//------------------------------------------------------------
// Note
// 1. When member added, modify "Copy_CRPkt" function in "UD_Bus.cpp"
//------------------------------------------------------------
#ifndef CCRPKT_H
#define CCRPKT_H

#include <stdio.h>
#include <string>
#include "Top.h"

using namespace std;

// CR pkt
typedef struct tagSCRPkt* SPCRPkt;
typedef struct tagSCRPkt{
	int nResp;
}SCRPkt;

// CR pkt class
typedef class CCRPkt* CPCRPkt;
class CCRPkt{

public:
	// 1. Contructor and Destructor
	CCRPkt(string cName);
	CCRPkt();
	~CCRPkt();

	// 2. Control
	// Set value
	// EResultType	SetPkt(SPCRPkt spPkt_new);
	EResultType	SetName(string cName);
	EResultType	SetResp(int nResp);
	EResultType	SetFinalTrans(EResultType eResult);

	// Get value
	SPCRPkt		GetPkt();
	string		GetName();
	int			GetID();
	int			GetResp();
	EResultType	IsFinalTrans();

	// Debug
	EResultType	Display();
	EResultType	CheckPkt();

private:
	// Original pkt info
	SPCRPkt		spPkt;

	// Control info
	string		cName;
	EResultType	eFinalTrans;
};

#endif
