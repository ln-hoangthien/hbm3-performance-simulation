//------------------------------------------------------------
// FileName	: CAddrGen.h
// Version	: 0.81
// DATE 	: 1 Nov 2021
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Address generator header 
//------------------------------------------------------------
// Configuration 
//	1. MAX_TRANS_SIZE		: 64 bytes	
//	2. MAX_BURST_LENGTH		: 4 
//	3. IMG_HORIZONTAL_SIZE		: Multiple of 16 pixels 
//	3. TILEH			: 16 or 64 pixels
//	4. TILEV			: 16 pixels (tile), 1 pixel (linear)
//------------------------------------------------------------
#ifndef CADDRGEN_H
#define CADDRGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <string>

#include "Top.h"


using namespace std;

//-------------------------------------------------------------------------------
// Tile size 
//-------------------------------------------------------------------------------
#ifdef TILEH
	#define TILEH			TILEH*(MAX_TRANS_SIZE/BYTE_PER_PIXEL)
#else
	#define TILEH			MAX_TRANS_SIZE/BYTE_PER_PIXEL
#endif

#define TILEV				16								// Vertical 

#define TILEHB				(TILEH * BYTE_PER_PIXEL)					// Bytes. Horizontal. Power-of-2

#define TILEH_MASK			(TILEH - 1)							// 0xF for TILEH 16
#define TILEV_MASK			(TILEV - 1)							// 0xF for TILEV 16

#define TILE_SIZE_BYTE			(TILEH * TILEV * BYTE_PER_PIXEL)				// bytes		


//-------------------------------------------------------------------------------
// Tile mode
// 	Number of tiles 
// Note 
// 	1. Assumed ImgH aligned TILEH. 
//-------------------------------------------------------------------------------
#define NUM_TILE_IN_ROW			((int)(ceilf( (float)IMG_HORIZONTAL_SIZE / (float)TILEH)))				// Number of tiles in image horizontal row 
#define NUM_TILE_IN_COL			((int)(ceilf( (float)IMG_VERTICAL_SIZE   / (float)TILEV)))				// Number of tiles in image vertical column
#define TOTAL_NUM_TILE			((int)(ceilf( (float)NUM_TILE_IN_COL * (float)IMG_HORIZONTAL_SIZE / (float)TILEH)))	// Total number of tiles. ImgV maybe unaligned with TILEV


//-------------------------------------------------------------------------------
// Hierarchical tile
// 	Hierarchical tile size 
//-------------------------------------------------------------------------------
#define hTILEN				2								// (hTILEN) x (hTILEN) cluster tiles in "level-1"
#define hTILEH				(hTILEN * TILEH)						// tile size
#define hTILEV				(hTILEN * TILEV)


//-------------------------------------------------------------------------------
// Hierarchical tile "HT_MINUS"
// 	1. "ht-" algorithm. Band-of-tile
//	2. Simple version of "ht+"
// 	3. Granular row = 2^(GRANULAR_ROW_EXPONENT) tiles. 
//	4. Simple hardware. Performance issue when un-aligned.
//-------------------------------------------------------------------------------
#define GRANULAR_ROW_EXPONENT		((int)(floorf(log2f(IMG_HORIZONTAL_SIZE / TILEH))))		// floor 
#define GRANULAR_ROW_SIZE		((int)pow(2,GRANULAR_ROW_EXPONENT))				// Number of tiles in a granular row
#define logN				((int)(ceilf(log2f(hTILEN))))					// hTILEN power-of-2
// #define HT_MINUS_ENABLE


//-------------------------------------------------------------------------------
// Hierarchical tile "HT_PLUS"
// 	1. "ht+" algorithm. Band-of-tile
// 	2. Granular row = (GRANULAR_COEFFICIENT) x N. 
//	3. General hardware. Perform good when un-aligned
//	4. Multi-level
//		LEVEL 0 : Samsung tile 
//		LEVEL 1 : One   level cluster size (hTILEN   x hTILEN)
//		LEVEL 2 : Two   level cluster size (hTILEN/2 x hTILEN/2)
//		LEVEL 3 : Three level cluster size (hTILEN/4 x hTILEN/4)
//-------------------------------------------------------------------------------
#define GRANULAR_COEFFICIENT		(NUM_TILE_IN_ROW / hTILEN) 
// #define HT_PLUS_LEVEL		1								// Number of levels
// #define HT_PLUS_ENABLE


