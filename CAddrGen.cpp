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
#include <array>

#include "CAddrGen.h"
#include "Memory.h"

/**
 * Remaps bits from a 64-bit input into a 32-bit sequence based on a fixed-size map.
 * * @param 	bit_order A fixed-size array of 32 integers. 
 * 	bit_order[i] is the source position in 'input_val' for the i-th bit of the result.
 * @param input_val The 64-bit source integer.
 * @return          The remapped integer (effectively 32 bits, stored in int64_t).
 */
int64_t remap_bits_32(const std::array<uint8_t, 32>& bit_order, int64_t input_val) {
    // Cast to unsigned to prevent arithmetic shifting issues
    uint64_t u_input = static_cast<uint64_t>(input_val);
    uint64_t result = 0;
	std::array<uint8_t, (32+COL2_WIDTH)> new_bit_order;

	// Filling the new array with your custom logic
    for (int i = 32; i < (32 + COL2_WIDTH); ++i) {
        new_bit_order[i] = i - 32; 
    }
    // Fill the rest from the passed bit_order
    for (int i = 0; i < 32; ++i) {
        new_bit_order[i] = bit_order[i] + COL2_WIDTH;
    }

	//for (int8_t i = (sizeof(new_bit_order) - 1); i >= 0; i = i-1){
	//	printf("bit_order[%d]: %d - new_bit_order[%d]: %d\n", i, bit_order[i], i, new_bit_order[i]);
	//}

    // The compiler will likely unroll this loop because the size (32) is constant.
    //for (uint64_t i = 0; i < sizeof(bit_order); ++i) {
	for (int8_t i = (sizeof(new_bit_order) - 1); i >= 0; i = i-1) {
        int src_pos = 	new_bit_order[i];

        // Safety check: Ensure we don't read outside the 64-bit input boundaries
        if (src_pos >= 0 && src_pos < 64) {
            // 1. Shift the input right to move the target bit to position 0
            // 2. Mask with 1 to isolate it
            // 3. Shift it left to the new position 'i'
            // 4. OR it into the result
            uint64_t bit = (u_input >> src_pos) & 1ULL;
            result |= (bit << ((sizeof(new_bit_order) - 1) - i));
        }
    }
    return static_cast<int64_t>(result);
}

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
		 this->cOperation == "STRIDE"    or 
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
			if ((nQuotient == 0.25) or
				(nQuotient == 0.5) or
				(nQuotient == 1.0) or
				(nQuotient == 1.25) or
				(nQuotient == 1.5) or
				(nQuotient == 2.0) or
				(nQuotient == 2.25) or
				(nQuotient == 2.5) or
				(nQuotient == 3.0) or
				(nQuotient == 3.25) or
				(nQuotient == 3.5) or
				(nQuotient == 4.0) or
				(nQuotient == 4.25) or
				(nQuotient == 4.5) )
			{
				nAddr = this->GetAddr_BGFAM();
				return nAddr;
			};
		} else if ((BANK_NUM==32) or (BANK_NUM==48) or (BANK_NUM==64)) {
			if ((nQuotient == 0.25) or
				(nQuotient == 0.5) or
				(nQuotient == 0.75) or
				(nQuotient == 1.0) or
				(nQuotient == 1.25) or
				(nQuotient == 1.5) or
				(nQuotient == 1.75) or
				(nQuotient == 2.0) or
				(nQuotient == 2.25) or
				(nQuotient == 2.5) or
				(nQuotient == 2.75) or
				(nQuotient == 3.0) or
				(nQuotient == 3.25) or
				(nQuotient == 3.5) or
				(nQuotient == 3.75) or
				(nQuotient == 4.0) or
				(nQuotient == 4.25) or
				(nQuotient == 4.5) )
			{
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
	else if (cAddrMap == "OIRAM") {
		nAddr = this->GetAddr_OIRAM();
	}
	else if (cAddrMap == "FLATFISH") {
		nAddr = this->GetAddr_FlatFish();
	}

	else if (cAddrMap == "MIN_K_UNION_ROW_WISE") {
		nAddr = this->GetAddr_MIN_K_UNION_ROW_WISE();
	}
	else if (cAddrMap == "MIN_K_UNION_COL_WISE") {
		nAddr = this->GetAddr_MIN_K_UNION_COL_WISE();
	}
	else if (cAddrMap == "MIN_K_UNION_BOTH_WISE") {
		nAddr = this->GetAddr_MIN_K_UNION_BOTH_WISE();
	}
	else if (cAddrMap == "MIN_K_UNION_TILE_ONLY") {
		nAddr = this->GetAddr_MIN_K_UNION_TILE();
	}
	else if (cAddrMap == "MIN_K_UNION_TILE_ROW_WISE") {
		nAddr = this->GetAddr_MIN_K_UNION_TILE_ROW_WISE();
	}
	else if (cAddrMap == "MIN_K_UNION_TILE_COL_WISE") {
		nAddr = this->GetAddr_MIN_K_UNION_TILE_COL_WISE();
	}
	else if (cAddrMap == "MIN_K_UNION_TILE_ROW_COL_WISE") {
		nAddr = this->GetAddr_MIN_K_UNION_TILE_ROW_COL_WISE();
	}
	else if (cAddrMap == "MIN_K_UNION_STRIDE") {
		nAddr = this->GetAddr_MIN_K_UNION_STRIDE();
	}

	else if (cAddrMap == "NEAR_OPTIMAL_ROW_WISE") {
		nAddr = this->GetAddr_NEAR_OPTIMAL_ROW_WISE();
	}
	else if (cAddrMap == "NEAR_OPTIMAL_COL_WISE") {
		nAddr = this->GetAddr_NEAR_OPTIMAL_COL_WISE();
	}
	else if (cAddrMap == "NEAR_OPTIMAL_BOTH_WISE") {
		nAddr = this->GetAddr_NEAR_OPTIMAL_BOTH_WISE();
	}
	else if (cAddrMap == "NEAR_OPTIMAL_TILE_ONLY") {
		nAddr = this->GetAddr_NEAR_OPTIMAL_TILE();
	}
	else if (cAddrMap == "NEAR_OPTIMAL_TILE_ROW_WISE") {
		nAddr = this->GetAddr_NEAR_OPTIMAL_TILE_ROW_WISE();
	}
	else if (cAddrMap == "NEAR_OPTIMAL_TILE_COL_WISE") {
		nAddr = this->GetAddr_NEAR_OPTIMAL_TILE_COL_WISE();
	}
	else if (cAddrMap == "NEAR_OPTIMAL_TILE_ROW_COL_WISE") {
		nAddr = this->GetAddr_NEAR_OPTIMAL_TILE_ROW_COL_WISE();
	}
	else if (cAddrMap == "NEAR_OPTIMAL_STRIDE") {
		nAddr = this->GetAddr_NEAR_OPTIMAL_STRIDE();
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
// Get Address (BGFAM)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Bank-Group-Interleaving
//---------------------------------------------------

uint16_t rotate_left(uint64_t num, uint8_t shift, uint8_t bits) {
    if (bits == 0 || bits > 64) return num; // Safety check
    
    // Create a bitmask of 'n' 1s. 
    // (Note: shifting by 64 is Undefined Behavior in C, so we handle it explicitly)
    uint16_t bit_mask = (bits == 64) ? ~0ULL : (1ULL << bits) - 1;
    
    // Ensure the input number doesn't have stray bits outside our custom bit width
    num &= bit_mask;
    
    shift %= bits;
    if (shift == 0) return num;
    
    // Shift left, shift right to wrap, combine, and apply the custom mask
    uint16_t rotated = ((num << shift) | (num >> (bits - shift))) & bit_mask;
    return rotated;
}

// Helper function: Integer log2
static int log2_int(int n) {
    if (n <= 0) return 0;
    int log = 0;
    while (n >>= 1) ++log;
    return log;
}

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
	uint64_t nQuotient = (int)floor(this->nSuperPageNum / k) % BANK_NUM;
	int16_t bit_flipped = rotate_left(nQuotient, 2, log2_int(BANK_NUM)); // Rotate left by 1 bit (flip MSB for power of 2 banks)

	nBank_new = (nBank ^ bit_flipped) % BANK_NUM; 		// Flip MSB. Experimentally, better than LSB

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

// Helper function: Get bit at position
static int get_bit(int64_t value, int position) {
    return (value >> position) & 1;
}

// Helper function: Calculate row jump
static int cal_row_jump(int curr_col_offset, int img_width_pixels, int bit_num) {
    int previous_bit = 0;
    int jump_value = 0;
    for (int i = 1; i <= bit_num; i++) {
        int curr_bit = bit_num - i;
        int curr_col_offset_bit = get_bit(curr_col_offset, curr_bit);
        int img_width_pixels_bit = get_bit(img_width_pixels, curr_bit);
        int power_of_two = ((curr_col_offset_bit ^ 1) & img_width_pixels_bit) & (previous_bit ^ 1);
        jump_value += (power_of_two << curr_bit);
        previous_bit |= power_of_two;
    }
    return jump_value;
}

// Helper function: Count set bits
static int bit_count(int n) {
    int count = 0;
    while (n > 0) {
        if (n & 1) count++;
        n >>= 1;
    }
    return count;
}

//// Helper function: Integer log2
//static int log2_int(int n) {
//    if (n <= 0) return 0;
//    int log = 0;
//    while (n >>= 1) ++log;
//    return log;
//}

//---------------------------------------------------
// Get Address (Proposal address map)
//	Matching Python generate_bankgroup_interleaving_matrix_proposal
//---------------------------------------------------
int64_t CAddrGen::GetAddr_OIRAM() {
    // Check AddrGen finished
    if (this->eFinalTrans == ERESULT_TYPE_YES) {
        return (-1);
    };

    // Configuration and Constants
    int num_banks           = BANK_NUM;
    int banks_per_group     = BANK_NUM_PER_GROUP;
    int num_bankgroups      = num_banks / banks_per_group;
    
    int num_cols            = 32;                   // Fixed as per Python default
    int num_rows_const      = 32768;                // Fixed as per Python default
    int num_cols_log        = log2_int(num_cols);
    int banks_per_group_log = log2_int(banks_per_group);

    this->SetNumPixelTrans();
    
    // Current coordinates mapping from pixels to transactions
    int img_width_pixels      = IMG_HORIZONTAL_SIZE;
    int img_height_pixels     = IMG_VERTICAL_SIZE;
    int image_width_trans_all = img_width_pixels / PIXELS_PER_TRANS; // 1 transaction = 16 pixels
    int curr_col_offset       = this->nApos / PIXELS_PER_TRANS;
    int curr_row_offset       = this->nBpos;

    int block_size_trans      = num_banks << num_cols_log;
    int block_size_trans_log  = log2_int(block_size_trans);

    // Row Jump Pre-calculation
    int row_jump = cal_row_jump(curr_col_offset, image_width_trans_all, 64);
    int row_jump_log = log2_int(std::max(row_jump, 1));

    // Bank Pre-calculation
    int num_row_per_banks = (num_cols >> row_jump_log);
    num_row_per_banks = log2_int(std::max(num_row_per_banks, 1));
    int num_row_per_allbankgroup = num_bankgroups << num_row_per_banks;
    int num_row_per_allbankgroup_log = log2_int(std::max(num_row_per_allbankgroup, 1));

    // Backward and Forward calculations corresponding to ~row_jump operations
    int rjb1 = ~row_jump + 1;
    int rjf  = ~rjb1;
    int rjb2 = rjb1 & ~row_jump;
    
    int row_jump_backward = image_width_trans_all & rjb2;
    int row_jump_forward  = image_width_trans_all & rjf;

    if (row_jump_backward == 0) row_jump_backward = 1; // Software hack to avoid div-by-zero
    int curr_col_offset_row_bank = (row_jump_backward == 1) ? curr_col_offset : (curr_col_offset % row_jump_backward);

    // Ratio Optimizations
    int a_lsb = log2_int(std::max(row_jump, 1));
    int b_lsb = log2_int(std::max(block_size_trans & (~block_size_trans + 1), 1));
    int min_lsb = std::min(a_lsb, b_lsb);
    
    int n_sp_pages = row_jump >> min_lsb;
    int n_rows = block_size_trans >> min_lsb;
    
    int n_sp_pages_log = log2_int(std::max(n_sp_pages, 1));
    int n_rows_log = log2_int(std::max(n_rows, 1));

    int high_bit_num = bit_count(row_jump_forward);
    
    int row_utilization = 0;
    bool is_power_of_two_banks = ((num_banks & (num_banks - 1)) == 0); // Replaces math.log2(num_banks).is_integer()
    
    // Row reduction calculation
    if (is_power_of_two_banks) {
        row_utilization = row_jump_forward - (((row_jump_forward * (img_height_pixels & (block_size_trans - 1))) >> block_size_trans_log) + high_bit_num);
    } else {
        row_utilization = row_jump_forward - (((row_jump_forward * (img_height_pixels % block_size_trans)) / block_size_trans) + high_bit_num);
    }

    // Row pre-calculation
    int row_inc_firstblock = row_jump_forward;
    int row_reduce_location = 0;
    int row_reducion = 0;
    int row_verical_linear = 0;
    int row_horizonal_linear = 0;
    int row_inc_block = 0;

    if (is_power_of_two_banks) {
        row_reduce_location = img_height_pixels >> block_size_trans_log;
        row_reducion = ((curr_row_offset >> block_size_trans_log) == row_reduce_location) ? row_utilization : 0;
        row_verical_linear = (curr_row_offset >> n_rows_log) << n_sp_pages_log;
        row_horizonal_linear = (curr_col_offset_row_bank >> num_cols_log) >> log2_int(std::max((row_jump >> num_cols_log) >> n_sp_pages_log, 1));
        row_inc_block = (curr_row_offset >> block_size_trans_log) * (image_width_trans_all & ~row_jump);
    } else {
        row_reduce_location = img_height_pixels / block_size_trans;
        row_reducion = ((curr_row_offset / block_size_trans) == row_reduce_location) ? row_utilization : 0;
        row_verical_linear = (curr_row_offset / std::max(n_rows, 1)) * n_sp_pages;
        row_horizonal_linear = (curr_col_offset_row_bank >> num_cols_log) / std::max(((row_jump >> num_cols_log) / std::max(n_sp_pages, 1)), 1);
        row_inc_block = (curr_row_offset / block_size_trans) * (image_width_trans_all & ~row_jump);
    }

    // Bank pre-calculation
    int bank_inc_block = (row_jump_backward == 1) ? 0 : (curr_col_offset / row_jump_backward);
    int bank_linear = ((curr_row_offset >> num_row_per_banks) % num_bankgroups) << banks_per_group_log;
    
    int bank_inc_img_bankgroup = 0;
    int bank_inc_col = 0;
    int bank_inc_col_bg = 0;

    if (is_power_of_two_banks) {
        bank_inc_img_bankgroup = (curr_row_offset >> num_row_per_allbankgroup_log) & (banks_per_group - 1);
        bank_inc_col = ((curr_col_offset_row_bank >> num_cols_log) * std::max((n_rows / num_bankgroups), 1)) & (banks_per_group - 1);
        bank_inc_col_bg = (curr_col_offset_row_bank >> num_cols_log >> banks_per_group_log) << log2_int(n_rows << banks_per_group_log);
    } else {
        bank_inc_img_bankgroup = (curr_row_offset / std::max(num_row_per_allbankgroup, 1)) & (banks_per_group - 1);
        bank_inc_col = ((curr_col_offset_row_bank >> num_cols_log) * std::max((n_rows / num_bankgroups), 1)) & (banks_per_group - 1);
        bank_inc_col_bg = (curr_col_offset_row_bank >> num_cols_log >> banks_per_group_log) * (n_rows << banks_per_group_log);
    }

    // Calculate final new address boundaries
    int64_t new_row_val = (int64_t)row_verical_linear + row_horizonal_linear + row_inc_firstblock + row_inc_block - row_reducion;
    uint64_t new_row = new_row_val % num_rows_const;
    int new_bank = (bank_linear + bank_inc_img_bankgroup + bank_inc_col + bank_inc_col_bg + bank_inc_block) % num_banks;
    int new_col  = (curr_col_offset + (curr_row_offset << row_jump_log)) % num_cols;

    // Safety checks against bound limits
    assert(new_row_val >= 0);
    assert(new_bank >= 0);
    assert(new_bank < num_banks);
    assert(new_col >= 0);
    assert(new_col < num_cols);

    // Get final physical address utilizing the global address map function
    int64_t nAddr_new = GetAddr_AMap_Global(new_row, new_bank, new_col);

    // Check last transaction application
    if (this->nCurTrans == this->nNumTotalTrans) {
        this->eFinalTrans = ERESULT_TYPE_YES;
    };

    // Set temp signal
    this->IsTransGenThisCycle = ERESULT_TYPE_YES;

    return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Min-K-Union's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Min-K-Union address mapping
//---------------------------------------------------
int64_t CAddrGen::GetAddr_MIN_K_UNION_ROW_WISE() {
	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();
	return (nAddr);
}

//---------------------------------------------------
// Get Address (Min-K-Union's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Min-K-Union address mapping
//---------------------------------------------------
int64_t CAddrGen::GetAddr_MIN_K_UNION_COL_WISE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();
	return (nAddr);
}

	if (BANK_NUM == 16) {
		if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 8, 7, 6, 5, 13, 12, 11, 10, 9};
		else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 16, 15, 14, 13, 12, 11, 10, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 21, 20, 19, 18, 17};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 9, 4, 3, 2, 1, 0, 8, 7, 6, 5, 14, 13, 12, 11, 10};
		else {
			printf("Error: Unsupported IMG size for Min-K-Union address mapping!");
			assert(0);
		}
	} else if (BANK_NUM == 32) {
		if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 14, 13, 12, 11, 10};
		else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 15, 14, 13, 12, 11};
		else {
			printf("Error: Unsupported IMG size for Min-K-Union address mapping!");
			assert(0);
		}
	} else if (BANK_NUM == 48) {
		if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 14, 13, 12, 11, 10};
		else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
		else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 15, 14, 13, 12, 11};
		else {
			printf("Error: Unsupported IMG size for Min-K-Union address mapping!");
			assert(0);
		}
	} else if (BANK_NUM == 64) {
		if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 15, 14, 13, 12, 11};
		else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 23, 22, 21, 20, 19};
		else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 11, 4, 3, 2, 1, 0, 10, 9, 8, 7, 6, 5, 16, 15, 14, 13, 12};
		else {
			printf("Error: Unsupported IMG size for Min-K-Union address mapping!");
			assert(0);
		}
	} else {
		printf("Error: Unsupported this number of banks!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Min-K-Union's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Min-K-Union address mapping
//---------------------------------------------------
int64_t CAddrGen::GetAddr_MIN_K_UNION_BOTH_WISE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	if (BANK_NUM == 16) {
		if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 2, 8, 7, 6, 5, 11, 10, 9, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 4, 3, 2, 8, 7, 6, 5, 12, 11, 10, 1, 0};
		else {
			printf("Error: Unsupported IMG size for Min-K-Union address mapping!");
			assert(0);
		}
	} else if (BANK_NUM == 32) {
		if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 4, 3, 2, 9, 8, 7, 6, 5, 13, 12, 11, 1, 0};
		else {
			printf("Error: Unsupported IMG size for Min-K-Union address mapping!");
			assert(0);
		}
	} else if (BANK_NUM == 48) {
		if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 4, 3, 2, 9, 8, 7, 6, 5, 13, 12, 11, 1, 0};
		else {
			printf("Error: Unsupported IMG size for Min-K-Union address mapping!");
			assert(0);
		}
	} else if (BANK_NUM == 64) {
		if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 10, 9, 8, 7, 6, 5, 13, 12, 11, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 11, 4, 3, 2, 10, 9, 8, 7, 6, 5, 14, 13, 12, 1, 0};
		else {
			printf("Error: Unsupported IMG size for Min-K-Union address mapping!");
			assert(0);
		}
	} else {
		printf("Error: Unsupported this number of banks!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Min-K-Union's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Min-K-Union address mapping
//---------------------------------------------------
int64_t CAddrGen::GetAddr_MIN_K_UNION_TILE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	if (TILE_SIZE == 3) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 7) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 11) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 16) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 32) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 18, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 9, 8, 7, 6, 5, 22, 21, 20, 19, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 10, 4, 3, 9, 8, 7, 6, 5, 12, 11, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else {
		printf("Error: Unsupported this tile size!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Min-K-Union's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Min-K-Union address mapping
//---------------------------------------------------
int64_t CAddrGen::GetAddr_MIN_K_UNION_TILE_ROW_WISE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	if (TILE_SIZE == 3) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 7) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 11) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 16) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 32) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else {
		printf("Error: Unsupported this tile size!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Min-K-Union's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Min-K-Union address mapping
//---------------------------------------------------
int64_t CAddrGen::GetAddr_MIN_K_UNION_TILE_COL_WISE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	if (TILE_SIZE == 3) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 9, 8, 7, 6, 5, 13, 12, 11, 10, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 10, 4, 3, 2, 1, 9, 8, 7, 6, 5, 14, 13, 12, 11, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 7) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 14, 13, 12, 11, 10};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 15, 14, 13, 12, 11};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 11) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 14, 13, 12, 11, 10};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 15, 14, 13, 12, 11};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 16) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 22, 21, 20, 19, 18};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 14, 13, 12, 11, 10};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 10, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 15, 14, 13, 12, 11};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 32) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 18, 17, 16, 15, 14, 13, 12, 11, 10, 4, 3, 2, 1, 9, 8, 7, 6, 5, 22, 21, 20, 19, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 8, 7, 6, 5, 10, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 9, 8, 7, 6, 5, 13, 12, 11, 10, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 9, 8, 7, 6, 5, 12, 11, 10, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 10, 4, 3, 9, 8, 7, 6, 5, 12, 11, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 10, 4, 3, 2, 1, 9, 8, 7, 6, 5, 14, 13, 12, 11, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else {
		printf("Error: Unsupported this tile size!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Min-K-Union's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Min-K-Union address mapping
//---------------------------------------------------
int64_t CAddrGen::GetAddr_MIN_K_UNION_TILE_ROW_COL_WISE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	if (TILE_SIZE == 3) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 10, 4, 3, 9, 8, 7, 6, 5, 12, 11, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 7) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 10, 4, 3, 9, 8, 7, 6, 5, 12, 11, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 11) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 10, 4, 3, 9, 8, 7, 6, 5, 12, 11, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 16) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 10, 4, 3, 9, 8, 7, 6, 5, 12, 11, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 32) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 8, 7, 6, 5, 11, 10, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 4, 3, 2, 9, 8, 7, 6, 5, 13, 12, 11, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else {
		printf("Error: Unsupported this tile size!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Min-K-Union's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Min-K-Union address mapping
//---------------------------------------------------
int64_t CAddrGen::GetAddr_MIN_K_UNION_STRIDE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	if (TILE_SIZE == 3) {
		if (BANK_NUM == 16) {
			if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 48) {
			if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 64) {
			if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 7) {
		if (BANK_NUM == 16) {
			if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 48) {
			if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 64) {
			if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 11) {
		if (BANK_NUM == 16) {
			if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 48) {
			if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 64) {
			if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 16) {
		if (BANK_NUM == 16) {
			if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 48) {
			if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 64) {
			if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 32) {
		if (BANK_NUM == 16) {
			if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 48) {
			if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 64) {
			if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for MIN_K_UNION_STRIDE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else {
		printf("Error: Unsupported this tile size!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Min-K-Union's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Min-K-Union address mapping
//---------------------------------------------------
int64_t CAddrGen::GetAddr_FlatFish() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	if (BANK_NUM == 16) {
		if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 20, 18, 19, 16, 12, 7, 10, 2, 21, 17, 11, 3, 4, 14, 5, 6, 1, 9, 22, 15, 8, 13, 0};
		else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 18, 3, 11, 21, 17, 14, 15, 16, 13, 19, 12, 10, 5, 9, 7, 8, 4, 0, 6, 2, 1, 20};
		else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 0, 1};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 20, 22, 16, 19, 18, 17, 4, 15, 14, 13, 21, 11, 10, 9, 8, 7, 6, 3, 12, 5, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 16, 19, 18, 17, 20, 15, 14, 13, 10, 11, 5, 12, 8, 7, 9, 6, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 21, 6, 16, 14, 7, 17, 3, 1, 15, 18, 2, 4, 13, 20, 19, 10, 12, 11, 5, 9, 22, 8, 0};
		else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 3, 4, 5, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 10, 2, 13, 15, 18, 17, 4, 16, 22, 14, 20, 19, 11, 7, 6, 12, 9, 3, 8, 0, 5, 21, 1};
		else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 8, 21, 20, 18, 14, 17, 16, 9, 19, 13, 12, 15, 10, 22, 5, 7, 6, 11, 4, 0, 2, 1, 3};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 13, 18, 17, 8, 15, 14, 4, 12, 19, 10, 9, 0, 7, 6, 11, 16, 3, 2, 1, 5};
		else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 0, 1};
		else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 15, 12, 4, 19, 7, 6, 17, 11, 1, 22, 3, 14, 13, 8, 10, 9, 2, 0, 18, 16, 20, 5, 21};
		else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 9, 21, 7, 17, 20, 2, 15, 13, 22, 10, 11, 3, 16, 18, 8, 6, 19, 5, 1, 4, 12, 14, 0};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 3, 4, 21, 0, 15, 13, 22, 19, 17, 8, 16, 14, 12, 7, 1, 2, 18, 10, 20, 6, 11, 5, 9};
		else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 0, 6, 5, 4, 3, 2, 1, 7};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 0, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 17};
		else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 3, 12, 15, 1, 22, 17, 16, 21, 19, 14, 18, 11, 6, 13, 8, 7, 5, 20, 4, 10, 2, 9, 0};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 16, 7, 18, 10, 3, 12, 14, 13, 17, 11, 15, 4, 8, 0, 6, 9, 19, 2, 20, 1, 5};
		else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 19, 4, 6, 2, 18, 13, 15, 0, 1, 20, 17, 9, 12, 7, 5, 8, 22, 10, 11, 14, 21, 16, 3};
		else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 0, 1};
		else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 15, 3, 14, 19, 18, 2, 16, 21, 11, 9, 20, 13, 10, 22, 17, 7, 6, 5, 4, 8, 12, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 7, 3, 22, 4, 16, 0, 13, 18, 6, 14, 12, 17, 19, 21, 10, 9, 20, 5, 2, 11, 15, 1, 8};
		else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 1, 11, 16, 10, 13, 4, 8, 3, 21, 22, 20, 18, 14, 9, 7, 6, 5, 0, 12, 19, 15, 17, 2};
		else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 14, 19, 18, 17, 5, 15, 16, 13, 12, 20, 1, 0, 9, 7, 3, 10, 8, 4, 2, 11, 6};
		else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 0, 1};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 2, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 17, 3, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 19, 1, 18, 22, 15, 17, 16, 10, 14, 13, 20, 11, 21, 9, 8, 7, 6, 5, 4, 2, 12, 3, 0};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 21, 2, 14, 22, 4, 20, 16, 9, 15, 19, 17, 13, 10, 8, 7, 5, 18, 12, 11, 3, 0, 1, 6};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 0, 14, 7, 4, 3, 19, 12, 13, 15, 20, 17, 6, 5, 1, 18, 2, 16, 9, 11, 22, 8, 21, 10};
		else {
			printf("Error: Unsupported IMG size for Flatfish address mapping!");
			assert(0);
		}
	} else if (BANK_NUM == 32) {
		if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 20, 18, 15, 10, 21, 13, 3, 12, 1, 14, 11, 17, 4, 22, 16, 0, 23, 5, 19, 9, 6, 2, 8, 7};
		else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 0, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 20};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 13, 16, 20, 6, 12, 4, 22, 17, 8, 2, 7, 11, 3, 10, 1, 9, 5, 19, 21, 15, 14, 18, 0};
		else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 0, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 14};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 0, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 19};
		else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 0, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 18};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 13, 1, 20, 3, 19, 21, 4, 16, 18, 14, 5, 6, 8, 0, 17, 22, 12, 2, 10, 15, 23, 11, 7, 9};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 0, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 21};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 3, 15, 16, 18, 5, 17, 23, 19, 11, 13, 22, 21, 12, 8, 14, 9, 7, 10, 4, 1, 2, 20, 6, 0};
		else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 15, 20, 12, 1, 5, 2, 6, 13, 11, 16, 7, 21, 9, 8, 17, 10, 0, 4, 19, 14, 18, 3};
		else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 10, 21, 20, 19, 18, 17, 16, 15, 14, 3, 22, 11, 13, 9, 12, 7, 6, 5, 4, 8, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 22, 23, 20, 8, 16, 17, 4, 15, 14, 13, 19, 11, 10, 9, 12, 7, 6, 5, 21, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 15, 21, 6, 12, 18, 3, 13, 20, 22, 17, 19, 8, 11, 23, 9, 5, 10, 7, 2, 4, 14, 0, 16, 1};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 6, 14, 7, 5, 15, 21, 9, 20, 3, 23, 12, 1, 19, 16, 11, 0, 13, 22, 8, 4, 10, 2, 17, 18};
		else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 0, 1};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 19, 22, 21, 20, 4, 13, 18, 17, 6, 14, 16, 12, 11, 10, 9, 5, 7, 15, 8, 3, 0, 23, 1, 2};
		else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 16, 19, 15, 10, 2, 11, 17, 8, 20, 23, 4, 12, 5, 3, 6, 1, 13, 7, 22, 14, 18, 21, 9, 0};
		else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 11, 21, 12, 23, 4, 15, 17, 18, 7, 14, 13, 8, 16, 3, 9, 22, 10, 6, 5, 19, 20, 0, 2, 1};
		else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 18, 21, 20, 19, 17, 14, 16, 15, 22, 4, 12, 9, 10, 11, 0, 8, 6, 5, 13, 3, 2, 1, 7};
		else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 16};
		else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 0, 1, 2};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 2, 6, 21, 1, 8, 9, 22, 3, 13, 16, 17, 10, 20, 18, 12, 5, 11, 14, 4, 15, 0, 7, 19, 23};
		else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 11, 4, 14, 15, 21, 5, 17, 7, 23, 20, 3, 13, 16, 22, 9, 8, 6, 2, 10, 19, 12, 18, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 21, 22, 2, 20, 6, 18, 11, 15, 23, 3, 14, 12, 7, 19, 9, 8, 13, 10, 5, 4, 17, 16, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 18, 22, 21, 20, 23, 11, 4, 16, 15, 14, 13, 12, 19, 10, 9, 8, 7, 6, 5, 17, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 10, 16, 22, 23, 6, 18, 12, 5, 15, 7, 17, 19, 13, 1, 0, 14, 20, 21, 9, 4, 3, 2, 11, 8};
		else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 19, 3, 20, 9, 4, 15, 17, 7, 14, 10, 23, 13, 2, 16, 8, 22, 12, 6, 18, 21, 11, 0, 5, 1};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 0, 3, 2, 1, 4};
		else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 22, 2, 23, 5, 14, 6, 18, 4, 7, 15, 16, 8, 20, 21, 11, 17, 3, 10, 13, 19, 0, 1, 12, 9};
		else {
			printf("Error: Unsupported IMG size for Flatfish address mapping!");
			assert(0);
		}
	} else if (BANK_NUM == 48) {
		if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 0, 8, 7, 6, 5, 4, 3, 2, 1, 9};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 16, 23, 22, 3, 5, 19, 18, 21, 20, 15, 14, 13, 2, 11, 10, 9, 8, 7, 24, 12, 4, 6, 17, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 18, 4, 3, 17, 19, 0, 16, 15, 20, 13, 12, 22, 6, 9, 8, 5, 21, 11, 10, 7, 2, 1, 14};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 7, 10, 19, 18, 17, 16, 21, 14, 6, 12, 11, 13, 9, 8, 15, 20, 5, 4, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 18, 9, 7, 22, 8, 17, 16, 15, 20, 13, 12, 11, 10, 14, 0, 19, 6, 5, 4, 3, 2, 1, 21};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 9, 20, 19, 10, 17, 16, 15, 14, 2, 12, 11, 13, 1, 8, 7, 3, 5, 4, 6, 18, 21, 0};
		else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 10};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 16, 21, 14, 24, 19, 3, 20, 17, 9, 18, 22, 15, 4, 23, 8, 10, 13, 0, 6, 7, 12, 5, 2, 1, 11};
		else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 18, 23, 14, 4, 10, 15, 8, 11, 2, 7, 20, 19, 17, 16, 12, 6, 1, 5, 9, 22, 24, 3, 0, 21, 13};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 22, 4, 17, 21, 19, 18, 5, 16, 15, 14, 13, 12, 23, 10, 9, 8, 7, 6, 20, 11, 3, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 4, 23, 22, 21, 20, 19, 18, 17, 16, 15, 1, 9, 12, 11, 10, 13, 8, 7, 6, 5, 3, 24, 2, 14, 0};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 18, 4, 19, 17, 22, 20, 2, 15, 8, 7, 14, 6, 13, 10, 9, 24, 11, 12, 5, 1, 23, 3, 21, 0, 16};
		else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 1, 23, 19, 12, 9, 16, 14, 24, 17, 5, 22, 15, 21, 3, 13, 0, 18, 6, 10, 11, 4, 8, 2, 20, 7};
		else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 10, 22, 3, 18, 24, 5, 14, 15, 4, 13, 19, 1, 12, 16, 7, 8, 6, 20, 2, 11, 17, 9, 0, 23, 21};
		else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 0, 5, 4, 3, 2, 1, 6};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 17, 7, 2, 20, 3, 19, 24, 8, 16, 0, 6, 23, 4, 22, 11, 5, 15, 13, 10, 1, 12, 14, 9, 21, 18};
		else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 19, 6, 18, 9, 12, 23, 5, 17, 3, 15, 24, 20, 21, 14, 16, 13, 7, 2, 22, 10, 4, 1, 11, 8, 0};
		else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 9, 23, 21, 18, 19, 24, 16, 13, 3, 20, 22, 11, 17, 7, 10, 12, 8, 14, 6, 15, 4, 5, 2, 1, 0};
		else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 0, 5, 4, 3, 2, 1, 6};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 17, 5, 6, 4, 9, 21, 20, 16, 23, 18, 19, 3, 24, 10, 11, 1, 14, 13, 7, 22, 15, 8, 12, 2, 0};
		else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 0, 1};
		else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 9, 23, 22, 21, 20, 19, 18, 17, 24, 15, 14, 12, 4, 11, 16, 13, 8, 7, 6, 5, 0, 3, 2, 1, 10};
		else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
		else {
			printf("Error: Unsupported IMG size for Flatfish address mapping!");
			assert(0);
		}
	} else {
		printf("Error: Unsupported this number of banks!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Near Optimal's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Near Optimal address mapping for Row-Wise access
//---------------------------------------------------
int64_t CAddrGen::GetAddr_NEAR_OPTIMAL_ROW_WISE() {
	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();
	return (nAddr);
}

//---------------------------------------------------
// Get Address (Near Optimal's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Near Optimal address mapping for Col-Wise access
//---------------------------------------------------
int64_t CAddrGen::GetAddr_NEAR_OPTIMAL_COL_WISE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	// ========== Column-Wise: True + Row-Wise: False ==========
	if (BANK_NUM == 16) {
		if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 10, 2, 1, 0, 9, 8, 7, 6, 5};
		else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 2, 12, 11, 1, 0, 10, 9, 8, 7, 6};
		else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 10, 9, 8, 6, 7, 5};
		else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 2, 1, 14, 13, 12, 0, 11, 10, 9, 8, 7};
		else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 10, 9, 6, 8, 7, 5};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 11, 10, 9, 7, 8, 6};
		else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 10, 7, 9, 6, 8, 5};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 14, 13, 12, 11, 10, 9, 8};
		else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 7, 10, 6, 9, 8, 5};
		else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 11, 10, 7, 9, 8, 6};
		else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 10, 6, 8, 9, 7, 5};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 8, 9, 7};
		else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 8, 10, 6, 7, 9, 5};
		else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 11, 8, 10, 7, 9, 6};
		else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 8, 7, 10, 6, 9, 5};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 8, 7, 6, 5, 4, 3, 2, 1, 0, 17, 16, 15, 14, 13, 12, 11, 10, 9};
		else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 8, 7, 11, 6, 10, 9, 5};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 8, 11, 7, 10, 9, 6};
		else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 8, 6, 10, 7, 9, 5};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 8, 10, 9, 7};
		else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 6, 8, 10, 9, 7, 5};
		else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 11, 7, 9, 10, 8, 6};
		else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 7, 6, 9, 10, 8, 5};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 14, 13, 12, 11, 9, 10, 8};
		else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 7, 9, 6, 10, 8, 5};
		else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 9, 11, 7, 8, 10, 6};
		else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 9, 6, 8, 7, 10, 5};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 9, 11, 8, 10, 7};
		else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 9, 8, 11, 6, 7, 10, 5};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 9, 8, 11, 7, 10, 6};
		else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 9, 8, 7, 11, 6, 10, 5};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 18, 17, 16, 15, 14, 13, 12, 11, 10};
		else {
			printf("Error: Unsupported IMG size for NEAR_OPTIMAL_COL_WISE address mapping!");
			return -1;
		}
	} else if (BANK_NUM == 32) {
		if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 0, 12, 11, 2, 1, 10, 9, 8, 7, 6};
		else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 2, 0, 14, 13, 12, 1, 11, 10, 9, 8, 7};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 11, 15, 14, 13, 12, 10, 9, 7, 8, 6};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 14, 13, 12, 11, 10, 9, 8};
		else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 11, 15, 14, 13, 12, 10, 7, 9, 8, 6};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 11, 10, 8, 9, 7};
		else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 11, 15, 14, 13, 12, 8, 10, 7, 9, 6};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 8, 7, 6, 5, 4, 3, 2, 1, 0, 14, 18, 17, 16, 15, 13, 12, 11, 10, 9};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 8, 15, 14, 13, 12, 11, 7, 10, 9, 6};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 11, 8, 10, 9, 7};
		else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 11, 15, 14, 13, 12, 7, 9, 10, 8, 6};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 7, 6, 5, 4, 3, 2, 1, 0, 13, 17, 16, 15, 14, 12, 11, 9, 10, 8};
		else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 9, 15, 14, 13, 12, 11, 7, 8, 10, 6};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 9, 11, 8, 10, 7};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 12, 15, 14, 13, 9, 8, 11, 7, 10, 6};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 19, 18, 17, 16, 14, 13, 12, 11, 10};
		else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 8, 15, 14, 13, 9, 12, 7, 11, 10, 6};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 9, 16, 15, 14, 13, 12, 8, 11, 10, 7};
		else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 9, 15, 14, 13, 12, 7, 11, 8, 10, 6};
		else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 7, 6, 5, 4, 3, 2, 1, 0, 13, 17, 16, 15, 14, 12, 9, 11, 10, 8};
		else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 7, 15, 14, 13, 12, 9, 11, 10, 8, 6};
		else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 8, 10, 11, 9, 7};
		else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 12, 15, 14, 13, 8, 7, 10, 11, 9, 6};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 8, 7, 6, 5, 4, 3, 2, 1, 0, 14, 18, 17, 16, 15, 13, 12, 10, 11, 9};
		else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 12, 15, 14, 13, 8, 10, 7, 11, 9, 6};
		else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 10, 16, 15, 14, 13, 12, 8, 9, 11, 7};
		else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 12, 15, 14, 13, 10, 7, 9, 8, 11, 6};
		else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 7, 6, 5, 4, 3, 2, 1, 0, 13, 17, 16, 15, 14, 10, 12, 9, 11, 8};
		else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 9, 15, 14, 13, 10, 12, 7, 8, 11, 6};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 13, 16, 15, 14, 10, 9, 12, 8, 11, 7};
		else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 13, 15, 14, 10, 9, 8, 12, 7, 11, 6};
		else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 20, 19, 18, 17, 15, 14, 13, 12, 11};
		else {
			printf("Error: Unsupported IMG size for NEAR_OPTIMAL_COL_WISE address mapping!");
			return -1;
		}
	} else if (BANK_NUM == 48) {
		if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 10, 13, 12, 11, 0, 9, 8, 6, 7, 5};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 11, 15, 14, 13, 12, 10, 9, 7, 8, 6};
		else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 10, 14, 13, 12, 11, 7, 9, 6, 8, 5};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 11, 10, 8, 9, 7};
		else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 11, 14, 13, 12, 8, 7, 10, 6, 9, 5};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 11, 15, 14, 13, 12, 8, 10, 7, 9, 6};
		else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 10, 14, 13, 12, 11, 6, 8, 9, 7, 5};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 7, 6, 5, 4, 3, 2, 1, 0, 13, 17, 16, 15, 14, 12, 11, 10, 9, 8};
		else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 6, 14, 13, 12, 11, 10, 8, 9, 7, 5};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 8, 15, 14, 13, 12, 11, 7, 10, 9, 6};
		else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 7, 14, 13, 12, 8, 11, 6, 9, 10, 5};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 11, 9, 8, 10, 7};
		else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 14, 13, 12, 7, 11, 6, 10, 8, 5};
		else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 9, 15, 14, 13, 12, 11, 7, 10, 8, 6};
		else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 8, 14, 13, 12, 9, 11, 6, 10, 7, 5};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 19, 18, 17, 16, 14, 13, 12, 11, 10};
		else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 8, 14, 13, 12, 9, 6, 11, 10, 7, 5};
		else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 12, 15, 14, 13, 9, 7, 11, 10, 8, 6};
		else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 14, 13, 12, 7, 6, 11, 10, 8, 5};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 8, 9, 11, 10, 7};
		else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 12, 14, 13, 8, 7, 6, 9, 11, 10, 5};
		else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 12, 15, 14, 13, 8, 7, 10, 9, 11, 6};
		else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 10, 14, 13, 12, 6, 8, 9, 11, 7, 5};
		else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 7, 6, 5, 4, 3, 2, 1, 0, 13, 17, 16, 15, 14, 12, 10, 9, 11, 8};
		else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 6, 14, 13, 12, 10, 8, 9, 7, 11, 5};
		else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 10, 15, 14, 13, 8, 12, 7, 9, 11, 6};
		else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 10, 14, 13, 8, 7, 12, 6, 9, 11, 5};
		else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 10, 16, 15, 14, 13, 12, 9, 8, 11, 7};
		else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 14, 13, 10, 7, 12, 6, 8, 11, 5};
		else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 9, 15, 14, 13, 10, 12, 7, 8, 11, 6};
		else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 8, 14, 13, 10, 9, 12, 6, 7, 11, 5};
		else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 20, 19, 18, 17, 15, 14, 13, 12, 11};
		else {
			printf("Error: Unsupported IMG size for NEAR_OPTIMAL_COL_WISE address mapping!");
			return -1;
		}
	} else if (BANK_NUM == 64) {
		if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 1, 0, 14, 13, 12, 2, 11, 10, 9, 8, 7};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 14, 13, 12, 11, 10, 9, 8};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 12, 17, 16, 15, 14, 11, 10, 8, 9, 7};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 8, 7, 6, 5, 4, 3, 2, 1, 14, 0, 18, 17, 16, 15, 13, 12, 11, 10, 9};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 12, 17, 16, 15, 14, 11, 8, 10, 9, 7};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 13, 18, 17, 16, 15, 12, 11, 9, 10, 8};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 12, 17, 16, 15, 14, 9, 11, 8, 10, 7};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 20, 19, 18, 17, 14, 13, 12, 11, 10};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 9, 17, 16, 15, 14, 12, 8, 11, 10, 7};
		else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 13, 18, 17, 16, 15, 12, 9, 11, 10, 8};
		else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 12, 17, 16, 15, 14, 8, 10, 11, 9, 7};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 19, 18, 17, 16, 13, 12, 10, 11, 9};
		else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 10, 17, 16, 15, 14, 12, 8, 9, 11, 7};
		else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 13, 18, 17, 16, 15, 10, 12, 9, 11, 8};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 10, 17, 16, 15, 14, 9, 12, 8, 11, 7};
		else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 17, 16, 21, 20, 19, 18, 15, 14, 13, 12, 11};
		else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 14, 9, 17, 16, 15, 10, 13, 8, 12, 11, 7};
		else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 10, 18, 17, 16, 15, 13, 9, 12, 11, 8};
		else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 10, 17, 16, 15, 14, 8, 12, 9, 11, 7};
		else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 19, 18, 17, 16, 13, 10, 12, 11, 9};
		else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 8, 17, 16, 15, 14, 10, 12, 11, 9, 7};
		else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 13, 18, 17, 16, 15, 9, 11, 12, 10, 8};
		else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 9, 17, 16, 15, 14, 8, 11, 12, 10, 7};
		else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 20, 19, 18, 17, 14, 13, 11, 12, 10};
		else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 9, 17, 16, 15, 14, 11, 8, 12, 10, 7};
		else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 11, 18, 17, 16, 15, 13, 9, 10, 12, 8};
		else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 11, 17, 16, 15, 14, 8, 10, 9, 12, 7};
		else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 19, 18, 17, 16, 11, 13, 10, 12, 9};
		else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 14, 10, 17, 16, 15, 11, 13, 8, 9, 12, 7};
		else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 11, 18, 17, 16, 15, 10, 13, 9, 12, 8};
		else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 14, 10, 17, 16, 15, 11, 9, 13, 8, 12, 7};
		else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 18, 17, 22, 21, 20, 19, 16, 15, 14, 13, 12};
		else {
			printf("Error: Unsupported IMG size for NEAR_OPTIMAL_COL_WISE address mapping!");
			return -1;
		}
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	//printf("COL_WIDTH = %d | Old Address: %lx\t- New Col: %d - New Bank: %d - New Row: %ld\n", COL_WIDTH, nAddr, GetColNum_AMap_Global(nAddr), GetBankNum_AMap_Global(nAddr), GetRowNum_AMap_Global(nAddr));
	//printf("COL_WIDTH = %d | New Address: %lx\t- New Col: %d - New Bank: %d - New Row: %ld\n", COL_WIDTH, nAddr_new, GetColNum_AMap_Global(nAddr_new), GetBankNum_AMap_Global(nAddr_new), GetRowNum_AMap_Global(nAddr_new));
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Near Optimal's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Near Optimal address mapping for Both-Wise access
//---------------------------------------------------
int64_t CAddrGen::GetAddr_NEAR_OPTIMAL_BOTH_WISE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	// ========== Column-Wise: True + Row-Wise: True ==========
	if (BANK_NUM == 16) {
		if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 4, 9, 8, 3, 2, 7, 1, 6, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 4, 10, 9, 3, 2, 8, 1, 7, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 4, 9, 8, 3, 2, 1, 6, 7, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 6, 5, 4, 11, 10, 3, 2, 9, 1, 8, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 10, 9, 2, 1, 6, 8, 7, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 4, 10, 9, 3, 2, 1, 7, 8, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 10, 9, 7, 2, 1, 6, 8, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 6, 5, 4, 12, 11, 3, 2, 10, 1, 9, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 10, 7, 2, 1, 6, 9, 8, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 3, 11, 10, 2, 1, 7, 9, 8, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 10, 6, 2, 1, 8, 9, 7, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 6, 5, 4, 11, 10, 3, 2, 1, 8, 9, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 10, 8, 2, 1, 6, 7, 9, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 3, 11, 10, 8, 2, 1, 7, 9, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 8, 4, 3, 11, 10, 7, 2, 1, 6, 9, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 8, 7, 6, 5, 4, 13, 12, 3, 2, 11, 1, 10, 0, 9};
		else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 8, 4, 3, 11, 7, 2, 1, 6, 10, 9, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 3, 11, 8, 2, 1, 7, 10, 9, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 2, 11, 8, 6, 1, 10, 7, 9, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 3, 12, 11, 2, 1, 8, 10, 9, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 2, 11, 8, 6, 1, 10, 9, 7, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 3, 11, 7, 2, 1, 9, 10, 8, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 2, 11, 7, 6, 1, 9, 10, 8, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 6, 5, 4, 12, 11, 3, 2, 1, 9, 10, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 2, 11, 9, 7, 1, 6, 10, 8, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 3, 11, 9, 2, 1, 7, 8, 10, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 2, 11, 9, 6, 1, 8, 7, 10, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 3, 12, 11, 9, 2, 1, 8, 10, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 9, 4, 3, 11, 8, 2, 1, 6, 7, 10, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 5, 4, 3, 12, 11, 8, 2, 1, 7, 10, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 8, 4, 3, 12, 11, 7, 2, 1, 6, 10, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 9, 8, 7, 6, 5, 4, 14, 13, 3, 2, 12, 1, 11, 0, 10};
		else {
			printf("Error: Unsupported IMG size for NEAR_OPTIMAL_BOTH_WISE address mapping!");
			return -1;
		}
	} else if (BANK_NUM == 32) {
		if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 2, 10, 9, 4, 3, 8, 1, 7, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 6, 5, 2, 11, 10, 4, 3, 9, 1, 8, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 9, 11, 10, 3, 2, 1, 7, 8, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 6, 5, 2, 12, 11, 4, 3, 10, 1, 9, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 10, 11, 3, 2, 1, 7, 9, 8, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 10, 12, 11, 3, 2, 1, 8, 9, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 10, 11, 8, 3, 2, 1, 7, 9, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 8, 7, 6, 5, 2, 13, 12, 4, 3, 11, 1, 10, 0, 9};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 8, 2, 1, 7, 10, 9, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 11, 12, 3, 2, 1, 8, 10, 9, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 11, 2, 1, 9, 10, 8, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 4, 11, 13, 12, 3, 2, 1, 9, 10, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 9, 2, 1, 7, 8, 10, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 11, 12, 9, 3, 2, 1, 8, 10, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 9, 8, 2, 1, 7, 10, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 9, 8, 7, 6, 5, 2, 14, 13, 4, 3, 12, 1, 11, 0, 10};
		else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 9, 5, 4, 3, 12, 13, 8, 2, 1, 7, 11, 10, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 3, 12, 13, 9, 2, 1, 8, 11, 10, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 9, 2, 1, 11, 8, 10, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 4, 12, 13, 3, 2, 1, 9, 11, 10, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 9, 12, 7, 2, 1, 11, 10, 8, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 3, 8, 13, 12, 2, 1, 10, 11, 9, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 8, 2, 1, 10, 11, 9, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 8, 7, 6, 5, 4, 12, 14, 13, 3, 2, 1, 10, 11, 0, 9};
		else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 8, 2, 1, 7, 11, 9, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 3, 12, 13, 10, 2, 1, 8, 9, 11, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 10, 2, 1, 9, 8, 11, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 4, 12, 13, 10, 3, 2, 1, 9, 11, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 2, 1, 7, 8, 11, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 3, 12, 13, 10, 9, 2, 1, 8, 11, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 8, 2, 1, 7, 11, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 10, 9, 8, 7, 6, 5, 2, 15, 14, 4, 3, 13, 1, 12, 0, 11};
		else {
			printf("Error: Unsupported IMG size for NEAR_OPTIMAL_BOTH_WISE address mapping!");
			return -1;
		}
	} else if (BANK_NUM == 48) {
		if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 9, 3, 2, 1, 6, 7, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 9, 11, 10, 3, 2, 1, 7, 8, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 7, 3, 2, 1, 6, 8, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 10, 12, 11, 3, 2, 1, 9, 8, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 8, 7, 2, 1, 6, 9, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 10, 11, 8, 3, 2, 1, 7, 9, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 6, 11, 10, 2, 1, 8, 9, 7, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 4, 2, 13, 12, 11, 3, 1, 10, 9, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 6, 2, 1, 8, 9, 7, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 8, 2, 1, 7, 10, 9, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 4, 3, 11, 12, 7, 2, 1, 6, 9, 10, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 11, 12, 3, 2, 1, 9, 8, 10, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 11, 9, 7, 2, 1, 6, 10, 8, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 9, 2, 1, 7, 10, 8, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 4, 3, 11, 12, 8, 2, 1, 6, 10, 7, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 9, 8, 7, 6, 5, 2, 14, 13, 4, 3, 12, 1, 11, 10, 0};
		else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 4, 3, 6, 12, 8, 2, 1, 11, 10, 7, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 9, 2, 1, 11, 10, 8, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 6, 12, 9, 7, 1, 11, 10, 8, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 3, 9, 13, 12, 2, 1, 8, 11, 10, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 4, 3, 6, 12, 7, 2, 1, 9, 11, 10, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 8, 2, 1, 10, 9, 11, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 8, 12, 10, 6, 1, 9, 11, 7, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 4, 12, 13, 3, 2, 1, 10, 9, 11, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 8, 12, 10, 6, 1, 9, 7, 11, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 12, 10, 8, 2, 1, 7, 9, 11, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 4, 3, 12, 10, 7, 2, 1, 6, 9, 11, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 3, 12, 13, 10, 2, 1, 9, 8, 11, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 10, 4, 3, 12, 9, 7, 2, 1, 6, 8, 11, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 2, 1, 7, 8, 11, 0, 6};
		else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 9, 4, 3, 12, 13, 8, 2, 1, 6, 7, 11, 0, 5};
		else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 10, 9, 8, 7, 6, 5, 2, 15, 14, 4, 3, 13, 1, 12, 0, 11};
		else {
			printf("Error: Unsupported IMG size for NEAR_OPTIMAL_BOTH_WISE address mapping!");
			return -1;
		}
	} else if (BANK_NUM == 64) {
		if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 3, 2, 12, 11, 10, 4, 9, 1, 8, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 3, 2, 13, 12, 11, 4, 10, 1, 9, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 10, 2, 12, 11, 4, 3, 1, 8, 9, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 8, 7, 6, 5, 3, 2, 14, 13, 12, 4, 11, 1, 10, 0, 9};
		else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 11, 1, 13, 12, 3, 2, 8, 10, 9, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 11, 2, 13, 12, 4, 3, 1, 9, 10, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 11, 9, 13, 12, 3, 2, 1, 8, 10, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 9, 8, 7, 6, 5, 3, 2, 15, 14, 13, 4, 12, 1, 11, 0, 10};
		else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 12, 1, 13, 9, 3, 2, 8, 11, 10, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 6, 5, 4, 12, 1, 14, 13, 3, 2, 9, 11, 10, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 12, 8, 13, 3, 2, 1, 10, 11, 9, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 8, 7, 6, 5, 12, 2, 14, 13, 4, 3, 1, 10, 11, 0, 9};
		else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 12, 10, 13, 3, 2, 1, 8, 9, 11, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 6, 5, 4, 12, 10, 14, 13, 3, 2, 1, 9, 11, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 12, 9, 13, 10, 3, 2, 1, 8, 11, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 10, 9, 8, 7, 6, 5, 3, 2, 16, 15, 14, 4, 13, 1, 12, 0, 11};
		else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 13, 1, 14, 10, 9, 2, 8, 12, 11, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 6, 5, 4, 13, 1, 14, 10, 3, 2, 9, 12, 11, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 10, 8, 14, 13, 2, 1, 12, 9, 11, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 8, 7, 6, 5, 4, 13, 1, 15, 14, 3, 2, 10, 12, 11, 0, 9};
		else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 13, 10, 14, 8, 2, 1, 12, 11, 9, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 6, 5, 4, 13, 9, 14, 3, 2, 1, 11, 12, 10, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 13, 8, 14, 9, 2, 1, 11, 12, 10, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 9, 8, 7, 6, 5, 13, 2, 15, 14, 4, 3, 1, 11, 12, 0, 10};
		else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 13, 11, 14, 9, 2, 1, 8, 12, 10, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 6, 5, 4, 13, 11, 14, 3, 2, 1, 9, 10, 12, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 13, 8, 14, 11, 2, 1, 10, 9, 12, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 8, 7, 6, 5, 4, 13, 11, 15, 14, 3, 2, 1, 10, 12, 0, 9};
		else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 13, 10, 14, 11, 2, 1, 8, 9, 12, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 6, 5, 4, 13, 10, 14, 11, 3, 2, 1, 9, 12, 0, 8};
		else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 11, 6, 5, 4, 13, 9, 14, 10, 3, 2, 1, 8, 12, 0, 7};
		else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 11, 10, 9, 8, 7, 6, 5, 3, 2, 17, 16, 15, 4, 14, 1, 13, 0, 12};
		else {
			printf("Error: Unsupported IMG size for NEAR_OPTIMAL_BOTH_WISE address mapping!");
			return -1;
		}
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

