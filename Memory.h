//-------------------------------------------------
// Filename	: Memory.h
// Version	: 0.75
// Date		: 6 May 2018
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Memory related header
//-------------------------------------------------
#ifndef MEMORY_H
#define MEMORY_H

#include <string>
#include <math.h>
#include "Top.h" 

//-------------------------------------------------
// TIming parameter 
//-------------------------------------------------
// tRCD	 : Activating. Miminum time between activate and RD/WR commands on the same bank
// tCL	 : All banks. Fixed. CAS latency. Time after RD command until first data is available on the bus.
// tRP	 : Precharging. Fixed. Minimum time between a precharge command and an activate command on the same bank
// tRAS	 : Per-bank. Minimum time after an activate command to a bank until that bank is allowed to be precharged.
// tRTP	 : Per-bank. Minimum time between RD and PRE command
// tRRD	 : Different banks. Minimum time between ACTs to all banks 
// tCCD	 : CAS to CAS command delay. Minimum time between two RD commands or two WR commands.
// 
// tRC	 : Minimum time between successive activate commands to the same bank
// tWL	 : Write latency. Time after WR command until first data is available on the bus.
// tWR	 : Write recovery time. Minimum time after the last data has been written until PRE may be issued on the same bank 
// tWTR	 : Internal write to read command delay	
//
// tFAW	 : Window in which maximally four banks may be activated
// tRFC	 : Minimum time between a refresh command and a successive refresh or activate command
// tREFI : Average refresh interval


//-------------------------------------------------
// Cycle counts that requires direction change
//-------------------------------------------------
#define CYCLE_COUNT_DIR_CHANGE  150	


//-------------------------------------------------
// Cycle time-out (scheduler)
// * If no service in req Q entry, force scheduling
//-------------------------------------------------
#define CYCLE_TIMEOUT           400


//-------------------------------------------------
// DDR2-400 64MB x16 (512Mb), Page 2kB
//-------------------------------------------------
// #define	tRCD	3	// ACT2RD or ACT2WR
// #define	tCL	3	// RD2DATA 
// #define	tRP	3	// PRE2ACT
// #define	tRAS	8	// ACT2PRE
// #define	tRTP	2	// RD2PRE
// #define	tRRD	2	// ACT2ACT (all banks)
// #define	tCCD	2	// RD2RD. WR2WR. CAS to CAS command delay. Minimum time between two RD commands or two WR commands.
// 
// #define	tRC	11	// RC = RAS + RP
// #define	tWL	2	// WR2DATA
// #define	tWR	3	// 
// #define	tFAW	10	// 
// #define	tWTR	2	// WR2RD 
// #define	tRFC	21	// 
// #define	tREFI	1560	// Average refresh interval


//-------------------------------------------------
// DDR2-800 64MB x16 (512Mb), Page 2kB
//-------------------------------------------------
// #define	tRCD	4	// ACT2RD or ACT2WR
// #define	tCL	4	// RD2DATA 
// #define	tRP	4	// PRE2ACT
// #define	tRAS	18	// ACT2PRE
// #define	tRTP	3	// RD2PRE
// #define	tRRD	4	// ACT2ACT (all banks)
// #define	tCCD	2	// RD2RD. WR2WR. CAS to CAS command delay. Minimum time between two RD commands or two WR commands.
// 
// #define	tRC	22	// RC = RAS + RP
// #define	tWL	3	// WR2DATA
// #define	tWR	6	// 
// #define	tFAW	18	// 
// #define	tWTR	3	// WR2RD 
// #define	tRFC	42	// 
// #define	tREFI	3120	// Average refresh interval


