///-------------------------------------------------------------
// Filename	: Top.h
// Version	: 0.83a
// Date		: March  2023
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Global header
//-------------------------------------------------------------
// History
// ----2023.03.15: Duong support ARMv8
#ifndef TOP_H
#define TOP_H

#include <string>
#include <math.h>
#include <stdint.h>

//-------------------------------------------------------------
// SIM cycle
//-------------------------------------------------------------
//:#define SIM_CYCLE 			100 
// #define SIM_CYCLE 			99999999
// #define SIM_CYCLE	 			1000000000
// #define SIM_CYCLE 			1999999999
// #define SIM_CYCLE 				5999999999
#define SIM_CYCLE 			9999999999999


//-------------------------------------------------------------
// Terminate sim 
//-------------------------------------------------------------
#define TERMINATE_BY_CYCLE 


//-------------------------------------------------------------
// DEBUG 
//-------------------------------------------------------------
// #define DEBUG
// #define DEBUG_Cache
// #define DEBUG_MMU 
// #define DEBUG_MST 
// #define DEBUG_SLV 
// #define DEBUG_BUS 
// #define DEBUG_MIU 
// #define DEBUG_BUDDY 
 #define STAT_DETAIL 


// ------------------------------------------------------------
// RISC-V NAPOT (Naturally Aligned Power-Of-Two)
// ------------------------------------------------------------
#ifdef NAPOT
	#define NAPOT_BITS 4	// 64 KiB contiguous region, page size 4 KiB
#endif

// PBS
#define GRANULARITY 9	// ARMv8: 9, ARMv7: 4

// cRCPT
#define MAX_MINI_ZONE 5
#define MAX_NUMBER_PAGE_IN_MINI_ZONE 65536

//------------------------------------------------------------
// Background traffic 
// 	In following cases, transactions can be in buffers when simulation finishes. This is intentional.
// 	1. Background transactions 
// 	2. Cache evicts 
// 	3. Range PTWs in RMM 
//-------------------------------------------------------------
#define BACKGROUND_TRAFFIC_ON


//-------------------------------------------------------------
// DMA bckground traffic enable
//-------------------------------------------------------------
// #define DMA_ENABLE								// MST background DMA traffic gen	FIXME 	


//-------------------------------------------------------------
// File out 
//-------------------------------------------------------------
// #define FILE_OUT 


//-------------------------------------------------------------
// Max MO count
//-------------------------------------------------------------
#ifndef MAX_MO_COUNT
	#define MAX_MO_COUNT			16
#endif


//-------------------------------------------------------------
// Starvation  
//-------------------------------------------------------------
// #define STARVATION_CYCLE		10000					// Check Q, Tracker entry startvation. If exceeded, terminate sim
#define STARVATION_CYCLE		50000					// Check Q, Tracker entry startvation. If exceeded, terminate sim


//-------------------------------------------------------------
// AxID 
//-------------------------------------------------------------
#define ARID				4 
#define AWID				3 
#define ARID_RANDOM
// #define AWID_RANDOM


//-------------------------------------------------------------
// Ax address range 
//-------------------------------------------------------------
#define MIN_ADDR			0 
// #define MAX_ADDR			0x7FFFFFFF 				// Arch32
#define MAX_ADDR			0x7FFFFFFFFFFFFFFF 			// Arch64


//-------------------------------------------------------------
// Max. Burst length (Length of a transaction)
// 	Actual burst length less than or equal to 4
//-------------------------------------------------------------
#define MAX_BURST_LENGTH		8					// MAX_TRANS_SIZE / BURST_SIZE 
// #define AR_BURST_LEN_RANDOM


//-------------------------------------------------------------
// Burst size (bytes data width)
//-------------------------------------------------------------
#ifndef BURST_SIZE
	#define BURST_SIZE          8                  // bytes
	// #define BURST_SIZE          1                  // bytes
#endif


//-------------------------------------------------------------
// Max transaction size (bytes) 
//-------------------------------------------------------------
#define MAX_TRANS_SIZE			(MAX_BURST_LENGTH * BURST_SIZE)		// 64 bytes when MAX_BURST_LENGTH 4 and BURST_SIZE 16 bytes	


