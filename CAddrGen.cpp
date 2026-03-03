//------------------------------------------------------------------------------------------
// FileName	: CAddrGen.cpp
// Version	: 0.84
// Date		: 1 Nov 2021
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Address generator class definition
//------------------------------------------------------------------------------------------
// Address gen spec
//		1.  Address		: RBC format
//		2.  Pixel		: 4 bytes or 1 byte.
//		3.  TILEH		: 16 pixels (BYTE_PER_PIXEL 4), 64 pixels (BYTE_PER_PIXEL 1)
//		4.  TILEV		: 16 pixels (tile), 1 pixel (linear)
// 		5.  int64_t		: Max address 0x7FFF_FFFF_FFFF_FFFF (64-bit VA)
//		6.  IMG_HORIZONTAL_SIZE	: Multiple of 16 pixels
//		7.  IMG_VERTICAL_SIZE	: Multiple of 16 pixels 
//		8.  Data width		: 16 bytes
//		9.  Max burst length	: 4 data
//		10. Max trans size	: 64 bytes
//		11. Operation 		: Coordinate "(A, B)". Block size "(Asize, Bsize)"
//------------------------------------------------------------------------------------------
// Address map type
//  		1. "LIAM"		: Raster-scan linear 
//  		   "SLIAM"		: Split LIAM. Map0 (LIAM). Map1 (LIAM)
//  		2. "BFAM"		: Raster-scan linear. Bank flip 
// 		3. "TILE"		: (1) Tile-level linear (2) Linear inside a tile 
//------------------------------------------------------------------------------------------
// Address map options
//	Linear
//		1. "LIAM"		: Linear
//		   "SLIAM"		: Split LIAM
//		2. "BANK_FLIP"		: BFAM. Samsung patent
//		3. "BANK_FLIP_PLUS"	: BFAM
//		4. "BANK_FLIP_MINUS"	: BFAM. Group size power-of-2
//		5. "BANK_SHUFFLE"	: BSAM
//		6. "BANK_SHUFFLE_MINUS"	: BSAM. Group size power-of-2
//	Tile
//		1. "TILE"		: Tile. Samsung patent
//		2. "HT_PLUS"		: Hierarchical tile (hTILE). Band-of-tile. Hierarchy 1 level. Graunular row kxn.
//		3. "HT_MINUS"		: Simplified "HT_PLUS"
//		4. "HTA_PLUS"		: Hierarchical tile (hTILE). 2D tile
//		5. "HTA_MINUS"		: Simplified "HTA_PLUS"
//------------------------------------------------------------------------------------------
// Operation
// 	"RASTER_SCAN"	
//		1. Operation		: Block level raster-scan
// 		2. Block size		: (Asize, Bsize) = (16, 1)
//		3. Address		: Aligned with "max transaction size" 64 bytes
//		4. Burst length		: 4
//		5. Num. trans		: ImgH * IMG_VERTICAL_SIZE * 4 / MAX_TRANS_SIZE
// 	"ROTATION"	
//		1. Operation		: Block level vertical
// 		2. Block size		: (Asize, Bsize) = (16, 1)
//		3. Address 		: Aligned with "max transaction size" 64 bytes
//		4. Burst length		: 4
//		5. Num. trans		: ImgH * IMG_VERTICAL_SIZE * 4 / MAX_TRANS_SIZE
//	"CNN" READ
//		1. Operation		: Block level "convolved"
// 		2. Block size		: (Asize, Bsize) = (BLOCK_CNN, BLOCK_CNN) = (3, 3)
//		3. Address		: Can be un-aligned with transaction size.
//		4. Burst length		: 1
//		5. Num. trans		: (ImgH - 2) * (IMG_VERTICAL_SIZE - 2) * 3 + (Total number of tiles) * 6 when BLOCK_CNN is 3
//	"CNN" WRITE
// 		1. Operation		: Block level raster-scan
// 		2. Block size		: (Asize, Bsize) = (1, 1)
//		3. Address		: Can be un-aligned with transaction size.
//		4. Burst length		: 1
//		5. Num. trans		: ImgH * IMG_VERTICAL_SIZE
//	"JPEG" READ
//		1. Operation		: "Block level linear"
// 		2. Block size		: (Asize, Bsize) = (BLOCK_JPEG, BLOCK_JPEG) = (8, 8)
//		3. Address		: Can be un-aligned with transaction size.
//		4. Burst length		: 4
//		5. Num. trans		: Ceil (ImgH / BLOCK_JPG) x Ceil (IMG_VERTICAL_SIZE / BLOCK_JPEG)
//	"JPEG" WRITE
// 		1. Operation		: Block level stream compressed
// 		2. Block size		: (Asize, Bsize) = (8, 1)
//		3. Address		: Can be un-aligned with transaction size.
//		4. Burst length		: 2
//		5. Num. trans		: Ceil (ImgH / BLOCK_JPG) x Ceil (IMG_VERTICAL_SIZE / BLOCK_JPEG)
//		6. Note			: Address map is "linear"
//	"RANDOM"
// 		1. Operation		: Random address
// 		2. Block size		: (Asize, Bsize) = (16, 1)
//		3. Address 		: Aligned with "max transaction size" 64 bytes
//		4. Burst length		: 4
//		5. Num. trans		: ImgH * IMG_VERTICAL_SIZE * BYTE_PER_PIXEL / MAX_TRANS_SIZE
//	"TRACE"
// 		1. Operation		: Trace 
//------------------------------------------------------------------------------------------
// Operation "ROTATION" scenarios
//	Different degree, different pattern.
//	Default is "ROTATION_LEFT_TOP_VER"
//	
// 					  Start coordinate	Access		Scenario			Pattern	
//	1. "ROTATION_LEFT_BOT_VER"	: left  bottom		Vertical	90  degree rotation		Descend			
//	2. "ROTATION_RIGHT_BOT_HOR"	: right bottom		Horizontal	180 degree rotation		Descend
//	3. "ROTATION_RIGHT_TOP_VER"	: right top		Vertical	270 degree rotation		Ascend
//
//	4. "ROTATION_LEFT_TOP_VER"	: left  top		Vertical	90  degree rotation flip	Ascend
//	5. "ROTATION_LEFT_BOT_HOR"	: left  bottom		Horizontal	180 degree rotation flip	Ascend
//	6. "ROTATION_RIGHT_BOT_VER"	: right bottom		Vertical	270 degree rotation flip	Descend
//------------------------------------------------------------------------------------------
// Known issues 
//		1. When BYTE_PER_PIXEL 1 byte, ImgH should be aligned to TILEH 
//		2. When BYTE_PER_PIXEL 1 byte, PIXELS_PER_TRANS is 64. // FIXME check
//------------------------------------------------------------------------------------------
// To do 
//		1. Conversion tile and linear
//------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <string>

#include "CAddrGen.h"
#include "Memory.h"


// Construct
CAddrGen::CAddrGen(string cName, ETransDirType eDir, string cOperation, int64_t nStartAddr) {

	// Generate and initialize
	this->cName = cName;

	this->cOperation = cOperation;
	this->cAddrMap   = "LIAM";
	this->eDir       = eDir;

	// this->nCurRow   = 0;
	// this->nCurCol   = 0;
	this->nLinearBankMetricTotal   = -1;

	this->nNumTotalTrans = 0;
	this->nCurTrans      = 0;

	this->nStartAddr     = nStartAddr;
	this->eFinalTrans    = ERESULT_TYPE_UNDEFINED;

	this->IsTransGenThisCycle = ERESULT_TYPE_UNDEFINED;

	this->ScalingFactor  = -1;

	this->nSuperPageNum  = 0;
	this->TileHBase  	 = 0;
	this->TileVBase  	 = 0;

	// SLIAM
	this->ImgH0    = IMG_HORIZONTAL_SIZE;
	this->ImgHB0   = IMG_HORIZONTAL_SIZE * BYTE_PER_PIXEL;
	this->ImgH1    = 0;
	this->ImgSize0 = IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE;

	this->nTileNum       = -1;
	this->nTileRowNum    = -1;
	this->nTileColNum    = -1;
	this->nNumPixelTrans = -1;
	this->nApos          = -1;
	this->nBpos          = -1;
	this->nAposInTile    = -1;
	this->nBposInTile    = -1;
	this->nAsizeT	     = -1;
	this->nBsizeT	     = -1;

	this->eFinished_ThisBlock = ERESULT_TYPE_UNDEFINED;

	#ifdef METRIC_ANALYSIS_ENABLE
	this->spLinearMap = new SLinearMap** [IMG_VERTICAL_SIZE];
	for (int i=0; i<IMG_VERTICAL_SIZE; i++) {
		this->spLinearMap[i] = new SLinearMap* [NUM_COLUMNS_PER_ROW];
		for (int j=0; j<NUM_COLUMNS_PER_ROW; j++) {
			this->spLinearMap[i][j] = new SLinearMap;
		};
	};

	this->spTileMap = new STileMap** [NUM_TILE_IN_COL];
	for (int i=0; i<NUM_TILE_IN_COL; i++) {
		this->spTileMap[i] = new STileMap* [NUM_TILE_IN_ROW];
		for (int j=0; j<NUM_TILE_IN_ROW; j++) {
			this->spTileMap[i][j] = new STileMap;
		};
	};
	#endif

	this->nTileBankMetricTotal = -1;
	this->nTileMMUMetricTotal  = -1;
	
	this->nA     = -1;
	this->nB     = -1;
	this->nAsize = -1;
	this->nBsize = -1;
};


// Construct
CAddrGen::CAddrGen(string cName, ETransDirType eDir) {

	// Generate and initialize
	this->cName = cName;
	this->eDir  = eDir;

	this->nNumTotalTrans = 0;
	this->nCurTrans      = 0;

	// this->nCurRow   = 0;
	// this->nCurCol   = 0;
	this->nLinearBankMetricTotal   = -1;

	this->nStartAddr  = -1;
	this->eFinalTrans = ERESULT_TYPE_UNDEFINED;

	this->ScalingFactor = -1;

	this->IsTransGenThisCycle = ERESULT_TYPE_UNDEFINED;

	this->nSuperPageNum = 0;
	this->TileHBase  	 = 0;
	this->TileVBase  	 = 0;

	// SLIAM
	this->ImgH0    = IMG_HORIZONTAL_SIZE;
	this->ImgHB0   = IMG_HORIZONTAL_SIZE * BYTE_PER_PIXEL;
	this->ImgH1    = 0;
	this->ImgSize0 = IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE;

	// Tile
	this->nTileNum       = -1;
	this->nTileRowNum    = -1;
	this->nTileColNum    = -1;
	this->nNumPixelTrans = -1;
	this->nApos          = -1;
	this->nBpos          = -1;
	this->nAposInTile    = -1;
	this->nBposInTile    = -1;
	this->nAsizeT        = -1;
	this->nBsizeT        = -1;

	this->eFinished_ThisBlock = ERESULT_TYPE_UNDEFINED;

	#ifdef METRIC_ANALYSIS_ENABLE
	this->spLinearMap = new SLinearMap** [IMG_VERTICAL_SIZE];
	for (int i=0; i<IMG_VERTICAL_SIZE; i++) {
		this->spLinearMap[i] = new SLinearMap* [NUM_COLUMNS_PER_ROW];
		for (int j=0; j<NUM_COLUMNS_PER_ROW; j++) {
			this->spLinearMap[i][j] = new SLinearMap;
		};
	};

	this->spTileMap = new STileMap** [NUM_TILE_IN_COL];
	for (int i=0; i<NUM_TILE_IN_COL; i++) {
		this->spTileMap[i] = new STileMap* [NUM_TILE_IN_ROW];
		for (int j=0; j<NUM_TILE_IN_ROW; j++) {
			this->spTileMap[i][j] = new STileMap;
		};
	};
	#endif

	this->nTileBankMetricTotal = -1;
	this->nTileMMUMetricTotal  = -1;

	this->nA     = -1;
	this->nB     = -1;
	this->nAsize = -1;
	this->nBsize = -1;

};


// Destruct
CAddrGen::~CAddrGen() {

	// delete this->nMap;
	// this->nMap = NULL;
};


