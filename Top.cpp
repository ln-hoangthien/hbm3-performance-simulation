//-----------------------------------------------------------
// Filename     : Top.cpp 
// Version	: 0.83a
// Date         : 22 Feb 2023
// Contact	: JaeYoung.Hur@gmail.com
// Description  : Global functions
// History:
// ----2023.03.15: Duong support ARMv8
//-----------------------------------------------------------
// pRBC
//	(1) Same format as RBC
//	(2) Bank number = (RBC bank number) XOR (Least significant k bits of row number). Bank k bits.
//-----------------------------------------------------------
// Note
//	(1) RBC, RCBC, BRC, pRBC, pRCBC do not support multi channel.
//	(2) We implemented other memory maps (RCBC, pRCBC, BRC). See Top.cpp in IEEE access 2019 submitted paper.
//-----------------------------------------------------------
// Multi channel 
//	(1) Only RMBC, RBCMC, RCBMC supported. 
//-----------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <time.h>

#include "Top.h"

// Random
int random(int min, int max){
	return (min + (rand() % (max + 1 - min)));
}

//-------------------------
// Get row num
// 	Address map (RBC, RMBC, RBCMC, RCBMC)
//-------------------------
int64_t GetRowNum_AMap_Global(int64_t nAddr) {

#ifdef RBC
	return	( (uint64_t) nAddr >> (BANK_WIDTH + COL_WIDTH));			// RBC
#endif

#ifdef RMBC
	return	( (uint64_t) nAddr >> (MEM_CH_WIDTH + BANK_WIDTH + COL_WIDTH));		// RMBC
#endif

#ifdef RBCMC
	return	( (uint64_t) nAddr >> (MEM_CH_WIDTH + BANK_WIDTH + COL_WIDTH));		// RBCMC
#endif

#ifdef RCBMC
	return	( (uint64_t) nAddr >> (MEM_CH_WIDTH + BANK_WIDTH + COL_WIDTH));		// RCBMC 
#endif
};


//-------------------------
// Get bank num
// 	Address map (RBC, RMBC, RBCMC, RCBMC)
//-------------------------
int GetBankNum_AMap_Global(int64_t nAddr) {

	uint64_t nBank_tmp = nAddr >> (COL_WIDTH);

	// int nBank = ( (unsigned) (nBank_tmp << (32-BANK_WIDTH)) >> (32-BANK_WIDTH));	// 32-bit VA
	int nBank    = ( (uint64_t) (nBank_tmp << (64-BANK_WIDTH)) >> (64-BANK_WIDTH));	// 64-bit VA

	#ifdef RBCMC
	nBank_tmp = nAddr >> (MEM_CH_WIDTH + COL_WIDTH);				// 64-byte line

	// int nBank = ( (unsigned) (nBank_tmp << (32-BANK_WIDTH)) >> (32-BANK_WIDTH));	// 32-bit VA
	nBank        = ( (uint64_t) (nBank_tmp << (64-BANK_WIDTH)) >> (64-BANK_WIDTH));	// 64-bit VA
	#endif

	#ifdef RCBMC
	nBank_tmp = nAddr >> (MEM_CH_WIDTH + COL2_WIDTH);				// 64-byte line
	// int nColWidthTmp = COL_WIDTH - COL2_WIDTH;

	// int nBank = ( (unsigned) (nBank_tmp << (32-BANK_WIDTH)) >> (32-BANK_WIDTH));	// 32-bit VA
	//nBank        = ( (uint64_t) (nBank_tmp << (64-BANK_WIDTH-nColWidthTmp)) >> (64-BANK_WIDTH-nColWidthTmp));	// 64-bit VA
	nBank        = ( (uint64_t) (nBank_tmp << (64-BANK_WIDTH) >> (64-BANK_WIDTH)));	// 64-bit VA
	#endif

	#ifdef DEBUG
	assert (nBank >= 0);
	assert (nBank < BANK_NUM);
	#endif

	return (nBank);
};


//-------------------------
// Get col num
// 	Address map (RBC, RMBC, RBCMC)
//-------------------------
int GetColNum_AMap_Global(int64_t nAddr) {

	// int nCol = ( (unsigned) (nAddr << (32- COL_WIDTH)) >> (32-COL_WIDTH));		// 32-bit VA
	int nCol    = ( (uint64_t) (nAddr << (64- COL_WIDTH)) >> (64-COL_WIDTH));		// 64-bit VA

#ifdef RBCMC
	// uint64_t nCol2  = (uint64_t) (nAddr << (32 - COL2_WIDTH)) >> (32 - COL2_WIDTH);	// Arch32
	uint64_t    nCol2  = (uint64_t) (nAddr << (64 - COL2_WIDTH)) >> (64 - COL2_WIDTH);	// Arch64
	
	uint64_t nCol1_tmp = nAddr >> (MEM_CH_WIDTH + COL2_WIDTH);
	// uint64_t nCol1 = ( (uint64_t) (nCol1_tmp << (32 - COL1_WIDTH)) >> (32 - COL1_WIDTH));	// Arch32
	uint64_t    nCol1 = ( (uint64_t) (nCol1_tmp << (64 - COL1_WIDTH)) >> (64 - COL1_WIDTH));	// Arch64
	
	nCol = ( (uint64_t) (nCol1 << COL2_WIDTH) + nCol2 );					// RBCMC
#endif
	
#ifdef RCBMC
	// uint64_t nCol2  = (uint64_t) (nAddr << (32 - COL2_WIDTH)) >> (32 - COL2_WIDTH);	// Arch32
	uint64_t    nCol2  = (uint64_t) (nAddr << (64 - COL2_WIDTH)) >> (64 - COL2_WIDTH);	// Arch64
	
	uint64_t nCol1_tmp = nAddr >> (BANK_WIDTH + MEM_CH_WIDTH + COL2_WIDTH);
	// uint64_t nCol1 = ( (uint64_t) (nCol1_tmp << (32 - COL1_WIDTH)) >> (32 - COL1_WIDTH));	// Arch32
	uint64_t    nCol1 = ( (uint64_t) (nCol1_tmp << (64 - COL1_WIDTH)) >> (64 - COL1_WIDTH));	// Arch64
	
	nCol = ( (uint64_t) (nCol1 << COL2_WIDTH) + nCol2 );					// RCBMC
#endif
	
	#ifdef DEBUG
	assert (nCol >= 0);
	assert (nCol < PAGE_SIZE);
	#endif

	return (nCol);
};


