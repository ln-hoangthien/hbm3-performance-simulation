//-----------------------------------------------------------
// FileName	: CBank.cpp
// Version	: 0.81
// Date		: 18 Nov 2019
// Contact	: JaeYoung.Hur@gmail.com
//-----------------------------------------------------------
// Description	: Memory bank FSM class definition
//-----------------------------------------------------------
// Refer to timing diagram
//-----------------------------------------------------------

//--------------------------------------------------
// nCnt_RP      PRE2ACT. Precharging. Auto state change
//              Min 0, Max tRP
//              At PRE, start increase
//              At ACT, reset 0
//--------------------------------------------------
// nCnt_RAS     ACT2PRE
//              Min 0, Max any
//              At ACT, start increase
//              At PRE, reset 0
//--------------------------------------------------
// nCnt_RCD     ACT2RD. ACT2WR. Activating. Auto state change
//              Min 0, Max tRCD-1
//              At ACT,    start increase
//              At tRCD-1, reset 0
//--------------------------------------------------
// nCnt_RTP     RD2PRE
//              Min 0, Max any
//              At RD,  start increase
//              At PRE, reset 0
//--------------------------------------------------
// nCnt_WR      WR2PRE
//              Min 0, Max any
//              At WR,  start increase
//              At PRE, reset 0
//--------------------------------------------------
// nCnt_RL      RD2DATA
//              Min 0, Max tRL
//              At RD,  start increase
//              At tRL, reset 0
//--------------------------------------------------
// nCnt_WL      WR2DATA
//              Min 0, Max tWL
//              At RD,  start increase
//              At tWL, reset 0
//--------------------------------------------------
// nCnt_CCD     RD2RD. WR2WR.
//              Min 0, Max tCCD
//              At RD/WR, start increase
//              At tCCD,  reset
//-----------------------------------------------------------

#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "CBank.h"

// Construct
CBank::CBank(string cName) {

  // Initialize
  this->cName = cName;

  this->eMemCmd = EMEM_CMD_TYPE_UNDEFINED;
  this->nBankCmd = -1;
  this->nRowCmd = -1;

  this->eMemState = EMEM_STATE_TYPE_IDLE;
  this->nActivatedRow = -1;

  this->nCnt_RP = -1;                       // PRE2ACT. Precharging.
  this->nCnt_RAS = -1;                      // ACT2PRE
  this->nCnt_RCD = -1;                      // ACT2RD. ACT2WR. Activating.
  this->nCnt_RTP = -1;                      // RD2PRE
  this->nCnt_WR = -1;                       // WR2PRE
  this->nCnt_RL = -1;                       // RD2DATA
  this->nCnt_WL = -1;                       // WR2DATA
  this->nCnt_CCD = -1;                      // RD2RD. WR2WR.
  this->nCnt_RFCpb = -1;                    // PER BANK REFRESH command period (same bank)
  this->nCnt_REFIab = -1;                   // PER BANK REFRESH command period (different bank)
  this->ongoing_read = -1;                  // number of ongoing read commands
  this->ongoing_write = -1;                 // number of ongoing write commands
  this->IsBankPrepared_r = ERESULT_TYPE_NO; // Bank prepared (activated, not yet RD/WR)
  this->nCnt_Data = -1;
  this->firstRefresh = false;
};

// Destruct
CBank::~CBank(){
    // Do Nothing
};

// Reset
EResultType CBank::Reset() {

  this->eMemCmd = EMEM_CMD_TYPE_NOP;
  this->nBankCmd = -1;
  this->nRowCmd = -1;
  this->eMemState = EMEM_STATE_TYPE_IDLE;
  this->nActivatedRow = -1;
  this->nCnt_RP = tRP;     // PRE2ACT. Initially in cycle 1, bank state idle (or
                           // precharge finished)
  this->nCnt_RAS = 0;      // ACT2PRE
  this->nCnt_RCD = 0;      // ACT2RD, ACT2WR. Activating.
  this->nCnt_RTP = 0;      // RD2PRE
  this->nCnt_WR = 0;       // WR2PRE
  this->nCnt_RL = 0;       // RD2DATA
  this->nCnt_WL = 0;       // WR2DATA
  this->nCnt_CCD = 0;      // RD2RD, WR2WR
  this->nCnt_RFCpb = 0;    // PER BANK REFRESH command period (same bank)
  this->nCnt_REFIab = 0;   // PER BANK REFRESH command period (different bank)
  this->ongoing_read = 0;  // number of ongoing read commands
  this->ongoing_write = 0; // number of ongoing write commands
  this->IsBankPrepared_r = ERESULT_TYPE_NO;
  this->nCnt_Data = 0;
  this->firstRefresh = false;

  return (ERESULT_TYPE_SUCCESS);
};

// Set mem cmd
EResultType CBank::SetMemCmd(EMemCmdType eCmd) {

  this->eMemCmd = eCmd;
  return (ERESULT_TYPE_SUCCESS);
};

// Set mem addr
EResultType CBank::SetMemAddr(int nBank, int nRow) {

  this->nBankCmd = nBank;
  this->nRowCmd = nRow;
  return (ERESULT_TYPE_SUCCESS);
};

// Get Mem name
string CBank::GetName() { return (this->cName); };

// Get state
EMemStateType CBank::GetMemState() { return (this->eMemState); };

// Get activated row
int CBank::GetActivatedRow() { return (this->nActivatedRow); };