// Reset 
EResultType CAddrGen::Reset() {

	this->nCurTrans = 1;
	// this->nNumTotalTrans = 0;				// If set, before reset

	// this->nCurRow   = 0;
	// this->nCurCol   = 0;
	this->nLinearBankMetricTotal   = -1;

	// this->nStartAddr = 0; 				// Set before reset

	this->eFinalTrans = ERESULT_TYPE_NO;			// Set all trans of application generated

	this->IsTransGenThisCycle = ERESULT_TYPE_NO;

	this->ScalingFactor  = 1; 				// No scaling

	this->nSuperPageNum  = 0;
	this->TileHBase  	 = 0;
	this->TileVBase  	 = 0;
	
	// SLIAM
	#ifdef SPLIT_BANK_SHUFFLE 
	int ImgHB = IMG_HORIZONTAL_SIZE * BYTE_PER_PIXEL;
	this->ImgHB0 = ImgHB;

	if (this->cOperation == "ROTATION" or this->cOperation == "ROTATION_LEFT_BOT_VER" or this->cOperation == "ROTATION_RIGHT_TOP_VER" or \
		       	this->cOperation == "ROTATION_LEFT_TOP_VER" or this->cOperation == "ROTATION_RIGHT_BOT_VER") {
		if (ImgHB > (SUPER_PAGE_SIZE/2)) {
			this->ImgHB0 = (SUPER_PAGE_SIZE/2) * (ImgHB / (SUPER_PAGE_SIZE/2)); 
		};
	};

	this->ImgH0    = ImgHB0 / BYTE_PER_PIXEL;
	this->ImgH1    = IMG_HORIZONTAL_SIZE - ImgH0;
	this->ImgSize0 = this->ImgH0 * IMG_VERTICAL_SIZE; 
	#endif


	this->nTileNum       = 0;
	this->nTileRowNum    = 0;
	this->nTileColNum    = 0;
	this->nNumPixelTrans = 0;
	this->nAposInTile    = 0;
	this->nBposInTile    = 0;

	this->eFinished_ThisBlock = ERESULT_TYPE_NO;		// Set when last trans of block generated

	#ifdef METRIC_ANALYSIS_ENABLE
	for (int i=0; i<IMG_VERTICAL_SIZE; i++) {
		for (int j=0; j<NUM_COLUMNS_PER_ROW; j++) {
			this->spLinearMap[i][j]->nAddr = -1;
			this->spLinearMap[i][j]->nBank = -1;
			this->spLinearMap[i][j]->nSuperPageNumber = -1;
			this->spLinearMap[i][j]->nMetric = -1;
		};
	};

	for (int i=0; i<NUM_TILE_IN_COL; i++) {
		for (int j=0; j<NUM_TILE_IN_ROW; j++) {
			this->spTileMap[i][j]->nTileNum = -1;
			this->spTileMap[i][j]->nMetric  = -1;
		};
	};
	#endif

	this->nTileBankMetricTotal  = -1;
	this->nTileMMUMetricTotal   = -1;

	// Initial block size 
	//	Assume operation set before reset
	if (this->cOperation == "CNN") {

		// Initial block coordinate
		this->nA = 0;
		this->nB = 0;

		this->nApos = 0;
		this->nBpos = 0;

		if (this->GetTransDirType() == ETRANS_DIR_TYPE_READ) {
			this->nAsize  = BLOCK_CNN;
			this->nBsize  = BLOCK_CNN;
			this->nAsizeT = BLOCK_CNN;
			this->nBsizeT = BLOCK_CNN;
		}
		else if (this->GetTransDirType() == ETRANS_DIR_TYPE_WRITE) {
			this->nAsize  = 1;
			this->nBsize  = 1;
			this->nAsizeT = 1;
			this->nBsizeT = 1;
		}
		else {
			assert (0);
		};
	}
	else if (this->cOperation == "JPEG") {

		// Initial block coordinate
		this->nA = 0;
		this->nB = 0;

		this->nApos = 0;
		this->nBpos = 0;

		if (this->GetTransDirType() == ETRANS_DIR_TYPE_READ) {
			this->nAsize  = BLOCK_JPEG;
			this->nBsize  = BLOCK_JPEG;
			this->nAsizeT = BLOCK_JPEG;
			this->nBsizeT = BLOCK_JPEG;
		}
		else if (this->GetTransDirType() == ETRANS_DIR_TYPE_WRITE) {
			this->nAsize  = 8;
			this->nBsize  = 1;
			this->nAsizeT = 8;
			this->nBsizeT = 1;
		}
		else {
			assert (0);
		};
	}
	else if (this->cOperation == "RASTER_SCAN" or 
			this->cOperation == "TILE" or 
		 	this->cOperation == "ROTATION"    or 
		 	this->cOperation == "ROTATION_LEFT_TOP_VER"  or 
		 	this->cOperation == "RANDOM") {

		// Initial block coordinate
		this->nA = 0;
		this->nB = 0;

		this->nApos = 0;
		this->nBpos = 0;

		this->nAsize  = TILEH;
		this->nBsize  = 1;
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;
	}
	else if (this->cOperation == "ROTATION_LEFT_BOT_VER"  or
		 this->cOperation == "ROTATION_LEFT_BOT_HOR") {

		// Initial block coordinate
		this->nA = 0;
		this->nB = IMG_VERTICAL_SIZE - 1;

		this->nApos = 0;
		this->nBpos = IMG_VERTICAL_SIZE - 1;

		this->nAsize  = TILEH;
		this->nBsize  = 1;
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;
	}
	else if (this->cOperation == "ROTATION_RIGHT_BOT_HOR" or
		 this->cOperation == "ROTATION_RIGHT_BOT_VER") {

		// Initial block coordinate
		this->nA = IMG_HORIZONTAL_SIZE - TILEH;
		this->nB = IMG_VERTICAL_SIZE - 1;

		this->nApos = IMG_HORIZONTAL_SIZE - TILEH;
		this->nBpos = IMG_VERTICAL_SIZE - 1;

		this->nAsize  = TILEH;
		this->nBsize  = 1;
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;
	}
	else if (this->cOperation == "ROTATION_RIGHT_TOP_VER") {

		// Initial block coordinate
		this->nA = IMG_HORIZONTAL_SIZE - TILEH;
		this->nB = 0;

		this->nApos = IMG_HORIZONTAL_SIZE - TILEH;
		this->nBpos = 0;

		this->nAsize  = TILEH;
		this->nBsize  = 1;
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;
	}
	else if (this->cOperation == "TRACE") {
		// No reset op
	}
	else {
		assert (0);
	};
	

	return (ERESULT_TYPE_SUCCESS);
};


// // Set final trans 
// EResultType CAddrGen::SetFinalTrans(EResultType eResult) {
//
//	this->eFinalTrans = eResult;
//	return (ERESULT_TYPE_SUCCESS);
// };


// Set start addr 
EResultType CAddrGen::SetStartAddr(int64_t nAddr) {

	this->nStartAddr = nAddr;
	return (ERESULT_TYPE_SUCCESS);
};


// Set total number Ax 
EResultType CAddrGen::SetNumTotalTrans(int nNum) {

	this->nNumTotalTrans = nNum;
	return (ERESULT_TYPE_SUCCESS);
};


//---------------------------------------------------
// Set operation 
// 	RASTER_SCAN, ROTATION, RANDOM, CNN, JPEG
//---------------------------------------------------
EResultType CAddrGen::SetOperation(string cOperation) {

	this->cOperation = cOperation;
	return (ERESULT_TYPE_SUCCESS);
};


// Set image size scaling factor 
EResultType CAddrGen::Set_ScalingFactor(float Num) {

	this->ScalingFactor = Num;
	return (ERESULT_TYPE_YES);
};


//-----------------------------------------------
// Set address map 
// 	LIAM, BFAM, TILE
//-----------------------------------------------
EResultType CAddrGen::SetAddrMap(string cAddrMap) {

	this->cAddrMap = cAddrMap;
	return (ERESULT_TYPE_SUCCESS);
};


// Set (A, B) 
EResultType CAddrGen::Set_nA(int Num) {

	this->nA = Num;
	return (ERESULT_TYPE_SUCCESS);
};


// Set (A, B) 
EResultType CAddrGen::Set_nB(int Num) {

	this->nB = Num;
	return (ERESULT_TYPE_SUCCESS);
};


// Set (Asize, Bsize) 
EResultType CAddrGen::Set_nAsize(int Num) {

	this->nAsize = Num;
	return (ERESULT_TYPE_SUCCESS);
};


// Set (Asize, Bsize) 
EResultType CAddrGen::Set_nBsize(int Num) {

	this->nBsize = Num;
	return (ERESULT_TYPE_SUCCESS);
};


// Set block (A, B), (Asize, Bsize) 
EResultType CAddrGen::Set_Block(int nA, int nB, int nAsize, int nBsize) {

	this->nA = nA;
	this->nB = nB;
	this->nAsize = nAsize;
	this->nBsize = nBsize;
	return (ERESULT_TYPE_SUCCESS);
};


// Set addr gen block finished 
EResultType CAddrGen::Set_Finished_ThisBlock(EResultType eResult) {

     this->eFinished_ThisBlock = eResult;
     return (ERESULT_TYPE_SUCCESS);
};


// Get total number of transactions 
int CAddrGen::GetNumTotalTrans() {

	return (this->nNumTotalTrans);
};


// Get name
string CAddrGen::GetName() {

	return (this->cName);
};


// Get operation 
string CAddrGen::GetOperation() {

	return (this->cOperation);
};


// Get address map 
string CAddrGen::GetAddrMap() {

	return (this->cAddrMap);
};


// Check final trans
EResultType CAddrGen::IsFinalTrans() {

	return (this->eFinalTrans);
};


// Check addr gen block finished 
EResultType CAddrGen::IsFinished_ThisBlock() {

	return (this->eFinished_ThisBlock);
};


// Get metric 
int CAddrGen::GetLinearBankMetric() {

	return (this->nLinearBankMetricTotal);
};


// Get metric 
int CAddrGen::GetTileBankMetric() {

	return (this->nTileBankMetricTotal);
};


// Calculate metric 
EResultType CAddrGen::CalculateLinearBankMetric() {

	//---------------------------------------------------
	// Analyze metric when all finished
	// 	Same bank, same row		: best case
	// 	Different bank			: ok 
	// 	Same bank, different row	: not ok 
	//---------------------------------------------------
	this->nLinearBankMetricTotal = 0; 

	int nMetric_me_up    = 0;
	int nMetric_me_down  = 0;
	int nMetric_me_left  = 0;
	int nMetric_me_right = 0;

	int bank_me    = 0;
	int bank_up    = 0;
	int bank_down  = 0;
	int bank_left  = 0;
	int bank_right = 0;

	int row_me    = 0;
	int row_up    = 0;
	int row_down  = 0;
	int row_left  = 0;
	int row_right = 0;

	for (int i=0; i<IMG_VERTICAL_SIZE; i++) {		// Row
		for (int j=0; j<NUM_COLUMNS_PER_ROW; j++) {	// Column

			// Get bank number	
			bank_me    = this->spLinearMap[i][j]->nBank; 		
			bank_left  = (j > 0)                     ?	this->spLinearMap[i][j-1]->nBank: 0;		// 0 dummy 		
			bank_right = (j < NUM_COLUMNS_PER_ROW-1) ?	this->spLinearMap[i][j+1]->nBank: 0; 		
			bank_up    = (i > 0 )                    ?	this->spLinearMap[i-1][j]->nBank: 0; 		
			bank_down  = (i < IMG_VERTICAL_SIZE-1)   ?	this->spLinearMap[i+1][j]->nBank: 0; 		

			// Get row number
			row_me    = this->spLinearMap[i][j]->nSuperPageNumber; 		
			row_left  = (j > 0)                     ?	this->spLinearMap[i][j-1]->nSuperPageNumber: 0;	// 0 dummy 		
			row_right = (j < NUM_COLUMNS_PER_ROW-1) ?	this->spLinearMap[i][j+1]->nSuperPageNumber: 0; 		
			row_up    = (i > 0 )                    ?	this->spLinearMap[i-1][j]->nSuperPageNumber: 0; 		
			row_down  = (i < IMG_VERTICAL_SIZE-1)   ?	this->spLinearMap[i+1][j]->nSuperPageNumber: 0; 		

			// Check bank interleave 
			if      (bank_me == bank_up    and row_me == row_up) 	nMetric_me_up    = 2;
			else if (bank_me != bank_up                        ) 	nMetric_me_up    = 1;
			else if (bank_me == bank_up    and row_me != row_up) 	nMetric_me_up    = -1;
			else 							assert(0);

			if      (bank_me == bank_down  and row_me == row_down) 	nMetric_me_down  = 2;
			else if (bank_me != bank_down                        ) 	nMetric_me_down  = 1;
			else if (bank_me == bank_down  and row_me != row_down) 	nMetric_me_down  = -1;
			else 							assert(0);

			if      (bank_me == bank_left  and row_me == row_left) 	nMetric_me_left  = 2;
			else if (bank_me != bank_left                        ) 	nMetric_me_left  = 1;
			else if (bank_me == bank_left  and row_me != row_left) 	nMetric_me_left  = -1;
			else 							assert(0);

			if      (bank_me == bank_right and row_me == row_right) nMetric_me_right = 2;
			else if (bank_me != bank_right                        ) nMetric_me_right = 1;
			else if (bank_me == bank_right and row_me != row_right) nMetric_me_right = -1;
			else 							assert(0);

			// Calculate only access pattern	
			if (this->cOperation == "RASTER_SCAN") {	// Don't care up, down
				nMetric_me_up    = 0;
				nMetric_me_down  = 0;
			} 
			else if (this->cOperation == "ROTATION") {	// Don't care left, right
				nMetric_me_left  = 0;
				nMetric_me_right = 0;
			} 
			else {
				assert(0);
			};
			

			// Metric of a tile	
			if      (i == 0                   and j == 0) 				this->spLinearMap[i][j]->nMetric = nMetric_me_down + nMetric_me_right;
			else if (i == IMG_VERTICAL_SIZE-1 and j == 0)  				this->spLinearMap[i][j]->nMetric = nMetric_me_up   + nMetric_me_right;
			else if (i == 0                   and j == NUM_COLUMNS_PER_ROW-1)  	this->spLinearMap[i][j]->nMetric = nMetric_me_down + nMetric_me_left;
			else if (i == IMG_VERTICAL_SIZE-1 and j == NUM_COLUMNS_PER_ROW-1) 	this->spLinearMap[i][j]->nMetric = nMetric_me_up   + nMetric_me_left;
			else if (                             j == 0) 				this->spLinearMap[i][j]->nMetric = nMetric_me_up   + nMetric_me_down   + nMetric_me_right;
			else if (                             j == NUM_COLUMNS_PER_ROW-1) 	this->spLinearMap[i][j]->nMetric = nMetric_me_up   + nMetric_me_down   + nMetric_me_left;
			else if (i == 0) 							this->spLinearMap[i][j]->nMetric = nMetric_me_down + nMetric_me_left   + nMetric_me_right;
			else if (i == IMG_VERTICAL_SIZE-1) 					this->spLinearMap[i][j]->nMetric = nMetric_me_up   + nMetric_me_left   + nMetric_me_right;
			else if (i <  IMG_VERTICAL_SIZE-1 and j  < NUM_COLUMNS_PER_ROW-1) 	this->spLinearMap[i][j]->nMetric = nMetric_me_up   + nMetric_me_down   + nMetric_me_left   + nMetric_me_right;
			else									assert(0);


			// Total metric accumulated
			this->nLinearBankMetricTotal = this->nLinearBankMetricTotal + this->spLinearMap[i][j]->nMetric;
		};
	};

	return (ERESULT_TYPE_SUCCESS);
};