//-------------------------
// Get mem channel num
// 	Address map (RMBC, RBCMC, RCBMC)
//-------------------------
int GetChannelNum_AMap_Global(int64_t nAddr) {

#ifdef RBC
	return    (0);
#endif


#ifdef RMBC

	if (MEM_CH_WIDTH == 0) {
		return (0);
	};

	uint64_t nCh_tmp = nAddr >> (BANK_WIDTH + COL_WIDTH);

	// int nCh = ( (unsigned) (nCh_tmp << (32 - MEM_CH_WIDTH)) >> (32 - MEM_CH_WIDTH));	// 32-bit VA
	int nCh    = ( (uint64_t) (nCh_tmp << (64 - MEM_CH_WIDTH)) >> (64 - MEM_CH_WIDTH));	// 64-bit VA 

	#ifdef DEBUG
	assert (nCh >= 0);
	assert (nCh <= MEM_CHANNEL_NUM);
	#endif

	return (nCh);
#endif

#ifdef RBCMC
	uint64_t nCh_tmp = nAddr >> (COL2_WIDTH);					// 64-byte line

	// int nCh = ( (unsigned) (nCh_tmp << (32 - MEM_CH_WIDTH)) >> (32 - MEM_CH_WIDTH));	// 32-bit VA
	// int nCh = ( (uint64_t) (nCh_tmp << (64 - MEM_CH_WIDTH)) >> (64 - MEM_CH_WIDTH));	// 64-bit VA

	int nCh = -1;
	if      (MEM_CH_WIDTH == 0) nCh = nCh_tmp & 0;		// 1 ch
	else if (MEM_CH_WIDTH == 1) nCh = nCh_tmp & 1;		// 2 ch
	else if (MEM_CH_WIDTH == 2) nCh = nCh_tmp & 3;		// 4 ch
	else if (MEM_CH_WIDTH == 3) nCh = nCh_tmp & 7;		// 8 ch
	else assert(-1); 
	

	#ifdef DEBUG
	assert (nCh >= 0);
	assert (nCh <= MEM_CHANNEL_NUM);
	#endif

	return (nCh);
#endif

#ifdef RCBMC
	uint64_t nCh_tmp = nAddr >> (COL2_WIDTH);					// 64-byte line

	// int nCh = ( (unsigned) (nCh_tmp << (32 - MEM_CH_WIDTH)) >> (32 - MEM_CH_WIDTH));	// 32-bit VA
	// int nCh = ( (uint64_t) (nCh_tmp << (64 - MEM_CH_WIDTH)) >> (64 - MEM_CH_WIDTH));	// 64-bit VA

	int nCh = -1;
	if      (MEM_CH_WIDTH == 0) nCh = nCh_tmp & 0;		// 1 ch
	else if (MEM_CH_WIDTH == 1) nCh = nCh_tmp & 1;		// 2 ch
	else if (MEM_CH_WIDTH == 2) nCh = nCh_tmp & 3;		// 4 ch
	else if (MEM_CH_WIDTH == 3) nCh = nCh_tmp & 7;		// 8 ch
	else assert(-1); 
	

	#ifdef DEBUG
	assert (nCh >= 0);
	assert (nCh <= MEM_CHANNEL_NUM);
	#endif

	return (nCh);
#endif
};


//-------------------------
// Get cache channel num
//	64-byte cache line
//-------------------------
int GetCacheChNum_Global(int64_t nAddr) {

	if (CACHE_WIDTH == 0) {
		return (0);
	};

	uint64_t nCh_tmp = nAddr >> 6;							// 64-byte line

	// int nCh = ( (unsigned) (nCh_tmp << (32 - CACHE_WIDTH)) >> (32 - CACHE_WIDTH));	// 32-bit VA
	// int nCh = ( (uint64_t) (nCh_tmp << (64 - CACHE_WIDTH)) >> (64 - CACHE_WIDTH));	// 64-bit VA FIXME overflow

	int nCh = -1;
	if      (CACHE_WIDTH == 0) nCh = nCh_tmp & 0;		// 1 ch
	else if (CACHE_WIDTH == 1) nCh = nCh_tmp & 1;		// 2 ch
	else if (CACHE_WIDTH == 2) nCh = nCh_tmp & 3;		// 4 ch
	else if (CACHE_WIDTH == 3) nCh = nCh_tmp & 7;		// 8 ch
	else assert(-1); 
	

	#ifdef DEBUG
	assert (nCh >= 0);
	assert (nCh < CACHE_NUM);
	#endif

	return (nCh);
};


//-------------------------
// Get row num 
// 	Memory map
//-------------------------
// int64_t GetRowNum_MMap_Global(int64_t nAddr) {
//
//	return	( (uint64_t) nAddr >> (COL_WIDTH + BANK_WIDTH));  // RBC or RCBC or pRBC
// };


