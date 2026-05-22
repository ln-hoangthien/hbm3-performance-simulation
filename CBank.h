//------------------------------------------------------------
// FileName	: CBank.h
// Version	: 0.8
// DATE 	: 9 May 2019
// Contact	: JaeYoung.Hur@gmail.com
//------------------------------------------------------------
// Description	: Memory bank FSM class header
//------------------------------------------------------------
#ifndef CBank_H
#define CBank_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "Memory.h"
#include "Top.h"

using namespace std;

// Mem bank class
typedef class CBank *CPBank;
class CBank {

public:
  // 1. Contructor and Destructor
  CBank(string cName);
  ~CBank();

  // 2. Control
  // Control
  EResultType Reset();

  // Set value
  EResultType SetMemCmd(EMemCmdType eMemCmd);
  EResultType SetMemAddr(int nBankCmd, int nRowCmd);

public:
  // Get value
  string GetName();
  EMemStateType GetMemState();
  int GetActivatedRow();

  EResultType IsRD_ready(int nCnt_CCD_max = tCCD); // This bank can get RD cmd.
  EResultType IsWR_ready(int nCnt_CCD_max = tCCD);
  EResultType IsPRE_ready();
  EResultType IsACT_ready();
  EResultType IsREF_ready();
  EResultType forced_PRE();
  EResultType forced_REFI();
  EResultType IsFirstData_ready(); // This bank can put first data.
  EResultType IsBankPrepared();    // This bank is activated.

  // Control
  EResultType UpdateState();

  // Debug
  EResultType CheckCmdReady();
  EResultType CheckCnt();
  EResultType Display();

  // Stat
  EResultType IsData_busy();

private:
  // Original
  string cName; // Bank name

  // Control cmd
  EMemCmdType eMemCmd; // Set by MC
  int nBankCmd;
  int nRowCmd;

  // Control state
  EMemStateType eMemState;
  int nActivatedRow; // Active row number

  int nCnt_RP;      // PRE2ACT.        Per-bank. Auto (Precharging)
  int nCnt_RC;      // ACT2ACT.
  int nCnt_RAS;     // ACT2PRE.        Per-bank (wait PRE).
  int nCnt_RCD;     // ACT2RD. ACT2WR. Per-bank. Auto (Activating).
  int nCnt_FAW;     // No more than 4 banks may be activated in a rolling tFAW
                    // window.
  int nCnt_ACT_cmd; // Number of ongoing activations in tFAW window
  int nCnt_RFCpb;   // Refreshing. Per-bank. Auto (Refreshing)
  int nCnt_REFIab;  // Average periodic refresh interval for REFRESH command for
                    // each bank.

  int nCnt_RTP; // RD2PRE.         Per-bank (wait PRE)
  int nCnt_WR;  // WR2PRE.         Per-bank (wait PRE)

  int nCnt_RL;              // RD2DATA.        Per-bank (wait data)  Also global
  int nCnt_RL_ongoing[tRL]; // number of ongoing read commands
  int ongoing_read;         // number of ongoing read commands
  int nCnt_WL;              // WR2DATA.        Per-bank (wait data)  Also global
  int nCnt_WL_ongoing[tWL]; // number of ongoing read commands
  int ongoing_write;        // number of ongoing read commands

  int nCnt_CCD; // RD2RD. WR2WR.   Per-bank (wait RD/WR) Also global
  bool firstRefresh;

  EResultType IsBankPrepared_r; // Bank prepared (PRE, ACT). Not yet RD/WR

  // Stat
  int nCnt_Data; // Data counter for transaction under service
};

#endif
