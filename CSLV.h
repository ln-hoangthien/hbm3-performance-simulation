//------------------------------------------------------------
// Filename	: CSLV.h
// Version	: 0.74
// Date		: 28 Feb 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Slave handler
//------------------------------------------------------------
#ifndef CSLV_H
#define CSLV_H

#include <string>
#include <vector>

#include "CFIFO.h"
#include "CMem.h"
#include "CQ.h"
#include "CScheduler.h"
#include "CTRx.h"
#include "Memory.h"
#include "Top.h"
#include "UD_Bus.h"

using namespace std;

//------------------------------------
// FIFO R, B latency
//------------------------------------
#define SLV_FIFO_R_LATENCY 3
#define SLV_FIFO_B_LATENCY 3

//------------------------------------
// Slave class
//------------------------------------
typedef class CSLV *CPSLV;
class CSLV {

public:
  // 1. Contructor. Destructor
  CSLV(string cName, int nMemCh);
  ~CSLV();

  // 2. Control
  EResultType Reset();

  // Handler ideal mem
  EResultType Do_AR_fwd(int64_t nCycle); // Ideal memory
  EResultType Do_AW_fwd(int64_t nCycle);

  // Handler MC
  EResultType Do_AR_fwd_MC_Frontend(int64_t nCycle); // Ax into request CQ_AR
  EResultType Do_AW_fwd_MC_Frontend(int64_t nCycle); // Ax
  EResultType Do_W_fwd_MC_Frontend(int64_t nCycle);  // W

  EResultType Do_Ax_fwd_MC_Backend_Request(int64_t nCycle); // Schedule (from CQ). Ax into request FIFO_Ax

  EResultType Do_AR_fwd_MC_Backend_Response(int64_t nCycle); // R into response FIFO_R from bank
  EResultType Do_AW_fwd_MC_Backend_Response(int64_t nCycle); // B

  // Handler common
  EResultType Do_AR_bwd(int64_t nCycle);
  EResultType Do_AW_bwd(int64_t nCycle);
  EResultType Do_W_fwd(int64_t nCycle);
  EResultType Do_W_bwd(int64_t nCycle);
  EResultType Do_R_fwd(int64_t nCycle);
  EResultType Do_R_bwd(int64_t nCycle);
  EResultType Do_B_fwd(int64_t nCycle);
  EResultType Do_B_bwd(int64_t nCycle);

  // Cmd
  EResultType SetMemCmdPkt(EMemCmdType eCmd, int nBank,
                           int nRow); // Cmd (to send to Mem) in current cycle
  EResultType SetMemCmdPkt(SPMemCmdPkt spMemCmdPkt);
  EResultType Handle_PRE_and_REF(int64_t nCycle);

  EResultType UpdateState(int64_t nCycle);

  // Debug
  EResultType CheckLink();
  EResultType CheckSLV();
  EResultType PrintStat(int64_t nCycle, FILE *fp);

  // Port
  CPTRx cpRx_AR;
  CPTRx cpRx_AW;
  CPTRx cpRx_W;
  CPTRx cpTx_R;
  CPTRx cpTx_B;

private:
  // Original
  string cName;
  int nMemCh;                       // Slave (channel) num
  int previous_nBank;               // Previous Access Bank (for PRE/REF handling)
  vector<int> need_Refresing;       // Vector of Banks need to be Refreshed
  bool prepare_Refresing[BANK_NUM]; // Prepare for refreshing (for PRE/REF
                                    // handling)

  // Request queue
  CPQ cpQ_AR; // Request Q Ax (before schedule)
  CPQ cpQ_AW;
  CPQ cpQ_W; // Request Q W  (before schedule)

  // Request FIFO
  CPFIFO cpFIFO_AR; // Request FIFO Ax (after schedule)
  CPFIFO cpFIFO_AW;
  CPFIFO cpFIFO_R; // Response
  CPFIFO cpFIFO_B;

  // MC
  CPScheduler cpScheduler;

  // Cmd
  SPMemCmdPkt spMemCmdPkt; // Cmd, bank, row in current cycle (after schedule)
  // State
  SPMemStatePkt spMemStatePkt; // Cmd, bank, row in current cycle (after schedule)

  // Memory
  CPMem cpMem;

  // Stat
  int nAR; // Number AR
  int nAW;
  int nW;
  int nR;
  int nB;
  int nPTW; // Number any PTW

  int nACT_cmd_AR; // ACT cmd for AR
  int nPRE_cmd_AR;
  int nRD_cmd_AR;
  int nNOP_cmd_AR;

  int nACT_cmd_AW; // ACT cmd for AW
  int nPRE_cmd_AW;
  int nWR_cmd_AW;
  int nNOP_cmd_AW;

  int nACT_cmd; // ACT cmd
  int nPRE_cmd;
  int nRD_cmd;
  int nWR_cmd;
  int nNOP_cmd;

  int nMax_Q_AR_Occupancy; // Max occupancy req Q
  int nMax_Q_AW_Occupancy;
  int nTotal_Q_AR_Occupancy; // Accumulate occupancy
  int nTotal_Q_AW_Occupancy;

  int nMax_Q_AR_Wait; // Max waiting cycles req Q all entries
  int nMax_Q_AW_Wait;

  int nEmpty_Q_AR_cycles;
  int nEmpty_Q_AW_cycles;
  int nEmpty_Q_Ax_cycles;

  uint64_t nMax_Q_AR_Scheduled_Wait; // Max waiting cycle req Q "scheduled" entry
  uint64_t nMax_Q_AW_Scheduled_Wait;
  int nTotal_Q_AR_Scheduled_Wait; // Accumulate waiting cycle req Q "scheduled"
  int nTotal_Q_AW_Scheduled_Wait;
};

#endif