//-------------------------
// Get row num
//      Memory map
//-------------------------
int64_t GetRowNum_MMap_Global(int64_t nAddr) {

#ifdef BRC
	uint64_t nRow_tmp = nAddr >> (COL_WIDTH);
	// uint64_t nRow = ( (uint64_t) (nRow_tmp << (32 - ROW_WIDTH)) >> (32 - ROW_WIDTH));	// Arch32
	// uint64_t nRow = ( (uint64_t) (nRow_tmp << (64 - ROW_WIDTH)) >> (64 - ROW_WIDTH));	// Arch64. FIXME overflow

	int nRow = -1;
	if      (ROW_WIDTH == 0) nRow = nRow  & 0b0;		// 2^0 row
	else if (ROW_WIDTH == 1) nRow = nRow  & 0b1;		// 2^1 rows
	else if (ROW_WIDTH == 2) nRow = nRow  & 0b11;		// 2^2 rows
	else if (ROW_WIDTH == 3) nRow = nRow  & 0b111;		// 2^3 rows
	else if (ROW_WIDTH == 4) nRow = nRow  & 0b1111;		// 2^4 rows
	else if (ROW_WIDTH == 5) nRow = nRow  & 0b11111;		// 2^5 rows
	else if (ROW_WIDTH == 6) nRow = nRow  & 0b111111;		// 2^6 rows
	else if (ROW_WIDTH == 7) nRow = nRow  & 0b1111111;		// 2^7 rows
	else if (ROW_WIDTH == 8) nRow = nRow  & 0b11111111;		// 2^8 rows
	else if (ROW_WIDTH == 9) nRow = nRow  & 0b111111111;		// 2^9 rows
	else if (ROW_WIDTH == 10) nRow = nRow & 0b1111111111;		// 2^10 rows
	else if (ROW_WIDTH == 11) nRow = nRow & 0b11111111111;		// 2^11 rows
	else if (ROW_WIDTH == 12) nRow = nRow & 0b111111111111;		// 2^12 rows
	else if (ROW_WIDTH == 13) nRow = nRow & 0b1111111111111;		// 2^13 rows
	else if (ROW_WIDTH == 14) nRow = nRow & 0b11111111111111;		// 2^14 rows
	else if (ROW_WIDTH == 15) nRow = nRow & 0b111111111111111;		// 2^15 rows
	else if (ROW_WIDTH == 16) nRow = nRow & 0b1111111111111111;		// 2^16 rows
	else if (ROW_WIDTH == 17) nRow = nRow & 0b11111111111111111;		// 2^17 rows
	else if (ROW_WIDTH == 18) nRow = nRow & 0b111111111111111111;		// 2^18 rows
	else if (ROW_WIDTH == 19) nRow = nRow & 0b1111111111111111111;		// 2^19 rows
	else if (ROW_WIDTH == 20) nRow = nRow & 0b11111111111111111111;		// 2^20 rows
	else if (ROW_WIDTH == 21) nRow = nRow & 0b111111111111111111111;		// 2^21 rows
	else if (ROW_WIDTH == 22) nRow = nRow & 0b1111111111111111111111;		// 2^22 rows
	else if (ROW_WIDTH == 23) nRow = nRow & 0b11111111111111111111111;		// 2^23 rows
	else if (ROW_WIDTH == 24) nRow = nRow & 0b111111111111111111111111;		// 2^24 rows
	else if (ROW_WIDTH == 25) nRow = nRow & 0b1111111111111111111111111;		// 2^25 rows
	else if (ROW_WIDTH == 26) nRow = nRow & 0b11111111111111111111111111;		// 2^26 rows
	else if (ROW_WIDTH == 27) nRow = nRow & 0b111111111111111111111111111;		// 2^27 rows
	else if (ROW_WIDTH == 28) nRow = nRow & 0b1111111111111111111111111111;		// 2^28 rows
	else if (ROW_WIDTH == 29) nRow = nRow & 0b11111111111111111111111111111;		// 2^29 rows
	else if (ROW_WIDTH == 30) nRow = nRow & 0b111111111111111111111111111111;		// 2^30 rows
	else if (ROW_WIDTH == 31) nRow = nRow & 0b1111111111111111111111111111111;		// 2^31 rows
	else if (ROW_WIDTH == 32) nRow = nRow & 0b11111111111111111111111111111111;		// 2^32 rows
	else if (ROW_WIDTH == 33) nRow = nRow & 0b111111111111111111111111111111111;		// 2^33 rows
	else if (ROW_WIDTH == 34) nRow = nRow & 0b1111111111111111111111111111111111;		// 2^34 rows
	else if (ROW_WIDTH == 35) nRow = nRow & 0b11111111111111111111111111111111111;		// 2^35 rows
	else if (ROW_WIDTH == 36) nRow = nRow & 0b111111111111111111111111111111111111;		// 2^36 rows
	else if (ROW_WIDTH == 37) nRow = nRow & 0b1111111111111111111111111111111111111;		// 2^37 rows
	else if (ROW_WIDTH == 38) nRow = nRow & 0b11111111111111111111111111111111111111;		// 2^38 rows
	else if (ROW_WIDTH == 39) nRow = nRow & 0b111111111111111111111111111111111111111;		// 2^39 rows
	else if (ROW_WIDTH == 40) nRow = nRow & 0b1111111111111111111111111111111111111111;		// 2^40 rows
	else if (ROW_WIDTH == 41) nRow = nRow & 0b11111111111111111111111111111111111111111;		// 2^41 rows
	else if (ROW_WIDTH == 42) nRow = nRow & 0b111111111111111111111111111111111111111111;		// 2^42 rows
	else if (ROW_WIDTH == 43) nRow = nRow & 0b1111111111111111111111111111111111111111111;		// 2^43 rows
	else if (ROW_WIDTH == 44) nRow = nRow & 0b11111111111111111111111111111111111111111111;		// 2^44 rows
	else if (ROW_WIDTH == 45) nRow = nRow & 0b111111111111111111111111111111111111111111111;		// 2^45 rows
	else if (ROW_WIDTH == 46) nRow = nRow & 0b1111111111111111111111111111111111111111111111;		// 2^46 rows
	else if (ROW_WIDTH == 47) nRow = nRow & 0b11111111111111111111111111111111111111111111111;		// 2^47 rows
	else if (ROW_WIDTH == 48) nRow = nRow & 0b111111111111111111111111111111111111111111111111;		// 2^48 rows
	else if (ROW_WIDTH == 49) nRow = nRow & 0b1111111111111111111111111111111111111111111111111;		// 2^49 rows
	else if (ROW_WIDTH == 50) nRow = nRow & 0b11111111111111111111111111111111111111111111111111;		// 2^50 rows
	else if (ROW_WIDTH == 51) nRow = nRow & 0b111111111111111111111111111111111111111111111111111;		// 2^51 rows
	else if (ROW_WIDTH == 52) nRow = nRow & 0b1111111111111111111111111111111111111111111111111111;		// 2^52 rows
	else if (ROW_WIDTH == 53) nRow = nRow & 0b11111111111111111111111111111111111111111111111111111;	// 2^53 rows
	else assert(-1); 
	
	assert (nRow >= 0);
	return (nRow);
#endif
	
	// return  ( (uint64_t) nAddr >> (BANK_WIDTH + COL_WIDTH));				// RBC, RCBC, pRBC, pRCBC.
	return  ( (uint64_t) nAddr >> (MEM_CH_WIDTH + BANK_WIDTH + COL_WIDTH));			// RMBC, RBC, RCBC, pRBC, pRCBC.
};