//--------------------------------------
// This bank can get RD cmd
//--------------------------------------
EResultType CBank::IsRD_ready(int nCnt_CCD_max) {

  // Check state
  if (this->eMemState != EMEM_STATE_TYPE_ACTIVE && this->eMemState != EMEM_STATE_TYPE_ACTIVE_for_READ) {
    return (ERESULT_TYPE_NO);
  };

#ifdef DEBUG
  assert(this->eMemState == EMEM_STATE_TYPE_ACTIVE or this->eMemState == EMEM_STATE_TYPE_ACTIVE_for_READ);
#endif

  // Check counter
  /*
  if (this->nCnt_CCD == 0 or 					// No on-going
  RD this->nCnt_CCD >= nCnt_CCD_max) {		// On-going RD

          #ifdef DEBUG
          assert (this->GetActivatedRow() != -1);
          #endif

          return (ERESULT_TYPE_YES);
  };*/

  // return (ERESULT_TYPE_NO);
  return (ERESULT_TYPE_YES);
};

//--------------------------------------
// This bank can get WR cmd
//--------------------------------------
EResultType CBank::IsWR_ready(int nCnt_CCD_max) {

  // Check state
  if (this->eMemState != EMEM_STATE_TYPE_ACTIVE && this->eMemState != EMEM_STATE_TYPE_ACTIVE_for_WRITE) {
    return (ERESULT_TYPE_NO);
  };

#ifdef DEBUG
  assert(this->eMemState == EMEM_STATE_TYPE_ACTIVE or this->eMemState == EMEM_STATE_TYPE_ACTIVE_for_WRITE);
#endif

  // Check counter
  /*
  if (this->nCnt_CCD == 0 or 					// No on-going
  RD this->nCnt_CCD >= nCnt_CCD_max) {		// On-going RD

          #ifdef DEBUG
          assert (this->GetActivatedRow() != -1);
          #endif

          return (ERESULT_TYPE_YES);
  };*/

  // return (ERESULT_TYPE_NO);
  return (ERESULT_TYPE_YES);
};

//--------------------------------------
// Check data channel busy
//	Be careful timing.
//	nCnt_Data UpdateState here and use next cycle.
//	This function should be called every cycle.
//--------------------------------------
//	nCnt_Data	Min 0, Max BURST LENGHTH
//			At IsFirstData_ready, set 2 for next cycle.
//			At Cnt < BURSTLENGTH, increase
//			At BURSTLENGTGH, reset 0
//--------------------------------------
EResultType CBank::IsData_busy() {

  // Check data channel on-going
  if (this->nCnt_Data > 1 and this->nCnt_Data < MAX_BURST_LENGTH) { // On going
    this->nCnt_Data++;                                              // Update for next cycle
    return (ERESULT_TYPE_YES);
  };

  // Check finish
  if (this->nCnt_Data == MAX_BURST_LENGTH) { // Finish in this cycle
    this->nCnt_Data = 0;                     // No data in next cycle
    return (ERESULT_TYPE_YES);
  };

  // Check 1st data
  if (this->IsFirstData_ready() == ERESULT_TYPE_YES) { // First data in this cycle
    this->nCnt_Data = 2;                               // 2nd data in next cycle
    return (ERESULT_TYPE_YES);
  };

#ifdef DEBUG
  assert(this->nCnt_Data == 0);
#endif

  return (ERESULT_TYPE_NO);
};

//---------------------------------------------
// Check this bank can put data
//---------------------------------------------
// Assert 1 cycle
//  At RD/WR, set
//	Otherwise, reset (See UpdateState)
//---------------------------------------------
EResultType CBank::IsFirstData_ready() {

  // Check counter
  if (this->nCnt_RL == tRL) { // RD2DATA
    // printf("The First READ Data is READY at %s nCnt_RL %d, ongoing = %d\n",
    // this->cName.c_str(), this->nCnt_RL, this->ongoing_read);
    return (ERESULT_TYPE_YES);
  };

  // Check counter
  if (this->nCnt_WL == tWL) { // WR2DATA
    // printf("The First WRITE Data is READY at %s nCnt_WL %d, ongoing = %d\n",
    // this->cName.c_str(), this->nCnt_WL, this->ongoing_write);
    return (ERESULT_TYPE_YES);
  };

  return (ERESULT_TYPE_NO);
};

//---------------------------------------------
// Check this bank is prepared
//---------------------------------------------
//	At activation, set
//	At RD/WR/PRE, reset
//---------------------------------------------
EResultType CBank::IsBankPrepared() {

  if (this->IsBankPrepared_r == ERESULT_TYPE_YES) {
    return (ERESULT_TYPE_YES);
  } else {
    return (ERESULT_TYPE_NO);
  };
};

//--------------------------------------
// This bank can get PRE
//--------------------------------------
EResultType CBank::IsPRE_ready() {

  // Check state
  if (this->eMemState != EMEM_STATE_TYPE_ACTIVE && this->eMemState != EMEM_STATE_TYPE_ACTIVE_for_READ &&
      this->eMemState != EMEM_STATE_TYPE_ACTIVE_for_WRITE) {
    return (ERESULT_TYPE_NO);
  };

#ifdef DEBUG
  assert(this->eMemState == EMEM_STATE_TYPE_ACTIVE or this->eMemState == EMEM_STATE_TYPE_ACTIVE_for_READ or
         this->eMemState == EMEM_STATE_TYPE_ACTIVE_for_WRITE);
#endif

  // Check counter
  if (this->nCnt_RAS >= tRAS and                          // ACT2PRE
      (this->nCnt_RTP == 0 or this->nCnt_RTP >= tRTP) and // RD2PRE. When RD, cnt increases. When no RD, cnt 0
      (this->nCnt_WR == 0 or this->nCnt_WR >= tWR)) {     // WR2PRE. When WR, cnt increases. When no WR,
                                                          // cnt 0. FIXME check back-to-back write. Other
                                                          // master read access same bank

#ifdef DEBUG
    assert(this->GetActivatedRow() != -1);
#endif

    return (ERESULT_TYPE_YES);
  };

  return (ERESULT_TYPE_NO);
};