//-------------------------------------------------------------
// Use W channel 
//-------------------------------------------------------------
// #define USE_W_CHANNEL


//-------------------------------------------------------------
// Cache interleave
//-------------------------------------------------------------
#define CACHE_NUM	1		// Number of caches 
#define CACHE_WIDTH		((int)(ceilf(log2f(CACHE_NUM))))        // 2  bits for MEM_CHANNEL_NUM 4


//-------------------------------------------------------------
// (Cache) ON or OFF 
//-------------------------------------------------------------
// #define Cache_OFF
// #define Cache_ON


//-------------------------------------------------------------
// (Cache) Demension 2D array
//-------------------------------------------------------------
//#define CACHE_NUM_WAY			16
//#define CACHE_NUM_SET			32

// CacheL3 4 MB
#define CACHE_NUM_WAY			16
#define CACHE_NUM_SET			4096

// #define CACHE_NUM_WAY		1	// FIXME
// #define CACHE_NUM_SET		8192	// FIXME	

//-------------------------------------------------------------
// (Cache) Number of set bits
//-------------------------------------------------------------
#define CACHE_BIT_SET			((int)(ceilf(log2f(CACHE_NUM_SET))))


//-------------------------------------------------------------
// (Cache) Cache line 64 byte
//-------------------------------------------------------------
#define CACHE_BIT_LINE			6


//-------------------------------------------------------------
// (Cache) Number of tag bits
//-------------------------------------------------------------
// #define CACHE_BIT_TAG		(32 - CACHE_BIT_SET - CACHE_BIT_LINE)   // Arch32
#define CACHE_BIT_TAG			(64 - CACHE_BIT_SET - CACHE_BIT_LINE)   // Arch64


//-------------------------------------------------------------
// (MMU) ON or OFF 
//-------------------------------------------------------------
#define MMU_OFF
// #define MMU_ON

//-------------------------------------------------------------
// (MMU) Page size
//-------------------------------------------------------------
#define BIT_4K_PAGE			12				// Bits. 4K = 2^12
#define BIT_1M_PAGE			20				// Bits. 1M = 2^20
#define BIT_2MB_PAGE		21				// Duong add 3 new huge page 2MB, 1GB, and 512GB
#define BIT_1GB_PAGE		30
#define BIT_512GB_PAGE		39


//-------------------------------------------------------------
// (MMU) Page table walk
//-------------------------------------------------------------
// #define SINGLE_FETCH    
// #define BLOCK_FETCH								// Full    1/1. 64 bytes (= 16 PTEs). Burst length 4.   
// #define HALF_FETCH								// Half    1/2. 32 bytes (= 8  PTEs). Burst length 2.
// #define QUARTER_FETCH							// Quarter 1/4. 16 bytes (= 4  PTEs). Burst length 1.


//-------------------------------------------------------------
// (MMU) TLB demension 2D array
//-------------------------------------------------------------
#define MMU_NUM_WAY			32					// Number of TLB entries (power-of-2) for fully associative
#define MMU_NUM_SET			1					// 1 for fully associative


//-------------------------------------------------------------
// (MMU) Number of PTEs a PTW obtain 
//-------------------------------------------------------------
#if defined SINGLE_FETCH 
	#define NUM_PTE_PTW		1
#elif defined BLOCK_FETCH 
	#define NUM_PTE_PTW		(MAX_TRANS_SIZE / PTW_SIZE)					// Power-of-2. Fully associative. Assume 16. FIXME
#elif defined HALF_FETCH 
	#define NUM_PTE_PTW		(MAX_TRANS_SIZE / PTW_SIZE / 2)
#elif defined QUARTER_FETCH 
	#define NUM_PTE_PTW		(MAX_TRANS_SIZE / PTW_SIZE / 4)
#endif

//-------------------------------------------------------------
// (MMU) Buddy page-table 
//-------------------------------------------------------------
// Why I have to comment out this to make it work????
// #define BUDDY_ENABLE	   // DUONGTRAN uncommnent

// #define PTW_SIZE  8			// Arch64

// #define GROUP_SIZE			NUM_PTE_PTW				// Number of PTEs ared group (CAMB)


