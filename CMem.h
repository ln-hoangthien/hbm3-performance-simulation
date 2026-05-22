//------------------------------------------------------------
// FileName	: CMem.h
// Version	: 0.7
// DATE 	: 3 Mar 2018
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Memory wrapper header
//------------------------------------------------------------
#ifndef CMem_H
#define CMem_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "CBank.h"
#include "Memory.h"
#include "Top.h"

using namespace std;

//---------------------------
// Mem bank class
//---------------------------
typedef class CMem *CPMem;
class CMem {

public:
  // 1. Contructor and Destructor
  CMem(string cName);
  ~CMem();

  // 2. Control
  EResultType Reset();

  // Set value
  EResultType SetMemCmdPkt(EMemCmdType eMemCmd, int nBank, int nRow);
  EResultType SetMemCmdPkt(SPMemCmdPkt spMemCmdPkt);

  // Get value
  string GetName();

  SPMemStatePkt GetMemStatePkt();
  int Get_tCCD(int bank,
               int nbank); // Get CCD timing based on current configuration
  int Get_tRRD(int bank,
               int nbank); // Get RRD timing based on current configuration
  int Get_tWTR(int bank,
               int nbank); // Get WTR timing based on current configuration

  EResultType IsRD_global_ready(int bank);  // Global CCD (RD2RD)
  EResultType IsWR_global_ready(int bank);  // Global CCD (WR2WR)
  EResultType IsACT_global_ready(int bank); // Global RRD (ACT2ACT)
  EResultType IsREF_global_ready(int bank); // Global RREFD (PER BANK REFRESH command period)
  // EResultType	IsFirstData_global_ready();
  // EResultType	IsData_global_busy();

  // Control
  EResultType UpdateState();

  // Debug
  // EResultType	CheckCnt();
  EResultType Display();

  // Memory
  CPBank cpBank[BANK_NUM];

private:
  // Original info
  string cName; // Mem name

  // Cmd
  SPMemCmdPkt spMemCmdPkt; // cmd, addr

  // Mem status
  SPMemStatePkt spMemStatePkt; // state, ready (RD,ACT,PRE,DATA), ActivatedRow

  // Control state
  int nCnt_CCD[BANK_NUM / BanksPerBGroup]; // Bank Group. RD2RD. WR2WR
  int nCnt_RRD[BANK_NUM / BanksPerBGroup]; // Bank Group. ACT2ACT
  int nCnt_WTR[BANK_NUM / BanksPerBGroup]; // Bank Group. WR2RD
  int nCnt_RTW[BANK_NUM / BanksPerBGroup]; // Bank Group. RD2WR
  int nCnt_RREFD;                          // Bank. PER BANK REFRESH command period (different bank)
  std::vector<int> Refresh_Counter;        // Counter for refresh command interval

  // int		nCnt_RD2Data;
  // // Cycle counter to wait for data
};

#endif
