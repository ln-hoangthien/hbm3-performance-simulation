//-----------------------------------------------------------
// FileName	: CAxPkt.cpp
// Version	: 0.81
// Date		: 14 Mar 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Ax Pkt class definition
//-----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <string>

#include "CAxPkt.h"

// Construct
CAxPkt::CAxPkt(string cName, ETransDirType eDir) {

	// Generate and initialize
	// Transaction
	this->spPkt = new SAxPkt; 
	this->spPkt->nID   = -1;
	this->spPkt->nAddr = -1;
	this->spPkt->nLen  = -1;
	this->spPkt->nSnoop = -1;

	this->cName        = cName;
	this->eDir         = eDir;
	this->eTransType   = ETRANS_TYPE_UNDEFINED;
	this->eFinalTrans  = ERESULT_TYPE_NO;
	this->cSrcName     = "Src";
	this->nTransNum    = -1;
	this->nVA          = -1;
	this->nTileNum     = -1;
	// this->nMemCh    = -1;
	// this->nCacheCh  = -1;
};


// Construct
CAxPkt::CAxPkt(ETransDirType eDir) {

	// Generate and initialize
	// Transaction
	this->spPkt = new SAxPkt; 
	this->spPkt->nID   = -1;
	this->spPkt->nAddr = -1;
	this->spPkt->nLen  = -1;
	this->spPkt->nSnoop = -1;

	this->cName        = "Ax_Pkt";
	this->eDir         = eDir;
	this->eTransType   = ETRANS_TYPE_UNDEFINED;
	this->eFinalTrans  = ERESULT_TYPE_NO;
	this->cSrcName     = "Src";
	this->nTransNum    = -1;
	this->nVA          = -1;
	this->nTileNum     = -1;
	// this->nMemCh    = -1;
	// this->nCacheCh  = -1;
};


// Destruct
CAxPkt::~CAxPkt() {

	this->CheckPkt();

	delete (this->spPkt);	
	this->spPkt = NULL;
};


// Set name
EResultType CAxPkt::SetName(string cName) {

	this->cName = cName;	
	return (ERESULT_TYPE_SUCCESS);
};


// Set transaction (read or write) direction
EResultType CAxPkt::SetTransDirType(ETransDirType eDir) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->eDir = eDir;
	return (ERESULT_TYPE_SUCCESS);
};


// Set transaction type (normal, PTW, evict, line fill)
EResultType CAxPkt::SetTransType(ETransType eType) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->eTransType = eType;
	return (ERESULT_TYPE_SUCCESS);
};


//// Set Ax pkt
//// Be careful spPkt_new delete correctly
// EResultType CAxPkt::SetPkt(SPAxPkt spPkt_new) {
//
//	// Debug
//	assert (this->spPkt != NULL);
//
//	this->spPkt = spPkt_new;  // this->spPkt generated when CAxPkt generated 
//	return (ERESULT_TYPE_SUCCESS);
// };


// Set Ax pkt
EResultType CAxPkt::SetPkt(int nID, int64_t nAddr, int nLen) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nID   = nID;
	this->spPkt->nAddr = nAddr;
	this->spPkt->nLen  = nLen;
	return (ERESULT_TYPE_SUCCESS);
};

#ifdef CCI_ON
// Set Ax pkt
EResultType CAxPkt::SetPkt(int nID, int64_t nAddr, int nLen, int nSnoop) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nID   = nID;
	this->spPkt->nAddr = nAddr;
	this->spPkt->nLen  = nLen;
	this->spPkt->nSnoop  = nSnoop;
	return (ERESULT_TYPE_SUCCESS);
};

EResultType CAxPkt::SetSnoop(int nSnoop) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nSnoop  = nSnoop;
	return (ERESULT_TYPE_SUCCESS);
};
#endif

// Set Ax ID 
EResultType CAxPkt::SetID(int nID) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nID   = nID;
	return (ERESULT_TYPE_SUCCESS);
};


// Set Ax Addr 
EResultType CAxPkt::SetAddr(int64_t nAddr) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nAddr = nAddr;
	return (ERESULT_TYPE_SUCCESS);
};


// Set Ax Len 
EResultType CAxPkt::SetLen(int nLen) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->spPkt->nLen = nLen;
	return (ERESULT_TYPE_SUCCESS);
};


// Set src name 
EResultType CAxPkt::SetSrcName(string cName) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->cSrcName = cName;
	return (ERESULT_TYPE_SUCCESS);
};


// Set trans num 
EResultType CAxPkt::SetTransNum(int nNum) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->nTransNum = nNum;
	return (ERESULT_TYPE_SUCCESS);
};