//-------------------------
// Get bank num
// 	Memory map
//-------------------------
// int GetBankNum_MMap_Global(int64_t nAddr) {
//	
//	uint64_t nBank_tmp = -1;
//	uint64_t nBank = -1;
//	
//	#ifdef RBC
//	nBank_tmp = nAddr >> (COL_WIDTH);
//	nBank = ( (uint64_t) (nBank_tmp << (64-BANK_WIDTH)) >> (64-BANK_WIDTH));  // RBC
//	#endif
//	
//	#ifdef RCBC
//	nBank_tmp = nAddr >> (COL2_WIDTH);
//	nBank = ( (uint64_t) (nBank_tmp << (64-BANK_WIDTH)) >> (64-BANK_WIDTH));  // RCBC
//	#endif
//	
//	#ifdef pRBC
//	//------------------------------
//	// (1) Get RBC bank number
//	// (2) Get RBC row number
//	// (3) Get least significant k bits of RBC row number
//	// (4) Bank number = (RBC bank number) XOR (Least significant k bits of RBC row number)
//	//------------------------------
//	
//	// (1) Get RBC bank number 
//	nBank_tmp = nAddr >> (COL_WIDTH);
//	nBank = ( (uint64_t) (nBank_tmp << (64-BANK_WIDTH)) >> (64-BANK_WIDTH));  // RBC
//	
//	// // Row LSB	
//	// uint64_t nRow = GetRowNum_MMap_Global(nAddr);
//	// uint64_t nRowLSB = nRow & 1;  // Bitwise and 	
//	// 
//	// // Bank LSB
//	// uint64_t nBankLSB = nBank & 1;  // Bitwise and 
//	// 
//	// // (Row LSB) xor (Bank LSB)
//	// nBankLSB = nBankLSB ^ nRowLSB;  // Bitwise xor
//	// 
//	// // New bank num
//	// nBank = (uint64_t) ((nBank >> 1) << 1) + nBankLSB;
//	
//	// (2) Get RBC row number 
//	uint64_t nRow = GetRowNum_MMap_Global(nAddr);
//	
//	// (3) Get least significant k bits of RBC row number
//	uint64_t nRowLS = -1;  
//	if (BANK_WIDTH == 3) {  // 8 banks
//		nRowLS = nRow & 7;  // Bitwise and
//	} 
//	else if (BANK_WIDTH == 2) { // 4 banks
//		nRowLS = nRow & 3;
//	} 
//	else if (BANK_WIDTH == 1) { // 2 banks
//		nRowLS = nRow & 1;
//	} 
//	else {
//		assert (0);
//	};
//	
//	// (4) Get new bank number 
//	nBank = nBank ^ nRowLS;  // Bitwise xor
//	#endif
//	
//	return nBank;
//};


