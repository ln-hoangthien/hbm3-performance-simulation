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
// tCK		: Clock Cycle at 6.4GB/s.
// 
// tRC		: ACTIVE to ACTIVE command period. Minimum time between successive activate commands to the same bank.
// tRAS		: ACTIVE to PRECHARGE command period. Per-bank. Minimum time after an activate command to a bank until that bank is allowed to be precharged.
// 
// tRCDRD	: ACTIVE to READ command delay. Miminum time between activate and RD commands on the same bank.
// tRCDWR	: ACTIVE to WRITE command delay. Miminum time between activate and WR commands on the same bank.
// 
// tRRDL 	: ACTIVE to ACTIVE or PER BANK REFRESH bank B command delay same bank group.
// tRRDS	: ACTIVE to ACTIVE or PER BANK REFRESH bank B command delay different bank group.
// 
// tRTP		: Minimum time between READ to PRECHARGE command delay same bank.
// tRP		: Minimum time between a precharge command and an activate command on the same bank.
// tWR		: WRITE recovery time. Minimum time after the last data has been written until PRE may be issued on the same bank.
// 
// tCCDL	: RD/WR bank A to RD/WR bank B command delay same bank group.
// tCCDS	: RD/WR bank A to RD/WR bank B command delay different bank group.
// tCCDR	: RD SID A to RD SID B command delay.
// tWTRL	: Internal WRITE to READ command delay same bank group.
// tWTRS	: Internal WRITE to READ command delay different bank group.
// tRTW		: READ to WRITE command delay.
// tRL		: Read Latency.
// tWL		: Write Latency.
// 
// tRFCab	: Refresh command period
// tRFCpb	: PER BANK REFRESH command period (same bank).
// tRREFD	: PER BANK REFRESH command period (different bank).
// tREFI	: PER BANK REFRESH to ACTIVATE command delay (different bank).
// tREFIab	: Average periodic refresh interval for REFRESH command.
// tREFIpb	: Average periodic refresh interval for PER BANK REFRESH command.
// 
// FAW		: Four Bank Active Window.
// tDAL		: Auto precharge write recovery + Precharge time.
// tPPD		: PRECHARGE to PRECHARGE delay same pseudo channel.
// RAA		: Rolling Accumulated ACTIVATE count.
// 
// tWDQS2DQ_O(max)	: WDQS to read data and RDQS offset.
// tWDQS2DQ_I(min)	: WDQS to write data offset.

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
// JDESD238B.01 - March 2025 
//-------------------------------------------------
  #define tCK				        0.625			  // Clock cycle time (ns) in ns at 6.4Gb/s.
  #define HBM_BURST_LENGTH	8				    // HBM burst length in bytes.
  #define BanksPerBGroup	  4					  // Number of banks per bank group. HBM3 has 4 banks per bank group.
  #define BanksPerSID		    16					// Number of banks per stack. HBM3 has 16 banks per stack.

  // Row Access Timing Parameters
  #define tRP		    static_cast<int>(std::ceil(15/tCK))-1			// (Min) PRE2ACT - Plus 1 due to the ACT command takes 2 CK cycles to complete
  #define tRAS	    static_cast<int>(std::ceil(33/tCK))+1			// (Min) ACT2PRE - Plus 1 due to the ACT command takes 2 CK cycles to complete
  #define tRAS_max	9*tREFI				                            // (Max) ACT2PRE
  #define tRC		    tRAS+tRP                                  // RC = RAS + RP.  ACT2ACT. Inherently implemented

  #define tRCDRD	static_cast<int>(std::ceil(12/tCK))				  // ACT2RD or ACT2WR - Copied from Gem5's HBM2: https://github.com/gem5/gem5/blob/ddd4ae35adb0a3df1f1ba11e9a973a5c2f8c2944/src/python/gem5/components/memory/dram_interfaces/hbm.py#L207
  #define tRCDWR	static_cast<int>(std::ceil(6/tCK))					// ACT2RD or ACT2WR - Copied from Gem5's HBM2: https://github.com/gem5/gem5/blob/ddd4ae35adb0a3df1f1ba11e9a973a5c2f8c2944/src/python/gem5/components/memory/dram_interfaces/hbm.py#L207
  #define tRCD		(std::max(tRCDRD, tRCDWR))	                // ACT2RD or ACT2WR - Copied from Gem5's HBM2:

  #define tRRDL		static_cast<int>(std::ceil(6/tCK))					// ACT2ACT or ACT2REF - Copied from Gem5's HBM2: https://github.com/gem5/gem5/blob/ddd4ae35adb0a3df1f1ba11e9a973a5c2f8c2944/src/python/gem5/components/memory/dram_interfaces/hbm.py#L207
  #define tRRDS		static_cast<int>(std::ceil(4/tCK))					// ACT2ACT or ACT2REF - Copied from Gem5's HBM2: https://github.com/gem5/gem5/blob/ddd4ae35adb0a3df1f1ba11e9a973a5c2f8c2944/src/python/gem5/components/memory/dram_interfaces/hbm.py#L207
  #define tRRD		(std::max(tRRDL, tRRDS))	                  // ACT2ACT or ACT2REF - Copied from Gem5's HBM2:

  // Column Access Timing Parameters
  #define tRTP		static_cast<int>(std::ceil(5/tCK))					// RD2PRE
  #define tWR		  static_cast<int>(std::ceil(14/tCK))+1				// WR2PRE. tWR + BurstLength + tWL. 11=6+4+5.
                        

  #define tCCDL		std::max(4, static_cast<int>(std::ceil(2.5/tCK)))	// RD2RD or WR2WR. CAS to CAS command delay same bank group. Minimum time between two RD commands or two WR commands.
  #define tCCDS		2							                                    // RD2RD or WR2WR. CAS to CAS command delay different bank group. Minimum time between two RD commands or two WR commands.
  #define tCCDR		tCCDS+2						                                // RD2RD. CAS to CAS command delay different stack ID. Minimum time between two RD commands or two WR commands.
	                                                                  // NOTE 17 of Table 93 — Timings Parameters: The tCCDR(min) value is vendor specific and a range of tCCDS + 1 to 2nCK is supported. The tCCDR(min) is dependent on the operation frequency. The vendor datasheet should be consulted for details.
  #define tCCD 		(std::max(tCCDL, tCCDS))	                        // RD2RD or WR2WR. CAS to CAS command delay. Minimum time between two RD commands or two WR commands.

  #define tWTRL		static_cast<int>(std::ceil(9/tCK))					      // WR2RD. Internal WRITE to READ command delay same bank group.  We assume CCD covers this - Copied from Gem5's HBM2: https://github.com/gem5/gem5/blob/ddd4ae35adb0a3df1f1ba11e9a973a5c2f8c2944/src/python/gem5/components/memory/dram_interfaces/hbm.py#L207
  #define tWTRS		static_cast<int>(std::ceil(4/tCK))					      // WR2RD. Internal WRITE to READ command delay different bank group.  We assume CCD covers this - Copied from Gem5's HBM2: https://github.com/gem5/gem5/blob/ddd4ae35adb0a3df1f1ba11e9a973a5c2f8c2944/src/python/gem5/components/memory/dram_interfaces/hbm.py#L207
  #define tWTR		(std::max(tWTRL, tWTRS))	                        // WR2RD. Internal WRITE to READ command delay.  We assume CCD covers this - Copied from Gem5's HBM2:

  #define tRL		6                           // Read latency. Time after RD command until first data is available on the bus.
  #define tWL		4							              // Write latency. Time after WR command until first data is available on the bus.
  #define tRL_simulation		1               // We skip returning data delay in simulation for simplicity. This is only for simulation.
  #define tWL_simulation		1               // We skip returning data delay in simulation for simplicity. This is only for simulation.

  // Other Access Timing Parameters
  #define tWDQS2DQ_O_max	static_cast<int>(std::ceil(2.5/tCK))		// WDQS to read data and RDQS offset
  #define tWDQS2DQ_I_min	static_cast<int>(std::ceil(0.9/tCK))		// WDQS to write data offset

  #define tRTW				static_cast<int>(std::ceil(((tRL + HBM_BURST_LENGTH/4 - tWL + 0.5) + tWDQS2DQ_O_max - tWDQS2DQ_I_min)))	// READ to WRITE command delay = (RL + BL/4 - WL + 0.5) × tCK + tWDQS2DQ_O(max) - tWDQS2DQ_I(min)
                                                                                                                              // tRTW is not a DRAM device limit but determined by the system bus turnaround time.
    
  #define tFAW				20		                                            // No more than 4 banks may be activated in a rolling tFAW window.
  #define tREFI				static_cast<int>(std::floor(3900/tCK))	          // (Max Value) Average refresh interval. Not implemented.
  #define tREFIpb			static_cast<int>(std::floor(tREFI/BANK_NUM))	    // (Max Value) Average refresh interval. Not implemented.
  #define tRFCab			static_cast<int>(std::ceil(350/tCK))	            // (Min Value) Not implemented
  #define tRFCpb			static_cast<int>(std::ceil(200/tCK))	            // (Min Value) Not implemented
  #define tRREFD			std::max(3, static_cast<int>(std::ceil(8/tCK)))		// PER BANK REFRESH command period (different bank)