// Set VA 
EResultType CAxPkt::SetVA(int64_t nVA) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->nVA = nVA;
	return (ERESULT_TYPE_SUCCESS);
};


// Set final trans 
EResultType CAxPkt::SetFinalTrans(EResultType eThis) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

 	this->eFinalTrans = eThis;  // yes or no 
	return (ERESULT_TYPE_SUCCESS);
};


// Set tile num 
EResultType CAxPkt::SetTileNum(int nNum) {

	#ifdef DEBUG
	assert (this->spPkt != NULL);
	#endif

	this->nTileNum = nNum;
	return (ERESULT_TYPE_SUCCESS);
};


// // Set mem channel num 
// EResultType CAxPkt::SetMemCh(int nNum) {
//
//	#ifdef DEBUG
//	assert (this->spPkt != NULL);
//	#endif
//
//	this->nMemCh= nNum;
//	return (ERESULT_TYPE_SUCCESS);
// };


// // Set cache channel num 
// EResultType CAxPkt::SetCacheCh(int nNum) {
//
//	#ifdef DEBUG
//	assert (this->spPkt != NULL);
//	#endif
//
//	this->nCacheCh = nNum;
//	return (ERESULT_TYPE_SUCCESS);
// };


// Get Ax pkt name
string CAxPkt::GetName() {

	// this->CheckPkt();
	return (this->cName);
};


// Get src name 
string CAxPkt::GetSrcName() {

	// this->CheckPkt();
	return (this->cSrcName);
};


// Get Ax pkt
SPAxPkt CAxPkt::GetPkt() {

	// this->CheckPkt();
	return (this->spPkt);
};


// Get direction
ETransDirType CAxPkt::GetDir() {

	// this->CheckPkt();
	return (this->eDir);
};


// Get trans type 
ETransType CAxPkt::GetTransType() {

	// this->CheckPkt();
	return (this->eTransType);
};


// Get ID
int CAxPkt::GetID() {

	// this->CheckPkt();
	return (this->spPkt->nID);
};


// Get Address
int64_t CAxPkt::GetAddr() {

	// this->CheckPkt();

	return (this->spPkt->nAddr);
};


// Get burst length
int CAxPkt::GetLen() {

	// this->CheckPkt();
	return (this->spPkt->nLen);
};


// Get VA 
int64_t CAxPkt::GetVA() {

	// this->CheckPkt();
	return (this->nVA);
};


// Get tile num 
int CAxPkt::GetTileNum() {

	// this->CheckPkt();
	return (this->nTileNum);
};


// // Get mem channel num 
// int CAxPkt::GetMemCh() {
//
//	// this->CheckPkt();
//	return (this->nMemCh);
// };


// Get cache channel num 
int CAxPkt::GetCacheCh() {

	// this->CheckPkt();
	int64_t nAddr = this->GetAddr();

	int nCache = GetCacheChNum_Global(nAddr);

	return (nCache);
};


// Get final trans 
EResultType CAxPkt::IsFinalTrans() {

	// this->CheckPkt();
	return (this->eFinalTrans);
};

#ifdef CCI_ON
// Get snoop
int CAxPkt::GetSnoop() {
	// this->CheckPkt();
	return (this->spPkt->nSnoop);
};
#endif


//----------------------------
// Get mem bank 
// 	Address map (RBC)
//----------------------------
int CAxPkt::GetBankNum_AMap() {

	// this->CheckPkt();
	int64_t nAddr = this->GetAddr();

	int nBank = GetBankNum_AMap_Global(nAddr);
	return (nBank);
};


//----------------------------
// Get mem row 
// 	Address map (RBC)
//----------------------------
int64_t CAxPkt::GetRowNum_AMap() {

	// this->CheckPkt();
	int64_t nAddr = this->GetAddr();

	int64_t nRow = GetRowNum_AMap_Global(nAddr);
	return (nRow);
};


//----------------------------
// Get mem column 
// 	Address map (RBC)
//----------------------------
int CAxPkt::GetColNum_AMap() {

	// this->CheckPkt();
	int64_t nAddr = this->GetAddr();

	int nCol = GetColNum_AMap_Global(nAddr);
	return (nCol);
};


//----------------------------
// Get mem channel 
// 	Address map (RBC)
//----------------------------
int CAxPkt::GetChannelNum_AMap() {

	// this->CheckPkt();
	int64_t nAddr = this->GetAddr();

	int nMemCh = GetChannelNum_AMap_Global(nAddr);
	return (nMemCh);
};


//----------------------------
// Get mem bank 
// 	Memory map
//----------------------------
int CAxPkt::GetBankNum_MMap() {

	// this->CheckPkt();
	int64_t nAddr = this->GetAddr();

	int nBank = GetBankNum_MMap_Global(nAddr);
	return (nBank);
};