//-------------------------------------------------------------------------------
// Hierarchical tile "HTA_PLUS", "HTA_MINUS"
// 	1. "hta-" algorithm (Samsung hTILE patent, aligned)
// 	2. "hta+" algorithm (any)
//-------------------------------------------------------------------------------
// #define HTA_MINUS_ENABLE										// Assume ImgH aligned hTILEH
// #define HTA_PLUS_ENABLE										// ImgH any


//-------------------------------------------------------------------------------
// Hierarchical tile dimension
//	"ht+", "ht-", "hta-"	
//-------------------------------------------------------------------------------
#define TOTAL_NUM_GROWNUM		(TOTAL_NUM_TILE / (int)pow(2,GRANULAR_ROW_EXPONENT))			// Total number of granular rows 
#define TOTAL_NUM_hTILE			(TOTAL_NUM_GROWNUM - (TOTAL_NUM_GROWNUM % hTILEN))			// Total number of granular rows our technique applies to


//-------------------------------------------------------------------------------
// Hierarchical tile dimension
//	"hta+"	
//-------------------------------------------------------------------------------
#define NUM_hTILE_ROW			((int)(ceilf( (float)IMG_HORIZONTAL_SIZE / (float)hTILEH)))	// Total number of hTILEs in horizontal row 
#define NUM_hTILE_COL			((int)(ceilf( (float)IMG_VERTICAL_SIZE   / (float)TILEV)))	// Total number of hTILEs in vertical col 

#define LAST_ROW_NUM			((int)(ceilf( (float)IMG_VERTICAL_SIZE   / (float)TILEV)) - 1)	// Last RowNum
#define LAST_COL_NUM			((int)(ceilf( (float)IMG_HORIZONTAL_SIZE / (float)TILEH)) - 1)	// Last ColNum

#define NUM_NORMAL_ROW			((IMG_VERTICAL_SIZE   / hTILEV) * hTILEN)			// Number of normal rows
#define NUM_NORMAL_COL			((IMG_HORIZONTAL_SIZE / hTILEH) * hTILEN)			// Number of normal columns

#define LAST_NORMAL_ROWNUM		(NUM_NORMAL_ROW - 1)
#define LAST_NORMAL_COLNUM		(NUM_NORMAL_COL - 1)


//-------------------------------------------------------------------------------
// BFAM 
//-------------------------------------------------------------------------------
#define SUPER_PAGE_SIZE			(BANK_NUM * PAGE_SIZE)						// bytes
// #define AUTO_BFAM_ENABLE										// Auto map selection. If BFAM condition true, force BFAM even when LIAM chosen


//-------------------------------------------------------------------------------
// Metric analysis 
//-------------------------------------------------------------------------------
// #define METRIC_ANALYSIS_ENABLE


//-------------------------------------------------------------------------------
// CNN
// 	Mask matrix (BLOCK x BLOCK) pixels
//-------------------------------------------------------------------------------
#define BLOCK_CNN			5								// 5 x 5 pixels block 
// #define BLOCK_CNN			3


//-------------------------------------------------------------------------------
// JPEG 
// 	Block (8 x 8) pixels
//-------------------------------------------------------------------------------
#define BLOCK_JPEG			8								// 8 x 8 pixels block


//-------------------------------------------------------------------------------
// Single tile structure
//	"Metric" analysis. Tile, hTILE
//	Bank metric, MMU metric
//-------------------------------------------------------------------------------
typedef struct tagSTileMap* SPTileMap;
typedef struct tagSTileMap{
	int nTileNum;											// Tile number
	// int nTileNumNew;										// Tile number 
	int nMetric;											// Metric of a tile 
}STileMap;