//--------------------------------------
// This bank can get ACT
//--------------------------------------
EResultType CBank::IsACT_ready() {

  // Check state
  if (this->eMemState == EMEM_STATE_TYPE_IDLE and this->nCnt_ACT_cmd <= 4 and
      this->nCnt_RFCpb == 0) { // No refresh and no more than 4 ACT in tFAW

#ifdef DEBUG
    assert(this->GetActivatedRow() == -1);
#endif

    return (ERESULT_TYPE_YES);
  };

  return (ERESULT_TYPE_NO);
};

//--------------------------------------
// This bank can get REFpb
//--------------------------------------
EResultType CBank::IsREF_ready() {

  // Check state
  if (this->eMemState == EMEM_STATE_TYPE_IDLE and this->nCnt_RFCpb == 0) { // No refresh and no more than 4 ACT in tFAW

#ifdef DEBUG
    assert(this->GetActivatedRow() == -1);
#endif

    return (ERESULT_TYPE_YES);
  };

  return (ERESULT_TYPE_NO);
};

//--------------------------------------
// Force the MC issuing a PRE command
//--------------------------------------
EResultType CBank::forced_PRE() {

#ifdef PRECHARGE
  // Check state
  if (this->nCnt_RAS >= static_cast<int>(std::ceil(tRAS_max * 0.95))) { // The nCnt_RAS > 0 only if it is activated

#ifdef DEBUG
    assert(this->GetActivatedRow() != -1);
#endif
    return (ERESULT_TYPE_YES);
  };
#endif

  return (ERESULT_TYPE_NO);
};

//--------------------------------------
// Force the MC issuing a PRE command
//--------------------------------------
EResultType CBank::forced_REFI() {

#ifdef REFRESH
  // Check state
  if (this->firstRefresh == false) {

    if (this->nCnt_REFIab >= tREFIpb) { // The nCnt_REFIab > 0 only if it is activated

      return (ERESULT_TYPE_YES);
    };

  } else {
    if (this->nCnt_REFIab >= static_cast<int>(std::ceil(tREFI * 0.85))) { // The nCnt_REFIab > 0 only if it is activated

      return (ERESULT_TYPE_YES);
    };
  }
#endif

  return (ERESULT_TYPE_NO);
};