//----------------------------
// Get mem row 
// 	Memory map
//----------------------------
int64_t CAxPkt::GetRowNum_MMap() {

	// this->CheckPkt();
	int64_t nAddr = this->GetAddr();

	int64_t nRow = GetRowNum_MMap_Global(nAddr);
	return (nRow);
};


//----------------------------
// Get mem column 
// 	Address map
//----------------------------
int CAxPkt::GetColNum_MMap() {

	// this->CheckPkt();
	int64_t nAddr = this->GetAddr();

	int nCol = GetColNum_MMap_Global(nAddr);
	return (nCol);
};


//----------------------------
// Get mem channel 
// 	Address map
//----------------------------
int CAxPkt::GetChannelNum_MMap() {

	// this->CheckPkt();
	int64_t nAddr = this->GetAddr();

	int nMemCh = GetChannelNum_MMap_Global(nAddr);
	return (nMemCh);
};


// Get trans num 
int CAxPkt::GetTransNum() {

	// this->CheckPkt();
	return (this->nTransNum);
};


// Debug
EResultType CAxPkt::CheckPkt() {

	assert (this != NULL);
	assert (this->spPkt != NULL);
	assert (this->spPkt->nID   >= 0);
	assert (this->spPkt->nID   <  0xfff);	
	assert (this->spPkt->nAddr >= MIN_ADDR);	// 0
	assert (this->spPkt->nAddr <= MAX_ADDR);	// If nAddr int64_t. Max 7FFF_FFFF_FFFF_FFFF
	assert (this->spPkt->nLen  >= 0); 
	assert (this->spPkt->nLen  < 16);
	
	// assert (this->GetBankNum() >= 0);
	// assert (this->GetBankNum() < (2^BANK_WIDTH));
	// assert (this->GetRowNum()  >= 0);
	// assert (this->GetRowNum()  < (2^ROW_WIDTH));
	// assert (this->GetColNum()  >= 0);
	// assert (this->GetColNum()  < (2^COL_WIDTH));
	
	return (ERESULT_TYPE_SUCCESS);
};


// Debug
EResultType CAxPkt::Display() {

	// Debug
	this->CheckPkt();

	string cDir        = Convert_eDir2string(this->eDir);
	string cType       = Convert_eTransType2string(this->eTransType);
	string cFinalTrans = Convert_eResult2string(this->eFinalTrans);

	// printf("---------------------------------------------\n");	
	// printf("\t Ax pkt display\n");
	printf("---------------------------------------------\n");	
	printf("\t Name		: \t %s\n",     this->cName.c_str());
	printf("\t ID		: \t 0x%x\n",   this->spPkt->nID);
	printf("\t Addr		: \t 0x%lx\n",  this->spPkt->nAddr);
	printf("\t Len		: \t 0x%x\n",   this->spPkt->nLen);
	printf("\t Dir		: \t %s\n",     cDir.c_str());
	printf("\t Src		: \t %s\n",     this->cSrcName.c_str());
	printf("\t Type		: \t %s\n",     cType.c_str());
	printf("\t VA		: \t 0x%lx\n",  this->nVA);
	printf("\t TransNum	: \t %d\n",     this->nTransNum);
	printf("\t TileNum	: \t %d\n",     this->nTileNum);
	printf("\t FinalTrans	: \t %s\n",     cFinalTrans.c_str());
	printf("\t Row  (Amap)	: \t 0x%lx\n",  this->GetRowNum_AMap());
	printf("\t MemCh (Amap)	: \t 0x%x\n",   this->GetChannelNum_AMap());
	printf("\t Bank (Amap)	: \t 0x%x\n",   this->GetBankNum_AMap());
	printf("\t Col  (Amap)	: \t 0x%x\n",   this->GetColNum_AMap());
	printf("\t Superpage	: \t 0x%lx\n",	(uint64_t)(this->spPkt->nAddr)/(BANK_NUM * PAGE_SIZE));
	printf("\t Row  (Mmap)	: \t 0x%lx\n",  this->GetRowNum_MMap());
	printf("\t MemCh (Mmap)	: \t 0x%x\n",   this->GetChannelNum_MMap());
	printf("\t Bank (Mmap)	: \t 0x%x\n",   this->GetBankNum_MMap());
	printf("\t Col  (Mmap)	: \t 0x%x\n",   this->GetColNum_MMap());
	printf("\t Cache ch	: \t 0x%x\n",   GetCacheChNum_Global(this->spPkt->nAddr));
	printf("---------------------------------------------\n");	
	return (ERESULT_TYPE_SUCCESS);
};