//-------------------------
// Get bank num
//      Memory map
//-------------------------
int GetBankNum_MMap_Global(int64_t nAddr) {
 
	// uint64_t nBank_tmp = -1;
	// uint64_t nBank = -1;
	
	// Debug
	assert (nAddr >= 0);
	
#ifdef RBC
	uint64_t nBank_tmp = nAddr >> (COL_WIDTH);
	// uint64_t nBank = ( (uint64_t) (nBank_tmp << (32 - BANK_WIDTH)) >> (32 - BANK_WIDTH));	// RBC. Arch32
	uint64_t    nBank = ( (uint64_t) (nBank_tmp << (64 - BANK_WIDTH)) >> (64 - BANK_WIDTH));	// RBC. Arch64 FIXME overflow
#endif


#ifdef BRC
	uint64_t nBank = ( (uint64_t) nAddr >> (COL_WIDTH + ROW_WIDTH));
#endif


#ifdef RCBC
	uint64_t nBank_tmp = nAddr >> (COL2_WIDTH);
	// uint64_t nBank = ( (uint64_t) (nBank_tmp << (32 - BANK_WIDTH)) >> (32 - BANK_WIDTH));	// RCBC. Arch32
	uint64_t    nBank = ( (uint64_t) (nBank_tmp << (64 - BANK_WIDTH)) >> (64 - BANK_WIDTH));	// RCBC. Arch64 FIXME overflow
#endif


#ifdef pRBC
	//------------------------------
	// (1) Get RBC bank number
	// (2) Get RBC row number
	// (3) Get least significant k bits of RBC row number
	// (4) Bank number = (RBC bank number) XOR (Least significant k bits of RBC row number)
	//------------------------------
	
	// (1) Get RBC bank number 
	uint64_t nBank_tmp = nAddr >> (COL_WIDTH);
	// uint64_t nBank = ( (uint64_t) (nBank_tmp << (32 - BANK_WIDTH)) >> (32 - BANK_WIDTH));	// RBC. Arch32
	uint64_t    nBank = ( (uint64_t) (nBank_tmp << (64 - BANK_WIDTH)) >> (64 - BANK_WIDTH));	// RBC. Arch64
	
	// // Row LSB   
	// uint64_t nRow = GetRowNum_MMap_Global(nAddr);
	// uint64_t nRowLSB = nRow & 1;  // Bitwise and         
	// 
	// // Bank LSB
	// uint64_t nBankLSB = nBank & 1;  // Bitwise and 
	// 
	// // (Row LSB) xor (Bank LSB)
	// nBankLSB = nBankLSB ^ nRowLSB;  // Bitwise xor
	// 
	// // New bank num
	// nBank = (uint64_t) ((nBank >> 1) << 1) + nBankLSB;
	
	// (2) Get RBC row number 
	uint64_t nRow = GetRowNum_MMap_Global(nAddr);
	
	// (3) Get least significant k bits of RBC row number
	uint64_t nRowLS = -1;
	if (BANK_WIDTH == 3) {		// 8 banks
	        nRowLS = nRow & 7;	// Bitwise and
	}
	else if (BANK_WIDTH == 2) {	// 4 banks
	        nRowLS = nRow & 3;
	}
	else if (BANK_WIDTH == 1) {	// 2 banks
	        nRowLS = nRow & 1;
	}
	else {
	        assert (0);
	};
	
	// (4) Get new bank number 
	nBank = nBank ^ nRowLS;		// Bitwise xor
#endif
	
	
#ifdef pRCBC			// Same pRBC (except RBC -> RCBC).
	//------------------------------
	// (1) Get RCBC bank number
	// (2) Get RCBC row number
	// (3) Get least significant k bits of RCBC row number
	// (4) Bank number = (RCBC bank number) XOR (Least significant k bits of RCBC row number)
	//------------------------------
	
	// (1) Get RCBC bank number 
	uint64_t nBank_tmp = nAddr >> (COL2_WIDTH);
	// uint64_t nBank = ( (uint64_t) (nBank_tmp << (32 - BANK_WIDTH)) >> (32 - BANK_WIDTH));	// RCBC. Arch32
	uint64_t    nBank = ( (uint64_t) (nBank_tmp << (64 - BANK_WIDTH)) >> (64 - BANK_WIDTH));	// RCBC. Arch64
	
	// (2) Get RCBC row number 
	uint64_t nRow = GetRowNum_MMap_Global(nAddr);
	
	// (3) Get least significant k bits of RCBC row number
	uint64_t nRowLS = -1;
	if (BANK_WIDTH == 3) {		// 8 banks
	        nRowLS = nRow & 7;	// Bitwise and
	}
	else if (BANK_WIDTH == 2) {	// 4 banks
	        nRowLS = nRow & 3;
	}
	else if (BANK_WIDTH == 1) {	// 2 banks
	        nRowLS = nRow & 1;
	}
	else {
	        assert (0);
	};
	
	// (4) Get new bank number 
	nBank = nBank ^ nRowLS;		// Bitwise xor
#endif


#ifdef RMBC	// Same as RBC

	uint64_t nBank_tmp = nAddr >> (COL_WIDTH);

	// uint64_t nBank = ( (unsigned) (nBank_tmp << (32 - MEM_CH_WIDTH - BANK_WIDTH)) >> (32 - MEM_CH_WIDTH - BANK_WIDTH));  // 32-bit VA
	// uint64_t nBank = ( (uint64_t) (nBank_tmp << (64 - MEM_CH_WIDTH - BANK_WIDTH)) >> (64 - MEM_CH_WIDTH - BANK_WIDTH));  // 64-bit VA	
	uint64_t nBank    = ( (uint64_t) (nBank_tmp << (64 - BANK_WIDTH)) >> (64 - BANK_WIDTH));  // 64-bit VA
#endif

#ifdef RBCMC
	uint64_t nBank_tmp = nAddr >> (MEM_CH_WIDTH + COL_WIDTH);				// 64-byte line

	// int nBank = ( (unsigned) (nBank_tmp << (32-BANK_WIDTH)) >> (32-BANK_WIDTH));	// 32-bit VA
	int nBank    = ( (uint64_t) (nBank_tmp << (64-BANK_WIDTH)) >> (64-BANK_WIDTH));	// 64-bit VA
#endif

#ifdef RCBMC
	uint64_t nBank_tmp = nAddr >> (MEM_CH_WIDTH + COL2_WIDTH);				// 64-byte line

	// int nBank = ( (unsigned) (nBank_tmp << (32-BANK_WIDTH)) >> (32-BANK_WIDTH));	// 32-bit VA
	int nBank    = ( (uint64_t) (nBank_tmp << (64-BANK_WIDTH)) >> (64-BANK_WIDTH));	// 64-bit VA
#endif


	#ifdef DEBUG	
	assert (nBank >= 0);
	assert (nBank < BANK_NUM);
	#endif

	return nBank;
};