//--------------------------------------
// Update state
//--------------------------------------
// 	Update in the order of State, IsBankPrepared_r, Counter
//  Modeling DRAM operation as a FSM with states and counters
//--------------------------------------
//	nCnt_RP   PRE2ACT           auto state Precharging
//	nCnt_RAS  ACT2PRE           min
//	nCnt_RCD  ACT2RD. ACT2WR.   auto state Activating
//	nCnt_RTP  RD2PRE            min
//	nCnt_WR   WR2PRE            min
//	nCnt_RL   RD2DATA           min (related to Reading)
//	nCnt_WL   WR2DATA           min (related to Writing state)
//	nCnt_CCD  RD2RD. WR2WR.     min
//--------------------------------------
EResultType CBank::UpdateState() {

#ifdef DEBUG
  // this->CheckCnt();
  this->CheckCmdReady();
  assert(this->nCnt_ACT_cmd <= 4); // Always Check Condition - tFAW

#ifdef PRECHARGE
  assert(this->nCnt_RAS <= tRAS_max); // Always Check Condition - ACT2PRE
#endif

#ifdef REFRESH
  assert(this->nCnt_REFIab <= tREFI);
#endif

#endif

  //------------------------------------
  // State, counter
  //------------------------------------
  if (this->eMemState == EMEM_STATE_TYPE_IDLE) {

#ifdef DEBUG
    assert(this->eMemCmd == EMEM_CMD_TYPE_ACT or this->eMemCmd == EMEM_CMD_TYPE_NOP or
           this->eMemCmd == EMEM_CMD_TYPE_REFpb); // Only allow for receiving the ACT or NOP
    assert(this->nActivatedRow == -1);            // No row is activated
    assert(this->nCnt_RCD == 0);                  // (Start increase) ACT2RD. ACT2WR: No-Operation
    assert(this->nCnt_RAS == 0);                  // (Start increase) ACT2PRE: No-Operation
    assert(this->nCnt_RL == 0);                   // RD2DATA: No-Operation
    assert(this->nCnt_WL == 0);                   // WR2DATA: No-Operation
    assert(this->nCnt_CCD == 0);                  // RD2RD. WR2WR: No-Operation
    assert(this->nCnt_RTP == 0);                  // RD2PRE: No-Operation
    assert(this->nCnt_WR == 0);                   // WR2PRE: No-Operation
    assert(this->nCnt_RP == tRP);                 // (Reset) PRE2ACT. Precharging: No-Operation
#endif

    if (this->eMemCmd == EMEM_CMD_TYPE_ACT) {

      this->eMemState = EMEM_STATE_TYPE_ACTIVATING;

#ifdef DEBUG
      assert(this->nCnt_RC == 0); // (Reset) ACT2ACT.
#endif
    } else if (this->eMemCmd == EMEM_CMD_TYPE_NOP) {

      this->eMemState = EMEM_STATE_TYPE_IDLE;
    } else if (this->eMemCmd == EMEM_CMD_TYPE_REFpb) {

      this->firstRefresh = true;
      this->eMemState = EMEM_STATE_TYPE_REFRESHING;

    } else {
      assert(0);
    };
  } else if (this->eMemState == EMEM_STATE_TYPE_ACTIVATING) {

#ifdef DEBUG // Assert state and counter
    assert(this->eMemCmd == EMEM_CMD_TYPE_NOP);
    assert(this->nActivatedRow == -1);
    assert(this->nCnt_RCD > 0 or this->nCnt_RCD < tRCD); // (Min) ACT2RD, ACT2WR	: The counter
                                                         // still increase when the Bank is ativating
                                                         // and reseted when it reach the max value.
    assert(this->nCnt_RAS > 0 or this->nCnt_RAS < tRAS); // (Min) ACT2PRE 		: The counter
                                                         // still increase when the Bank is ativating.
    assert(this->nCnt_RC > 0 or this->nCnt_RC < tRC);    // (Reset) ACT2ACT		: The counter
                                                         // still increase when the Bank is ativating.
    assert(this->nCnt_RL == 0);                          // RD2DATA				: No-Operation
    assert(this->nCnt_WL == 0);                          // WR2DATA				: No-Operation
    assert(this->nCnt_CCD == 0);                         // RD2RD, WR2WR			:
                                                         // No-Operation
    assert(this->nCnt_RTP == 0);                         // RD2PRE				: No-Operation
    assert(this->nCnt_WR == 0);                          // WR2PRE				: No-Operation
    assert(this->nCnt_RP == 0);                          // PRE2ACT				: No-Operation
#endif

    if (this->nCnt_RCD == tRCDRD) { // This cycle last cycle activating for READ.
      this->eMemState = EMEM_STATE_TYPE_ACTIVE_for_READ;
      this->nActivatedRow = this->nRowCmd;
    } else if (this->nCnt_RCD == tRCDWR) { // This cycle last cycle activating for WRITE.
      this->eMemState = EMEM_STATE_TYPE_ACTIVE_for_WRITE;
      this->nActivatedRow = this->nRowCmd;
    } else if (this->nCnt_RCD < std::min(tRCDRD, tRCDWR)) {
      // Do nothing, keep activating
    } else {
      assert(0);
    };
  } else if (this->eMemState == EMEM_STATE_TYPE_ACTIVE || this->eMemState == EMEM_STATE_TYPE_ACTIVE_for_READ ||
             this->eMemState == EMEM_STATE_TYPE_ACTIVE_for_WRITE) {

#ifdef DEBUG // Assert state and counter
    assert(this->eMemCmd == EMEM_CMD_TYPE_RD or this->eMemCmd == EMEM_CMD_TYPE_WR or
           this->eMemCmd == EMEM_CMD_TYPE_PRE or this->eMemCmd == EMEM_CMD_TYPE_NOP);

    assert(this->nCnt_RCD > 0 or this->nCnt_RCD < tRCD);   // (Min) ACT2RD, ACT2WR	: Continue counting the time for
                                                           // ACT2RD, ACT2WR in case one of them is not finished.
    assert(this->nCnt_RAS > 0 or this->nCnt_RAS < tRAS);   // ACT2PRE				:
                                                           // Continue counting the time for ACT2PRE
    assert(this->nCnt_RC > 0 or this->nCnt_RC < tRC);      // (Reset) ACT2ACT		: Continue
                                                           // counting the time for ACT2ACT
    assert(this->nCnt_RL == 0 or this->nCnt_RL <= tRL);    // RD2DATA				: Start
                                                           // counter for Read Latency
    assert(this->nCnt_WL == 0 or this->nCnt_WL <= tWL);    // WR2DATA				: Start
                                                           // counter for Write Latency
    assert(this->nCnt_CCD == 0 or this->nCnt_CCD <= tCCD); // RD2RD, WR2WR			: Start
                                                           // counter for CAS to CAS command delay
    assert(this->nCnt_RP == 0);                            // PRE2ACT				: No-Operation
    if (this->eMemCmd != EMEM_CMD_TYPE_PRE)
      assert(this->nActivatedRow == this->nRowCmd); // NON-PRECHARGE ONLY	: PRECHARGE per bank do
                                                    // not require the ROW address.
#endif

    if (this->eMemCmd == EMEM_CMD_TYPE_RD &&
        (this->eMemState == EMEM_STATE_TYPE_ACTIVE_for_READ || this->eMemState == EMEM_STATE_TYPE_ACTIVE)) {

      // Update state
      // this->eMemState = EMEM_STATE_TYPE_ACTIVE;

#ifdef DEBUG
      // (Min) ACT2RD: tRCDRD is reached. The DRAM bank is able to receive the
      // READ.
      assert(this->nCnt_RCD == 0 or this->nCnt_RCD >= tRCDRD);
#endif
    } else if (this->eMemCmd == EMEM_CMD_TYPE_WR &&
               (this->eMemState == EMEM_STATE_TYPE_ACTIVE_for_WRITE || this->eMemState == EMEM_STATE_TYPE_ACTIVE)) {

      // Update state
      // this->eMemState = EMEM_STATE_TYPE_ACTIVE;

#ifdef DEBUG
      // (Min) ACT2WR: tRCDWR is reached. The DRAM bank is able to receive the
      // WRITE.
      assert(this->nCnt_RCD == 0 or this->nCnt_RCD >= tRCDWR);
#endif
    } else if (this->eMemCmd == EMEM_CMD_TYPE_PRE) {

      // Updtae state
      this->eMemState = EMEM_STATE_TYPE_PRECHARGING;
      this->nActivatedRow = -1;

#ifdef DEBUG
      // The DRAM bank is able to receive the PRECHARGE.
      assert(this->nCnt_RAS >= tRAS);
      assert(this->nCnt_RTP >= 0 or this->nCnt_RTP >= tRTP);
      assert(this->nCnt_WR == 0 or this->nCnt_WR >= tWR);
#endif

    } else if (this->nCnt_RCD == tRCDRD) { // The DRAM is ready to receive READ command.

      if (this->eMemState == EMEM_STATE_TYPE_ACTIVE_for_WRITE) {
        this->eMemState = EMEM_STATE_TYPE_ACTIVE; // Fully Active state
      } else {
        this->eMemState = EMEM_STATE_TYPE_ACTIVE_for_READ; // Active for READ-only state
      }
      this->nActivatedRow = this->nRowCmd;

#ifdef DEBUG
      // (Min) ACT2RD: tRCDRD is reached. The DRAM bank is able to receive the
      // READ.
      assert(this->nCnt_RCD == 0 or this->nCnt_RCD >= tRCDRD);
#endif

    } else if (this->nCnt_RCD == tRCDWR) { // The DRAM is ready to receive WRITE command.

      if (this->eMemState == EMEM_STATE_TYPE_ACTIVE_for_READ) {
        this->eMemState = EMEM_STATE_TYPE_ACTIVE; // Fully Active state
      } else {
        this->eMemState = EMEM_STATE_TYPE_ACTIVE_for_WRITE; // Active for WRITE-only state
      }
      this->nActivatedRow = this->nRowCmd;

#ifdef DEBUG
      // (Min) ACT2WR: tRCDWR is reached. The DRAM bank is able to receive the
      // WRITE.
      assert(this->nCnt_RCD == 0 or this->nCnt_RCD >= tRCDWR);
#endif
    } else if (this->nCnt_RCD == 0) { // This cycle last cycle activating

      this->eMemState = EMEM_STATE_TYPE_ACTIVE; // Fully Active state
      this->nActivatedRow = this->nRowCmd;

#ifdef DEBUG
      // (Min) ACT2RD, ACT2WR: tRCDRD or tRCDWR is reached. The DRAM bank is
      // able to receive the READ or WRITE.
      assert(this->nCnt_RCD == 0 or this->nCnt_RCD >= tRCDRD);
      assert(this->nCnt_RCD == 0 or this->nCnt_RCD >= tRCDWR);
#endif
    } else if (this->eMemCmd == EMEM_CMD_TYPE_NOP) {
      // Do nothing, keep activating
    } else {
      assert(0);
    };
  } else if (this->eMemState == EMEM_STATE_TYPE_PRECHARGING) {

#ifdef DEBUG
    assert(this->eMemCmd == EMEM_CMD_TYPE_NOP);
    assert(this->nActivatedRow == -1);                  // No activated row
    assert(this->nCnt_RCD == 0);                        // (Min) ACT2RD, ACT2WR	: No-Operation
    assert(this->nCnt_RAS == 0);                        // (Min) ACT2PRE		: Finished.
                                                        // Successfully receive PRE command.
    assert(this->nCnt_RC >= 0 or this->nCnt_RC >= tRC); // (Reset) ACT2ACT      : Continue uncreasing
                                                        // the counter for ACT2ACT
    assert(this->nCnt_RL == 0);                         // RD2DATA				: No-Operation
    assert(this->nCnt_WL == 0);                         // WR2DATA				: No-Operation
    assert(this->nCnt_CCD == 0);                        // (Min) RD2RD, WR2WR	: No-Operation
    assert(this->nCnt_RTP == 0);                        // RD2PRE				:
                                                        // Finished. Successfully receive PRE command.
    assert(this->nCnt_WR == 0);                         // WR2PRE				:
                                                        // Finished. Successfully receive PRE command.
    assert(this->nCnt_RP > 0);                          // PRE2ACT				: Precharging
#endif

    if (this->nCnt_RP < tRP) { // PRE2ACT. Precharge on going

      this->eMemState = EMEM_STATE_TYPE_PRECHARGING;

    } else if (this->nCnt_RP == tRP) { // This cycle last cycle precharge

      this->eMemState = EMEM_STATE_TYPE_IDLE;

    } else {
      assert(0);
    };
  } else if (this->eMemState == EMEM_STATE_TYPE_REFRESHING) {

#ifdef DEBUG
    assert(this->eMemCmd == EMEM_CMD_TYPE_NOP);
    assert(this->nActivatedRow == -1); // No row is activated
    assert(this->nCnt_RCD == 0);       // (Start increase) ACT2RD. ACT2WR: No-Operation
    assert(this->nCnt_RAS == 0);       // (Start increase) ACT2PRE: No-Operation
    assert(this->nCnt_RL == 0);        // RD2DATA: No-Operation
    assert(this->nCnt_WL == 0);        // WR2DATA: No-Operation
    assert(this->nCnt_CCD == 0);       // RD2RD. WR2WR: No-Operation
    assert(this->nCnt_RTP == 0);       // RD2PRE: No-Operation
    assert(this->nCnt_WR == 0);        // WR2PRE: No-Operation
    assert(this->nCnt_RP == tRP);      // (Reset) PRE2ACT. Precharging: No-Operation
    assert(this->nCnt_RC == 0);        // (Reset) ACT2ACT.
#endif

    if (this->nCnt_RFCpb < tRFCpb) {

      this->eMemState = EMEM_STATE_TYPE_REFRESHING;

    } else if (this->nCnt_RFCpb == tRFCpb) { // This cycle last cycle precharge

      this->eMemState = EMEM_STATE_TYPE_IDLE;

    } else {
      assert(0);
    };

  } else {
    assert(0);
  }

  //--------------------
  // IsBankPrepared_r
  //--------------------
  // 	Register.
  // 	Need this to indicate bank prepared (activated) and not yet RD/WR
  //--------------------
  // 	Note: Update this before counter
  //--------------------
  //	Initially, 0
  //	At nCnt_RCD == tRCD-1, set
  //	At PRE, reset
  //	At RD/WR, reset
  //--------------------
  if (this->IsBankPrepared_r == ERESULT_TYPE_NO and
      this->nCnt_RCD == std::max(tRCDRD, tRCDWR)) { // Now activated. Not yet RD/WR
    this->IsBankPrepared_r = ERESULT_TYPE_YES;
  } else if (this->eMemCmd == EMEM_CMD_TYPE_RD) {
    this->IsBankPrepared_r = ERESULT_TYPE_NO;
  } else if (this->eMemCmd == EMEM_CMD_TYPE_WR) {
    this->IsBankPrepared_r = ERESULT_TYPE_NO;
  } else if (this->eMemCmd == EMEM_CMD_TYPE_PRE) {
    this->IsBankPrepared_r = ERESULT_TYPE_NO;
  };

  //--------------------------------------------------
  // Counter
  // 	Counter value is dependent on cmd and counter value
  //--------------------------------------------------
  //--------------------------------------------------
  // Accumulative ACT commands
  //--------------------------------------------------
  //	Initially, 0
  //	Min 0, Max 4
  //	At ACT,    start increase
  //	At tFAW-1, reset 0
  //--------------------------------------------------
  if (this->eMemCmd == EMEM_CMD_TYPE_ACT and this->nCnt_ACT_cmd == 0) {
    this->nCnt_FAW = 1;                                       // Start increase
  } else if (this->nCnt_FAW == tFAW) {                        // FAW window finish
    this->nCnt_FAW = 0;                                       // Reset
    this->nCnt_ACT_cmd = 0;                                   // Reset
  } else if (this->nCnt_FAW >= 1 and this->nCnt_FAW < tFAW) { // FAW window on-going
    this->nCnt_FAW++;                                         // Increase
  };

  if (this->eMemCmd == EMEM_CMD_TYPE_ACT) {
    this->nCnt_ACT_cmd++; // Start increase
  }

  //--------------------------------------------------
  // RCD (ACT2RD. ACT2WR) Activating. Auto state change
  //--------------------------------------------------
  //	Initially, 0
  //	Min 0, Max tRCD-1
  //	At ACT,    start increase
  //	At tRCD-1, reset 0
  //--------------------------------------------------
  if (this->eMemCmd == EMEM_CMD_TYPE_ACT) {
    this->nCnt_RCD = 1;                                       // Start increase
  } else if (this->nCnt_RCD == tRCD) {                        // Activating finish
    this->nCnt_RCD = 0;                                       // Reset
  } else if (this->nCnt_RCD >= 1 and this->nCnt_RCD < tRCD) { // Activating on-going
    this->nCnt_RCD++;                                         // Increase
  };

  //--------------------------------------------------
  // RC (ACT2ACT)
  //--------------------------------------------------
  //	Initially, 0
  //	Min 0, Max any
  //	At ACT, start increase
  //	At tRC-1, reset 0
  //--------------------------------------------------
  if (this->eMemCmd == EMEM_CMD_TYPE_ACT) {
    this->nCnt_RC = 1; /// Start increase
  } else if (this->nCnt_RC == tRC) {
    this->nCnt_RC = 0; /// Reset
  } else if (this->nCnt_RC >= 1) {
    this->nCnt_RC++; /// Increase
  };

  //--------------------------------------------------
  // RP (PRE2ACT) Precharging. Auto state change
  //--------------------------------------------------
  //	Initially, tRP
  //	Min 0, Max tRP (Can issue a command at tRP)
  //	At PRE, start increase
  //	At ACT, reset 0
  //--------------------------------------------------
  if (this->eMemCmd == EMEM_CMD_TYPE_PRE) {
    this->nCnt_RP = 1; // Start increase
  } else if (this->eMemCmd == EMEM_CMD_TYPE_ACT) {
    this->nCnt_RP = 0;                                     // Reset
  } else if (this->nCnt_RP >= 1 and this->nCnt_RP < tRP) { // Precharging on-going
    this->nCnt_RP++;                                       // Increase
  };

  //--------------------------------------------------
  // RAS (ACT2PRE)
  //--------------------------------------------------
  //	Initially, 0
  //	Min 0, Max any
  //	At ACT, start increase
  //	At PRE, reset 0
  //--------------------------------------------------
  if (this->eMemCmd == EMEM_CMD_TYPE_ACT) {
    this->nCnt_RAS = 1; /// Start increase
  } else if (this->eMemCmd == EMEM_CMD_TYPE_PRE) {
    this->nCnt_RAS = 0; /// Reset
  } else if (this->nCnt_RAS >= 1) {
    this->nCnt_RAS++; /// Increase
  };

  //--------------------------------------------------
  // RTP (RD2PRE)
  //--------------------------------------------------
  //	Initially, 0
  //	Min 0, Max any
  //	At RD,  start increase
  //	At PRE, reset 0
  //--------------------------------------------------
  if (this->eMemCmd == EMEM_CMD_TYPE_RD) {
    this->nCnt_RTP = 1; // Start increase
  } else if (this->eMemCmd == EMEM_CMD_TYPE_PRE) {
    this->nCnt_RTP = 0;             // Reset
  } else if (this->nCnt_RTP >= 1) { // RD issued
    this->nCnt_RTP++;               // Increase
  };

  //--------------------------------------------------
  // WR (WR2PRE)
  //--------------------------------------------------
  //	Initially, 0
  //	Min 0, Max any
  //	At WR,  start increase
  //	At PRE, reset 0
  //--------------------------------------------------
  if (this->eMemCmd == EMEM_CMD_TYPE_WR) {
    this->nCnt_WR = 1; // Start increase
  } else if (this->eMemCmd == EMEM_CMD_TYPE_PRE) {
    this->nCnt_WR = 0;             // Reset
  } else if (this->nCnt_WR >= 1) { // WR issued
    this->nCnt_WR++;               // Increase
  };

  //--------------------------------------------------
  // RL (RD2DATA)
  //--------------------------------------------------
  //	Initially, 0
  //	Min 0, Max tRL
  //	At RD, start increase
  //	At tRL, reset 0
  //--------------------------------------------------
  if ((this->nCnt_RL == tRL) && (ongoing_read == 1)) { // RD Conitued
    for (int i = 0; i < tRL; i++) {
      nCnt_RL_ongoing[i] = 0;
    }
    ongoing_read--;
  } else if ((this->nCnt_RL == tRL) && (ongoing_read > 1)) { // RD finished
    this->nCnt_RL = nCnt_RL_ongoing[1];                      // Reset
    for (int i = 0; i < ongoing_read; i++) {
      nCnt_RL_ongoing[i] = nCnt_RL_ongoing[i + 1];
    }
    ongoing_read--;
  } else if (this->nCnt_RL >= 1 and this->nCnt_RL < tRL) { // RD issued
    for (int i = 0; i < ongoing_read; i++) {
      nCnt_RL_ongoing[i]++;
    }
  };
  nCnt_RL = nCnt_RL_ongoing[0];

  if (this->eMemCmd == EMEM_CMD_TYPE_RD) {
    if (this->nCnt_RL == 0) {
      this->nCnt_RL_ongoing[0] = 1; // Start increase
      ongoing_read++;
    } else {
      this->nCnt_RL_ongoing[ongoing_read] = 1;
      ongoing_read++;
    }
    // printf("READ command issued at %s nCnt_RL %d, ongoing = %d\n",
    // this->cName.c_str(), this->nCnt_RL, this->ongoing_read);
  }
  nCnt_RL = nCnt_RL_ongoing[0];

#ifdef DEBUG
  assert(this->ongoing_read >= 0);
#endif

  //--------------------------------------------------
  // WL (WR2DATA)
  //--------------------------------------------------
  //	Initially, 0
  //	Min 0, Max tWL
  //	At WR, start increase
  //	At tWL, reset 0
  //--------------------------------------------------
  /*if ((this->nCnt_WL == tWL) && (ongoing_write == 1)) {		// WR
  Conitued this->nCnt_WL = 0;					// Reset
          ongoing_write --;
  }
  else if ((this->nCnt_WL == tWL) && (ongoing_write > 1)) {		// WR
  finished this->nCnt_WL = 1;					// Reset
          ongoing_write --;
  }
  else if (this->nCnt_WL >= 1 and this->nCnt_WL < tWL) {		// WR
  issued this->nCnt_WL ++;					// Increase
  };

  if (this->eMemCmd == EMEM_CMD_TYPE_WR) {
          if (this->nCnt_WL == 0) {
                  this->nCnt_WL = 1;					// Start
  increase ongoing_write ++;
          }
          else {
                  ongoing_write ++;
          }
  }*/

  if ((this->nCnt_WL == tWL) && (ongoing_write == 1)) { // RD Conitued
    for (int i = 0; i < tWL; i++) {
      nCnt_WL_ongoing[i] = 0;
    }
    ongoing_write--;
  } else if ((this->nCnt_WL == tWL) && (ongoing_write > 1)) { // RD finished
    this->nCnt_WL = nCnt_WL_ongoing[1];                       // Reset
    for (int i = 0; i < ongoing_write; i++) {
      nCnt_WL_ongoing[i] = nCnt_WL_ongoing[i + 1];
    }
    ongoing_write--;
  } else if (this->nCnt_WL >= 1 and this->nCnt_WL < tWL) { // RD issued
    for (int i = 0; i < ongoing_write; i++) {
      nCnt_WL_ongoing[i]++;
    }
  };
  nCnt_WL = nCnt_WL_ongoing[0];

  if (this->eMemCmd == EMEM_CMD_TYPE_WR) {
    if (this->nCnt_WL == 0) {
      this->nCnt_WL_ongoing[0] = 1; // Start increase
      ongoing_write++;
    } else {
      this->nCnt_WL_ongoing[ongoing_write] = 1;
      ongoing_write++;
    }
  }
  nCnt_WL = nCnt_WL_ongoing[0];

#ifdef DEBUG
  assert(this->ongoing_write >= 0);
#endif

  //--------------------------------------------------
  // CCD (RD2RD, WR2WR)
  //--------------------------------------------------
  //	Initially, 0
  //	Min 0, Max tCCD
  //	At RD/WR, start increase
  //	At tCCD,  reset 0
  //--------------------------------------------------
  if (this->eMemCmd == EMEM_CMD_TYPE_RD) {
    this->nCnt_CCD = 1; // Start increase
  } else if (this->eMemCmd == EMEM_CMD_TYPE_WR) {
    this->nCnt_CCD = 1;                                       // Start increase
  } else if (this->nCnt_CCD == tCCD) {                        // RD/WR finished
    this->nCnt_CCD = 0;                                       // Increase
  } else if (this->nCnt_CCD >= 1 and this->nCnt_CCD < tCCD) { // RD/WR issued
    this->nCnt_CCD++;                                         // Increase
  };

  //--------------------------------------------------
  // RFCpb (REFpb2REFpb or REFpb2ACT)
  //--------------------------------------------------
  //	Initially, 0
  //	Min 0, Max tRFCpb
  //	At REFpb, start increase
  //	At tRFCpb-1, reset 0
  //--------------------------------------------------
  if (this->eMemCmd == EMEM_CMD_TYPE_REFpb) {
    this->nCnt_RFCpb = 1; /// Start increase
  } else if (this->nCnt_RFCpb == tRFCpb) {
    this->nCnt_RFCpb = 0; /// Reset
  } else if (this->nCnt_RFCpb >= 1) {
    this->nCnt_RFCpb++; /// Increase
  };

  //--------------------------------------------------
  // tREFIpb - Average periodic refresh interval for REFRESH command for each
  // bank
  //--------------------------------------------------
  //	Initially, 0
  //	Min 0, Max tREFIpb
  //	Start increase at any time
  //	At tREFIpb - 1, reset 0
  //--------------------------------------------------
  if (this->eMemCmd == EMEM_CMD_TYPE_REFpb) {
    this->nCnt_REFIab = 0; /// Reset
  } else {
    this->nCnt_REFIab++; /// Increase
  };

  return (ERESULT_TYPE_SUCCESS);
};