int64_t CAddrGen::GetAddr_NEAR_OPTIMAL_TILE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	if (TILE_SIZE == 3) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 1, 7, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 6, 4, 3, 2, 9, 1, 0, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 4, 3, 2, 1, 9, 0, 7, 6, 8};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 7, 4, 3, 2, 1, 5, 9, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 4, 3, 2, 1, 0, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 4, 3, 2, 1, 5, 0, 9, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 4, 3, 2, 1, 5, 0, 9, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 8, 4, 3, 2, 1, 6, 5, 0, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 3, 2, 1, 0, 5, 9, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 6, 10, 3, 2, 1, 0, 5, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 0, 10, 3, 2, 1, 5, 9, 8, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 7, 10, 3, 2, 1, 6, 0, 5, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 3, 2, 1, 0, 5, 8, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 3, 2, 1, 6, 0, 5, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 3, 2, 1, 8, 0, 5, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 9, 3, 2, 1, 7, 6, 0, 5};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 3, 2, 1, 8, 0, 5, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 3, 2, 1, 0, 6, 5, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 3, 2, 1, 0, 5, 8, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 7, 10, 3, 2, 1, 0, 6, 5, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 0, 10, 3, 2, 1, 5, 9, 8, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 0, 10, 3, 2, 1, 6, 5, 9, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 3, 2, 1, 0, 5, 9, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 3, 2, 1, 7, 5, 0, 6, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 3, 2, 1, 5, 0, 9, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 3, 2, 1, 0, 6, 9, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 3, 2, 1, 0, 9, 8, 6, 7};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 3, 2, 1, 7, 0, 6, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 3, 2, 1, 9, 0, 6, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 3, 2, 1, 9, 0, 6, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 5, 3, 2, 1, 8, 0, 6, 7};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 9, 11, 10, 3, 2, 1, 8, 7, 0, 6};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 7) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 4, 3, 2, 1, 0, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 4, 3, 2, 1, 5, 9, 0, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 3, 2, 1, 0, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 7, 10, 3, 2, 1, 6, 0, 5, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 0, 10, 3, 2, 1, 5, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 0, 10, 3, 2, 1, 6, 5, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 0, 10, 3, 2, 1, 8, 5, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 3, 2, 1, 7, 0, 6, 5, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 0, 10, 3, 2, 1, 8, 5, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 0, 10, 3, 2, 1, 6, 5, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 5, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 7, 6, 9, 5, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 9, 7, 8, 5, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 9, 6, 8, 5, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 11, 10, 2, 1, 0, 8, 7, 5, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 11, 10, 2, 1, 8, 7, 0, 6, 5};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 11, 10, 2, 1, 0, 8, 7, 5, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 9, 6, 8, 5, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 9, 7, 8, 5, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 7, 11, 10, 2, 1, 0, 6, 9, 5, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 5, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 5, 6, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 8, 5, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 8, 11, 10, 2, 1, 0, 7, 5, 6, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 8, 5, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 5, 8, 9, 6, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 5, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 5, 7, 9, 6, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 5, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 5, 11, 10, 2, 1, 0, 9, 8, 6, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 5, 2, 1, 0, 9, 8, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 11, 2, 1, 9, 0, 8, 7, 6};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 11) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 4, 3, 2, 1, 0, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 4, 3, 2, 1, 5, 0, 9, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 3, 2, 1, 0, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 7, 10, 3, 2, 1, 6, 0, 5, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 0, 10, 3, 2, 1, 5, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 0, 10, 3, 2, 1, 6, 5, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 8, 5, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 8, 11, 10, 2, 1, 0, 7, 6, 5, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 8, 5, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 6, 8, 5, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 7, 9, 5, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 7, 6, 9, 5, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 9, 7, 5, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 9, 6, 8, 5, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 9, 8, 7, 5, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 11, 10, 2, 1, 0, 8, 7, 6, 5};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 9, 8, 7, 5, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 9, 6, 8, 5, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 9, 7, 5, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 7, 9, 6, 5, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 9, 7, 5, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 5, 8, 6, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 8, 5, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 8, 11, 10, 2, 1, 0, 7, 5, 6, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 0, 11, 10, 2, 1, 8, 5, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 1, 0, 5, 8, 6, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 1, 0, 5, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 1, 0, 5, 7, 9, 6, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 11, 5, 1, 10, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 11, 5, 1, 10, 9, 8, 6, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 11, 5, 1, 10, 9, 8, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 0, 12, 11, 2, 1, 9, 10, 8, 7, 6};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 16) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 1, 5, 4, 3, 2, 0, 9, 8, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 1, 6, 4, 3, 2, 5, 0, 9, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 10, 3, 2, 1, 0, 5, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 1, 7, 4, 3, 2, 6, 0, 5, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 5, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 10, 3, 2, 1, 0, 6, 5, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 8, 5, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 1, 8, 4, 3, 2, 7, 0, 6, 5, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 8, 5, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 6, 8, 5, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 7, 5, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 7, 6, 9, 5, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 9, 7, 5, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 9, 6, 8, 5, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 9, 8, 7, 5, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 1, 9, 4, 3, 2, 8, 0, 7, 6, 5};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 9, 8, 7, 5, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 9, 6, 8, 5, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 1, 0, 9, 7, 8, 5, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 7, 9, 6, 5, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 1, 0, 9, 7, 5, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 1, 0, 8, 6, 5, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 1, 0, 8, 5, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 2, 1, 0, 8, 7, 5, 6, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 1, 0, 8, 5, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 1, 0, 5, 8, 6, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 1, 0, 5, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 1, 0, 5, 7, 9, 6, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 1, 0, 5, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 5, 12, 11, 1, 0, 10, 9, 8, 6, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 5, 12, 11, 1, 0, 10, 9, 8, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 1, 5, 4, 3, 2, 9, 0, 8, 7, 6};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 32) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 1, 5, 4, 3, 2, 9, 8, 7, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 1, 6, 4, 3, 2, 5, 9, 8, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 1, 10, 5, 3, 2, 9, 7, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 1, 7, 4, 3, 2, 6, 5, 9, 8, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 5, 2, 7, 9, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 1, 10, 6, 3, 2, 5, 8, 9, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 8, 2, 5, 7, 9, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 1, 8, 4, 3, 2, 7, 6, 5, 9, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 8, 2, 7, 5, 9, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 6, 2, 8, 5, 9, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 7, 2, 9, 5, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 7, 2, 6, 9, 5, 8, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 9, 2, 7, 8, 5, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 9, 2, 6, 8, 5, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 9, 2, 8, 7, 5, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 1, 9, 4, 3, 2, 8, 7, 6, 5, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 9, 2, 8, 7, 5, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 9, 2, 8, 6, 5, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 10, 9, 7, 8, 5, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 7, 2, 9, 6, 5, 8, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 10, 7, 9, 5, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 10, 8, 5, 6, 9, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 10, 8, 5, 7, 9, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 1, 11, 10, 8, 2, 7, 5, 6, 9, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 10, 8, 5, 7, 9, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 10, 5, 8, 9, 6, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 10, 5, 9, 7, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 10, 5, 7, 9, 6, 8, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 10, 5, 9, 7, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 10, 5, 9, 8, 6, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 10, 5, 9, 8, 7, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 1, 10, 4, 3, 2, 9, 8, 7, 6, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else {
		printf("Error: Unsupported this tile size!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Near Optimal's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Near Optimal address mapping for Row-Wise access
//---------------------------------------------------
int64_t CAddrGen::GetAddr_NEAR_OPTIMAL_TILE_ROW_WISE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	if (TILE_SIZE == 3) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 5, 4, 3, 2, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 6, 5, 4, 3, 2, 8, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 7, 6, 4, 3, 9, 2, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 6, 4, 3, 2, 9, 8, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 8, 4, 3, 2, 9, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 8, 7, 4, 3, 2, 9, 5, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 4, 3, 2, 7, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 6, 4, 3, 2, 8, 5, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 4, 3, 2, 5, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 7, 4, 3, 2, 9, 5, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 4, 3, 2, 5, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 9, 4, 3, 2, 8, 5, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 4, 3, 2, 5, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 9, 8, 4, 3, 2, 6, 5, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 4, 3, 2, 5, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 9, 4, 3, 2, 8, 5, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 4, 3, 2, 8, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 7, 4, 3, 2, 6, 5, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 8, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 6, 4, 3, 2, 5, 8, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 4, 3, 2, 5, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 8, 4, 3, 2, 6, 5, 9, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 8, 4, 3, 2, 9, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 5, 4, 3, 2, 9, 8, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 8, 6, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 5, 4, 3, 2, 6, 9, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 6, 8, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 6, 8, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 8, 10, 9, 4, 3, 2, 6, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 8, 10, 9, 4, 3, 2, 7, 6, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 7) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 5, 4, 3, 2, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 6, 5, 4, 3, 2, 8, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 7, 6, 4, 3, 2, 9, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 6, 4, 3, 2, 8, 9, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 8, 4, 3, 2, 7, 9, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 8, 7, 4, 3, 2, 5, 9, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 8, 4, 3, 2, 7, 9, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 6, 4, 3, 2, 5, 9, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 9, 4, 3, 2, 5, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 7, 4, 3, 2, 9, 5, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 9, 4, 3, 2, 8, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 9, 4, 3, 2, 8, 5, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 4, 3, 2, 7, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 9, 8, 4, 3, 2, 6, 5, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 4, 3, 2, 7, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 9, 4, 3, 2, 8, 5, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 9, 4, 3, 2, 8, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 7, 4, 3, 2, 6, 5, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 5, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 6, 10, 5, 3, 2, 8, 9, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 8, 3, 2, 7, 9, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 7, 10, 8, 3, 2, 5, 6, 9, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 5, 3, 2, 7, 9, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 5, 3, 2, 9, 6, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 5, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 7, 10, 5, 3, 2, 9, 6, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 5, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 5, 3, 2, 8, 6, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 9, 5, 3, 2, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 8, 10, 9, 4, 3, 2, 7, 6, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 11) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 5, 4, 3, 2, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 6, 5, 4, 3, 8, 2, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 7, 6, 4, 3, 9, 2, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 5, 4, 3, 2, 9, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 6, 4, 3, 2, 8, 9, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 8, 4, 3, 2, 7, 9, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 8, 7, 4, 3, 2, 5, 9, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 8, 4, 3, 2, 7, 9, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 6, 4, 3, 2, 5, 9, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 7, 4, 3, 2, 5, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 7, 4, 3, 2, 9, 5, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 9, 4, 3, 2, 8, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 9, 4, 3, 2, 8, 5, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 4, 3, 2, 7, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 9, 8, 4, 3, 2, 6, 5, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 4, 3, 2, 7, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 9, 4, 3, 2, 8, 5, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 7, 10, 9, 3, 2, 8, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 7, 3, 2, 6, 5, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 7, 10, 9, 3, 2, 5, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 5, 3, 2, 6, 9, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 8, 3, 2, 7, 9, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 7, 10, 8, 3, 2, 5, 6, 9, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 8, 3, 2, 7, 9, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 5, 3, 2, 6, 9, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 5, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 7, 10, 5, 3, 2, 9, 6, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 5, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 5, 3, 2, 8, 6, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 9, 5, 3, 2, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 8, 10, 9, 4, 3, 2, 7, 6, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 16) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 5, 4, 7, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 6, 5, 4, 8, 2, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 7, 6, 5, 4, 9, 2, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 5, 4, 3, 2, 9, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 6, 4, 3, 2, 8, 9, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 8, 4, 3, 2, 7, 9, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 8, 7, 6, 4, 2, 5, 9, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 8, 4, 3, 2, 7, 9, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 6, 4, 3, 2, 5, 9, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 7, 4, 3, 2, 5, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 7, 4, 3, 2, 9, 5, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 9, 4, 3, 2, 8, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 9, 4, 3, 2, 8, 5, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 4, 3, 2, 7, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 7, 4, 6, 2, 5, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 8, 9, 4, 3, 2, 7, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 6, 9, 4, 3, 2, 8, 5, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 7, 10, 9, 3, 2, 8, 5, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 7, 4, 3, 2, 6, 5, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 7, 3, 2, 5, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 6, 10, 8, 3, 2, 5, 9, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 8, 3, 2, 7, 9, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 7, 8, 4, 3, 2, 5, 6, 9, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 8, 3, 2, 7, 9, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 5, 3, 2, 6, 9, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 5, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 7, 10, 5, 3, 2, 9, 6, 8, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 5, 3, 2, 7, 8, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 5, 3, 2, 8, 6, 7, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 9, 5, 3, 2, 7, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 5, 4, 7, 2, 6, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 32) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 5, 4, 7, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 4, 9, 8, 6, 5, 3, 7, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 7, 5, 4, 8, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 4, 9, 7, 6, 5, 3, 8, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 7, 5, 4, 8, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 8, 6, 5, 4, 9, 7, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 8, 7, 5, 4, 9, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 8, 7, 6, 4, 5, 9, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 8, 7, 5, 4, 9, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 8, 6, 5, 4, 9, 7, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 7, 4, 5, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 7, 6, 4, 5, 8, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 7, 4, 5, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 6, 4, 5, 7, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 7, 4, 2, 6, 5, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 7, 4, 6, 2, 5, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 7, 4, 2, 6, 5, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 6, 4, 5, 7, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 7, 4, 5, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 7, 6, 4, 5, 8, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 7, 5, 4, 8, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 8, 6, 5, 4, 9, 7, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 8, 7, 5, 4, 9, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 8, 7, 5, 4, 6, 9, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 8, 7, 5, 4, 9, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 5, 4, 6, 7, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 7, 5, 4, 8, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 7, 5, 4, 6, 8, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 7, 5, 4, 8, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 5, 4, 6, 7, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 3, 9, 8, 5, 4, 7, 2, 6, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 3, 10, 9, 8, 4, 7, 2, 6, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else {
		printf("Error: Unsupported this tile size!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Near Optimal's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Near Optimal address mapping for Row-Wise access
//---------------------------------------------------
int64_t CAddrGen::GetAddr_NEAR_OPTIMAL_TILE_COL_WISE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	if (TILE_SIZE == 3) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 3, 2, 1, 9, 0, 8, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 6, 4, 5, 11, 10, 3, 2, 1, 0, 9, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 5, 2, 1, 0, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 4, 3, 6, 12, 11, 2, 1, 5, 10, 0, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 5, 2, 1, 0, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 5, 12, 11, 6, 1, 0, 10, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 11, 5, 1, 8, 10, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 4, 3, 6, 12, 7, 2, 1, 11, 5, 0, 10, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 11, 5, 1, 8, 10, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 11, 6, 1, 5, 10, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 11, 12, 5, 1, 0, 10, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 7, 6, 1, 5, 11, 10, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 11, 12, 5, 1, 0, 9, 10, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 6, 5, 1, 9, 11, 8, 10, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 9, 5, 1, 8, 11, 7, 10, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 9, 4, 3, 2, 7, 13, 12, 8, 1, 6, 0, 5, 11, 10};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 9, 5, 1, 8, 11, 7, 10, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 6, 5, 1, 9, 11, 8, 10, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 11, 12, 5, 1, 0, 9, 10, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 7, 6, 1, 5, 11, 10, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 10, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 6, 5, 0, 10, 11, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 8, 13, 12, 5, 0, 10, 11, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 8, 4, 3, 2, 0, 13, 7, 5, 1, 6, 12, 10, 11, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 8, 13, 12, 5, 0, 10, 11, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 6, 5, 0, 10, 11, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 10, 5, 0, 11, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 10, 13, 7, 5, 0, 6, 12, 9, 11, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 5, 4, 3, 2, 12, 13, 10, 1, 0, 9, 7, 11, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 5, 4, 3, 2, 0, 13, 10, 6, 1, 9, 12, 8, 11, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 5, 4, 3, 2, 0, 13, 10, 9, 1, 12, 8, 7, 11, 6};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 10, 5, 4, 3, 2, 8, 14, 13, 9, 1, 7, 0, 6, 12, 11};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 7) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 5, 11, 10, 2, 1, 0, 9, 8, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 5, 11, 6, 2, 1, 10, 0, 9, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 11, 5, 1, 10, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 6, 12, 11, 7, 1, 5, 0, 10, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 11, 12, 5, 1, 0, 10, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 5, 12, 6, 1, 0, 11, 10, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 5, 12, 11, 1, 0, 8, 10, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 8, 4, 3, 2, 6, 13, 12, 7, 1, 0, 11, 5, 10, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 11, 12, 5, 1, 0, 8, 10, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 5, 13, 12, 6, 0, 11, 10, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 10, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 6, 13, 12, 7, 0, 5, 11, 10, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 9, 7, 10, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 9, 13, 12, 6, 0, 5, 11, 8, 10, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 5, 13, 12, 9, 0, 8, 11, 7, 10, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 9, 4, 3, 2, 0, 13, 8, 7, 1, 12, 6, 5, 11, 10};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 5, 13, 12, 9, 0, 8, 11, 7, 10, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 9, 13, 12, 6, 0, 5, 11, 8, 10, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 9, 13, 12, 5, 0, 11, 7, 10, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 7, 6, 0, 5, 11, 9, 10, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 10, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 6, 5, 0, 10, 11, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 8, 13, 12, 5, 0, 10, 11, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 5, 13, 8, 7, 0, 6, 12, 10, 11, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 8, 13, 12, 5, 0, 10, 11, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 6, 5, 0, 10, 8, 11, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 10, 5, 0, 7, 11, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 10, 13, 7, 5, 0, 6, 12, 9, 11, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 10, 5, 0, 9, 7, 11, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 6, 14, 13, 10, 0, 9, 12, 8, 11, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 9, 14, 13, 10, 0, 8, 12, 7, 11, 6};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 10, 9, 8, 13, 7, 6, 12, 11};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 11) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 5, 11, 10, 2, 1, 0, 9, 8, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 5, 12, 11, 6, 1, 0, 10, 9, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 11, 5, 1, 10, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 0, 12, 7, 6, 1, 11, 5, 10, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 5, 12, 11, 1, 0, 10, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 5, 13, 12, 6, 0, 11, 10, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 8, 10, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 8, 7, 6, 11, 5, 10, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 8, 10, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 5, 13, 12, 6, 0, 11, 10, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 10, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 6, 13, 12, 7, 0, 5, 11, 9, 10, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 9, 7, 10, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 9, 13, 12, 6, 0, 5, 11, 8, 10, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 8, 13, 12, 9, 0, 5, 11, 7, 10, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 9, 4, 3, 2, 1, 7, 14, 13, 8, 0, 12, 6, 5, 11, 10};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 8, 13, 12, 9, 0, 5, 11, 7, 10, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 9, 13, 12, 6, 0, 5, 11, 8, 10, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 9, 13, 12, 5, 0, 11, 7, 10, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 7, 6, 0, 5, 11, 9, 10, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 10, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 6, 5, 0, 10, 11, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 8, 14, 13, 12, 5, 10, 11, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 8, 4, 3, 2, 1, 5, 14, 13, 7, 0, 6, 12, 10, 11, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 8, 14, 13, 12, 5, 10, 11, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 12, 14, 13, 6, 5, 10, 8, 11, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 12, 14, 13, 10, 5, 7, 11, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 10, 14, 13, 7, 5, 6, 12, 9, 11, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 12, 14, 13, 10, 5, 9, 7, 11, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 14, 13, 10, 5, 6, 12, 8, 11, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 9, 14, 13, 10, 0, 8, 12, 7, 11, 6};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 8, 14, 10, 9, 0, 13, 7, 6, 12, 11};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 16) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 1, 12, 11, 10, 2, 0, 9, 8, 7, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 4, 3, 1, 12, 11, 5, 2, 0, 10, 9, 8, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 11, 12, 5, 1, 0, 10, 9, 7, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 4, 3, 2, 0, 13, 12, 6, 1, 11, 5, 10, 9, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 10, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 5, 13, 12, 6, 0, 11, 10, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 8, 10, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 8, 4, 3, 2, 0, 13, 12, 7, 1, 6, 11, 5, 10, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 8, 10, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 5, 13, 12, 6, 0, 11, 10, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 10, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 7, 6, 0, 5, 11, 9, 10, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 5, 0, 9, 7, 10, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 9, 13, 12, 6, 0, 5, 11, 8, 10, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 8, 13, 12, 9, 0, 5, 11, 7, 10, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 9, 4, 3, 2, 1, 0, 14, 13, 8, 7, 12, 6, 11, 5, 10};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 8, 14, 13, 12, 9, 5, 11, 7, 10, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 14, 13, 12, 6, 5, 11, 8, 10, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 14, 13, 12, 5, 11, 7, 10, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 12, 14, 13, 7, 6, 5, 11, 9, 10, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 11, 14, 13, 12, 5, 10, 7, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 12, 14, 13, 6, 5, 10, 11, 8, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 8, 14, 13, 12, 5, 10, 11, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 8, 4, 3, 2, 1, 5, 14, 13, 7, 0, 6, 12, 10, 11, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 8, 14, 13, 12, 5, 10, 11, 7, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 12, 14, 13, 6, 5, 10, 8, 11, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 12, 14, 13, 10, 5, 7, 11, 9, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 10, 14, 13, 7, 5, 6, 12, 9, 11, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 12, 14, 13, 10, 5, 9, 7, 11, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 14, 13, 10, 5, 6, 12, 8, 11, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 13, 15, 14, 10, 9, 8, 12, 7, 11, 6};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 10, 5, 4, 3, 2, 1, 0, 15, 14, 9, 8, 7, 13, 6, 12, 11};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 32) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 10, 5, 9, 8, 7, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 1, 12, 11, 6, 5, 10, 9, 8, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 10, 13, 12, 11, 5, 9, 7, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 4, 3, 2, 1, 13, 12, 6, 5, 11, 10, 9, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 10, 13, 12, 11, 5, 7, 9, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 6, 5, 10, 8, 9, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 8, 13, 12, 11, 5, 10, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 8, 4, 3, 2, 1, 6, 14, 13, 12, 7, 5, 11, 10, 0, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 8, 5, 10, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 6, 5, 8, 10, 9, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 7, 13, 12, 11, 5, 10, 9, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 4, 3, 2, 1, 12, 14, 13, 6, 5, 11, 9, 10, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 9, 5, 7, 10, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 9, 13, 12, 6, 5, 11, 8, 10, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 8, 13, 12, 9, 5, 11, 7, 10, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 9, 4, 3, 2, 1, 6, 14, 13, 8, 7, 12, 5, 11, 10, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 9, 8, 5, 11, 7, 10, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 12, 13, 9, 6, 5, 11, 8, 10, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 9, 5, 7, 10, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 4, 3, 2, 1, 12, 14, 13, 6, 5, 9, 11, 10, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 11, 13, 12, 7, 5, 10, 9, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 8, 13, 12, 6, 5, 10, 11, 9, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 10, 13, 12, 8, 5, 7, 11, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 8, 4, 3, 2, 1, 13, 14, 7, 6, 5, 12, 10, 11, 0, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 10, 13, 12, 8, 5, 7, 11, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 12, 14, 13, 10, 6, 8, 11, 9, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 7, 14, 13, 12, 10, 9, 11, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 10, 14, 13, 7, 6, 12, 9, 11, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 12, 14, 13, 10, 9, 7, 11, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 9, 14, 13, 10, 6, 12, 8, 11, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 8, 14, 13, 10, 9, 12, 7, 11, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 10, 5, 4, 3, 2, 1, 7, 15, 14, 9, 8, 13, 6, 12, 11, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else {
		printf("Error: Unsupported this tile size!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Near Optimal's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Near Optimal address mapping for Row-Wise access
//---------------------------------------------------
int64_t CAddrGen::GetAddr_NEAR_OPTIMAL_TILE_ROW_COL_WISE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	if (TILE_SIZE == 3) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 8, 1, 7, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 4, 3, 2, 9, 1, 8, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 5, 3, 2, 1, 7, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 7, 4, 5, 11, 10, 6, 3, 2, 1, 9, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 10, 5, 3, 2, 1, 7, 9, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 6, 4, 10, 11, 5, 3, 2, 1, 8, 9, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 8, 11, 10, 5, 2, 1, 7, 9, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 7, 4, 6, 12, 11, 3, 2, 5, 10, 1, 0, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 8, 5, 2, 1, 7, 9, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 6, 5, 2, 1, 8, 9, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 5, 2, 1, 7, 9, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 4, 3, 11, 12, 6, 5, 2, 1, 10, 9, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 11, 5, 2, 1, 10, 7, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 11, 6, 5, 2, 1, 8, 10, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 11, 9, 8, 5, 2, 1, 7, 10, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 8, 4, 6, 12, 7, 3, 2, 5, 11, 1, 10, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 4, 3, 11, 12, 8, 5, 2, 1, 7, 10, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 11, 9, 6, 5, 2, 1, 8, 10, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 11, 5, 2, 1, 10, 7, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 4, 3, 11, 12, 6, 5, 2, 1, 10, 9, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 5, 1, 7, 9, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 6, 5, 1, 11, 8, 9, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 8, 5, 1, 11, 7, 9, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 4, 3, 12, 7, 6, 5, 2, 10, 1, 11, 0, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 8, 2, 1, 11, 7, 9, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 6, 2, 1, 11, 8, 9, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 10, 2, 1, 7, 9, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 12, 10, 7, 6, 2, 1, 9, 11, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 9, 12, 10, 2, 1, 7, 11, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 6, 2, 1, 8, 11, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 8, 13, 12, 9, 2, 1, 7, 11, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 9, 5, 4, 7, 13, 8, 3, 2, 6, 12, 1, 11, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 7) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 8, 1, 7, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 4, 3, 2, 9, 1, 8, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 10, 5, 3, 2, 1, 9, 7, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 7, 4, 5, 11, 10, 6, 3, 2, 1, 9, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 5, 2, 1, 7, 9, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 6, 5, 2, 1, 8, 9, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 8, 11, 10, 5, 2, 1, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 7, 4, 6, 12, 11, 3, 2, 5, 10, 1, 0, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 8, 11, 5, 2, 1, 10, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 4, 3, 11, 12, 5, 2, 1, 10, 8, 9, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 5, 2, 1, 7, 9, 8, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 4, 3, 5, 12, 11, 6, 2, 1, 10, 9, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 11, 5, 2, 1, 7, 10, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 11, 6, 5, 2, 1, 8, 10, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 11, 9, 8, 5, 2, 1, 7, 10, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 8, 4, 6, 12, 7, 3, 2, 5, 11, 1, 10, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 4, 3, 11, 12, 8, 5, 2, 1, 7, 10, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 5, 12, 9, 6, 1, 11, 8, 10, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 11, 12, 9, 5, 1, 7, 10, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 4, 3, 5, 12, 6, 2, 1, 11, 9, 10, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 5, 1, 7, 9, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 6, 5, 1, 11, 8, 9, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 8, 5, 1, 11, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 8, 7, 4, 3, 6, 13, 12, 5, 2, 1, 10, 11, 0, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 8, 2, 1, 11, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 6, 2, 1, 8, 11, 0, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 10, 2, 1, 11, 9, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 12, 10, 7, 6, 2, 1, 9, 11, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 9, 12, 10, 2, 1, 7, 11, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 6, 2, 1, 8, 11, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 8, 2, 1, 7, 11, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 9, 5, 4, 7, 13, 8, 3, 2, 6, 12, 1, 11, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 11) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 5, 4, 3, 2, 8, 1, 7, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 5, 10, 6, 3, 2, 9, 1, 8, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 10, 5, 3, 2, 1, 9, 7, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 7, 4, 5, 11, 10, 6, 3, 2, 1, 9, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 5, 2, 1, 7, 9, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 6, 4, 10, 11, 5, 3, 2, 1, 8, 0, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 8, 11, 10, 5, 2, 1, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 7, 4, 6, 12, 11, 3, 2, 5, 10, 1, 0, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 8, 11, 5, 2, 1, 10, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 11, 6, 5, 2, 1, 10, 8, 0, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 5, 2, 1, 7, 9, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 4, 3, 5, 12, 11, 6, 2, 1, 9, 10, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 11, 5, 2, 1, 7, 10, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 4, 3, 9, 12, 11, 5, 2, 1, 8, 10, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 11, 9, 8, 5, 2, 1, 7, 10, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 8, 4, 6, 12, 7, 3, 2, 5, 11, 1, 10, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 4, 3, 5, 12, 8, 2, 1, 11, 7, 10, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 5, 12, 9, 6, 1, 11, 8, 10, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 11, 12, 9, 5, 1, 7, 10, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 4, 3, 5, 12, 6, 2, 1, 11, 9, 10, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 5, 1, 7, 9, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 6, 5, 1, 11, 8, 0, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 8, 5, 1, 11, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 8, 7, 4, 3, 6, 13, 12, 5, 2, 1, 10, 11, 0, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 8, 2, 1, 11, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 6, 2, 1, 8, 11, 0, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 10, 2, 1, 11, 9, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 5, 4, 3, 12, 13, 10, 6, 2, 1, 9, 11, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 9, 12, 10, 2, 1, 7, 11, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 6, 2, 1, 8, 11, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 8, 2, 1, 7, 11, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 9, 5, 4, 7, 13, 8, 3, 2, 6, 12, 1, 11, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 16) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 2, 10, 9, 4, 3, 8, 1, 7, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 2, 10, 5, 4, 3, 9, 1, 8, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 10, 11, 3, 2, 1, 9, 7, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 7, 4, 3, 11, 10, 6, 5, 2, 1, 9, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 5, 2, 1, 7, 9, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 6, 4, 10, 11, 5, 3, 2, 1, 8, 0, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 8, 11, 10, 5, 2, 1, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 7, 4, 2, 12, 11, 6, 3, 5, 10, 1, 0, 9};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 8, 11, 5, 2, 1, 10, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 4, 3, 11, 12, 5, 2, 1, 10, 8, 0, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 5, 2, 1, 7, 9, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 4, 3, 11, 12, 6, 5, 2, 1, 9, 10, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 9, 11, 5, 2, 1, 7, 10, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 4, 3, 9, 12, 11, 5, 2, 1, 8, 10, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 4, 3, 11, 12, 8, 5, 2, 1, 7, 10, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 8, 4, 2, 12, 7, 6, 3, 11, 5, 1, 10, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 4, 3, 5, 12, 8, 2, 1, 11, 7, 10, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 5, 12, 9, 6, 1, 11, 8, 10, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 11, 12, 9, 5, 1, 7, 10, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 4, 3, 5, 12, 6, 2, 1, 11, 9, 10, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 11, 5, 1, 7, 9, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 6, 5, 1, 11, 8, 0, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 10, 12, 8, 5, 1, 11, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 8, 7, 4, 3, 12, 13, 6, 5, 2, 1, 10, 11, 0, 9};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 8, 2, 1, 11, 7, 0, 9, 6};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 6, 2, 1, 8, 11, 0, 9, 7};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 10, 2, 1, 11, 9, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 5, 4, 3, 12, 13, 10, 6, 2, 1, 9, 11, 0, 8};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 9, 12, 10, 2, 1, 7, 11, 0, 8, 6};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 6, 2, 1, 8, 11, 0, 7};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 8, 2, 1, 7, 11, 0, 6};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 9, 5, 4, 2, 13, 8, 7, 3, 12, 6, 1, 11, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 32) {
		if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 2, 10, 9, 4, 3, 8, 1, 7, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 6, 5, 2, 11, 10, 4, 3, 9, 1, 8, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 9, 11, 10, 3, 2, 1, 7, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 7, 6, 3, 11, 10, 5, 4, 2, 1, 9, 8, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 10, 11, 3, 2, 1, 7, 9, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 6, 4, 10, 11, 5, 3, 2, 1, 8, 9, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 8, 5, 2, 1, 7, 9, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 7, 4, 3, 12, 11, 6, 5, 2, 10, 1, 9, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 11, 8, 5, 2, 1, 10, 7, 9, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 4, 3, 11, 12, 5, 2, 1, 8, 10, 9, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 7, 11, 5, 2, 1, 10, 9, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 6, 4, 11, 12, 5, 3, 2, 1, 9, 10, 8, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 11, 9, 5, 2, 1, 7, 10, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 4, 3, 11, 12, 9, 5, 2, 1, 8, 10, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 4, 3, 11, 12, 8, 5, 2, 1, 7, 10, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 9, 8, 7, 4, 2, 13, 12, 6, 3, 5, 11, 1, 10, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 4, 3, 12, 8, 5, 2, 1, 11, 7, 10, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 4, 3, 12, 9, 5, 2, 1, 11, 8, 10, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 9, 2, 1, 7, 10, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 4, 3, 12, 6, 5, 2, 1, 9, 11, 10, 8, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 7, 2, 1, 10, 9, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 4, 3, 8, 12, 5, 2, 1, 10, 11, 9, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 8, 2, 1, 7, 11, 9, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 8, 7, 4, 3, 12, 13, 6, 5, 2, 1, 10, 11, 9, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 8, 2, 1, 7, 11, 9, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 12, 10, 6, 2, 1, 8, 11, 9, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 10, 2, 1, 9, 11, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 5, 4, 3, 12, 13, 10, 6, 2, 1, 9, 11, 8, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 2, 1, 7, 11, 8, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 6, 2, 1, 8, 11, 7, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 8, 2, 1, 7, 11, 6, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 10, 9, 8, 5, 4, 2, 14, 13, 7, 3, 6, 12, 1, 11, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_TILE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else {
		printf("Error: Unsupported this tile size!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
	return (nAddr_new);
}

//---------------------------------------------------
// Get Address (Near Optimal's address map)
//	Bank interleaved address map
//	1. Get LIAM address
//	2. Do the Near Optimal address mapping for Row-Wise access
//---------------------------------------------------
int64_t CAddrGen::GetAddr_NEAR_OPTIMAL_STRIDE() {
	std::array<uint8_t, 32> bit_order;

	// Check AddrGen finished
	if (this->eFinalTrans == ERESULT_TYPE_YES) {
		return (-1);
	};

	// Generate LIAM address	
	int64_t nAddr = this->GetAddr_LIAM();

	if (TILE_SIZE == 3) {
		if (BANK_NUM == 16) {
			if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 48) {
			if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 64) {
			if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 7) {
		if (BANK_NUM == 16) {
			if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 48) {
			if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 64) {
			if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 11) {
		if (BANK_NUM == 16) {
			if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 48) {
			if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 64) {
			if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 16) {
		if (BANK_NUM == 16) {
			if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 48) {
			if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 64) {
			if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else if (TILE_SIZE == 32) {
		if (BANK_NUM == 16) {
			if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 32) {
			if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 48) {
			if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 5, 9, 8, 7, 6, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else if (BANK_NUM == 64) {
			if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 6, 5, 10, 9, 8, 7, 4, 3, 2, 1, 0};
			else {
				printf("Error: Unsupported IMG size for NEAR_OPTIMAL_STRIDE address mapping!");
				return -1;
			}
		} else {
			printf("Error: Unsupported this number of banks!");
			assert(0);
		}
	}
	else {
		printf("Error: Unsupported this tile size!");
		assert(0);
	}

	// Apply Near Optimal address mapping
	int64_t nAddr_new = remap_bits_32(bit_order, nAddr);
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
		if (this->nApos + this->nNumPixelTrans == ImgH) {
			this->nApos = 0;
			this->nBpos ++;
		}
		else {
			this->nApos = this->nApos + TILEH;
		};
		#endif

	}
	else 	if (this->cOperation == "TILE") {
	
		// Set block size fixed
		this->nAsizeT = TILEH;
		this->nBsizeT = 1;

		// Update coordinate temp
		if ((this->nApos + this->nNumPixelTrans >= ImgH)            && 
		    (this->nApos + this->nNumPixelTrans >= (TILE_SIZE - 1)) &&
			(this->nBpos - this->TileVBase  == (TILE_SIZE - 1))
		) {
			this->TileHBase = 0;
			this->TileVBase += TILE_SIZE;
			this->nApos = this->TileHBase;
			this->nBpos = this->TileVBase;
		}
		if (((this->nApos - this->TileHBase) + this->nNumPixelTrans < ImgH)              &&
		    ((this->nApos - this->TileHBase) + this->nNumPixelTrans >= (TILE_SIZE - 1))  &&
			(this->nBpos - this->TileVBase  == (TILE_SIZE - 1))                           
		) {
			this->TileHBase += TILE_SIZE;
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

	if (this->cAddrMap == "LIAM"  			or
		this->cAddrMap == "BFAM"  			or
		this->cAddrMap == "BGFAM" 			or
		this->cAddrMap == "CIAM"  			or
		this->cAddrMap == "OIRAM" 			or
		this->cAddrMap == "MIN_K_UNION_ROW_WISE" 	or
		this->cAddrMap == "MIN_K_UNION_COL_WISE" 	or
		this->cAddrMap == "MIN_K_UNION_BOTH_WISE" 	or
		this->cAddrMap == "MIN_K_UNION_TILE_ONLY" 	or
		this->cAddrMap == "MIN_K_UNION_TILE_ROW_WISE" 	or
		this->cAddrMap == "MIN_K_UNION_TILE_COL_WISE" 	or
		this->cAddrMap == "MIN_K_UNION_TILE_ROW_COL_WISE" 	or
		this->cAddrMap == "MIN_K_UNION_STRIDE" 	or
		this->cAddrMap == "FLATFISH" 	or
		this->cAddrMap == "NEAR_OPTIMAL_ROW_WISE" or
		this->cAddrMap == "NEAR_OPTIMAL_COL_WISE" or
		this->cAddrMap == "NEAR_OPTIMAL_BOTH_WISE" or
		this->cAddrMap == "NEAR_OPTIMAL_TILE_ONLY" 	or
		this->cAddrMap == "NEAR_OPTIMAL_TILE_ROW_WISE" 	or
		this->cAddrMap == "NEAR_OPTIMAL_TILE_COL_WISE" 	or
		this->cAddrMap == "NEAR_OPTIMAL_TILE_ROW_COL_WISE" 	or
		this->cAddrMap == "NEAR_OPTIMAL_STRIDE"
	) {

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