// Calculate metric 
EResultType CAddrGen::CalculateTileBankMetric() {

	//---------------------------------------------------
	// Analyze metric when all finished
	// 	Page hit 			: best case
	// 	Page miss, within PTW coverage	: ok 
	// 	Page miss, beyond PTW coverage	: not desired 
	//---------------------------------------------------
	this->nTileBankMetricTotal = 0;

	// Tile number distance PTW can cover
	int nDistPageHit  = (pow(2,BIT_4K_PAGE)) / TILE_SIZE_BYTE;			// 4  (= 4096/1024)    for BYTE_PER_PIXEL 4. 16  for BYTE_PER_PIXEL 1 
	int nDistPTWCover = NUM_PTE_PTW * (pow(2,BIT_4K_PAGE)) / TILE_SIZE_BYTE;	// 64 (= 16x4096/1024) for BYTE_PER_PIXEL 4. 256 for BYTE_PER_PIXEL 1. ARMv7 16 PTEs = 64kB

	int nMetric_me_up    = 0;
	int nMetric_me_down  = 0;
	int nMetric_me_left  = 0;
	int nMetric_me_right = 0;

	int me    = 0;
	int up    = 0;
	int down  = 0;
	int left  = 0;
	int right = 0;

	for (int i=0; i<NUM_TILE_IN_COL; i++) {
		for (int j=0; j<NUM_TILE_IN_ROW; j++) {
	
			// Tile number neighbor	
			me    = this->spTileMap[i][j]->nTileNum; 		
			left  = (j > 0)               	? this->spTileMap[i][j-1]->nTileNum: 0;	// 0 dummy 		
			right = (j < NUM_TILE_IN_ROW-1) ? this->spTileMap[i][j+1]->nTileNum: 0; 		
			up    = (i > 0 )              	? this->spTileMap[i-1][j]->nTileNum: 0; 		
			down  = (i < NUM_TILE_IN_COL-1)	? this->spTileMap[i+1][j]->nTileNum: 0; 		

			// Distance tile number
			int dist_me_up    = abs(me - up); 
			int dist_me_down  = abs(me - down); 
			int dist_me_left  = abs(me - left); 
			int dist_me_right = abs(me - right); 

			// Check PTW can cover distance
			if      (dist_me_up    <= nDistPageHit) 	nMetric_me_up    = 2;
			else if (dist_me_up    <  nDistPTWCover)	nMetric_me_up    = 1;	
			else if (dist_me_up    >= nDistPTWCover)	nMetric_me_up    = -1;	
			else 						assert(0);
	
			if      (dist_me_down  <= nDistPageHit) 	nMetric_me_down  = 2;
			else if (dist_me_down  <  nDistPTWCover)	nMetric_me_down  = 1;	
			else if (dist_me_down  >= nDistPTWCover)	nMetric_me_down  = -1;	
			else 		 	     			assert(0);
	
			if      (dist_me_left  <= nDistPageHit) 	nMetric_me_left  = 2;
			else if (dist_me_left  <  nDistPTWCover)	nMetric_me_left  = 1;	
			else if (dist_me_left  >= nDistPTWCover)	nMetric_me_left  = -1;	
			else 		 	     			assert(0);
	
			if      (dist_me_right <= nDistPageHit) 	nMetric_me_right = 2;
			else if (dist_me_right <  nDistPTWCover)	nMetric_me_right = 1;	
			else if (dist_me_right >= nDistPTWCover)	nMetric_me_right = -1;	
			else 						assert(0);

			// Calculate only access pattern	
			if (this->cOperation == "RASTER_SCAN") {	// Don't care up, down
				nMetric_me_up    = 0;
				nMetric_me_down  = 0;
			} 
			else if (this->cOperation == "ROTATION") {	// Don't care left, right
				nMetric_me_left  = 0;
				nMetric_me_right = 0;
			} 
			else {
				assert(0);
			};

			// Metric of a tile	
			if      (i == 0                 and j == 0) 			this->spTileMap[i][j]->nMetric = nMetric_me_down + nMetric_me_right;
			else if (i == NUM_TILE_IN_COL-1 and j == 0)  			this->spTileMap[i][j]->nMetric = nMetric_me_up   + nMetric_me_right;
			else if (i == 0                 and j == NUM_TILE_IN_ROW-1) 	this->spTileMap[i][j]->nMetric = nMetric_me_down + nMetric_me_left;
			else if (i == NUM_TILE_IN_COL-1 and j == NUM_TILE_IN_ROW-1) 	this->spTileMap[i][j]->nMetric = nMetric_me_up   + nMetric_me_left;
			else if (                           j == 0) 			this->spTileMap[i][j]->nMetric = nMetric_me_up   + nMetric_me_down   + nMetric_me_right;
			else if (                           j == NUM_TILE_IN_ROW-1) 	this->spTileMap[i][j]->nMetric = nMetric_me_up   + nMetric_me_down   + nMetric_me_left;
			else if (i == 0) 						this->spTileMap[i][j]->nMetric = nMetric_me_down + nMetric_me_left   + nMetric_me_right;
			else if (i == NUM_TILE_IN_COL-1) 				this->spTileMap[i][j]->nMetric = nMetric_me_up   + nMetric_me_left   + nMetric_me_right;
			else if (i <  NUM_TILE_IN_COL-1 and j  < NUM_TILE_IN_ROW-1)	this->spTileMap[i][j]->nMetric = nMetric_me_up   + nMetric_me_down   + nMetric_me_left   + nMetric_me_right;
			else								assert(0);

			// Total metric accumulated
			this->nTileBankMetricTotal = this->nTileBankMetricTotal + this->spTileMap[i][j]->nMetric;
		};
	};

	return (ERESULT_TYPE_SUCCESS);
};


// Get Address
int64_t CAddrGen::GetAddr(string cAddrMap) {

	int64_t nAddr = -1;

	if (cAddrMap == "LIAM") {

		// Auto BFAM selection 	
		#ifdef AUTO_BFAM_ENABLE
		float nQuotient = (float)( (float)(IMGHB)/(float)(SUPER_PAGE_SIZE) );
		assert(nQuotient != -1);
		
		if 	( (nQuotient > 0.75 and nQuotient < 1.25) or	\
		 	  (nQuotient > 1.75 and nQuotient < 2.25) or	\
		 	  (nQuotient > 2.75 and nQuotient < 3.25) or	\
		 	  (nQuotient > 3.75 and nQuotient < 4.25) or	\
		 	  (nQuotient > 4.75 and nQuotient < 5.25) or	\
		 	  (nQuotient > 5.75 and nQuotient < 6.25) or	\
		 	  (nQuotient > 6.75 and nQuotient < 7.25) or	\
		 	  (nQuotient > 7.75 and nQuotient < 8.25) ) {
		
			nAddr = this->GetAddr_BFAM();
			return nAddr;
		};
		#endif

		// Auto BGFAM selection 	
		#ifdef AUTO_BGFAM_ENABLE
		float nQuotient = (float)( (float)(IMGHB)/(float)(SUPER_PAGE_SIZE) );
		assert(nQuotient != -1);

		if(BANK_NUM==16) {
			if ((nQuotient > 0.4375 and nQuotient < 0.5625) or \
		 	  	(nQuotient > 0.9375 and nQuotient < 1.0625) or \
				(nQuotient > 1.4375 and nQuotient < 1.5625) or \
		 	  	(nQuotient > 1.9375 and nQuotient < 2.0625) or \
				(nQuotient > 2.4375 and nQuotient < 2.5625) or \
		 	  	(nQuotient > 2.9375 and nQuotient < 3.0625) or \
				(nQuotient > 3.4375 and nQuotient < 3.5625) or \
		 	  	(nQuotient > 3.9375 and nQuotient < 4.0625) or \
				(nQuotient > 4.4375 and nQuotient < 4.5625) or \
		 	  	(nQuotient > 4.9375 and nQuotient < 5.0625) or \
				(nQuotient > 5.4375 and nQuotient < 5.5625) or \
		 	  	(nQuotient > 5.9375 and nQuotient < 6.0625) or \
				(nQuotient > 6.4375 and nQuotient < 6.5625) or \
		 	  	(nQuotient > 6.9375 and nQuotient < 7.0625) or \
				(nQuotient > 7.4375 and nQuotient < 7.5625) or \
		 	  	(nQuotient > 7.9375 and nQuotient < 8.0625) ) {
			
				nAddr = this->GetAddr_BGFAM();
				return nAddr;
			};
		} else if (BANK_NUM==32) {
			if ((nQuotient > 0.1875 and nQuotient < 0.3125) or \
				(nQuotient > 0.4375 and nQuotient < 0.5625) or \
				(nQuotient > 0.6875 and nQuotient < 0.8125) or \
		 	  	(nQuotient > 0.9375 and nQuotient < 1.0625) or \
				(nQuotient > 1.1875 and nQuotient < 1.3125) or \
				(nQuotient > 1.4375 and nQuotient < 1.5625) or \
				(nQuotient > 1.6875 and nQuotient < 1.8125) or \
		 	  	(nQuotient > 1.9375 and nQuotient < 2.0625) or \
				(nQuotient > 2.1875	and nQuotient < 2.3125) or \
				(nQuotient > 2.4375	and nQuotient < 2.5625) or \
				(nQuotient > 2.6875	and nQuotient < 2.8125) or \
		 	  	(nQuotient > 2.9375 and nQuotient < 3.0625) or \
				(nQuotient > 3.1875 and nQuotient < 3.3125) or \
				(nQuotient > 3.4375 and nQuotient < 3.5625) or \
				(nQuotient > 3.6875 and nQuotient < 3.8125) or \
		 	  	(nQuotient > 3.9375 and nQuotient < 4.0625) or \
				(nQuotient > 4.1875 and nQuotient < 4.3125) or \
				(nQuotient > 4.4375 and nQuotient < 4.5625) or \
				(nQuotient > 4.6875 and nQuotient < 4.8125) or \
		 	  	(nQuotient > 4.9375 and nQuotient < 5.0625) or \
				(nQuotient > 5.1875 and nQuotient < 5.3125) or \
				(nQuotient > 5.4375 and nQuotient < 5.5625) or \
				(nQuotient > 5.6875 and nQuotient < 5.8125) or \
		 	  	(nQuotient > 5.9375 and nQuotient < 6.0625) or \
				(nQuotient > 6.1875 and nQuotient < 6.3125) or \
				(nQuotient > 6.4375 and nQuotient < 6.5625) or \
				(nQuotient > 6.6875 and nQuotient < 6.8125) or \
		 	  	(nQuotient > 6.9375 and nQuotient < 7.0625) or \
				(nQuotient > 7.1875 and nQuotient < 7.3125) or \
				(nQuotient > 7.4375 and nQuotient < 7.5625) or \
				(nQuotient > 7.6875 and nQuotient < 7.8125) or \
		 	  	(nQuotient > 7.9375 and nQuotient < 8.0625) ) {
			
				nAddr = this->GetAddr_BGFAM();
				return nAddr;
			};
		} else if (BANK_NUM==48) {
			if 	((nQuotient > 0.0625 and nQuotient < 0.1875) or \
				 (nQuotient > 0.1875 and nQuotient < 0.3125) or \
				 (nQuotient > 0.3125 and nQuotient < 0.4375) or \
				 (nQuotient > 0.4375 and nQuotient < 0.5625) or \
				 (nQuotient > 0.5625 and nQuotient < 0.6875) or \
				 (nQuotient > 0.6875 and nQuotient < 0.8125) or \
				 (nQuotient > 0.8125 and nQuotient < 0.9375) or \
				 (nQuotient > 0.9375 and nQuotient < 1.0625) or \
				 (nQuotient > 1.0625 and nQuotient < 1.1875) or \
				 (nQuotient > 1.1875 and nQuotient < 1.3125) or \
				 (nQuotient > 1.3125 and nQuotient < 1.4375) or \
				 (nQuotient > 1.4375 and nQuotient < 1.5625) or \
				 (nQuotient > 1.5625 and nQuotient < 1.6875) or \
				 (nQuotient > 1.6875 and nQuotient < 1.8125) or \
				 (nQuotient > 1.8125 and nQuotient < 1.9375) or \
				 (nQuotient > 1.9375 and nQuotient < 2.0625) or \
				 (nQuotient > 2.0625 and nQuotient < 2.1875) or \
				 (nQuotient > 2.1875 and nQuotient < 2.3125) or \
				 (nQuotient > 2.3125 and nQuotient < 2.4375) or \
				 (nQuotient > 2.4375 and nQuotient < 2.5625) or \
				 (nQuotient > 2.5625 and nQuotient < 2.6875) or \
				 (nQuotient > 2.6875 and nQuotient < 2.8125) or \
				 (nQuotient > 2.8125 and nQuotient < 2.9375) or \
				 (nQuotient > 2.9375 and nQuotient < 3.0625) or \
				 (nQuotient > 3.0625 and nQuotient < 3.1875) or \
				 (nQuotient > 3.1875 and nQuotient < 3.3125) or \
				 (nQuotient > 3.3125 and nQuotient < 3.4375) or \
				 (nQuotient > 3.4375 and nQuotient < 3.5625) or \
				 (nQuotient > 3.5625 and nQuotient < 3.6875) or \
				 (nQuotient > 3.6875 and nQuotient < 3.8125) or \
				 (nQuotient > 3.8125 and nQuotient < 3.9375) or \
				 (nQuotient > 3.9375 and nQuotient < 4.0625) or \
				 (nQuotient > 4.0625 and nQuotient < 4.1875) or \
				 (nQuotient > 4.1875 and nQuotient < 4.3125) or \
				 (nQuotient > 4.3125 and nQuotient < 4.4375) or \
				 (nQuotient > 4.4375 and nQuotient < 4.5625) or \
				 (nQuotient > 4.5625 and nQuotient < 4.6875) or \
				 (nQuotient > 4.6875 and nQuotient < 4.8125) or \
				 (nQuotient > 4.8125 and nQuotient < 4.9375) or \
				 (nQuotient > 4.9375 and nQuotient < 5.0625) or \
				 (nQuotient > 5.0625 and nQuotient < 5.1875) or \
				 (nQuotient > 5.1875 and nQuotient < 5.3125) or \
				 (nQuotient > 5.3125 and nQuotient < 5.4375) or \
				 (nQuotient > 5.4375 and nQuotient < 5.5625) or \
				 (nQuotient > 5.5625 and nQuotient < 5.6875) or \
				 (nQuotient > 5.6875 and nQuotient < 5.8125) or \
				 (nQuotient > 5.8125 and nQuotient < 5.9375) or \
				 (nQuotient > 5.9375 and nQuotient < 6.0625) or \
				 (nQuotient > 6.0625 and nQuotient < 6.1875) or \
				 (nQuotient > 6.1875 and nQuotient < 6.3125) or \
				 (nQuotient > 6.3125 and nQuotient < 6.4375) or \
				 (nQuotient > 6.4375 and nQuotient < 6.5625) or \
				 (nQuotient > 6.5625 and nQuotient < 6.6875) or \
				 (nQuotient > 6.6875 and nQuotient < 6.8125) or \
				 (nQuotient > 6.8125 and nQuotient < 6.9375) or \
				 (nQuotient > 6.9375 and nQuotient < 7.0625) or \
				 (nQuotient > 7.0625 and nQuotient < 7.1875) or \
				 (nQuotient > 7.1875 and nQuotient < 7.3125) or \
				 (nQuotient > 7.3125 and nQuotient < 7.4375) or \
				 (nQuotient > 7.4375 and nQuotient < 7.5625) or \
				 (nQuotient > 7.5625 and nQuotient < 7.6875) or \
				 (nQuotient > 7.6875 and nQuotient < 7.8125) or \
				 (nQuotient > 7.8125 and nQuotient < 7.9375) or \
				 (nQuotient > 7.9375 and nQuotient < 8.0625) ) {
			
				nAddr = this->GetAddr_BGFAM();
				return nAddr;
			};
		} else if (BANK_NUM==64) {
			if 	((nQuotient > 0.0625 and nQuotient < 0.1875) or \
				 (nQuotient > 0.1875 and nQuotient < 0.3125) or \
				 /*(nQuotient > 0.3125 and nQuotient < 0.4375) or \*/
				 (nQuotient > 0.4375 and nQuotient < 0.5625) or \
				 /*(nQuotient > 0.5625 and nQuotient < 0.6875) or \*/
				 (nQuotient > 0.6875 and nQuotient < 0.8125) or \
				 (nQuotient > 0.8125 and nQuotient < 0.9375) or \
				 (nQuotient > 0.9375 and nQuotient < 1.0625) or \
				 (nQuotient > 1.0625 and nQuotient < 1.1875) or \
				 (nQuotient > 1.1875 and nQuotient < 1.3125) or \
				 (nQuotient > 1.3125 and nQuotient < 1.4375) or \
				 (nQuotient > 1.4375 and nQuotient < 1.5625) or \
				 (nQuotient > 1.5625 and nQuotient < 1.6875) or \
				 (nQuotient > 1.6875 and nQuotient < 1.8125) or \
				 (nQuotient > 1.8125 and nQuotient < 1.9375) or \
				 (nQuotient > 1.9375 and nQuotient < 2.0625) or \
				 (nQuotient > 2.0625 and nQuotient < 2.1875) or \
				 (nQuotient > 2.1875 and nQuotient < 2.3125) or \
				 (nQuotient > 2.3125 and nQuotient < 2.4375) or \
				 (nQuotient > 2.4375 and nQuotient < 2.5625) or \
				 (nQuotient > 2.5625 and nQuotient < 2.6875) or \
				 (nQuotient > 2.6875 and nQuotient < 2.8125) or \
				 (nQuotient > 2.8125 and nQuotient < 2.9375) or \
				 (nQuotient > 2.9375 and nQuotient < 3.0625) or \
				 (nQuotient > 3.0625 and nQuotient < 3.1875) or \
				 (nQuotient > 3.1875 and nQuotient < 3.3125) or \
				 (nQuotient > 3.3125 and nQuotient < 3.4375) or \
				 (nQuotient > 3.4375 and nQuotient < 3.5625) or \
				 (nQuotient > 3.5625 and nQuotient < 3.6875) or \
				 (nQuotient > 3.6875 and nQuotient < 3.8125) or \
				 (nQuotient > 3.8125 and nQuotient < 3.9375) or \
				 (nQuotient > 3.9375 and nQuotient < 4.0625) or \
				 (nQuotient > 4.0625 and nQuotient < 4.1875) or \
				 (nQuotient > 4.1875 and nQuotient < 4.3125) or \
				 (nQuotient > 4.3125 and nQuotient < 4.4375) or \
				 (nQuotient > 4.4375 and nQuotient < 4.5625) or \
				 (nQuotient > 4.5625 and nQuotient < 4.6875) or \
				 (nQuotient > 4.6875 and nQuotient < 4.8125) or \
				 (nQuotient > 4.8125 and nQuotient < 4.9375) or \
				 (nQuotient > 4.9375 and nQuotient < 5.0625) or \
				 (nQuotient > 5.0625 and nQuotient < 5.1875) or \
				 (nQuotient > 5.1875 and nQuotient < 5.3125) or \
				 (nQuotient > 5.3125 and nQuotient < 5.4375) or \
				 (nQuotient > 5.4375 and nQuotient < 5.5625) or \
				 (nQuotient > 5.5625 and nQuotient < 5.6875) or \
				 (nQuotient > 5.6875 and nQuotient < 5.8125) or \
				 (nQuotient > 5.8125 and nQuotient < 5.9375) or \
				 (nQuotient > 5.9375 and nQuotient < 6.0625) or \
				 (nQuotient > 6.0625 and nQuotient < 6.1875) or \
				 (nQuotient > 6.1875 and nQuotient < 6.3125) or \
				 (nQuotient > 6.3125 and nQuotient < 6.4375) or \
				 (nQuotient > 6.4375 and nQuotient < 6.5625) or \
				 (nQuotient > 6.5625 and nQuotient < 6.6875) or \
				 (nQuotient > 6.6875 and nQuotient < 6.8125) or \
				 (nQuotient > 6.8125 and nQuotient < 6.9375) or \
				 (nQuotient > 6.9375 and nQuotient < 7.0625) or \
				 (nQuotient > 7.0625 and nQuotient < 7.1875) or \
				 (nQuotient > 7.1875 and nQuotient < 7.3125) or \
				 (nQuotient > 7.3125 and nQuotient < 7.4375) or \
				 (nQuotient > 7.4375 and nQuotient < 7.5625) or \
				 (nQuotient > 7.5625 and nQuotient < 7.6875) or \
				 (nQuotient > 7.6875 and nQuotient < 7.8125) or \
				 (nQuotient > 7.8125 and nQuotient < 7.9375) or \
				 (nQuotient > 7.9375 and nQuotient < 8.0625) ) {
			
				nAddr = this->GetAddr_BGFAM();
				return nAddr;
			};
		}
		
		#endif

		// LIAM address
		nAddr = this->GetAddr_LIAM();
	}
	else if (cAddrMap == "CIAM") {
		nAddr = this->GetAddr_CIAM();
	}
	else if (cAddrMap == "BFAM") {
		nAddr = this->GetAddr_BFAM();
	}
	else if (cAddrMap == "BGFAM") {
		nAddr = this->GetAddr_BGFAM();
	}
	else if (cAddrMap == "TILE") {
		nAddr = this->GetAddr_TILE();
	}
	else if (cAddrMap == "HBM_INTERLEAVE") {
		nAddr = this->GetAddr_HBM_Interleaving();
	}
	else {
		printf("Error: AddrGen %s, Unknown address map %s\n", this->cName.c_str(), cAddrMap.c_str());
		assert(0);
	};

	return nAddr;
};


