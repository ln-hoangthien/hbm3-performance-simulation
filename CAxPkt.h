//------------------------------------------------------------
// FileName	: CAxPkt.h
// Version	: 0.80
// DATE 	: 1 Nov 2021
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Ax packet class header 
//------------------------------------------------------------
// Note
// (1) 64-bit addrress. <stdint.h> int64_t. %lx, %ld in printf
// (2) When new member added, 
//      (a) modify "Copy_AxPkt" function in "UD_Bus.cpp" 
//      (b) set value when generated
//------------------------------------------------------------
#ifndef CAXPKT_H
#define CAXPKT_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#include "Top.h"
#include "Memory.h"

using namespace std;

//-----------------------------------------------
// Raw Ax pkt structure
//-----------------------------------------------
typedef struct tagSAxPkt* SPAxPkt;
typedef struct tagSAxPkt{
	int     nID;
	int64_t nAddr;
	int     nLen;
	#ifdef CCI_ON
		int nSnoop;
	#endif
}SAxPkt;


//-----------------------------------------------
// Ax pkt class
//-----------------------------------------------
typedef class CAxPkt* CPAxPkt;
class CAxPkt{

public:
	// 1. Contructor and Destructor
	CAxPkt(string cName, ETransDirType eDir);
	CAxPkt(ETransDirType eDir);
	~CAxPkt();

	// 2. Control
	// Set value
	// EResultType	SetPkt(SPAxPkt spPkt_new);
	EResultType	SetPkt(int nID, int64_t nAddr, int nLen);
	EResultType	SetName(string cName);

	#ifdef CCI_ON
		EResultType	SetPkt(int nID, int64_t nAddr, int nLen, int nSnoop);
		EResultType	SetSnoop(int nSnoop);
	#endif

	EResultType	SetTransDirType(ETransDirType eDir);
	EResultType	SetTransType(ETransType eType);
	EResultType	SetTransNum(int nNum);				// nTransNum
	EResultType	SetVA(int64_t nVA);				// nVA
	EResultType	SetSrcName(string cName);			// Initiator name
	EResultType	SetFinalTrans(EResultType eResult);		// Yes, No
	EResultType	SetTileNum(int nNum);				// Tile number VA (debug) 
	// EResultType	SetMemCh(int nNum);				// Mem channel 
	// EResultType	SetCacheCh(int nNum);				// Cache channel 

	EResultType	SetID(int nID);
	EResultType	SetAddr(int64_t nAddr);
	EResultType	SetLen(int nLen);

	// Get value
	string			GetName();
	SPAxPkt			GetPkt();
	ETransDirType	GetDir();
	ETransType		GetTransType();
	int				GetTransNum();					// nTransNum
	int64_t			GetVA();					// nVA 
	int				GetTileNum();					// nTileNum
	string			GetSrcName();					// cSrcName
	EResultType		IsFinalTrans();

	#ifdef CCI_ON
		int			GetSnoop();
	#endif

	int				GetID();
	int64_t			GetAddr();
	int				GetLen();

	// Get memory value
	int				GetBankNum_AMap();				// Address map
	int64_t			GetRowNum_AMap();
	int				GetColNum_AMap();
	int				GetChannelNum_AMap();

	int				GetBankNum_MMap();				// Memory map
	int64_t			GetRowNum_MMap();
	int				GetColNum_MMap();
	int				GetChannelNum_MMap();

	int				GetCacheCh();
	
	// Debug
	EResultType	CheckPkt();
	EResultType	Display();

private:
	// Original pkt info
	SPAxPkt 		spPkt;

	// Control info
	string			cName;						// Pkt name
	ETransDirType	eDir;						// Read, write direction
	ETransType		eTransType;					// Normal, PTW, evict, line fill 

	string			cSrcName;					// Who generates pkt. Master, cache, MMU, Bus
	EResultType		eFinalTrans;				// Last trans application. Yes, No 

	int64_t			nVA;						// Virtual addr
	int				nTransNum;					// Transaction number generate order
	int				nTileNum;					// Tile number VA map (Debug) 
	// int				nMemCh;					// Memory channel FIXME 
	// int				nCacheCh;				// Cache channel FIXME 
};

#endif

