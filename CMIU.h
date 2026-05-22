//------------------------------------------------------------
// Filename	: CMIU.h
// Version	: 0.12
// Date		: 5 Feb 2023
// Contact	: JaeYoung.Hur@gmail.com
// Description	: MIU header
//------------------------------------------------------------
// Known issues
//  (1) W channel not supported yet
//------------------------------------------------------------
#ifndef CMIU_H
#define CMIU_H

#include <math.h>
#include <string>

#include "CArb.h"
#include "CFIFO.h"
#include "CTRx.h"
#include "Top.h"
#include "UD_Bus.h"

#include "CROB.h"

using namespace std;

#define ROB_RESP_LATENCY 2 // Min cycles to be allocated in ROB_R,B.
// #define R_ARBITER_TRANSACTION_LEVEL

// #define CACHE_MIU_MODE			// Cache interleave mode

typedef class CMIU *CPMIU;
class CMIU {

public:
  // 1. Contructor. Destructor
  CMIU(string cName, int NUM_PORT_SI, int NUM_PORT_MI);
  ~CMIU();

  // 2. Control
  // Handler
  EResultType Do_AR_fwd(int64_t nCycle);
  // EResultType	Do_AR_bwd(int64_t nCycle);
  EResultType Do_AW_fwd(int64_t nCycle);
  // EResultType	Do_AW_bwd(int64_t nCycle);
  // EResultType	Do_R_fwd(int64_t nCycle);
  // EResultType	Do_R_bwd(int64_t nCycle);
  EResultType Do_W_fwd(int64_t nCycle);
  EResultType Do_W_bwd(int64_t nCycle);
  // EResultType	Do_B_fwd(int64_t nCycle);
  // EResultType	Do_B_bwd(int64_t nCycle);

  // Handler MIU
  // EResultType	Do_AR_fwd_SI(int64_t nCycle);
  // EResultType	Do_AR_fwd_MI(int64_t nCycle);
  // EResultType	Do_AW_fwd_SI(int64_t nCycle);
  // EResultType	Do_AW_fwd_MI(int64_t nCycle);
  // EResultType	Do_W_fwd_SI(int64_t nCycle);
  // EResultType	Do_W_fwd_MI(int64_t nCycle);
  EResultType Do_R_fwd_SI(int64_t nCycle);
  EResultType Do_R_fwd_Arb(int64_t nCycle);
  EResultType Do_B_fwd_SI(int64_t nCycle);
  EResultType Do_B_fwd_Arb(int64_t nCycle);
  // EResultType	Do_B_fwd_MI(int64_t nCycle);

  EResultType Do_AR_bwd_SI(int64_t nCycle);
  EResultType Do_AR_bwd_MI(int64_t nCycle);
  EResultType Do_AW_bwd_SI(int64_t nCycle);
  EResultType Do_AW_bwd_MI(int64_t nCycle);
  EResultType Do_R_bwd_SI(int64_t nCycle);
  EResultType Do_R_bwd_MI(int64_t nCycle);
  EResultType Do_B_bwd_SI(int64_t nCycle);
  EResultType Do_B_bwd_MI(int64_t nCycle);

  // Get value
  int GetMO_AR_SI(int nPort);
  int GetMO_AW_SI(int nPort);

  int GetPortNum_SI(int nID);

  // SPLinkedRUD	GetResp_poppable_ROB(EUDType eType);

  // Control
  EResultType UpdateState(int64_t nCycle);
  EResultType Reset();
  EResultType PrintStat(int64_t nCycle, FILE *fp);
  EResultType Set_CACHE_MIU_MODE(EResultType IsCacheMode);

  // Debug
  EResultType CheckLink();

  // Port slave interface
  CPTRx *cpRx_AR; // [NUM_PORT_SI]
  CPTRx *cpTx_R;
  CPTRx *cpRx_AW;
  CPTRx *cpRx_W;
  CPTRx *cpTx_B;

  // Port master interface				// [NUM_PORT_MI]
  CPTRx *cpTx_AR;
  CPTRx *cpRx_R;
  CPTRx *cpTx_AW;
  CPTRx *cpTx_W;
  CPTRx *cpRx_B;

private:
  // Original
  string cName;

  EResultType Is_CACHE_MIU_MODE;

  // Control
  int NUM_PORT_SI; // Num port
  int NUM_PORT_MI;
  int BIT_PORT_SI; // Bits port

  // Stat
  int *nMO_AR_SI;
  int *nMO_AW_SI;

  int *nAR_SI;
  int *nAW_SI;
  int *nR_SI;
  int *nB_SI;

  int *nMax_alloc_cycles_ROB_AR_SI;
  int *nMax_alloc_cycles_ROB_R_SI;
  int *nTotal_alloc_cycles_ROB_AR_SI;
  int *nTotal_alloc_cycles_ROB_R_SI;

  int *nMax_alloc_cycles_ROB_AW_SI;
  int *nMax_alloc_cycles_ROB_B_SI;
  int *nTotal_alloc_cycles_ROB_AW_SI;
  int *nTotal_alloc_cycles_ROB_B_SI;

  int *nMax_Occupancy_ROB_AR_SI;
  int *nMax_Occupancy_ROB_R_SI;
  int *nTotal_Occupancy_ROB_AR_SI;
  int *nTotal_Occupancy_ROB_R_SI;

  int *nMax_Occupancy_ROB_AW_SI;
  int *nMax_Occupancy_ROB_B_SI;
  int *nTotal_Occupancy_ROB_AW_SI;
  int *nTotal_Occupancy_ROB_B_SI;

  // Arbiter MI
  CPArb *cpArb_AR; // MI
  CPArb *cpArb_AW;

  // Arbiter SI
  CPArb *cpArb_R; // SI
  CPArb *cpArb_B;

  // FIFO MI
  CPFIFO cpFIFO_AW; // MI
  // CPFIFO	cpFIFO_W;

  // ROB SI
  CPROB *cpROB_AR_SI;
  CPROB *cpROB_AW_SI;

  CPROB *cpROB_R_SI;
  CPROB *cpROB_B_SI;
};

#endif