//-------------------------------------------------------------------------------
// Single linear tile structure
//	"Metric" analysis. LIAM, BFAM 
//-------------------------------------------------------------------------------
typedef struct tagSLinearMap* SPLinearMap;
typedef struct tagSLinearMap{
	int64_t nAddr;			// Tile number
	int     nBank;
	int     nSuperPageNumber;
	int     nMetric;		// Metric of a linar tile 
}SLinearMap;


// Address generator 
typedef class CAddrGen* CPAddrGen;
class CAddrGen{

public:
	// 1. Contructor and Destructor
	CAddrGen(string cName, ETransDirType eDir, string cOperation, int64_t nStartAddr);
	CAddrGen(string cName, ETransDirType eDir);
	~CAddrGen();

	// 2. Control
	// Set value
	EResultType	Reset();
	EResultType	UpdateState();

	EResultType	SetOperation(string cOperation);					// ROTATIOIN, RASTER_SCAN, CNN, JPEG 
	EResultType	SetAddrMap(string cAddrMap);						// LIAM, BFAM, TILE 
	EResultType	Set_ScalingFactor(float Num);						// Image size scaling factor

	// EResultType	SetFinalTrans(EResultType eResult);
	EResultType     SetStartAddr(int64_t nAddr);						// Start address
	EResultType     SetNumTotalTrans(int nNum);						// Total number of transactions set by user

	// Block input info
	EResultType     Set_nA(int Num);							// (A, B)  
	EResultType     Set_nB(int Num);
	EResultType     Set_nAsize(int Num);							// (Asize, Bsize)  
	EResultType     Set_nBsize(int Num);
	EResultType     Set_Block(int A, int B, int Asize, int Bsize);				// (A, B), (Asize, Bsize)
	EResultType     Set_Finished_ThisBlock(EResultType);					// Flag

	// Get value
	string		GetName();

	string		GetOperation();
	string		GetAddrMap();
	ETransDirType	GetTransDirType();

	EResultType	IsFinalTrans();								// Last trans of application
	int			GetNumTotalTrans();							// Forced by user 
	EResultType	SetNumPixelTrans();							// Number of pixels of transaction
	int			GetNumPixelTrans();							// Number of pixels of transaction
	EResultType IsFinished_ThisBlock();							// Flag. Block addr gen finished
	int			GetTileNum();								// Tile number of transaction 

	int64_t		GetAddr(string cAddrMap);
	int64_t		GetAddr_TILE();								// Tile, hTile
	int64_t		GetAddr_LIAM();
	int64_t		GetAddr_CIAM();
	int64_t		GetAddr_BFAM();
	int64_t		GetAddr_BGFAM();
	int64_t		GetAddr_OIRAM();
	int64_t		GetAddr_FlatFish();
	
	int64_t		GetAddr_MIN_K_UNION_ROW_WISE();
	int64_t		GetAddr_MIN_K_UNION_COL_WISE();
	int64_t		GetAddr_MIN_K_UNION_BOTH_WISE();
	int64_t		GetAddr_MIN_K_UNION_TILE();
	int64_t		GetAddr_MIN_K_UNION_TILE_ROW_WISE();
	int64_t		GetAddr_MIN_K_UNION_TILE_COL_WISE();
	int64_t		GetAddr_MIN_K_UNION_TILE_ROW_COL_WISE();
	int64_t		GetAddr_MIN_K_UNION_STRIDE();

	int64_t		GetAddr_NEAR_OPTIMAL_ROW_WISE();
	int64_t		GetAddr_NEAR_OPTIMAL_COL_WISE();
	int64_t		GetAddr_NEAR_OPTIMAL_BOTH_WISE();
	int64_t		GetAddr_NEAR_OPTIMAL_TILE();
	int64_t		GetAddr_NEAR_OPTIMAL_TILE_ROW_WISE();
	int64_t		GetAddr_NEAR_OPTIMAL_TILE_COL_WISE();
	int64_t		GetAddr_NEAR_OPTIMAL_TILE_ROW_COL_WISE();
	int64_t		GetAddr_NEAR_OPTIMAL_STRIDE();