//-------------------------------------------------------------
// (MMU) DBPF (Double Buffer Prefetch)
//-------------------------------------------------------------
// #define DBPF_ENABLE
// #define DBPF_DISABLE


//-------------------------------------------------------------
// (MMU) RMM 
//-------------------------------------------------------------
// #define RMM_ENABLE
// #define RMM_DISABLE


//-------------------------------------------------------------
// (MMU) AT 
//-------------------------------------------------------------
// #define AT_ENABLE
// #define AT_DISABLE


//-------------------------------------------------------------
// (MMU) BCT 
//-------------------------------------------------------------
// #define BCT_ENABLE
// #define BCT_DISABLE


//-------------------------------------------------------------
// (MMU) CAMB 
//-------------------------------------------------------------
// #define CAMB_ENABLE
// #define CAMB_DISABLE


//-------------------------------------------------------------
// (Bus) Number of SI ports
//-------------------------------------------------------------
// #define MAX_BUS_NUM_PORT		6

#define BUS_LATENCY 1

//-------------------------------------------------------------
// (Bus) Arbiter
//-------------------------------------------------------------
#define BUS_ROUND_ROBIN
// #define BUS_FIXED_PRIORITY


//-------------------------------------------------------------
// (Master) Ax Address gen
//-------------------------------------------------------------
#define AR_ADDR_GEN_TEST
#define AW_ADDR_GEN_TEST


//-------------------------------------------------------------
// (Master) Trace Ax Address gen FIXME
//-------------------------------------------------------------
#define AR_ADDR_GEN_TRACE
#define AW_ADDR_GEN_TRACE
#define Ax_ADDR_GEN_TRACE


//-------------------------------------------------------------
// (Master) Address increment
//-------------------------------------------------------------
#define AR_ADDR_INC			0x40

#define AW_ADDR_INC			0x40


//-------------------------------------------------------------
// (Master) Ramdom Address gen
//-------------------------------------------------------------
#define AR_ADDR_RANDOM
#define AW_ADDR_RANDOM


//-------------------------------------------------------------
// MATRIX OPERATION
//-------------------------------------------------------------
#define ELEMENT_SIZE 4 // bytes
#ifndef TILE_SIZE
	#define TILE_SIZE 4
#endif
		      
#ifndef IMG_VERTICAL_SIZE_B
        #define IMG_VERTICAL_SIZE_B IMG_VERTICAL_SIZE
#endif

#ifndef IMG_HORIZONTAL_SIZE_B
        #define IMG_HORIZONTAL_SIZE_B IMG_HORIZONTAL_SIZE
#endif

#ifndef IMG_VERTICAL_SIZE_C
        #define IMG_VERTICAL_SIZE_C IMG_VERTICAL_SIZE
#endif

#ifndef IMG_HORIZONTAL_SIZE_C
        #define IMG_HORIZONTAL_SIZE_C IMG_HORIZONTAL_SIZE
#endif

#ifndef IMG_VERTICAL_SIZE_D
        #define IMG_VERTICAL_SIZE_D IMG_VERTICAL_SIZE
#endif

#ifndef IMG_HORIZONTAL_SIZE_D
        #define IMG_HORIZONTAL_SIZE_D IMG_HORIZONTAL_SIZE
#endif

#ifndef IMG_VERTICAL_SIZE_E
        #define IMG_VERTICAL_SIZE_E IMG_VERTICAL_SIZE
#endif

#ifndef IMG_HORIZONTAL_SIZE_E
        #define IMG_HORIZONTAL_SIZE_E IMG_HORIZONTAL_SIZE
#endif


#define MATRIX_A_ROW    IMG_VERTICAL_SIZE 
#define MATRIX_B_ROW    IMG_VERTICAL_SIZE_B
#define MATRIX_C_ROW    IMG_VERTICAL_SIZE_C
#define MATRIX_D_ROW    IMG_VERTICAL_SIZE_D
#define MATRIX_E_ROW    IMG_VERTICAL_SIZE_E
#define MATRIX_A_COL    IMG_HORIZONTAL_SIZE
#define MATRIX_B_COL    IMG_HORIZONTAL_SIZE_B
#define MATRIX_C_COL    IMG_HORIZONTAL_SIZE_C
#define MATRIX_D_COL    IMG_HORIZONTAL_SIZE_D
#define MATRIX_E_COL    IMG_HORIZONTAL_SIZE_E