//-------------------------
// Get col num
// 	Memor map
//-------------------------
// int GetColNum_MMap_Global(int64_t nAddr) {
//	
//	#ifdef RBC
//	return    ( (uint64_t) (nAddr << (64 - COL_WIDTH)) >> (64 - COL_WIDTH));  // RBC
//	#endif
//	
//	#ifdef RCBC
//	uint64_t nCol2  = (uint64_t) (nAddr << (64 - COL2_WIDTH)) >> (64 - COL2_WIDTH);
//	
//	uint64_t nCol1_tmp = nAddr >> (BANK_WIDTH + COL2_WIDTH);
//	uint64_t nCol1 = ( (uint64_t) (nCol1_tmp << (64 - BANK_WIDTH - COL2_WIDTH)) >> (64 - BANK_WIDTH - COL2_WIDTH));
//	
//	return    ( (uint64_t) (nCol1 << COL2_WIDTH) + nCol2 );  // RCBC
//	#endif
//	
//	#ifdef pRBC
//	return    ( (uint64_t) (nAddr << (64 - COL_WIDTH)) >> (64 - COL_WIDTH));  // same as RBC
//	#endif
//};


//-------------------------
// Get col num
//      Memor map
//-------------------------
int GetColNum_MMap_Global(int64_t nAddr) {
	
#ifdef RBC
	// int nCol = ( (uint64_t) (nAddr << (32 - COL_WIDTH)) >> (32 - COL_WIDTH));		// RBC. Arch32
	int nCol    = ( (uint64_t) (nAddr << (64 - COL_WIDTH)) >> (64 - COL_WIDTH));		// RBC. Arch64
#endif
	
#ifdef BRC
	// int nCol = ( (uint64_t) (nAddr << (32 - COL_WIDTH)) >> (32 - COL_WIDTH));		// Same RBC
	int nCol    = ( (uint64_t) (nAddr << (64 - COL_WIDTH)) >> (64 - COL_WIDTH));
#endif
	
#ifdef pRBC
	// int nCol = ( (uint64_t) (nAddr << (32 - COL_WIDTH)) >> (32 - COL_WIDTH));		// Same RBC
	int nCol    = ( (uint64_t) (nAddr << (64 - COL_WIDTH)) >> (64 - COL_WIDTH));
#endif
	
#ifdef RCBC
	// uint64_t nCol2  = (uint64_t) (nAddr << (32 - COL2_WIDTH)) >> (32 - COL2_WIDTH);	// Arch32
	uint64_t    nCol2  = (uint64_t) (nAddr << (64 - COL2_WIDTH)) >> (64 - COL2_WIDTH);	// Arch64
	
	uint64_t nCol1_tmp = nAddr >> (BANK_WIDTH + COL2_WIDTH);
	// uint64_t nCol1 = ( (uint64_t) (nCol1_tmp << (32 - BANK_WIDTH - COL2_WIDTH)) >> (32 - BANK_WIDTH - COL2_WIDTH));	// Arch32
	// uint64_t nCol1 = ( (uint64_t) (nCol1_tmp << (64 - BANK_WIDTH - COL2_WIDTH)) >> (64 - BANK_WIDTH - COL2_WIDTH));	// Arch64
	uint64_t    nCol1 = ( (uint64_t) (nCol1_tmp << (64 - COL1_WIDTH))              >> (64 - COL1_WIDTH));			// Arch64
	
	int nCol = ( (uint64_t) (nCol1 << COL2_WIDTH) + nCol2 );					// RCBC
#endif
	
#ifdef pRCBC											// Same RCBC.
	// uint64_t nCol2  = (uint64_t) (nAddr << (32 - COL2_WIDTH)) >> (32 - COL2_WIDTH);	// Arch32
	uint64_t    nCol2  = (uint64_t) (nAddr << (64 - COL2_WIDTH)) >> (64 - COL2_WIDTH);	// Arch64
	
	uint64_t nCol1_tmp = nAddr >> (BANK_WIDTH + COL2_WIDTH);
	// uint64_t nCol1 = ( (uint64_t) (nCol1_tmp << (32 - BANK_WIDTH - COL2_WIDTH)) >> (32 - BANK_WIDTH - COL2_WIDTH));	// Arch32
	// uint64_t nCol1 = ( (uint64_t) (nCol1_tmp << (64 - BANK_WIDTH - COL2_WIDTH)) >> (64 - BANK_WIDTH - COL2_WIDTH));	// Arch64
	uint64_t    nCol1 = ( (uint64_t) (nCol1_tmp << (64 - COL1_WIDTH))              >> (64 - COL1_WIDTH));			// Arch64
	
	int nCol =  ( (uint64_t) (nCol1 << COL2_WIDTH) + nCol2 );					// RCBC
#endif

#ifdef RMBC
	// int nCol = ( (unsigned) (nAddr << (32- COL_WIDTH)) >> (32-COL_WIDTH));  // 32-bit VA
	int nCol    = ( (uint64_t) (nAddr << (64- COL_WIDTH)) >> (64-COL_WIDTH));  // 64-bit VA
#endif


#ifdef RBCMC
	// int nCol = ( (unsigned) (nAddr << (32- COL_WIDTH)) >> (32-COL_WIDTH));		// 32-bit VA
	int nCol    = ( (uint64_t) (nAddr << (64- COL_WIDTH)) >> (64-COL_WIDTH));		// 64-bit VA

	// uint64_t nCol2  = (uint64_t) (nAddr << (32 - COL2_WIDTH)) >> (32 - COL2_WIDTH);	// Arch32
	uint64_t    nCol2  = (uint64_t) (nAddr << (64 - COL2_WIDTH)) >> (64 - COL2_WIDTH);	// Arch64
	
	uint64_t nCol1_tmp = nAddr >> (MEM_CH_WIDTH + COL2_WIDTH);
	// uint64_t nCol1 = ( (uint64_t) (nCol1_tmp << (32 - COL1_WIDTH)) >> (32 - COL1_WIDTH));	// Arch32
	uint64_t    nCol1 = ( (uint64_t) (nCol1_tmp << (64 - COL1_WIDTH)) >> (64 - COL1_WIDTH));	// Arch64
	
	nCol = ( (uint64_t) (nCol1 << COL2_WIDTH) + nCol2 );					// RBCMC
#endif

#ifdef RCBMC
	// int nCol = ( (unsigned) (nAddr << (32- COL_WIDTH)) >> (32-COL_WIDTH));		// 32-bit VA
	int nCol    = ( (uint64_t) (nAddr << (64- COL_WIDTH)) >> (64-COL_WIDTH));		// 64-bit VA

	// uint64_t nCol2  = (uint64_t) (nAddr << (32 - COL2_WIDTH)) >> (32 - COL2_WIDTH);	// Arch32
	uint64_t    nCol2  = (uint64_t) (nAddr << (64 - COL2_WIDTH)) >> (64 - COL2_WIDTH);	// Arch64
	
	uint64_t nCol1_tmp = nAddr >> (BANK_WIDTH + MEM_CH_WIDTH + COL2_WIDTH);
	// uint64_t nCol1 = ( (uint64_t) (nCol1_tmp << (32 - COL1_WIDTH)) >> (32 - COL1_WIDTH));	// Arch32
	uint64_t    nCol1 = ( (uint64_t) (nCol1_tmp << (64 - COL1_WIDTH)) >> (64 - COL1_WIDTH));	// Arch64
	
	nCol = ( (uint64_t) (nCol1 << COL2_WIDTH) + nCol2 );					// RCBMC
#endif

	#ifdef DEBUG
	assert (nCol >= 0);
	assert (nCol < PAGE_SIZE);
	#endif

	return (nCol);
};