//-------------------------------------------------

using namespace std;

//-------------------------------------------------
// Memory state 
//-------------------------------------------------
typedef enum{
	EMEM_STATE_TYPE_IDLE,
  EMEM_STATE_TYPE_REFRESHING,			  // Doing activation
	EMEM_STATE_TYPE_ACTIVATING,			  // Doing activation
	EMEM_STATE_TYPE_ACTIVE_for_READ,	// Activated
  EMEM_STATE_TYPE_ACTIVE_for_WRITE,	// Activated
  EMEM_STATE_TYPE_ACTIVE,				    // Activated
	EMEM_STATE_TYPE_PRECHARGING,			// Doing precharge
	EMEM_STATE_TYPE_UNDEFINED
}EMemStateType;


//-------------------------------------------------
// Memory command 
//-------------------------------------------------
typedef enum{
	EMEM_CMD_TYPE_ACT,				// Activate
  EMEM_CMD_TYPE_REFpb,      // Per bank Refresh
	EMEM_CMD_TYPE_PRE,				// Precharge
	EMEM_CMD_TYPE_RD,				  // Read
	EMEM_CMD_TYPE_WR,				  // Write
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
  EResultType	IsREF_ready[BANK_NUM];
  EResultType	forced_PRE[BANK_NUM];
  EResultType	forced_REFI[BANK_NUM];
	
	EResultType	IsFirstData_ready[BANK_NUM];	// Can put first data in bank
	EResultType	IsBankPrepared[BANK_NUM];	// Bank activated (not yet RD/WR) 
	int		      nActivatedRow[BANK_NUM];
	EResultType	IsData_busy;			// Stat
}SMemStatePkt;

//-------------------------------------------------
// Convert 
//-------------------------------------------------
string Convert_eMemCmd2string(EMemCmdType eType);

#endif