//-------------------------------
// Check timing
// 	Use this before UpdateState
//-------------------------------
EResultType CBank::CheckCmdReady() {

  if (this->eMemState == EMEM_STATE_TYPE_IDLE) {

    assert(this->IsACT_ready() == ERESULT_TYPE_YES);
    assert(this->IsPRE_ready() == ERESULT_TYPE_NO);
    assert(this->IsRD_ready() == ERESULT_TYPE_NO);
    assert(this->IsWR_ready() == ERESULT_TYPE_NO);
  } else if (this->eMemState == EMEM_STATE_TYPE_ACTIVATING) {

    assert(this->IsACT_ready() == ERESULT_TYPE_NO);
    assert(this->IsPRE_ready() == ERESULT_TYPE_NO);
    assert(this->IsRD_ready() == ERESULT_TYPE_NO);
    assert(this->IsWR_ready() == ERESULT_TYPE_NO);
  } else if (this->eMemState == EMEM_STATE_TYPE_ACTIVE || this->eMemState == EMEM_STATE_TYPE_ACTIVE_for_READ ||
             this->eMemState == EMEM_STATE_TYPE_ACTIVE_for_WRITE) {

    assert(this->IsACT_ready() == ERESULT_TYPE_NO);
  } else if (this->eMemState == EMEM_STATE_TYPE_PRECHARGING) {

    assert(this->IsACT_ready() == ERESULT_TYPE_NO);
    assert(this->IsPRE_ready() == ERESULT_TYPE_NO);
    assert(this->IsRD_ready() == ERESULT_TYPE_NO);
    assert(this->IsWR_ready() == ERESULT_TYPE_NO);
  } else if (this->eMemState == EMEM_STATE_TYPE_REFRESHING) {

    assert(this->IsACT_ready() == ERESULT_TYPE_NO);
    assert(this->IsPRE_ready() == ERESULT_TYPE_NO);
    assert(this->IsRD_ready() == ERESULT_TYPE_NO);
    assert(this->IsWR_ready() == ERESULT_TYPE_NO);
  } else {
    assert(0);
  };

  return (ERESULT_TYPE_SUCCESS);
};