//-------------------------------------------------
// DDR2-800 Debug 
//-------------------------------------------------
//  #define	tRCD	4	// ACT2RD or ACT2WR
//  #define	tCL	4	// RD2DATA 
//  #define	tRP	4	// PRE2ACT
//  #define	tRAS	18	// ACT2PRE
//  #define	tRTP	3	// RD2PRE
//  #define	tRRD	4	// ACT2ACT (all banks)
//  #define	tCCD	2	// RD2RD. WR2WR. CAS to CAS command delay. Minimum time between two RD commands or two WR commands.
//  
//  #define	tRC	22	// RC = RAS + RP
//  // #define	tWL	3	// WR2DATA
//  #define	tWL	4	// WR2DATA
//  #define	tWR	6	// 
//  #define	tFAW	18	// 
//  #define	tWTR	3	// WR2RD 
//  #define	tRFC	42	// 
//  #define	tREFI	3120	// Average refresh interval


//-------------------------------------------------
// DDR3-800 64MB x16 (512Mb), Page 2kB
//-------------------------------------------------
// #define		tRCD	5	// ACT2RD or ACT2WR
// #define		tCL	5	// RD2DATA
// #define		tRP	5	// PRE2ACT
// #define		tRAS	15	// ACT2PRE
// #define		tRTP	4	// RD2PRE
// #define		tRRD	4	// ACT2ACT (all banks)
// #define		tCCD	4	// RD2RD or WR2WR. CAS to CAS command delay. Minimum time between two RD commands or two WR commands.  
// 
// #define		tRC	20	// RC = RAS + RP.  ACT2ACT. Inherently implemented
// 
// #define		tWL	5	// WR2DATA
// 
// // #define		tWR	6	// WR2PRE 
// // #define		tWR	10	// WR2PRE. tWR + BurstLength.       10=6+4.
// // #define		tWR	11	// WR2PRE. tWR + tWL.               11=6+5.
// #define		tWR	15	// WR2PRE. tWR + BurstLength + tWL. 11=6+4+5.
// #define		tWTR	4	// WR2RD.  We assume CCD covers this.
// 
// #define		tFAW	20	// Not implemented. We do not need this, because we have 4 banks.
// #define		tRFC	36	// Not implemented
// #define		tREFI	3120	// Average refresh interval. Not implemented.


//-------------------------------------------------
// DDR3-800 Debug 
//-------------------------------------------------
  #define		tRP	5	// PRE2ACT
  #define		tRAS	15	// ACT2PRE
  #define		tRCD	5	// ACT2RD or ACT2WR

  #define		tRRD	4	// ACT2ACT (all banks)
  #define		tCCD	4	// RD2RD or WR2WR. CAS to CAS command delay. Minimum time between two RD commands or two WR commands.  
  
  #define		tCL	5	// RD2DATA
  // #define		tCL	6	// RD2DATA
  #define		tWL	5	// WR2DATA
  // #define		tWL	6	// WR2DATA
  
  #define		tRTP	4	// RD2PRE
  // #define		tWR	6	// WR2PRE 
  // #define		tWR	10	// WR2PRE. tWR + BurstLength.       10=6+4.
  // #define		tWR	11	// WR2PRE. tWR + tWL.               11=6+5.
  #define		tWR	15	// WR2PRE. tWR + BurstLength + tWL. 11=6+4+5.

  #define		tRC	20	// RC = RAS + RP.  ACT2ACT. Inherently implemented

  #define		tWTR	4	// WR2RD.  We assume CCD covers this.
  #define		tFAW	20	// Not implemented. We do not need this, because we have 4 banks.
  #define		tRFC	36	// Not implemented
  #define		tREFI	3120	// Average refresh interval. Not implemented.



