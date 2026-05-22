//------------------------------------------------------------
// Filename  : CBUS.h
// Version  : 0.78
// Date    : 20 Aug 2018
// Contact  : JaeYoung.Hur@gmail.com
// Description  : Bus header
//------------------------------------------------------------
#ifndef CBUS_H
#define CBUS_H

#include <math.h>
#include <string>
#include <vector>

#include "CArb.h"
#include "CFIFO.h"
#include "CTRx.h"
#include "Top.h"

using namespace std;

typedef class CBUS *CPBUS;
class CBUS {

public:
  // 1. Contructor. Destructor
  CBUS(string cName, int NUM_PORT);
  ~CBUS();

  // 2. Control
  // Handler
  EResultType Do_AR_fwd(int64_t nCycle);
  EResultType Do_AR_bwd(int64_t nCycle);
  EResultType Do_AW_fwd(int64_t nCycle);
  EResultType Do_AW_bwd(int64_t nCycle);
#ifdef CCI_ON
  EResultType Do_AC_fwd(int64_t nCycle);
  EResultType Do_AC_bwd(int64_t nCycle);
  EResultType Do_CR_fwd(int64_t nCycle);
  EResultType Do_CR_bwd(int64_t nCycle);
  EResultType Do_W_snoop_fwd(int64_t nCycle);
  EResultType Do_CD_fwd(int64_t nCycle);
  EResultType Do_CD_bwd(int64_t nCycle);
#endif
  EResultType Do_R_fwd(int64_t nCycle);
  EResultType Do_R_bwd(int64_t nCycle);
  EResultType Do_W_fwd(int64_t nCycle);
  EResultType Do_W_bwd(int64_t nCycle);
  EResultType Do_B_fwd(int64_t nCycle);
  EResultType Do_B_bwd(int64_t nCycle);

  // Set value
  EResultType Increase_MO_AR();
  EResultType Decrease_MO_AR();
  EResultType Increase_MO_AW();
  EResultType Decrease_MO_AW();

  EResultType Set_nR_GEN_NUM(int nNum);
  EResultType Set_nB_GEN_NUM(int nNum);

  // Get value
  int GetMO_AR();
  int GetMO_AW();

  int GetPortNum(int nID);

  int Get_nRTrans();
  int Get_nBTrans();
  int Get_nR_GEN_NUM();
  int Get_nB_GEN_NUM();

  // Control
  EResultType UpdateState(int64_t nCycle);
  EResultType Reset();
  EResultType PrintStat(int64_t nCycle, FILE *fp);

  // Debug
  EResultType CheckLink();

  // Port slave interface
  CPTRx *cpRx_AR; // [NUM_PORT]
  CPTRx *cpTx_R;
  CPTRx *cpRx_AW;
  CPTRx *cpRx_W;
  CPTRx *cpTx_B;

  // Port master interface
  CPTRx cpTx_AR;
  CPTRx cpRx_R;
  CPTRx cpTx_AW;
  CPTRx cpTx_W;
  CPTRx cpRx_B;

#ifdef CCI_ON
  // CCI specific interface
  CPTRx *cpTx_AC; // [NUM_PORT]

  CPTRx *cpRx_CD; // [NUM_PORT]
  CPTRx *cpRx_CR; // [NUM_PORT]
#endif

private:
  // Original
  string cName;

  // Control
  int NUM_PORT; // Num port
  int BIT_PORT; // Bits port

  int nMO_AR; // MO count AR
  int nMO_AW;

  // Stat
  int *nAR_SI; // [NUM_PORT]
  int *nAW_SI;
  int *nR_SI;
  int *nB_SI;

  uint32_t nRTrans;
  uint32_t nBTrans;
  uint32_t nR_GEN_NUM;
  uint32_t nB_GEN_NUM;

  uint32_t nCentralStall;
  uint32_t nACStall;
  uint32_t nWaitResp;

  int nSnoopCnt;

  // Arbiter
  CPArb cpArb_AR;
  CPArb cpArb_AW;

  // FIFO
  // CPFIFO  cpFIFO_AR;
  // CPFIFO  cpFIFO_R;
  CPFIFO cpFIFO_AW;
  // CPFIFO  cpFIFO_W;
  // CPFIFO  cpFIFO_B;

#ifdef CCI_ON
  CPFIFO *cpFIFO_MstAW; // [NUM_PORT]
  CPFIFO *cpFIFO_MstAR; // [NUM_PORT]

  //=======================================================
  // Purpose:
  //    cpFIFO_SnoopReq: Storing the snooping request to the snooped master.
  //    cpFIFO_SnoopData: Storing the returned data from snooped master.
  //    cpFIFO_SnoopResp: Storing the returned response from snooped master.
  //    cpFIFO_Central: Unified FIFO for tracking active snoop transactions (Ax
  // and AC).
  //=======================================================
  CPFIFO cpFIFO_SnoopReq;
  CPFIFO cpFIFO_SnoopData;
  CPFIFO *cpFIFO_SnoopResp;
  CPFIFO cpFIFO_Central;
  int *nSnoopedMaster;
  bool *bArb; // Round-robin for CCI snoop
  std::vector<int> m_outstandingMemARID;
  std::vector<int> m_outstandingMemAWID;
  std::vector<int64_t> m_outstandingMemARAddr;
  std::vector<int64_t> m_outstandingMemAWAddr;
#endif
};

#endif