//-------------------------------------
// Check timing at this cycle
// 	Use this before UpdateState
// 	FIXME Check bug
//-------------------------------------
EResultType CBank::CheckCnt() {

  assert(this->nCnt_RCD >= 0 and this->nCnt_RCD < tRCD);  // ACT2RD. ACT2WR. Activating.
  assert(this->nCnt_RL >= 0 and this->nCnt_RL <= tRL);    // RD2DATA
  assert(this->nCnt_WL >= 0 and this->nCnt_WL <= tWL);    // WR2DATA
  assert(this->nCnt_CCD >= 0 and this->nCnt_CCD <= tCCD); // RD2RD. WR2WR.
  assert(this->nCnt_RTP >= 0);                            // RD2PRE. Max any
  assert(this->nCnt_WR >= 0);                             // WR2PRE. Max any
  assert(this->nCnt_RP >= 0 and this->nCnt_RP <= tRP);    // PRE2ACT. Precharging
  assert(this->nCnt_RAS >= 0);                            // ACT2PRE. Max any

  if (this->eMemState == EMEM_STATE_TYPE_IDLE) {

    assert(this->nCnt_RCD == 0);  // ACT2RD. ACT2WR. Activating.
    assert(this->nCnt_RL == 0);   // RD2DATA
    assert(this->nCnt_WL == 0);   // WR2DATA
    assert(this->nCnt_CCD == 0);  // RD2RD. WR2WR.
    assert(this->nCnt_RTP == 0);  // RD2PRE
    assert(this->nCnt_WR == 0);   // WR2PRE
    assert(this->nCnt_RP == tRP); // PRE2ACT. Precharging
    assert(this->nCnt_RAS == 0);  // ACT2PRE
    assert(this->nActivatedRow == -1);
  } else if (this->eMemState == EMEM_STATE_TYPE_ACTIVATING) {

    assert(this->nCnt_RCD > 0 and this->nCnt_RCD < tRCD); // ACT2RD. ACT2WR. Activating.
    assert(this->nCnt_RL == 0);                           // RD2DATA
    assert(this->nCnt_WL == 0);                           // WR2DATA
    assert(this->nCnt_CCD == 0);                          // RD2RD. WR2WR.
    assert(this->nCnt_RTP == 0);                          // RD2PRE
    assert(this->nCnt_WR == 0);                           // WR2PRE
    assert(this->nCnt_RP == 0);                           // PRE2ACT. Precharging
    assert(this->nCnt_RAS > 0 and this->nCnt_RAS < tRAS); // ACT2PRE
    assert(this->nActivatedRow == -1);
  } else if (this->eMemState == EMEM_STATE_TYPE_ACTIVE) {

    assert(this->nCnt_RCD == 0); // ACT2RD. ACT2WR. Activating.
    assert(this->nActivatedRow >= 0);
  } else if (this->eMemState == EMEM_STATE_TYPE_PRECHARGING) {

    assert(this->nCnt_RCD == 0); // ACT2RD. ACT2WR. Activating.
    // assert (this->nCnt_RL  == 0);			// RD2DATA. FIXME
    // irrelavnat assert (this->nCnt_WL  == 0);			// WR2DATA.
    // FIXME irrelavnat assert (this->nCnt_CCD == 0);			//
    // RD2RD. WR2WR.
    assert(this->nCnt_RTP == 0);                       // RD2PRE
    assert(this->nCnt_WR == 0);                        // WR2PRE
    assert(this->nCnt_RP > 0 and this->nCnt_RP < tRP); // PRE2ACT. Precharging
    assert(this->nCnt_RAS == 0);                       // ACT2PRE
    assert(this->nActivatedRow == -1);
  } else {
    assert(0);
  };

  return (ERESULT_TYPE_SUCCESS);
};

// Debug
EResultType CBank::Display() { return (ERESULT_TYPE_SUCCESS); };