//-------------------------
// Get col num
//      Memory map
//-------------------------
int GetChannelNum_MMap_Global(int64_t nAddr) {

	#ifdef DEBUG
	assert (MEM_CH_WIDTH >= 0);
	assert (MEM_CH_WIDTH <= 3);
	assert (BANK_WIDTH >= 0);
	assert (BANK_WIDTH <= 31);
	#endif

#ifdef RMBC

	if (MEM_CH_WIDTH == 0) {
		return (0);
	};

	uint64_t nCh_tmp = nAddr >> (BANK_WIDTH + COL_WIDTH);
	// int nCh = ( (unsigned) (nCh_tmp << (32 - MEM_CH_WIDTH)) >> (32 - MEM_CH_WIDTH));		// 32-bit VA
	// int nCh = ( (uint64_t) (nCh_tmp << (64 - MEM_CH_WIDTH)) >> (64 - MEM_CH_WIDTH));		// 64-bit VA 
	int nCh =    ( (uint64_t) (nCh_tmp << (64 - MEM_CH_WIDTH)) >> (64 - MEM_CH_WIDTH));		// 64-bit VA 

	#ifdef DEBUG
	assert (nCh >= 0);
	assert (nCh <= MEM_CHANNEL_NUM);
	#endif

	return (nCh);
#endif

#ifdef RBCMC
	uint64_t nCh_tmp = nAddr >> (COL2_WIDTH);					// 64-byte line

	// int nCh = ( (unsigned) (nCh_tmp << (32 - MEM_CH_WIDTH)) >> (32 - MEM_CH_WIDTH));	// 32-bit VA
	// int nCh = ( (uint64_t) (nCh_tmp << (64 - MEM_CH_WIDTH)) >> (64 - MEM_CH_WIDTH));	// 64-bit VA

	int nCh = -1;
	if      (MEM_CH_WIDTH == 0) nCh = nCh_tmp & 0;		// 1 ch
	else if (MEM_CH_WIDTH == 1) nCh = nCh_tmp & 1;		// 2 ch
	else if (MEM_CH_WIDTH == 2) nCh = nCh_tmp & 3;		// 4 ch
	else if (MEM_CH_WIDTH == 3) nCh = nCh_tmp & 7;		// 8 ch
	else assert(-1); 
	

	#ifdef DEBUG
	assert (nCh >= 0);
	assert (nCh <= MEM_CHANNEL_NUM);
	#endif

	return (nCh);
#endif

#ifdef RCBMC
	uint64_t nCh_tmp = nAddr >> (COL2_WIDTH);					// 64-byte line

	// int nCh = ( (unsigned) (nCh_tmp << (32 - MEM_CH_WIDTH)) >> (32 - MEM_CH_WIDTH));	// 32-bit VA
	// int nCh = ( (uint64_t) (nCh_tmp << (64 - MEM_CH_WIDTH)) >> (64 - MEM_CH_WIDTH));	// 64-bit VA

	int nCh = -1;
	if      (MEM_CH_WIDTH == 0) nCh = nCh_tmp & 0;		// 1 ch
	else if (MEM_CH_WIDTH == 1) nCh = nCh_tmp & 1;		// 2 ch
	else if (MEM_CH_WIDTH == 2) nCh = nCh_tmp & 3;		// 4 ch
	else if (MEM_CH_WIDTH == 3) nCh = nCh_tmp & 7;		// 8 ch
	else assert(-1); 
	

	#ifdef DEBUG
	assert (nCh >= 0);
	assert (nCh <= MEM_CHANNEL_NUM);
	#endif

	return (nCh);
#endif

	return (0);
};	


//-------------------------
// Get RBC addr (from R, B, C) 
// 	Address map (RBC only)
//-------------------------
int64_t GetAddr_AMap_Global(int64_t nRow, int nBank, int nCol) {

	return (uint64_t) ( (nRow << (BANK_WIDTH + COL_WIDTH)) + (nBank << COL_WIDTH) + (nCol) );
};