//-------------------------------------------------
// DDR3-1600 64MB x16 (512Mb), Page 2kB
//-------------------------------------------------
// #define	tRCD	8	// ACT2RD or ACT2WR
// #define	tCL	8	// RD2DATA 
// #define	tRP	8	// PRE2ACT
// #define	tRAS	28	// ACT2PRE
// #define	tRTP	6	// RD2PRE
// #define	tRRD	6	// ACT2ACT (all banks)
// #define	tCCD	4	// RD2RD. WR2WR. CAS to CAS command delay. Minimum time between two RD commands or two WR commands.
// 
// #define	tWL	5	// WR2DATA
// #define	tWR	12	// 
//
// #define	tRC	36	// RC = RAS + RP
// #define	tFAW	32	// 
// #define	tWTR	6	// WR2RD 
// #define	tRFC	72	// 
// #define	tREFI	6240	// Average refresh interval


//-------------------------------------------------

using namespace std;

//-------------------------------------------------
// Memory state 
//-------------------------------------------------
typedef enum{
	EMEM_STATE_TYPE_IDLE,
	EMEM_STATE_TYPE_ACTIVATING,			// Doing activation
	EMEM_STATE_TYPE_ACTIVE,				// Activated
	EMEM_STATE_TYPE_PRECHARGING,			// Doing precharge
	EMEM_STATE_TYPE_UNDEFINED
}EMemStateType;


//-------------------------------------------------
// Memory command 
//-------------------------------------------------
typedef enum{
	EMEM_CMD_TYPE_ACT,				// Activate
	EMEM_CMD_TYPE_PRE,				// Precharge
	EMEM_CMD_TYPE_RD,				// Read
	EMEM_CMD_TYPE_WR,				// Write
	EMEM_CMD_TYPE_NOP,				// No operation 
	EMEM_CMD_TYPE_UNDEFINED
}EMemCmdType;


//-------------------------------------------------
// Command pkt (from MC to Mem)
//-------------------------------------------------
typedef struct tagSMemCmdPkt* SPMemCmdPkt;
typedef struct tagSMemCmdPkt{
	EMemCmdType	eMemCmd;			// Command (that memory receives)
	// SPMemAddr	spMemAddr;			// Row, Bank, Column  
	int		nBank;
	int		nRow;
}SMemCmdPkt;


//-------------------------------------------------
// State pkt per bank (from Mem to MC) 
//-------------------------------------------------
typedef struct tagSMemStatePkt* SPMemStatePkt;
typedef struct tagSMemStatePkt{
	EMemStateType	eMemState[BANK_NUM];		// Bank state 

	EResultType	IsRD_ready[BANK_NUM];
	EResultType	IsWR_ready[BANK_NUM];
	EResultType	IsPRE_ready[BANK_NUM];
	EResultType	IsACT_ready[BANK_NUM];
	
	EResultType	IsFirstData_ready[BANK_NUM];	// Can put first data in bank
	EResultType	IsBankPrepared[BANK_NUM];	// Bank activated (not yet RD/WR) 
	int		nActivatedRow[BANK_NUM];
	EResultType	IsData_busy;			// Stat
}SMemStatePkt;


//-------------------------------------------------
// Counter
//-------------------------------------------------
// typedef enum{
//	EMEM_CNT_TYPE_ACT2RD,				// RC   (Activating)
//	EMEM_CNT_TYPE_PRE2ACT,				// RP   (Precharging)
//	EMEM_CNT_TYPE_RD2DATA,				// CL
//	EMEM_CNT_TYPE_RD2RD,				// tCCD
//	EMEM_CNT_TYPE_ACT2ACT,				// tRRD
//	EMEM_CNT_TYPE_RD2PRE,				// RTP 
//	EMEM_CNT_TYPE_ACT2PRE,				// RAS
//	EMEM_CNT_TYPE_UNDEFINED
// }EMemCntType;


//-------------------------------------------------
// Memory RBC address 
//-------------------------------------------------
// typedef struct tagSMemAddr* SPMemAddr;
// typedef struct tagSMemAddr{
//	int 	 nBank;
//	int_64_t nRow;
//	int 	 nCol;
// }SMemAddr;


//-------------------------------------------------
// Convert 
//-------------------------------------------------
string Convert_eMemCmd2string(EMemCmdType eType);

#endif