//-------------------------------------------------------------
// (Master) Image size (pixels)
//-------------------------------------------------------------
// #define IMG_HORIZONTAL_SIZE		1920					// pixels
// #define IMG_VERTICAL_SIZE		1088


//-------------------------------------------------------------
// (Master) Image format 
//-------------------------------------------------------------
#define RGB									// 4 bytes pixel	// FIXME
// #define YUV									// 1 byte

#define BYTE_PER_PIXEL			4					// bytes


//-------------------------------------------------------------
// (Master) Number of transactions (Debug)
//-------------------------------------------------------------
// #define AR_GEN_NUM			((IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE * BYTE_PER_PIXEL) / MAX_TRANS_SIZE)
// #define AW_GEN_NUM			((IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE * BYTE_PER_PIXEL) / MAX_TRANS_SIZE)	


//-------------------------------------------------------------
// (Master) Image static parameter 
//-------------------------------------------------------------
#define IMGHB				(IMG_HORIZONTAL_SIZE * BYTE_PER_PIXEL)	// bytes    
#define IMGVB				(IMG_VERTICAL_SIZE * BYTE_PER_PIXEL)	// bytes   
 
// #ifdef RGB
// #define PIXELS_PER_TRANS		(MAX_TRANS_SIZE / 4)			// 16 pixels	// FIXME
// #endif
// #ifdef YUV
// #define PIXELS_PER_TRANS		MAX_TRANS_SIZE				// 64 pixels
// #endif

#define PIXELS_PER_TRANS		(MAX_TRANS_SIZE / BYTE_PER_PIXEL)		// Check FIXME

//-------------------------------------------------------------
// (Master) Linear mode 
//-------------------------------------------------------------
#define NUM_COLUMNS_PER_ROW		((int)(ceilf( (float) IMG_HORIZONTAL_SIZE/ (float)PIXELS_PER_TRANS)))
#define NUM_TOTAL_TRANS			(NUM_COLUMNS_PER_ROW * IMG_VERTICAL_SIZE)


//-------------------------------------------------------------
// (Master) Application operation
//-------------------------------------------------------------
// #define SCENARIO_1 								// Camera preview
// #define SCENARIO_2 								// Rotated preview
// #define SCENARIO_5 								// Image scaling
// #define SCENARIO_6 								// Image blending
// #define SCENARIO_7 								// Rotated display

// #define RASTER_SCAN 
// #define ROTATION 
// #define RANDOM 
// #define CNN 
// #define JPEG 

// #define ROTATION_LEFT_BOT_VER
// #define ROTATION_RIGHT_BOT_HOR
// #define ROTATION_RIGHT_TOP_VER
// #define ROTATION_LEFT_TOP_VER						// Default rotation
// #define ROTATION_LEFT_BOT_HOR
// #define ROTATION_RIGHT_BOT_VER


//-------------------------------------------------------------
// (Master) Linear addr map
//-------------------------------------------------------------
// #define AR_LIAM
// #define AR_BFAM 
// #define AR_HBM_INTERLEAVING

// #define AW_LIAM
// #define AW_BFAM
// #define AW_HBM_INTERLEAVING


//-------------------------------------------------------------
// (Master) BFAM addr map
//-------------------------------------------------------------
// #define BANK_FLIP								// Patent
// #define BANK_FLIP_PLUS								// Superpage based BFAM
// #define BANK_SHUFFLE								// Bank Shuffling BSAM (on LIAM)
// #define SPLIT_BANK_SHUFFLE							// Bank Shuffling (on SLIAM)


//-------------------------------------------------------------
// (Master) Tile addr map 
//-------------------------------------------------------------
// #define AR_TILE								// Tiled map
// #define AR_hTILE								// Hierarchical tile
// #define AW_TILE
// #define AW_hTILE


//-------------------------------------------------------------
// (Memory) Number of banks
//-------------------------------------------------------------
#ifndef BANK_NUM
	#define BANK_NUM 16