//-------------------------
// Get RMBC addr (from R, M, B, C) 
// 	Address map (RMBC only)
//-------------------------
int64_t GetAddr_AMap_Global(int64_t nRow, int nMemCh, int nBank, int nCol) {

	return (uint64_t) ( (nRow << (MEM_CH_WIDTH + BANK_WIDTH + COL_WIDTH)) + (nMemCh << (BANK_WIDTH + COL_WIDTH)) + (nBank << COL_WIDTH) + (nCol) );
};


string Convert_eResult2string(EResultType eType) {

        switch(eType) {
                case ERESULT_TYPE_YES:  
			return("ERESULT_TYPE_YES");  
			break;
                case ERESULT_TYPE_NO:  
			return("ERESULT_TYPE_NO");  
			break;
                case ERESULT_TYPE_ACCEPT:  
			return("ERESULT_TYPE_ACCEPT");  
			break;
                case ERESULT_TYPE_REJECT:  
			return("ERESULT_TYPE_REJECT");  
			break;
                case ERESULT_TYPE_SUCCESS:  
			return("ERESULT_TYPE_SUCCESS");  
			break;
                case ERESULT_TYPE_FAIL:  
			return("ERESULT_TYPE_FAIL");  
			break;
                case ERESULT_TYPE_ON:  
			return("ERESULT_TYPE_ON");  
			break;
                case ERESULT_TYPE_OFF:  
			return("ERESULT_TYPE_OFF");  
			break;
                case ERESULT_TYPE_UNDEFINED:  
			return("ERESULT_TYPE_UNDEFINED");  
			break;
                default: 
			break;
        };
	return (NULL);
};


string Convert_eDir2string(ETransDirType eType) {

        switch(eType) {
                case ETRANS_DIR_TYPE_READ:  
			return("ETRANS_DIR_TYPE_READ");  
			break;
                case ETRANS_DIR_TYPE_WRITE: 
			return("ETRANS_DIR_TYPE_WRITE"); 
			break;
                case ETRANS_DIR_TYPE_UNDEFINED:    
			return("EUD_TYPE_UNDEFINED");
			break;
                default: 
			break;
        };
	return (NULL);
};


string Convert_eUDType2string(EUDType eType) {

        switch(eType) {
            case EUD_TYPE_AR:  
				return("EUD_TYPE_AR");  
				break;
            case EUD_TYPE_AW:  
				return("EUD_TYPE_AW");  
				break;
            case EUD_TYPE_R:   
				return("EUD_TYPE_R");   
				break;
            case EUD_TYPE_W:   
				return("EUD_TYPE_W");   
				break;
            case EUD_TYPE_B:   
				return("EUD_TYPE_B");   
				break;
            case EUD_TYPE_UNDEFINED:    
				return("EUD_TYPE_UNDEFINED");
				break;
			#ifdef CCI_ON // Adding channel for coherence
				case EUD_TYPE_AC:    
					return("EUD_TYPE_AC");
					break;
				case EUD_TYPE_CR:    
					return("EUD_TYPE_CR");
					break;
				case EUD_TYPE_CD:    
					return("EUD_TYPE_CD");
					break;
			#endif  
            default: 
				break;
        };
	return (NULL);
};

 
string Convert_eTransType2string(ETransType eType) {
	switch(eType) {
		case ETRANS_TYPE_NORMAL:  
		return("ETRANS_TYPE_NORMAL");  
		break;
		
		case ETRANS_TYPE_FIRST_PTW:  
		return("ETRANS_TYPE_FIRST_PTW");  
		break;
		case ETRANS_TYPE_SECOND_PTW:  
		return("ETRANS_TYPE_SECOND_PTW");  
		break;
		case ETRANS_TYPE_THIRD_PTW:  
		return("ETRANS_TYPE_THIRD_PTW");  
		break;
		case ETRANS_TYPE_FOURTH_PTW:  
		return("ETRANS_TYPE_FOURTH_PTW");  
		break;
		
		case ETRANS_TYPE_FIRST_PF:  
		return("ETRANS_TYPE_FIRST_PF");  
		break;
		case ETRANS_TYPE_SECOND_PF:  
		return("ETRANS_TYPE_SECOND_PF");  
		break;
		
		case ETRANS_TYPE_EVICT:  
		return("ETRANS_TYPE_EVICT");  
		break;
		case ETRANS_TYPE_LINE_FILL:  
		return("ETRANS_TYPE_LINE_FILL");  
		break;
		
		case ETRANS_TYPE_FIRST_APTW:  
		return("ETRANS_TYPE_FIRST_APTW");  
		break;
		case ETRANS_TYPE_SECOND_APTW:  
		return("ETRANS_TYPE_SECOND_APTW");  
		break;
		case ETRANS_TYPE_THIRD_APTW:  
		return("ETRANS_TYPE_THIRD_APTW");  
		break;
		case ETRANS_TYPE_FOURTH_APTW:  
		return("ETRANS_TYPE_FOURTH_APTW");  
		break;
		
		case ETRANS_TYPE_FIRST_RPTW:  
		return("ETRANS_TYPE_FIRST_RPTW");  
		break;
		case ETRANS_TYPE_SECOND_RPTW:  
		return("ETRANS_TYPE_SECOND_RPTW");  
		break;
		case ETRANS_TYPE_THIRD_RPTW:  
		return("ETRANS_TYPE_THIRD_RPTW");  
		break;
		
		case ETRANS_TYPE_UNDEFINED:  
		return("ETRANS_TYPE_UNDEFINED");  
		break;
		default: 
break;
        };
	return (NULL);
};