//---------------------------------------------------
// Get Address linear map
// 	Linear treated as tile when "TILEH" is "transaction size" and "TILEV" is "1".
//---------------------------------------------------
int64_t CAddrGen::GetAddr_LIAM() {

	// Check all trans finished application
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Get actual img size
	int ImgHB = (this->ScalingFactor) * IMGHB; 
	int ImgH  = (this->ScalingFactor) * IMG_HORIZONTAL_SIZE;

	// 1. Update Apos, Bpos, AsizeT, BsizeT. UpdateState()
	
	// 2. Get tile number
	
	// Get tile row
	this->nTileRowNum = this->nBpos / 1;						// TILEV 1
	
	// Debug
	// printf("Apos        = %d, ", this->nApos);
	// printf("Bpos        = %d\n", this->nBpos);
	// printf("nTileRowNum = %d, ", this->nTileRowNum);
	
	// Get tile number  
	int nTileNum = (this->nApos + (ImgH * nTileRowNum)) / TILEH;

	// Set tile num	
	this->nTileNum = nTileNum;

	// 3. Get AposInTile
	this->nAposInTile = (this->nApos + (ImgH * nTileRowNum)) % TILEH;
	
	// 4. Get BposInTile
	this->nBposInTile = this->nBpos % 1;						// TILEV 1
	
	// 5. Get NumPixelTrans
	this->SetNumPixelTrans();
	int nNumPixelTrans = this->GetNumPixelTrans();
	// printf("PixelTrans = %d, ", nNumPixelTrans);

	// 6. Get LIAM address 
	int64_t nAddr  = (this->nApos * BYTE_PER_PIXEL) + (this->nBpos * ImgHB);
	//printf("LIAM Addr Calc: Apos=%d, Bpos=%d, ImgHB=%d => Pre-Addr=0x%llx\n", this->nApos, this->nBpos, ImgHB, nAddr);
	uint16_t nRowBank = (uint64_t)(nAddr >> (COL_WIDTH));
	uint16_t nRow     = floor(nRowBank/BANK_NUM);
	uint16_t nBank    = nRowBank%BANK_NUM;
	uint16_t nCol     = (uint64_t)(nAddr << (BANK_WIDTH+ROW_WIDTH)) >> (BANK_WIDTH+ROW_WIDTH);
	nAddr = (nRow << (BANK_WIDTH+COL_WIDTH)) + (nBank << COL_WIDTH) + nCol;
	//printf("LIAM Addr Calc: Apos=%d, Bpos=%d, ImgHB=%d => Addr=0x%llx (Row=%d, Bank=%d, Col=%d)\n", this->nApos, this->nBpos, ImgHB, nAddr, nRow, nBank, nCol);

	nAddr = nAddr + this->nStartAddr;

	#ifdef SPLIT_BANK_SHUFFLE 
	//-------------------------------------------------------------------------
	// ImgHB0 
	//	ImgHB0 = (SPS/2) x floor (ImgHB / (SPS/2))	if ImgHB >= (SPS/2)
	//	         ImgHB					otherwise	
	// Address
	// 	Map0 : BytePixel x (A + B x ImgH0)
	// 	Map1 : BytePixel x (ImgSize0 + A - ImgH0 + B x ImgH1)
	//-------------------------------------------------------------------------
	// // Get ImgH0 (for Map0)
	// int ImgHB0 = -1;
	// if (ImgHB > (SUPER_PAGE_SIZE/2)) {
	// 	ImgHB0 = (SUPER_PAGE_SIZE/2) * (ImgHB / (SUPER_PAGE_SIZE/2)); 
	// }
	// else {
	// 	ImgHB0 = ImgHB;
	// };
	// int ImgH0 = ImgHB0 / BYTE_PER_PIXEL;
	// int ImgH1 = ImgH - ImgH0;
	// int ImgSize0 = ImgH0 * IMG_VERTICAL_SIZE; 

	// Get SLIAM addr 
	if (this->nApos < this->ImgH0) {	// Map0
		nAddr = BYTE_PER_PIXEL * (this->nApos + this->nBpos * this->ImgH0);
	}
	else {					// Map1
		nAddr = BYTE_PER_PIXEL * (this->ImgSize0 + this->nApos - this->ImgH0 + this->nBpos * this->ImgH1);
	};
	nAddr = nAddr + this->nStartAddr;

	#endif

	// Set metric
	#ifdef METRIC_ANALYSIS_ENABLE
	int nRow = this->nBpos; 
	int nCol = (this->nApos * BYTE_PER_PIXEL) / TILEH;

	this->spLinearMap[nRow][nCol]->nAddr = nAddr;
	this->spLinearMap[nRow][nCol]->nBank = GetBankNum_AMap_Global(nAddr);
	this->spLinearMap[nRow][nCol]->nSuperPageNumber = nAddr / SUPER_PAGE_SIZE;
	#endif

	// Set temp signal
	this->IsTransGenThisCycle = ERESULT_TYPE_YES;

	// Check last trans of application (given by user)
	if (this->nCurTrans == this->nNumTotalTrans) {
		this->eFinalTrans = ERESULT_TYPE_YES;
	};

	//-------------------------------------------
	// Check last trans of application
	// 	Assume. Start img left top. Finish img right bottom 
	//-------------------------------------------
	// if (this->nApos + nNumPixelTrans == ImgH) {
	if (this->nApos + nNumPixelTrans >= ImgH) {
		if (this->nBpos == (IMG_VERTICAL_SIZE - 1)) {
			this->eFinalTrans = ERESULT_TYPE_YES;
		};
	};

	//-------------------------------------------
	// Check last trans of application
	// 	Assume. Start img left bottom. Finish img right top 
	//-------------------------------------------
	if (this->cOperation == "ROTATION_LEFT_BOT_VER"  or
	    this->cOperation == "ROTATION_LEFT_BOT_HOR") {

		this->eFinalTrans = ERESULT_TYPE_NO;
		if (this->nApos + nNumPixelTrans >= ImgH) {
			if (this->nBpos == 0) {
				this->eFinalTrans = ERESULT_TYPE_YES;
			};
		};
	};

	//-------------------------------------------
	// Check last trans of application
	// 	Assume. Start img right bottom. Finish img left top 
	//-------------------------------------------
	if (this->cOperation == "ROTATION_RIGHT_BOT_VER"  or
	    this->cOperation == "ROTATION_RIGHT_BOT_HOR") {

		this->eFinalTrans = ERESULT_TYPE_NO;
		if (this->nApos == 0) {
			if (this->nBpos == 0) {
				this->eFinalTrans = ERESULT_TYPE_YES;
			};
		};
	};

	//-------------------------------------------
	// Check last trans of application
	// 	Assume. Start img right top. Finish img left bottom 
	//-------------------------------------------
	if (this->cOperation == "ROTATION_RIGHT_TOP_VER") {

		this->eFinalTrans = ERESULT_TYPE_NO;
		if (this->nApos == 0) {
			if (this->nBpos == (IMG_VERTICAL_SIZE - 1)) {
				this->eFinalTrans = ERESULT_TYPE_YES;
			};
		};
	};

	// Check last trans of block
	if (nNumPixelTrans == this->nAsizeT) {				// FIXME check
		if (this->nBsizeT == 0) {
			this->eFinished_ThisBlock = ERESULT_TYPE_YES;
		};
	};

	// Debug
	// this->CheckAddr();

	return (nAddr);
};