#endif
#define BANK_NUM_PER_GROUP		4
#define BANK_NUM_PER_STACK		16
#define BANKGROUP_NUM			(int)round(BANK_NUM / BANK_NUM_PER_GROUP)
#define STACK_NUM				(int)round(BANK_NUM / BANK_NUM_PER_STACK)



//------------------		-------------------------------------------
// (Memory) Number of channels 
//-------------------------------------------------------------
 #define MEM_CHANNEL_NUM	1


//-------------------------------------------------------------
// (Memory) page size
//	DDR3-800 64MB x16 (512Mb), Page 2kB
//	PAGE : Row in a bank
//-------------------------------------------------------------
#define PAGE_SIZE		2048							// 2kB
// #define PAGE_SIZE		1024						// 1kB
// #define PAGE_SIZE		256 						// 512B

#define HBM_COMPLICATED_INTERLEAVING


//---------------------------
// Memory map 
//---------------------------
//	R (Row), M (Memory channel), B (Bank), C (Column), A (Rank)
//---------------------------
//	RBC  : R, B, C
//	RCBC : R, C1, B, C2 (6-bits). C2 cache line 64 bytes
//	BRC  : B, R, C. Smith
//
//	RMBC : R, M, B, C
//	RBCMC: R, B, C, M, C
//	RCBMC: R, C, B, M, C
//	
//	pRBC : Bank LSB = (Bank LSB) xor (Row LSB). Permuted from RBC.  Zhang_MICRO'00.
//	pRCBC: Bank LSB = (Bank LSB) xor (Row LSB). Permuted from RCBC. Kaseridis.
//---------------------------
 #define RBC									// R,B,C
// #define BRC									// B,R,C
// #define RCBC									// R,C1,B,C2
// #define pRBC									// Permutated RBC
// #define pRCBC								// Permutated RCBC

// #define RMBC									// R,M,B,C
// #define RBCMC								// R,B,C,M,C
// #define RCBMC								// R,C,B, M,C


//-------------------------------------------------------------
// Memory config bits
// 	RBC, pRBC
// Note FIXME Be careful
// 	When address 32-bits, ROW_WIDTH = 32 - BANK_WIDTH - COL_WIDTH
//-------------------------------------------------------------
#define COL_WIDTH		((int)(ceilf(log2f(PAGE_SIZE))))		// 11 bits for PAGE_SIZE 2kB 
#define BANK_WIDTH		((int)(ceilf(log2f(BANK_NUM))))			// 2  bits for BANK_NUM 4
#define MEM_CH_WIDTH	((int)(ceilf(log2f(MEM_CHANNEL_NUM))))		// 2  bits for MEM_CHANNEL_NUM 4
// #define ROW_WIDTH		(32 - MEM_CH_WIDTH - BANK_WIDTH - COL_WIDTH)	// Arch32 
#define ROW_WIDTH		(64 - MEM_CH_WIDTH - BANK_WIDTH - COL_WIDTH)	// Arch64


//-------------------------------------------------------------
// RCBC (R, C1, B, C2)
// 	C2 6-bit. Cache line 64B 
//-------------------------------------------------------------
#define COL2_WIDTH		6						// Cache line 64B	
#define COL1_WIDTH		(COL_WIDTH - COL2_WIDTH)			// Remaining 


//-------------------------------------------------------------
// Memory controller
//-------------------------------------------------------------
// #define IDEAL_MEMORY 
#define MEMORY_CONTROLLER


//-------------------------------------------------------------
// (Debug) Uncomment if display needed
//-------------------------------------------------------------
// #define ENABLE_AR_DISPLAY
// #define ENABLE_AW_DISPLAY
// #define ENABLE_R_DISPLAY 
// #define ENABLE_W_DISPLAY 
// #define ENABLE_B_DISPLAY 
// #define ENABLE_FIFO_DISPLAY 
// #define ENABLE_Q_DISPLAY 
// #define ENABLE_CMD_DISPLAY 


using namespace std;

//-------------------------------------------------------------
// Arch 
//-------------------------------------------------------------
// #define Arch32
// #define Arch64

// #ifdef Arch32
// typedef int int_t;
// #endif

// #ifdef Arch64
// typedef int64_t int_t;
// #endif