	// BFAM
	int		GetBank_BFAM_BF(int nBank);  						// Get flipped bank number. BANK_FLIP, BANK_FLIP_MINUS algorithm
	int		GetBank_BFAM_BS_MINUS(int nBank, int nSPN, int nGSPN, int64_t nAddr);	// Get shuffled bank number. BANK_SHUFFLE algorithm
	int		GetBank_BFAM_BS_PLUS (int nBank, int nSPN, int nGSPN, int64_t nAddr);

	// Tile "ht-" 
	int		GetMask_PairNum(int Granular_Row_Exponent);				// "ht-" Mask.
	int		GetMask_CRowNum(int Granular_Row_Exponent);				// "ht-" Mask.
	
	// Metric
	EResultType	CalculateTileBankMetric();	
	EResultType	CalculateTileMMUMetric();	
	int		GetTileBankMetric();	
	int		GetTileMMUMetric();	
	EResultType	CalculateLinearBankMetric();	
	int		GetLinearBankMetric();	

	// Debug
	EResultType	CheckAddr();
	EResultType	Display();

private:

	// Control info
	string		cName;				// Name
	ETransDirType	eDir;				// Read, Write
	string		cOperation;			// RASTER_SCAN, ROTATION 
	string		cAddrMap;			// LIAM, BFAM, TILE

	int		nNumTotalTrans;			// Number of total transactions. Forced by user.
	int		nCurTrans;			// Current transaction number

	int64_t		nStartAddr;			// Start addr
	EResultType	eFinalTrans;			// Final trans of application 
	EResultType	IsTransGenThisCycle;		// Is address generated in this cycle
	float		ScalingFactor;			// Image horizontal size scale. Scaler scenario.

	// Linear, BFAM
	// int		nCurRow;			// Current row 
	// int		nCurCol;			// Current column
	int		nSuperPageNum;			// Superpage number. Number of rows (in memory) for super-page
	int		TileHBase;
	int		TileVBase;

	// SLIAM
	int		ImgH0;				// Map0 (pixels)
	int		ImgHB0;				// Map0 (bytes)
	int		ImgH1;				// Map0 (pixels)
	int		ImgSize0;			// Map0 size (pixels)

	// Block input info
	int		nA;				// (A, B) block start coordinate (pixel)
	int		nB;
	int		nAsize;				// (Asize, Bsize) block size (pixels)
	int		nBsize;

	// Variable 
	int		nTileNum;			// Tile number of (Apos, Bpos)
	int		nTileRowNum;			// Tile row number of Bpos
	int		nTileColNum;			// Tile row number of Apos. hTile
	int		nNumPixelTrans;			// Number of pixels of a transaction 
	int		nApos;				// Apos temporary horizontal (pixel)
	int		nBpos;				// Bpos temporary vertical (pixel)
	int		nAposInTile;			// Apos in tile (pixel)
	int		nBposInTile;
	int		nAsizeT;			// (Asize, Bsize) temporary block size (pixels)
	int		nBsizeT;
	EResultType	eFinished_ThisBlock;		// Block addr gen finished 

	// Metric
	SPLinearMap**	spLinearMap;			// [IMG_VERTICAL_SIZE][NUM_COLUMNS_PER_ROW]
	int		nLinearBankMetricTotal;		// Linear map total bank metric

	SPTileMap**	spTileMap;			// [NUM_TILE_IN_COL][NUM_TILE_IN_ROW]
	// int**	nTileMap;			// Tile map grid [NUM_TILE_IN_COL][NUM_TILE_IN_ROW]
	// int**	nTileMetric;			// Tile map metric [NUM_TILE_IN_COL][NUM_TILE_IN_ROW]
	int		nTileBankMetricTotal;		// Total bank metric
	int		nTileMMUMetricTotal;		// Total MMU metric
};

#endif