//---------------------------------------------------
// Get Address 
//	Cache interleaved address map
//	1. Get LIAM address
//	2. Modify cache number
//---------------------------------------------------
int64_t CAddrGen::GetAddr_CIAM() {

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	int64_t nSuperLineNum = nAddr & 0x00FF;				// SUPER_LINE_SIZE 256 bytes FIXME

	int nOffset = nSuperLineNum & 0x3F;

	int nCache = nSuperLineNum >> 6;				// Line 64 bytes FIXME 

	int SUPER_LINE_SIZE = 64 * 4;					// Line 64 bytes, 4 caches FIXME

	int64_t nLine = nAddr / SUPER_LINE_SIZE;

	int nSpsGroup = (IMGHB >= SUPER_LINE_SIZE) ? (int) round(IMGHB / SUPER_LINE_SIZE) : 1;

	int remainder = (nLine / nSpsGroup) % 4; 			// 4 caches FIXME
	
	nCache = nCache ^ remainder;

	int64_t nAddr_new = (nLine * SUPER_LINE_SIZE) + (nCache << 6) + nOffset;	// FIXME

	return (nAddr_new);
};


//---------------------------------------------------
// Get Address 
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Modify bank number
//---------------------------------------------------
int64_t CAddrGen::GetAddr_BFAM() {

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	// Get superpage number 
	this->nSuperPageNum = nAddr / (SUPER_PAGE_SIZE + (std::pow(2, BANK_WIDTH)-BANK_NUM)*PAGE_SIZE);

	int64_t nRow  = GetRowNum_AMap_Global(nAddr);			// Global. Address map
	int     nBank = GetBankNum_AMap_Global(nAddr);
	int     nCol  = GetColNum_AMap_Global(nAddr);
	
	int nBank_new = nBank;

	#ifdef BANK_FLIP 
	//----------------------------------------------------------------------------
	// BF (Bank flipping) algorithm
	//	If even-number superpage, bank flip
	//	This method is not good when image horizontal size (in bytes) is more than 2 times greater than superpage size (in bytes). 
	//	Samsung patent.
	//----------------------------------------------------------------------------
	if (this->nSuperPageNum % 2 == 0) {
		// Get flipped bank number
		nBank_new = this->GetBank_BFAM_BF(nBank);
	};
	#endif


	#ifdef BANK_FLIP_PLUS
	//----------------------------------------------------------------------------
	// BF+ (Bank flipping plus) algorithm
	//	Flip every other round(Quotient) superpages
	//	Quotient positive integer 
	//----------------------------------------------------------------------------
	int k = (IMGHB >= SUPER_PAGE_SIZE) ? (int) round( (float)IMGHB / SUPER_PAGE_SIZE) : 1;

	if (((this->nSuperPageNum / k) % 2) == 1) { 
		// Method (1) Flip MSB
		nBank_new = this->GetBank_BFAM_BF(nBank); 		// Flip MSB. Experimentally, better than LSB
		
		// Method (2) Flip LSB
		// nBank_new = nBank ^ 1;				// Flip LSB
	};
	#endif


	float nQuotient = (float) (IMGHB / SUPER_PAGE_SIZE);
	assert(nQuotient >= 0);

	#ifdef BANK_FLIP_MINUS
	//----------------------------------------------------------------------------
	// BF- (Bank flipping minus) algorithm
	//	Flip every other [Quotient] superpages
	//	[Quotient] is the nearest 2^m superpage number.
	//	See document
	//----------------------------------------------------------------------------
	if (nQuotient < 1.5) {      // [0.0, 1.5]  Every other 1 superpage. (0, 2, 4,...)
		if ( (nSuperPageNum & 0x1) == 0 ) nBank_new = this->GetBank_BFAM_BF(nBank);
	}
	else if (nQuotient < 3.0) { // [1.5, 3.0]  Every other 2 superpages (0,1, 4,5, 6,7, ...)
		if ( (nSuperPageNum & 0x2) == 0 ) nBank_new = this->GetBank_BFAM_BF(nBank);
	}
	else if (nQuotient < 7.0) { // [3.0, 7.0]  Every other 4 superpages (0,1,2,3, 8,9,10,11, ...)
		if ( (nSuperPageNum & 0x4) == 0 ) nBank_new = this->GetBank_BFAM_BF(nBank);
	}
	else if (nQuotient < 15.5) { // [7.0, big] Every other 8 superpages (0,1,2,3,4,5,6,7,  16,17,18,19,20,21,22,23, ...)
		if ( (nSuperPageNum & 0x8) == 0 ) nBank_new = this->GetBank_BFAM_BF(nBank);
	}
	else {
		assert (0);
	};
	#endif

	#ifdef SPLIT_BANK_SHUFFLE
	//----------------------------------------------------------------------------
	// Bank shuffling split
	// 	Map0 : For rotation, do shuffling for split Map0. 
	// 	Map1 : No change. Use SLIAM. 
	//----------------------------------------------------------------------------
	nBank_new = nBank; 			// Same LIAM

	// Shuffle 
	if (this->nApos < this->ImgH0) {	// Map0
	
		if (this->ImgHB0 >= (SUPER_PAGE_SIZE / 2)) {	

			if (this->cOperation == "ROTATION" or this->cOperation == "ROTATION_LEFT_BOT_VER" or this->cOperation == "ROTATION_RIGHT_TOP_VER" or \
			    this->cOperation == "ROTATION_LEFT_TOP_VER" or this->cOperation == "ROTATION_RIGHT_BOT_VER") {

				int k = (this->ImgHB0 >= SUPER_PAGE_SIZE) ? (int) round( (float)(this->ImgHB0) / SUPER_PAGE_SIZE) : 1;
				int Remainder = (this->nSuperPageNum / k) % BANK_NUM;

				// Permutation
				nBank_new = nBank ^ Remainder;	// bit-level xor
			};
		};
	}
	#endif


	#ifdef BANK_SHUFFLE_PLUS
	//----------------------------------------------------------------------------
	// BS+ (Bank shuffle plus) algorithm.
	// 	Check condition.
	//	Shuffle every superpage in the granularity of k superpages.
	//	k is positive integer. 
	//----------------------------------------------------------------------------
	int   k     = (IMGHB >=  SUPER_PAGE_SIZE)    ? (int)    round( (float)     IMGHB / SUPER_PAGE_SIZE)    : 1;	// BFAM. Round to integer.   IEEE Access 2019
	float k2    = (IMGHB >= (SUPER_PAGE_SIZE/2)) ? (float) (round( (float) 2 * IMGHB / SUPER_PAGE_SIZE))/2 : 0.5;	// BSAM. Round to 0.5 

	// float k2 = (IMGHB >= SUPER_PAGE_SIZE)     ? (float) (round( (float) 2 * IMGHB / SUPER_PAGE_SIZE))/2 : 1;	// BSAM. Round to 0.5 

	float    T  = (float)((float)IMGHB / (float)SUPER_PAGE_SIZE);

	// Check condition. Shuffle.
	if (this->cOperation == "ROTATION" or this->cOperation == "ROTATION_LEFT_BOT_VER" or this->cOperation == "ROTATION_RIGHT_TOP_VER" or this->cOperation == "ROTATION_LEFT_TOP_VER" or this->cOperation == "ROTATION_RIGHT_BOT_VER") {
		if (T >= (k2 - 0.125) and T < (k2 + 0.125) ) {			// FIXME Careful.	
		
			int Remainder = (this->nSuperPageNum / k) % BANK_NUM;
		
			// Method 1. Permutation
			nBank_new = nBank ^ Remainder;				// bit-level xor
			
			// Method 2. Permutation, invert. Experimentally, same as method 1 
			// nBank_new = nBank ^ Remainder;			// bit-level xor
			// nBank_new = nBank_new ^ (BANK_NUM-1);		// Invert
			
			// Method 3. Circular increment 
			// nBank_new = (nBank + Remainder) % BANK_NUM;
		};
	};
	#endif


	#ifdef BANK_SHUFFLE_MINUS
	//----------------------------------------------------------------------------
	// BS- (Bank shuffle minus) algorithm 
	//	Shuffle every superpage in the granularity of [Quotient] superpages
	//	[Quotient] is the nearest 2^m superpage number.
	//----------------------------------------------------------------------------
	int nGroupSuperPageNum = 0;

	if (nQuotient < 2) { // Every other 1 superpage. (0, 2, 4,...)
		nGroupSuperPageNum = 1;
		nBank_new = this->GetBank_BFAM_BS_MINUS(nBank, nSuperPageNum, nGroupSuperPageNum, nAddr);
	}
	else if (nQuotient < 4) { // Every other 2 superpages (0,1, 4,5, 6,7, ...)
		nGroupSuperPageNum = 2;
		nBank_new = this->GetBank_BFAM_BS_MINUS(nBank, nSuperPageNum, nGroupSuperPageNum, nAddr);
	}
	else if (nQuotient < 8) { // Every other 4 superpages (0,1,2,3, 8,9,10,11, ...)
		nGroupSuperPageNum = 4;
		nBank_new = this->GetBank_BFAM_BS_MINUS(nBank, nSuperPageNum, nGroupSuperPageNum, nAddr);
	}
	else if (nQuotient < 16) { // Every other 8 superpages (0,1,2,3,4,5,6,7,  16,17,18,19,20,21,22,23, ...)
		nGroupSuperPageNum = 8;
		nBank_new = this->GetBank_BFAM_BS_MINUS(nBank, nSuperPageNum, nGroupSuperPageNum, nAddr);
	}
	else {
		assert (0);
	};
	#endif
	//--------------------------------------------------------------------------
	
	// Debug
	assert (nBank_new >= 0);
	assert (nBank_new < BANK_NUM);

	// Get bank-flipped address
	int64_t nAddr_new = GetAddr_AMap_Global(nRow, nBank_new, nCol); 			// Address map

	// Set bank metric
	#ifdef METRIC_ANALYSIS_ENABLE
	int nRow = this->nBpos; 
	int nCol = (this->nApos * BYTE_PER_PIXEL) / TILEH;

	this->spLinearMap[nRow][nCol]->nAddr = nAddr_new;
	this->spLinearMap[nRow][nCol]->nBank = GetBankNum_AMap_Global(nAddr_new);
	this->spLinearMap[nRow][nCol]->nSuperPageNumber = nAddr_new / SUPER_PAGE_SIZE;
	#endif

	// Check last transaction application
	if (this->nCurTrans == this->nNumTotalTrans) {
		// this->SetFinalTrans(ERESULT_TYPE_YES);
		this->eFinalTrans = ERESULT_TYPE_YES;
	};

	// Set temp signal
	this->IsTransGenThisCycle = ERESULT_TYPE_YES;

	// Debug
	// this->CheckAddr();

	return (nAddr_new);
};


//---------------------------------------------------
// Get flipped bank number
// 	BF and BF- algorithm
// 	Flip MSB
//---------------------------------------------------
int CAddrGen::GetBank_BFAM_BF(int nBank) {

	int nBank_new = nBank;

	if (BANK_NUM == 2) {
		if      (nBank == 0) nBank_new = 1;
		else if (nBank == 1) nBank_new = 0;
		else assert (0);
	}
	else if (BANK_NUM == 4) {
		if      (nBank == 0) nBank_new = 2;
		else if (nBank == 1) nBank_new = 3;
		else if (nBank == 2) nBank_new = 0;
		else if (nBank == 3) nBank_new = 1;
		else assert (0);
	}
	else if (BANK_NUM == 8) {
		if      (nBank == 0) nBank_new = 4;
		else if (nBank == 1) nBank_new = 5;
		else if (nBank == 2) nBank_new = 6;
		else if (nBank == 3) nBank_new = 7;
		else if (nBank == 4) nBank_new = 0;
		else if (nBank == 5) nBank_new = 1;
		else if (nBank == 6) nBank_new = 2;
		else if (nBank == 7) nBank_new = 3;
		else assert (0);
	}	
	else if (BANK_NUM <= 64) {
		nBank_new = (floor(nBank/16)*BANK_NUM_PER_STACK) + ((nBank+BANK_NUM_PER_STACK/2) % BANK_NUM_PER_STACK); // Extract the stack and flip within the stack
	}
	else {
		assert (0);
	};
	
	return (nBank_new);
};