//-------------------------------------------------------------
// Tranaction packet type
//-------------------------------------------------------------
typedef enum{
	EPKT_TYPE_AR,
	EPKT_TYPE_AW,
	EPKT_TYPE_W,
	EPKT_TYPE_R,
	EPKT_TYPE_B,
	EPKT_TYPE_UNDEFINED
}EPktType;


//-------------------------------------------------------------
// Tranaction union type
//-------------------------------------------------------------
typedef enum{
	EUD_TYPE_AR,
	EUD_TYPE_AW,
	EUD_TYPE_W,
	EUD_TYPE_R,
	EUD_TYPE_B,
	EUD_TYPE_UNDEFINED
}EUDType;


//-------------------------------------------------------------
// Read write direction
//-------------------------------------------------------------
typedef enum{
	ETRANS_DIR_TYPE_READ,
	ETRANS_DIR_TYPE_WRITE,
	ETRANS_DIR_TYPE_UNDEFINED
}ETransDirType;


//-------------------------------------------------------------
// State
// 	(1) Valid state 
// 	(2) FIFO state
//-------------------------------------------------------------
typedef enum{
	ESTATE_TYPE_IDLE,					// Valid low 
	ESTATE_TYPE_BUSY,					// Valid high

	ESTATE_TYPE_FULL,					// FIFO full
	ESTATE_TYPE_EMPTY,					// FIFO empty

	ESTATE_TYPE_UNDEFINED
}EStateType;


//-------------------------------------------------------------
// Result
//	(1) Function result yes/no
//	(2) Function result success/fail
//	(3) Ready handshake
//	(4) Master operates on/off
//-------------------------------------------------------------
typedef enum{
	ERESULT_TYPE_YES,					// Function result YES
	ERESULT_TYPE_NO,					// Function result NO

	ERESULT_TYPE_ACCEPT,					// Ready high (handshaked) 
	ERESULT_TYPE_REJECT,					// Ready low

	ERESULT_TYPE_SUCCESS,					// Function success
	ERESULT_TYPE_FAIL,					// Function fails

	ERESULT_TYPE_ON,					// Master generates transactions
	ERESULT_TYPE_OFF,					// Master generates no transaction

	ERESULT_TYPE_UNDEFINED
}EResultType;


//-------------------------------------------------------------
// Port Tx Rx
//-------------------------------------------------------------
typedef enum{
	ETRX_TYPE_TX,
	ETRX_TYPE_RX,
	ETRX_TYPE_UNDEFINED
}ETRxType;


//-------------------------------------------------------------
// Transaction
//-------------------------------------------------------------
typedef enum{
	ETRANS_TYPE_NORMAL,

	ETRANS_TYPE_FIRST_PTW,					// MMU "Demand-PTW" 
	ETRANS_TYPE_SECOND_PTW,
	ETRANS_TYPE_THIRD_PTW,
	ETRANS_TYPE_FOURTH_PTW,

	ETRANS_TYPE_FIRST_PF,					// MMU prefetch 
	ETRANS_TYPE_SECOND_PF,
	ETRANS_TYPE_THIRD_PF,

	ETRANS_TYPE_FIRST_RPTW,					// MMU RTLB RMM RPTW
	ETRANS_TYPE_SECOND_RPTW,
	ETRANS_TYPE_THIRD_RPTW,

	ETRANS_TYPE_FIRST_APTW,					// MMU TLB AT APTW
	ETRANS_TYPE_SECOND_APTW,
	ETRANS_TYPE_THIRD_APTW,
	ETRANS_TYPE_FOURTH_APTW,

	ETRANS_TYPE_EVICT,					// Cache
	ETRANS_TYPE_LINE_FILL,  

	ETRANS_TYPE_UNDEFINED
}ETransType;


//-------------------------------------------------------------
// Arbiter 
//-------------------------------------------------------------
typedef enum{
	EARB_TYPE_RR,
	EARB_TYPE_FixedPriority,
	EARB_TYPE_UNDEFINED
}EArbType;


