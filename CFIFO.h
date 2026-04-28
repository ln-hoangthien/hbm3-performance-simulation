//------------------------------------------------------------
// Filename	: CFIFO.h 
// Version	: 0.7
// Date		: 28 Feb 2018
// Contact	: JaeYoung.Hur@gmail.com
// Description	: FIFO type header
//------------------------------------------------------------
#ifndef CFIFO_H
#define CFIFO_H

#include <string>

#include "UD_Bus.h"

using namespace std;

//----------------------------
// FIFO class
//----------------------------
typedef class CFIFO* CPFIFO;
class CFIFO{

public:
        // 1. Contructor and Destructor
	CFIFO(string cName, EUDType eType, int nMaxNum);
	~CFIFO();

	// 2. Control
	// Control	
	EResultType	Reset();

	// Set value
	EResultType	Push(UPUD upUD, int nLatency);
	EResultType	Push(UPUD upUD);
	UPUD		Pop();

	EResultType	UpdateState();	
	
	// Get value
	EUDType		GetUDType();
	int			GetCurNum();
	int			GetMaxNum();
	UPUD		GetTop();
	EResultType	IsEmpty();
	EResultType	IsFull();

	// Debug
	EResultType	CheckFIFO();
	EResultType	Display();

private:
	// Original info
	string		cName;
	EUDType		eUDType;
	int			nCurNum;
	int			nMaxNum;

	// Node 
	SPLinkedUD	spUDList_head;
	SPLinkedUD	spUDList_tail;
};

#endif