//---------------------------------------------------
// Get shuffled bank number
// 	Bank shuffling algorithm
//---------------------------------------------------
int CAddrGen::GetBank_BFAM_BS_MINUS(int nBank, int nSuperPageNum, int nGroupSuperPageNum, int64_t nAddr) {

	int nBank_new = nBank;
	int Remainder = 0;

	if (nGroupSuperPageNum == 1) {		// Group 2^0 superpages
		if      (BANK_NUM == 2) Remainder = (nSuperPageNum >> 0) & 0x1;
		else if (BANK_NUM == 4) Remainder = (nSuperPageNum >> 0) & 0x3;
		else if (BANK_NUM == 8) Remainder = (nSuperPageNum >> 0) & 0x7;
	}
	else if (nGroupSuperPageNum == 2) {	// Group 2^1 superpages
		if      (BANK_NUM == 2) Remainder = (nSuperPageNum >> 1) & 0x1;
		else if (BANK_NUM == 4) Remainder = (nSuperPageNum >> 1) & 0x3;
		else if (BANK_NUM == 8) Remainder = (nSuperPageNum >> 1) & 0x7;
	}
	else if (nGroupSuperPageNum == 4) {	// Group 2^2 superpages
		if      (BANK_NUM == 2) Remainder = (nSuperPageNum >> 2) & 0x1;
		else if (BANK_NUM == 4) Remainder = (nSuperPageNum >> 2) & 0x3;
		else if (BANK_NUM == 8) Remainder = (nSuperPageNum >> 2) & 0x7;
	}
	else if (nGroupSuperPageNum == 8) {	// Group 2^3 superpages
		if      (BANK_NUM == 2) Remainder = (nSuperPageNum >> 3) & 0x1;
		else if (BANK_NUM == 4) Remainder = (nSuperPageNum >> 3) & 0x3;
		else if (BANK_NUM == 8) Remainder = (nSuperPageNum >> 3) & 0x7;
	}
	else {
		assert (0);	
	};

	// Stride
	int Stride = 0;
	if      ((4 * IMG_HORIZONTAL_SIZE) < 4096)  Stride = (nAddr & 0x800)  >> 11; // 2^11 
	else if ((4 * IMG_HORIZONTAL_SIZE) < 8192)  Stride = (nAddr & 0x1000) >> 12; // 2^12
	else if ((4 * IMG_HORIZONTAL_SIZE) < 16384) Stride = (nAddr & 0x2000) >> 13; // 2^13
	else                                        Stride = (nAddr & 0x4000) >> 14; // 2^14
	assert(Stride < 2);

	// Permutation
	nBank_new = nBank ^ Remainder;			// logical bit-level xor
	//nBank_new = nBank ^ Remainder ^ Stride;	// logical bit-level xor stride
	
	return (nBank_new);
};


//-----------------------------------------------------------
// Tile mode
//-----------------------------------------------------------
//	Static parameters
//		TILE_SIZE_BYTE	: Tile size (in bytes)
//		MAX_TRANS_SIZE	: Max transaction size (in bytes).
//		nNumTotalTrans	: Total number of transactions.
//		IMGHB		: Image horizontal size (bytes) 
//		IMGH		: Image horizontal size (pixels) 
//	Input parameters. Addresses generated for all pixels in block (Asize, Bsize) starting from (A, B)
// 		Coordinate	: (A, B)
// 		Block size	: (Asize, Bsize)
// 		Tile size	: (TileH, TileV)
// 	Variables
//		Coordinate	: (Apos, Bpos)
//		TileRowNum	: Row number in tile level 
//		TileNum		: Tile number for (Apos, Bpos) 
//		AposInTile	: A position in tile (pixel)
//		BposInTile	: B position in tile (pixel)    
//		NumPixelTrans	: Number of pixels of a transaction size (pixels)
//		Remaining size	: (AsizeT, BsizeT) (pixels)
// 	Algorithm
//		1. Initialize Apos = A, Bpos = B, AsizeT = Asize, BsizeT = Bsize. Updtate()
//
//		2. Get TileNum
//		3. Get AposInTile
//		4. Get BposInTile
//		5. Get NumPixelTrans
//		6. Get address
//
//		// UpdateState()
//		7. Update AsizeT
//		8. Update BsizeT
//		8. Update coordinate (Apos, Bpos)
//		10.If AsizeT and BsizeT zero, stop.
//		   Update AsizeT. Goto step 2 
//------------------------------------------------------------
int64_t CAddrGen::GetAddr_TILE() {
	
	// Check AddrGenTile finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Get image horizontal size
	// int ImgHB = (this->ScalingFactor) * IMGHB;				// Bytes
	int ImgH  = IMG_HORIZONTAL_SIZE * (this->ScalingFactor);		// Pixels

	// 1. Update Apos, Bpos, AsizeT, Bsize. UpdateState()

	// 2. Get tile number
	
	// Tile row, col number
	this->nTileRowNum = this->nBpos / TILEV;
	this->nTileColNum = this->nApos / TILEH;

	// Debug
	// printf("Apos        = %d, ", this->nApos);
	// printf("Bpos        = %d\n", this->nBpos);
	// printf("nTileRowNum = %d, ", this->nTileRowNum);
	// printf("nTileColNum = %d\n", this->nTileColNum);

	// Tile number 	
	int nTileNum = (this->nApos + (ImgH * nTileRowNum)) / TILEH;


	//-----------------------------------------------------------------------
	// Hierarchical tile
	// 	Re-arrange. cluster tiles.
	//	Method (1) (ht+  algorithm): Granular row size T = (GRANULAR_COEFFIICIENT) x (hTILEN) tiles. Can be unaligned with n
	//	Method (2) (ht-  algorithm): Granular row size T = 2^r tiles 
	//	Method (3) (hta- algorithm): Granular row size T = (GRANULAR_COEFFIICIENT) x (hTILEN) tiles. Should be aligned with n
	//	Method (4) (hta+ algorithm): Granular row size T = ImgH
	//-----------------------------------------------------------------------
	#ifdef hTILE_ENABLE


	//--------------------------------
	// Method (1) (ht+): Hierarchical tile plus
	//	Granular row size T = (GRANULAR_COEFFIICIENT) x (hTILEN) tiles
	//	Band-of-tiles re-arrange. 
	//	Clustered tile in level-1 has (hTILEN) x (hTILEN) tiles.
	//--------------------------------
	#ifdef HT_PLUS_ENABLE

	int T           = GRANULAR_COEFFICIENT * hTILEN;						// Number of tiles in a granular row
	int nGRowNum    = nTileNum / T;									// Granular row number

	// HT_PLUS_LEVEL-1
	assert (hTILEN >= 1);
	int nPairNum1    = (nTileNum % T) / hTILEN;							// Pair number
	int nCRowNum1    = nGRowNum % hTILEN;								// Circular GRowNum		
	int nDistInRow1  = hTILEN * (hTILEN - 1);							// N x N hTILE
	int nDistOutRow1 = T - hTILEN;									// T - N
	int nTileNum1    = nTileNum + (nPairNum1 * nDistInRow1) - (nCRowNum1 * nDistOutRow1); 
	assert (nTileNum1 >= 0);

	// HT_PLUS_LEVEL-2
	int nTileNum2 = -1;
	if (HT_PLUS_LEVEL == 2) {

		// Level 2
		assert (hTILEN >= 2);
		int hTILEN2      = hTILEN / 2;								// Cluster (hTILEN/2  x  hTILEN/2) = N/2 x N/2
		int nPairNum2    = (nTileNum % hTILEN) / hTILEN2;					// Pair number
		int nCRowNum2    = nGRowNum % hTILEN2;							// Circular GRowNum		
		int nDistInRow2  = hTILEN2 * (hTILEN2 - 1);
		int nDistOutRow2 = hTILEN - hTILEN2;							// N - N/2 
		nTileNum2    = nTileNum1 + (nPairNum2 * nDistInRow2) - (nCRowNum2 * nDistOutRow2); 
		assert (nTileNum2 >= 0);
	};

	// HT_PLUS_LEVEL-3
	int nTileNum3 = -1;
	if (HT_PLUS_LEVEL == 3) {

		// Level 2 
		assert (hTILEN >= 2);
		int hTILEN2      = hTILEN / 2;								// Cluster (hTILEN/2  x  hTILEN/2) = N/2 x N/2
		int nPairNum2    = (nTileNum % hTILEN) / hTILEN2;					// Pair number
		int nCRowNum2    = nGRowNum % hTILEN2;							// Circular GRowNum		
		int nDistInRow2  = hTILEN2 * (hTILEN2 - 1);
		int nDistOutRow2 = hTILEN - hTILEN2;							// N - N/2 
		nTileNum2    = nTileNum1 + (nPairNum2 * nDistInRow2) - (nCRowNum2 * nDistOutRow2); 
		assert (nTileNum2 >= 0);

		// Level 3
		assert (hTILEN2 >= 2);
		int hTILEN3      = hTILEN2 / 2;								// Cluster (hTILEN2/2  x  hTILEN2/2) = N/4 x N/4
		int nPairNum3    = (nTileNum % hTILEN2) / hTILEN3;					// Pair number
		int nCRowNum3    = nGRowNum % hTILEN3;							// Circular GRowNum		
		int nDistInRow3  = hTILEN3 * (hTILEN3 - 1);
		int nDistOutRow3 = hTILEN2 - hTILEN3;							// N/2 - N/4 
		nTileNum3        = nTileNum2 + (nPairNum3 * nDistInRow3) - (nCRowNum3 * nDistOutRow3); 
		assert (nTileNum3 >= 0);
	};

	// Check condition to apply this technique un-align vertical
	// Check vertical un-align hTILEN 
	if (nGRowNum < TOTAL_NUM_hTILE) {								// No change TileNum if un-aligned 
		if      (HT_PLUS_LEVEL == 1) nTileNum = nTileNum1;  
		else if (HT_PLUS_LEVEL == 2) nTileNum = nTileNum2;  
		else if (HT_PLUS_LEVEL == 3) nTileNum = nTileNum3;  
		else  assert (0);
	};

	#endif // HT_PLUS_ENABLE
	//--------------------------------


	//--------------------------------
	// Method (2) (ht-): Hierarchical tile minus
	//	Granular row size T = 2^r tiles
	//	Simple version ht+
	//	Simple hardware. Performance issue. We don't consider this.
	//--------------------------------
	#ifdef HT_MINUS_ENABLE
	int r = GRANULAR_ROW_EXPONENT;								// Granular row 2^r tiles
	
	nGRowNum = nTileNum >> r;								// Granular row number
	
	int nMaskPairNum = GetMask_PairNum(r);
	nPairNum = (nTileNum & nMaskPairNum) >> logN;						// Masking [r-1 : logN]
	
	int nMaskCRowNum = GetMask_CRowNum(r);
	nCRowNum = (nTileNum & nMaskCRowNum) >> r;						// Masking [r+logN-1 : r]

	nDistOutRow = GRANULAR_ROW_SIZE - hTILEN;

	int nDistInRow  = hTILEN * (hTILEN - 1);      						// N x N hTILE

	// New tile number (ht+, ht-)
	if (nGRowNum < TOTAL_NUM_hTILE) {							// Condition to apply this technique
		nTileNum = nTileNum + (nPairNum * nDistInRow) - (nCRowNum * nDistOutRow); 
	};
	#endif // HT_MINUS_ENABLE
	//--------------------------------


	//--------------------------------
	// Method (3) (hta-): Hierarchical tile aligned 
	//	Granular row size T = (GRANULAR_COEFFIICIENT) x (hTILEN) tiles
	//	Simple hardware. Only work when ImgH aligned with hTILEH. We don't consider this.
	//	NUM_TILE_IN_ROW = (ImgH / TILEH)
	//	NUM_TILE_IN_COL = Ceiling (IMG_VERTICAL_SIZE / TILEV)
	//	nTileRowNum = Bpos / TILEV;
	//--------------------------------
	#ifdef HTA_MINUS_ENABLE
	int nTileColNum = this->nApos / TILEH;
	int nMRowNum   = nTileRowNum % hTILEN;
	int nMColNum   = nTileColNum % hTILEN;

	// Debug
	assert(ImgH % (hTILEN * TILEH) == 0);

	// New tile number (hta-)
	if (nTileRowNum < TOTAL_NUM_hTILE) {							// Condition to apply this technique
		int nTileNum3 =	( (nTileRowNum - nMRowNum) * NUM_TILE_IN_ROW ) +	\
				(  nMRowNum * hTILEN ) +				\
				( (nTileColNum - nMColNum) * hTILEN ) + nMColNum;
		// Debug
		assert(nTileNum == nTileNum3);
	};
	
	nTileNum = nTileNum3;
	#endif // HTA_MINUS_ENABLE
	//--------------------------------


	//--------------------------------
	// Method (4) (hta+): Hierarchical tile aligned plus
	//	Granular row size T = NUM_TILE_IN_ROW 
	//	ImgH can be un-aligned with hTILEH.
	//	NUM_TILE_IN_ROW = Ceiling (ImgH / TILEH)
	//	NUM_TILE_IN_COL = Ceiling (IMG_VERTICAL_SIZE   / TILEV)
	//--------------------------------
	#ifdef HTA_PLUS_ENABLE
	int nRowNum  = this->nBpos / TILEV;								// Row number
	int nColNum  = this->nApos / TILEH;

	int nMRowNum = nRowNum % hTILEN;								// Modulo row number
	int nMColNum = nColNum % hTILEN;

	// int nHRowNum = nRowNum - nMRowNum;								// Hierarchical row number
	// int nHColNum = nColNum - nMColNum;

	// int NumHPadPixel = TILEH - (ImgH % TILEH);							// Number of hor. pad pixels
	int NumHPadPixel = NUM_TILE_IN_ROW * TILEH - ImgH;						// Number of hor. pad pixels

	int NumVPadPixel = -1;
	if (nRowNum == LAST_ROW_NUM) {
		// NumVPadPixel = TILEV - (IMG_VERTICAL_SIZE   % TILEV);
		NumVPadPixel = NUM_TILE_IN_COL * TILEV - IMG_VERTICAL_SIZE;				// Number of ver. pad pixels
	}
	else {
		NumVPadPixel = 0;
	};

	int htilen   = (nRowNum > LAST_NORMAL_ROWNUM) ? 1: hTILEN;

	int NumPadCol = -1; 
	if (nColNum > LAST_NORMAL_COLNUM and nRowNum <= LAST_NORMAL_ROWNUM) {
		NumPadCol = NUM_hTILE_ROW * htilen - NUM_TILE_IN_ROW;
	}
	else {
		NumPadCol = 0;
	};

	// Debug
	// assert(ImgH % (hTILEN * TILEH) == 0);

	// New tile number (hta+)
	// 	Tile number for (nRowNum, nColNum
	// 	Tile number distance from (nRowNum, nColNum 0)
	// 	Hor. corner case
	int nTileNum4 =	  ( (nRowNum - nMRowNum) * NUM_TILE_IN_ROW ) + ( nMRowNum * htilen ) 	\
			+ ( (nTileColNum - nMColNum) * htilen ) + nMColNum 			\
			- ( nMRowNum * NumPadCol );
	// Debug
	// assert(nTileNum == nTileNum4);
	
	nTileNum = nTileNum4;
	#endif // HTA_PLUS_ENABLE
	//--------------------------------

	#endif	// hTILE_ENABLE
	//-------------------------------------------------------


	// Set bank metric analysis
	#ifdef METRIC_ANALYSIS_ENABLE
	this->spTileMap[nTileRowNum][nTileColNum]->nTileNum = nTileNum;
	#endif


	// 3. Get AposInTile
	// this->nAposInTile = (this->nApos + (ImgH * nTileRowNum) - (nTileNum * TILEH)) & TILEH_MASK;	// % TILEH_MASK 0xF for TILEH 16  
	this->nAposInTile =    (this->nApos + (ImgH * nTileRowNum)) % TILEH;

	#ifdef hTILE_ENABLE
	#ifdef HTA_PLUS_ENABLE
	this->nAposInTile = this->nApos & TILEH_MASK; 
	#endif	// HTA_PLUS_ENABLE
	#endif	// hTILE_ENABLE


	// 4. Get BposInTile
	// this->nBposInTile = this->nBpos & TILEV_MASK;						// TILEV_MASK 0xF for TILEV 16
	this->nBposInTile    = this->nBpos % TILEV;


	// 5. Get NumPixelTrans
	this->SetNumPixelTrans();
	int nNumPixelTrans = this->GetNumPixelTrans();
	// printf("PixelTrans = %d, ", nNumPixelTrans);


	// 6. Get Address
	// Generate tiled address 
	//	Tile number distance from TileNum 0
	//	Line distance from first line in tile
	//	Pixel distance from first pixel in lin
	int64_t nAddr = (nTileNum * TILE_SIZE_BYTE)  		\
		  + (this->nBposInTile * TILEHB) 		\
		  + (this->nAposInTile * BYTE_PER_PIXEL);	\

	#ifdef hTILE_ENABLE
	#ifdef HTA_PLUS_ENABLE
	//	Tile number distance from TileNum 0
	//	Line distance from first line in tile
	//	Pixel distance from first pixel in lin
	//	Hor. corner case
	//	Ver. corner case
	nAddr =   (nTileNum * TILE_SIZE_BYTE)  			\
		+ (this->nBposInTile * TILEHB)			\
		+ (this->nAposInTile * BYTE_PER_PIXEL)		\
		- (NumHPadPixel * TILEV * nRowNum )		\
		- (NumVPadPixel * TILEH * nColNum );
	#endif	// HTA_PLUS_ENABLE
	#endif	// hTILE_ENABLE

	nAddr = nAddr + this->nStartAddr;

	// Check last trans of application
	// if (this->nApos + nNumPixelTrans == ImgH) {
	if (this->nApos + nNumPixelTrans >= ImgH) {
		if (this->nBpos == (IMG_VERTICAL_SIZE - 1)) {
			this->eFinalTrans = ERESULT_TYPE_YES;
		};
	};

	// Check last trans of block
	if (nNumPixelTrans == this->nAsizeT) {				// FIXME check. Apos 14,  AposInTile 14, NumPixelTrans 3, AsizeT 3
		if (this->nBsizeT == 0) {
			this->eFinished_ThisBlock = ERESULT_TYPE_YES;
		};
	};

	// Temporary signal
	this->IsTransGenThisCycle = ERESULT_TYPE_YES;

	// Set tile num
	this->nTileNum = nTileNum;
	assert (nTileNum >= 0);

	// Debug
	// this->CheckAddr();

	return (nAddr);
};