//-------------------------------------------------------------
// Address map 
// 	LIAM, BFAM, TILE
//-------------------------------------------------------------
// typedef enum{
//	EADDR_MAP_TYPE_LIAM					// Linear Address Map
//	EADDR_MAP_TYPE_SLIAM					// Split linear Address Map (Map0, Map1 - both LIAM)
//	EADDR_MAP_TYPE_BFAM_BANK_FLIP,				// Bank Interleaed linear. Samsung patent
//	EADDR_MAP_TYPE_BFAM_BANK_FLIP_PLUS,			// BFAM
//	EADDR_MAP_TYPE_BFAM_BANK_SHUFFLE,			// BSAM
//
//	EADDR_MAP_TYPE_TILE					// Tiled 
//	EADDR_MAP_TYPE_hTILE					// Hierarchical tiled 
//
//	EADDR_MAP_TYPE_UNDEFINED
// }EAddrMapType;


//-------------------------------------------------------------
// Image operation 
//-------------------------------------------------------------
// typedef enum{
//	EOP_TYPE_RASTER_SCAN,					// Row-major horizontal
//
//	EOP_TYPE_ROTATION,					// Column-major vertical. Start left top
//	
//	EOP_TYPE_ROTATION_LEFT_BOT_VER
//	EOP_TYPE_ROTATION_RIGHT_BOT_HOR
//	EOP_TYPE_ROTATION_RIGHT_TOP_VER
//	EOP_TYPE_ROTATION_LEFT_TOP_VER				// Default rotation. Start left top
//	EOP_TYPE_ROTATION_LEFT_BOT_HOR
//	EOP_TYPE_ROTATION_RIGHT_BOT_VER
//
//	EOP_TYPE_RANDOM,					// Random address
//
//	EOP_TYPE_CNN,						// Convolutional neural network (block-level convolved read, write)
//
//	EOP_TYPE_JPEG,						// 8x8 Block-level linear
//
//	EOP_TYPE_UNDEFINED
// }EOpType;


//-------------------------------------------------------------
// Image format 
//-------------------------------------------------------------
// typedef enum{
//	EIMG_TYPE_RGB,
//	EIMG_TYPE_YUV,
//	EIMG_TYPE_UNDEFINED
// }EImgType;


//-------------------------------------------------------------
// Memory map 
//-------------------------------------------------------------
// typedef enum{
//	EMEM_MAP_TYPE_RBC,					// R,B,C
//	EMEM_MAP_TYPE_RCBC,					// R,C,B,C (cache line bank interleave)
//	EMEM_MAP_TYPE_pRBC,					// R,B,C (B permuted)
//	EMEM_MAP_TYPE_pRCBC,					// R,C,B,C (B permuted)
//
//	EMEM_MAP_TYPE_RMBC,					// R,M,B,C
//	EMEM_MAP_TYPE_RBCMC,					// R,B,C,M,C
//	EMEM_MAP_TYPE_RCBMC,					// R,C,B,M,C
//
//	EMEM_MAP_TYPE_UNDEFINED
// }EMemMapType;


//-------------------------------------------------------------
// Get row, bank, col
//-------------------------------------------------------------
int64_t GetRowNum_AMap_Global(int64_t nAddr);   		// Address map
int     GetBankNum_AMap_Global(int64_t nAddr);
int     GetColNum_AMap_Global(int64_t nAddr);
int     GetChannelNum_AMap_Global(int64_t nAddr);

int64_t GetRowNum_MMap_Global(int64_t nAddr);   		// Memory map
int     GetBankNum_MMap_Global(int64_t nAddr);
int     GetColNum_MMap_Global(int64_t nAddr);
int     GetChannelNum_MMap_Global(int64_t nAddr);


//-------------------------------------------------------------
// Get cache ch num 
//-------------------------------------------------------------
int GetCacheChNum_Global(int64_t nAddr);

//-------------------------------------------------------------
// Get addr
//-------------------------------------------------------------
int64_t GetAddr_AMap_Global(int64_t nRow, int nBank, int nCol);			// RBC
int64_t GetAddr_AMap_Global(int64_t nRow, int nMemCh, int nBank, int nCol);	// RBCMC,RCBMC, RMBC

//-------------------------------------------------------------
// Convert enum to string 
//-------------------------------------------------------------
string Convert_eDir2string(ETransDirType eType);
string Convert_eUDType2string(EUDType eType);
string Convert_eTransType2string(ETransType eType);
string Convert_eResult2string(EResultType eType); 

#endif

int random(int min, int max);