//---------------------------------------------------
// Get Address (HBM's experimental address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Bank-Group-Interleaving
//---------------------------------------------------
int64_t CAddrGen::GetAddr_BGFAM() {
	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();
	
		// Get superpage number 
	this->nSuperPageNum = floor(nAddr / (SUPER_PAGE_SIZE + (std::pow(2, BANK_WIDTH)-BANK_NUM)*PAGE_SIZE));

	int64_t nRow  = GetRowNum_AMap_Global(nAddr);			// Global. Address map
	int     nBank = GetBankNum_AMap_Global(nAddr);
	int     nCol  = GetColNum_AMap_Global(nAddr);
	
	int nBank_new = nBank;

	//----------------------------------------------------------------------------
	// BF+ (Bank flipping plus) algorithm
	//	Flip every other round(Quotient) superpages
	//	Quotient positive integer 
	//----------------------------------------------------------------------------
	int k = ceil((float)IMGHB / SUPER_PAGE_SIZE);
	nBank_new = (nBank + BANK_NUM_PER_GROUP*((int)floor(this->nSuperPageNum / k)%(BANK_NUM/BANK_NUM_PER_GROUP))) % BANK_NUM; 		// Flip MSB. Experimentally, better than LSB
	//printf("SuperPageNum = %d, k = %d, nBank = %d, nBank_new = %d\n", this->nSuperPageNum, k, nBank, nBank_new);

	//nBank = (nCol/64) % BANK_NUM; // FOR Test Bank/BankGroup/SID interleaving only
	//int nBank_new = (nBank + 4*(this->nBpos % (BANK_NUM/BANK_NUM_PER_GROUP))) % BANK_NUM;

	// Debug
	assert (nBank_new >= 0);
	assert (nBank_new < BANK_NUM);

	// Get bank-flipped address
	int64_t nAddr_new = GetAddr_AMap_Global(nRow, nBank_new, nCol); 			// Address map

	// Check last transaction application
	if (this->nCurTrans == this->nNumTotalTrans) {
		// this->SetFinalTrans(ERESULT_TYPE_YES);
		this->eFinalTrans = ERESULT_TYPE_YES;
	};

	// Set temp signal
	this->IsTransGenThisCycle = ERESULT_TYPE_YES;

	// Debug
	// this->CheckAddr();


	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (HBM's experimental address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Bank-Interleaving
//---------------------------------------------------
int64_t CAddrGen::GetAddr_HBM_Interleaving() {
		// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	//int64_t nRow  = GetRowNum_AMap_Global(nAddr);			// Global. Address map
	int     nBank = GetBankNum_AMap_Global(nAddr);
	int     nCol  = GetColNum_AMap_Global(nAddr);
	
	//printf("Row = %d, Bank = %d, Col = %d\n", nRow, nBank, nCol);

	//nBank = (nCol/64) % BANK_NUM; // FOR Test Bank/BankGroup/SID interleaving only
	int nBank_new = (nBank*4)%BANK_NUM;
	int nRow_new = nBank_new / 4;
	int img_row = nAddr / IMGHB;

	if(img_row > 0) {
		nBank_new = ((nBank_new + (img_row*4)) % BANK_NUM) + (((nBank_new + (img_row*4)) / ((uint64_t)1 << ROW_WIDTH)));
		nRow_new = nRow_new + img_row;
	}

/*
	#if defined(BANKGROUP_INTERLEAVING)
		nBank_new = (nBank*4) % BANK_NUM + static_cast<int>(std::floor((nBank*4) / BANK_NUM));
	#elif defined(SID_INTERLEAVING)
		nBank_new = (nBank*8) % BANK_NUM + (nBank*8) / BANK_NUM;
	#elif defined(BANK_INTERLEAVING)
		nBank_new = nBank; // Bank Interleaving
	#else
		assert(0);
	#endif
*/

	// Debug
	assert (nBank_new >= 0);
	assert (nBank_new < BANK_NUM);

	// Get bank-flipped address
	int64_t nAddr_new = GetAddr_AMap_Global(nRow_new, nBank_new, nCol); 			// Address map

	// Check last transaction application
	if (this->nCurTrans == this->nNumTotalTrans) {
		// this->SetFinalTrans(ERESULT_TYPE_YES);
		this->eFinalTrans = ERESULT_TYPE_YES;
	};

	// Set temp signal
	this->IsTransGenThisCycle = ERESULT_TYPE_YES;

	// Debug
	// this->CheckAddr();


	return (nAddr_new);
}

//------------------------------------------
// Update state 
// 	nApos, nBpos, nAsizeT, nBsizeT
//------------------------------------------
EResultType CAddrGen::UpdateState() {

	// Check trans
	if (this->IsTransGenThisCycle == ERESULT_TYPE_NO) {
		return (ERESULT_TYPE_SUCCESS);
	};

	//------------------------------
	// Analyze metric when all finished
	//------------------------------
	#ifdef METRIC_ANALYSIS_ENABLE
	if (this->cAddrMap == "LIAM" or this->cAddrMap == "BFAM") {

		if (this->eFinalTrans == ERESULT_TYPE_YES) {
			this->CalculateLinearBankMetric();
		};
	}
	else if (this->cAddrMap == "TILE") {

		if (this->eFinalTrans == ERESULT_TYPE_YES) {
			this->CalculateTileBankMetric();
		};
	}
	else {
		assert (0);	
	};
	#endif


	//------------------------------
	// Update
	// 	1. Temp coordinate (nApos, nBpos)
	// 	2. Temp block size (nAsizeT, nBsizeT)
	//------------------------------
	
	// Get image horizontal size (pixels) 
	int ImgH  = (this->ScalingFactor) * IMG_HORIZONTAL_SIZE;

	if (this->cOperation == "RASTER_SCAN") {
		printf("Accessing (H = %d, V = %d)\n", this->nApos, this->nBpos);
		// Set block size fixed
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;

		#ifdef PADDING_ENABLE
		// Update coordinate temp
		if (this->nApos + 2*(this->nNumPixelTrans) == ImgH) {
			this->nApos = 0;
			this->nBpos ++;
		}
		else {
			this->nApos = this->nApos + TILEH;
		};
		#else
		// Update coordinate temp
		if (this->nApos + this->nNumPixelTrans >= ImgH) {
			this->nApos = 0;
			this->nBpos ++;
		}
		else {
			this->nApos = this->nApos + TILEH * TILE_SIZE;
		};
		#endif

	} else 	if (this->cOperation == "TILE") {
	
		// Set block size fixed
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;
		printf("Accessing (H = %d, V = %d)\n", this->nApos, this->nBpos);

		// Update coordinate temp
		if (((this->nApos - this->TileHBase) + this->nNumPixelTrans >= ImgH)            && 
		    ((this->nApos - this->TileHBase) + this->nNumPixelTrans >= (TILE_SIZE - 1)) &&
			(this->nBpos - this->TileHBase  == (TILE_SIZE - 1))
		) {
			this->TileHBase = 0;
			this->TileVBase ++;
			this->nApos = this->TileHBase;
			this->nBpos = this->TileVBase;
		}
		if (((this->nApos - this->TileHBase) + this->nNumPixelTrans < ImgH)              &&
		    ((this->nApos - this->TileHBase) + this->nNumPixelTrans >= (TILE_SIZE - 1))  &&
			(this->nBpos - this->TileHBase  == (TILE_SIZE - 1))                           
		) {
			this->TileHBase++;
			this->nApos = this->TileHBase;
			this->nBpos = this->TileVBase;
		}
		else if ((this->nApos - this->TileHBase) + this->nNumPixelTrans >= (TILE_SIZE - 1)) {
			this->nApos = this->TileHBase;
			this->nBpos ++;
		}
		else {
			this->nApos = this->nApos + TILEH;
			this->nBpos = this->nBpos;
		};
	}
	else if (this->cOperation == "ROTATION" or 
		 this->cOperation == "ROTATION_LEFT_TOP_VER") {		// FIXME Different degree, different pattern (descending)

		// Set block size fixed
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;

		// Update coordinate temp
		if (this->nBpos == (IMG_VERTICAL_SIZE - 1)) {
			this->nApos = this->nApos + TILEH;
			this->nBpos = 0;
		}
		else {
			this->nBpos ++;
		};
	}
	else if (this->cOperation == "ROTATION_LEFT_BOT_VER") {

		// Set block size fixed
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;

		// Update coordinate temp
		if (this->nBpos == 0) {
			this->nApos = this->nApos + TILEH;
			this->nBpos = IMG_VERTICAL_SIZE - 1;
		}
		else {
			this->nBpos --;
		};
	}
	else if (this->cOperation == "ROTATION_LEFT_BOT_HOR") {

		// Set block size fixed
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;

		// Update coordinate temp
		if (this->nApos == (IMG_HORIZONTAL_SIZE - TILEH)) {
			this->nApos = 0;
			if (this->nBpos > 0) {
				this->nBpos --;
			};
		}
		else {
			this->nApos = this->nApos + TILEH;
		};
	}
	else if (this->cOperation == "ROTATION_RIGHT_BOT_VER") {

		// Set block size fixed
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;

		// Update coordinate temp
		if (this->nBpos == 0) {
			if (this->nApos >= TILEH) {
				this->nApos = this->nApos - TILEH;
			};
			this->nBpos = IMG_VERTICAL_SIZE - 1;
		}
		else {
			this->nBpos --;
		};
	}
	else if (this->cOperation == "ROTATION_RIGHT_BOT_HOR") {

		// Set block size fixed
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;

		// Update coordinate temp
		if (this->nApos == 0) {
			this->nApos = (IMG_HORIZONTAL_SIZE - TILEH);
			if (this->nBpos > 0) {
				this->nBpos --;
			};
		}
		else {
			this->nApos = this->nApos - TILEH;
		};
	}
	else if (this->cOperation == "ROTATION_RIGHT_TOP_VER") {

		// Set block size fixed
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;

		// Update coordinate temp
		if (this->nBpos == (IMG_VERTICAL_SIZE - 1)) {
			if (this->nApos >= TILEH) {
				this->nApos = this->nApos - TILEH;
			};
			this->nBpos = 0;
		}
		else {
			this->nBpos ++;
		};
	}
	//---------------------------------
	// CNN
	// 	1. Update (Asize, BsizeT), (Apos, Bpos)
	// 	2. Check block finished
	// 	2. Update (A, B), (Asize, Bsize)
	// 	   (Asize, Bsize) fixed (BLOCK_CNN, BLOCK_CNN)
	//---------------------------------
	else if (this->cOperation == "CNN") {

		// Update block size temp
		this->nAsizeT = this->nAsizeT - this->nNumPixelTrans;	

		if (this->nAsizeT == 0 and this->nBsizeT > 0) { 
			this->nBsizeT --;
		};

		// Update coordinate temp
		if (this->nAsizeT > 0) {
			this->nApos = this->nApos + this->nNumPixelTrans;
		}
		else if (this->nBsizeT > 0) {
			this->nApos = this->nA;
			this->nBpos ++;
		};

		// Check block finished trans gen 
		if (this->nAsizeT == 0 and this->nBsizeT == 0) {
		     	this->eFinished_ThisBlock = ERESULT_TYPE_YES;
		};

		// Set block size fixed (Asize, Bsize)
		int nBlockLen = -1;
		if (this->GetTransDirType() == ETRANS_DIR_TYPE_READ) {
			nBlockLen = BLOCK_CNN;
		}
		else if (this->GetTransDirType() == ETRANS_DIR_TYPE_WRITE) {
			nBlockLen = 1;
		}
		else {
			assert (0);
		};

		this->nAsize = nBlockLen;
		this->nBsize = nBlockLen;


		// Check current block addr gen finished
		// 	Initialize next block
		if (this->eFinished_ThisBlock == ERESULT_TYPE_YES) {	// Finished current block

			// // Update start coordinate 
			// // Method (1) Naive hardware. Need redundant memory access
			// if (this->nA == (ImgH - nBlockLen)) {
			// 	this->nA = 0;	
			// 	this->nB ++;	
			// }
			// else {
			// 	this->nA ++;	
			// };

			// // Update start coordinate 
			// // Method (2) Hardware has block register. Horizontal processing does not need redundant memory access. 
			// if (this->nA >= (ImgH - nBlockLen)) {
			// 	this->nA = 0;	
			// 	this->nB ++;				// Check img ver size FIXME	
			// }
			// else {
			// 	this->nA = this->nA + nBlockLen;	
			// };

			// Update start coordinate 
			// Method (3) Hardware has block register. Horizontal and Vertical processing do not need redundant memory access. 
			if (this->nA >= (ImgH - nBlockLen)) {
				this->nA = 0;	
				this->nB = this->nB + nBlockLen;	// Check img ver size FIXME	
			}
			else {
				this->nA = this->nA + nBlockLen;	
			};

			// Initialize block size temp
			this->nAsizeT = this->nAsize;
			this->nBsizeT = this->nBsize;

			// Initialize coordinate temp
			this->nApos = this->nA;
			this->nBpos = this->nB;

			// Restore
			this->eFinished_ThisBlock = ERESULT_TYPE_NO;	// FIXME value change same cycle
		};

		// Set AsizeT
		if (this->nAsizeT == 0) {
			this->nAsizeT = this->nAsize;
		};
	}
	//---------------------------------
	// JPEG
	// 	1. Update (Asize, BsizeT), (Apos, Bpos)
	// 	2. Check block finished
	// 	2. Update (A, B), (Asize, Bsize)
	// 	   (Asize, Bsize) fixed (BLOCK_JPEG, BLOCK_JPEG)
	// 	3. READ  : Same as CNN method (3)
	// 	4. WRITE : Address map "linear"
	//---------------------------------
	else if (this->cOperation == "JPEG") {

		// Update block size temp
		this->nAsizeT = this->nAsizeT - this->nNumPixelTrans;	

		if (this->nAsizeT == 0 and this->nBsizeT > 0) { 
			this->nBsizeT --;
		};

		// Update coordinate temp
		if (this->nAsizeT > 0) {
			this->nApos = this->nApos + this->nNumPixelTrans;
		}
		else if (this->nBsizeT > 0) {
			this->nApos = this->nA;
			this->nBpos ++;
		};

		// Check block finished trans gen 
		if (this->nAsizeT == 0 and this->nBsizeT == 0) {
		     	this->eFinished_ThisBlock = ERESULT_TYPE_YES;
		};

		// Set block size fixed (Asize, Bsize)
		int nBlockLenH = 1;
		int nBlockLenV = 1;
		if (this->GetTransDirType() == ETRANS_DIR_TYPE_READ) {
			nBlockLenH = BLOCK_JPEG;
			nBlockLenV = BLOCK_JPEG;
		}
		else if (this->GetTransDirType() == ETRANS_DIR_TYPE_WRITE) {
			nBlockLenH = 8;
			nBlockLenV = 1;
		}
		else {
			assert (0);
		};

		this->nAsize = nBlockLenH;
		this->nBsize = nBlockLenV;


		// Check current block addr gen finished
		// 	Initialize next block
		if (this->eFinished_ThisBlock == ERESULT_TYPE_YES) {	// Finished current block

			// Update start coordinate 
			if (this->nA >= (ImgH - nBlockLenH)) {
				this->nA = 0;	
				this->nB = this->nB + nBlockLenV;	// Check img ver size FIXME	
			}
			else {
				this->nA = this->nA + nBlockLenH;	
			};

			// Initialize block size temp
			this->nAsizeT = this->nAsize;
			this->nBsizeT = this->nBsize;

			// Initialize coordinate temp
			this->nApos = this->nA;
			this->nBpos = this->nB;

			// Restore
			this->eFinished_ThisBlock = ERESULT_TYPE_NO;	// FIXME value change same cycle
		};

		// Set AsizeT
		if (this->nAsizeT == 0) {
			this->nAsizeT = this->nAsize;
		};
	}
	//---------------------------------
	else {
		assert (0);
	};

	#ifdef DEBUG
	assert (this->nA      < ImgH);
	assert (this->nA      >= 0);
	// if (this->nB >= IMG_VERTICAL_SIZE) {
	// 	printf("Apos   = %d\n", this->nApos);
	// 	printf("Bpos   = %d\n", this->nBpos);
	// 	printf("AsizeT = %d\n", this->nAsizeT);
	// 	printf("BsizeT = %d\n", this->nBsizeT);
	// 	printf("A      = %d\n", this->nA);
	// 	printf("B      = %d\n", this->nB);
	// 	printf("Asize  = %d\n", this->nAsize);
	// 	printf("Bsize  = %d\n", this->nBsize);
	// };
	// assert (this->nB      < IMG_VERTICAL_SIZE);
	assert (this->nB      >= 0);
	// assert (this->nApos   < ImgH);
	assert (this->nApos   >= 0);
	// if (this->nBpos > IMG_VERTICAL_SIZE) {
	// 	printf("Apos   = %d\n", this->nApos);
	// 	printf("Bpos   = %d\n", this->nBpos);
	// 	printf("AsizeT = %d\n", this->nAsizeT);
	// 	printf("BsizeT = %d\n", this->nBsizeT);
	// 	printf("A      = %d\n", this->nA);
	// 	printf("B      = %d\n", this->nB);
	// 	printf("Asize  = %d\n", this->nAsize);
	// 	printf("Bsize  = %d\n", this->nBsize);
	// };
	// assert (this->nBpos   <=IMG_VERTICAL_SIZE);
	assert (this->nBpos   >= 0);
	assert (this->nAsizeT <= this->nAsize);
	assert (this->nAsizeT >= 0);
	assert (this->nBsizeT <= this->nBsize);
	assert (this->nBsizeT >= 0);
	#endif


	// Current trans number
	this->nCurTrans ++;

	// Set temp signal
	this->IsTransGenThisCycle = ERESULT_TYPE_NO;

	return (ERESULT_TYPE_SUCCESS);
};


// Set number of pixels of a transaction
EResultType CAddrGen::SetNumPixelTrans() {

	int nNumPixelTrans = -1; 

	if (this->cAddrMap == "LIAM" or this->cAddrMap == "BFAM" or this->cAddrMap == "BGFAM" or this->cAddrMap == "CIAM" or this->cAddrMap == "HBM_INTERLEAVE" ) {

		nNumPixelTrans = this->nAsizeT;						// FIXME check
	}
	else if (this->cAddrMap == "TILE") {

		if (this->nAsizeT >= TILEH and this->nAposInTile == 0) {
		
			// #ifdef RGB
			// nNumPixelTrans = MAX_TRANS_SIZE / BYTE_PER_PIXEL;		// 16 pixels // FIXME
			// #endif
			// #ifdef YUV
			// nNumPixelTrans = (MAX_TRANS_SIZE / BYTE_PER_PIXEL) / 4;	// 4 pixels  // FIXME
			// #endif

			nNumPixelTrans = MAX_TRANS_SIZE / BYTE_PER_PIXEL;		// 16 pixels // FIXME

		}
		else if ( (this->nAposInTile + this->nAsizeT) < TILEH) {
		
			nNumPixelTrans = this->nAsizeT;	
		}
		else { 
			nNumPixelTrans = TILEH - this->nAposInTile;
		};	
		
		#ifdef hTILE_ENABLE
		#ifdef HTA_PLUS_ENABLE

			// #ifdef RGB
			// nNumPixelTrans = MAX_TRANS_SIZE / BYTE_PER_PIXEL;		// 16 pixels // FIXME
			// #endif
			// #ifdef YUV
			// nNumPixelTrans = (MAX_TRANS_SIZE / BYTE_PER_PIXEL) / 4;	// 4 pixels  // FIXME
			// #endif

			nNumPixelTrans = MAX_TRANS_SIZE / BYTE_PER_PIXEL;		// 16 pixels // FIXME
		#endif
		#endif
	}
	else {
		assert (0);
	};

	// Set
	this->nNumPixelTrans = nNumPixelTrans; 

	return (ERESULT_TYPE_SUCCESS);
};


// Get number of pixels of a transaction
int CAddrGen::GetNumPixelTrans() {

	return (this->nNumPixelTrans);
};


//------------------------------------
// Get mask bits (for CRowNum) 
// 	ht- algorithm
//------------------------------------
int CAddrGen::GetMask_CRowNum(int r) {			// r : GRANULAR_ROW_EXPONENT

	int mask = 0;

	if (hTILEN == 2) { 				// 2 x 2 hTILE (N = 2^1)
		if      (r == 8) mask = 0x100;  	// Granular row 2^8 tiles. [8 : 8] 
		else if (r == 7) mask = 0x080;  	// Granular row 2^7 tiles. [7 : 7]
		else if (r == 6) mask = 0x040;  	// Granular row 2^6 tiles. [6 : 6]
		else if (r == 5) mask = 0x020;  	// Granular row 2^5 tiles. [5 : 5]
		else if (r == 4) mask = 0x010;  	// Granular row 2^4 tiles. [4 : 4]
		else if (r == 3) mask = 0x008;  	// Granular row 2^3 tiles. [3 : 3]
		else if (r == 2) mask = 0x004;  	// Granular row 2^2 tiles. [2 : 2]
	}
	else if (hTILEN == 4) { 			// 4 x 4 hTILE (N = 2^2)
		if      (r == 8) mask = 0x300;  	// Granular row 2^8 tiles. [9 : 8]
		else if (r == 7) mask = 0x180;  	// Granular row 2^7 tiles. [8 : 7]
		else if (r == 6) mask = 0x0C0;  	// Granular row 2^6 tiles. [7 : 6]
		else if (r == 5) mask = 0x060;  	// Granular row 2^5 tiles. [6 : 5]
		else if (r == 4) mask = 0x030;  	// Granular row 2^4 tiles. [5 : 4]
		else if (r == 3) mask = 0x018;  	// Granular row 2^3 tiles. [4 : 3]
		else if (r == 2) mask = 0x00C;  	// Granular row 2^2 tiles. [3 : 2]
	}
	else {
		assert(0);
	};

	return mask;
};


//--------------------------------------
// Get mask bits (for PairNum) 
// 	ht- algorithm
//--------------------------------------
int CAddrGen::GetMask_PairNum(int r) {			// r : GRANULAR_ROW_EXPONENT
	
	int mask = 0;

	if (hTILEN == 2) { 				// 2 x 2 hTILE (N = 2^1)
		if      (r == 8) mask = 0x0FE;  	// Granular row 2^8 tiles. Mask 7 bits
		else if (r == 7) mask = 0x07E;  	// Granular row 2^7 tiles. Mask 6 bits
		else if (r == 6) mask = 0x03E;  	// Granular row 2^6 tiles. Mask 5 bits
		else if (r == 5) mask = 0x01E;  	// Granular row 2^5 tiles. Mask 4 bits
		else if (r == 4) mask = 0x00E;  	// Granular row 2^4 tiles. Mask 3 bits
		else if (r == 3) mask = 0x006;  	// Granular row 2^3 tiles. Mask 2 bits
		else if (r == 2) mask = 0x002;  	// Granular row 2^2 tiles. Mask 1 bits
		else assert(0);
	}
	else if (hTILEN == 4) { 			// 4 x 4 hTILE (N = 2^2)
		if      (r == 8) mask = 0x0FC;  	// Granular row 2^8 tiles. Mask 6 bits
		else if (r == 7) mask = 0x07C;  	// Granular row 2^7 tiles. Mask 5 bits
		else if (r == 6) mask = 0x03C;  	// Granular row 2^6 tiles. Mask 4 bits
		else if (r == 5) mask = 0x01C;  	// Granular row 2^5 tiles. Mask 3 bits
		else if (r == 4) mask = 0x00C;  	// Granular row 2^4 tiles. Mask 2 bits
		else if (r == 3) mask = 0x00C;  	// Granular row 2^3 tiles. Mask 1 bits
		else assert(0);
	}
	else {
		assert(0);
	};

	return mask;
};


// Get dir
ETransDirType CAddrGen::GetTransDirType() {

	return (this->eDir);
};


// Get tile number of this trans 
int CAddrGen::GetTileNum() {

	return (this->nTileNum);
};


// Debug
EResultType CAddrGen::CheckAddr() {

	// Check trans number
	assert (this->nCurTrans <= this->nNumTotalTrans);
	// assert (this->nCurRow   <  IMG_VERTICAL_SIZE);
	// assert (this->nCurCol   <  NUM_COLUMNS_PER_ROW);

	// Check trans number
	if (this->nCurTrans < this->nNumTotalTrans) {
		assert (this->eFinalTrans == ERESULT_TYPE_NO);
	}
	else if (this->nCurTrans == this->nNumTotalTrans) {
		assert (this->eFinalTrans == ERESULT_TYPE_YES);
	}
	else {
		assert (0);	
	};

	return (ERESULT_TYPE_SUCCESS);
};


// Debug
EResultType CAddrGen::Display() {

	// Debug
	// this->CheckAddr();

	return (ERESULT_TYPE_SUCCESS);
};

