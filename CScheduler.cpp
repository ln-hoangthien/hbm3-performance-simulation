//-----------------------------------------------------------
// FileName	: CScheduler.cpp
// Version	: 0.82a
// Date		: 13 Mar 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Scheduler definition
// History
// ----2023.03.15: Duong support ARMv8
//-----------------------------------------------------------
// FIXME Bank preparation algorithm may need improvement
//-----------------------------------------------------------
// Version 0.7
//	PTW is higher priority than time-out
//-----------------------------------------------------------
// Version 0.76
//	Time-out is higher priority than PTW
//-----------------------------------------------------------
// Version 0.8
//	Aggressive PRE
//-----------------------------------------------------------
// Version 0.81
//	Optional aggressive PRE
//-----------------------------------------------------------
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "CScheduler.h"

// Construct
CScheduler::CScheduler(string cName) {

  // Generate and initialize
  this->cName = cName;

  // Initialize
  // this->cpQ_AR = NULL;
  // this->cpQ_AW = NULL;

  this->IsAR_priority = ERESULT_TYPE_UNDEFINED;
  this->IsAW_priority = ERESULT_TYPE_UNDEFINED;
};

// Destruct
CScheduler::~CScheduler(){

    // delete (this->cpQ_AR);
    // delete (this->cpQ_AW);
    // this->cpQ_AR = NULL;
    // this->cpQ_AW = NULL;
};

// Reset
EResultType CScheduler::Reset() {

  // Initialize
  this->IsAR_priority = ERESULT_TYPE_NO;
  this->IsAW_priority = ERESULT_TYPE_NO;

  return (ERESULT_TYPE_SUCCESS);
};

//------------------------------------------------
// Scheduler
//------------------------------------------------
// FIFO
//------------------------------------------------
// 1. If AR priority, Check AR first (AR Q head -> AW Q head)
// 2. If AW priority, Check AW first (AW Q head -> AR Q head)
// 3. NOP
//------------------------------------------------
SPLinkedMUD CScheduler::GetScheduledMUD_FIFO(CPQ cpQ_AR, CPQ cpQ_AW, int64_t nCycle) {

#ifdef DEBUG
  assert(cpQ_AR != NULL);
  assert(cpQ_AW != NULL);
#endif

  // Check AR or AW schedule priority
  int Quotient = nCycle / CYCLE_COUNT_DIR_CHANGE;
  if (Quotient % 2 == 0) { // Even period
    IsAR_priority = ERESULT_TYPE_YES;
    IsAW_priority = ERESULT_TYPE_NO;
  } else {
    IsAR_priority = ERESULT_TYPE_NO;
    IsAW_priority = ERESULT_TYPE_YES;
  };

  SPLinkedMUD spScan = NULL;

  // Search AR
  if (IsAR_priority == ERESULT_TYPE_YES) {

#ifdef DEBUG
    assert(IsAW_priority == ERESULT_TYPE_NO);
#endif

    // Get head
    spScan = cpQ_AR->spMUDList_head;
    if (spScan != NULL) {
// Check ID head
#ifdef DEBUG
      assert(spScan == cpQ_AR->GetIDHeadNode(spScan->nID));
#endif
      return (spScan);
    };

    // Get head
    spScan = cpQ_AW->spMUDList_head;
    if (spScan != NULL) {
// Check ID head
#ifdef DEBUG
      assert(spScan == cpQ_AW->GetIDHeadNode(spScan->nID));
#endif
      return (spScan);
    };
  };

  // Search AW
  if (IsAW_priority == ERESULT_TYPE_YES) {
#ifdef DEBUG
    assert(IsAR_priority == ERESULT_TYPE_NO);
#endif

    // Get head
    spScan = cpQ_AW->spMUDList_head;
    if (spScan != NULL) {
// Check ID head
#ifdef DEBUG
      assert(spScan == cpQ_AW->GetIDHeadNode(spScan->nID));
#endif
      return (spScan);
    };

    // Get head
    spScan = cpQ_AR->spMUDList_head;
    if (spScan != NULL) {
// Check ID head
#ifdef DEBUG
      assert(spScan == cpQ_AR->GetIDHeadNode(spScan->nID));
#endif
      return (spScan);
    };
  };

  // NOP (AR Q)
  if (cpQ_AR->spMUDList_head != NULL) {
    // assert (cpQ_AR->spMUDList_head->spMemCmdPkt->eMemCmd ==
    // EMEM_CMD_TYPE_NOP);
    return (cpQ_AR->spMUDList_head);
  };

  // NOP (AW Q)
  if (cpQ_AW->spMUDList_head != NULL) {
    // assert (cpQ_AW->spMUDList_head->spMemCmdPkt->eMemCmd ==
    // EMEM_CMD_TYPE_NOP);
    return (cpQ_AW->spMUDList_head);
  };

#ifdef DEBUG
  assert(cpQ_AR->spMUDList_head == NULL);
  assert(cpQ_AW->spMUDList_head == NULL);
#endif

  return (NULL);
};

//-----------------------------------------------------------------------------------------
// Scheduler
//-----------------------------------------------------------------------------------------
// Bank hit first
// 	Priority bank preparation
//-----------------------------------------------------------------------------------------
// 1. If AR priority, Check AR first (Tile-out AR -> PTW -> RD AR -> ACT AR ->
// PRE AR -> WR AW -> (If cpQ_AR empty) ACT AW -> PRE AW)
//    If AW priority, Check AW first (Time-out AW ->        WR AW -> ACT AW ->
//    PRE AW -> RD AR -> (If cpQ_AW empty) ACT AR -> PRE AR)
// 2. NOP
//-----------------------------------------------------------------------------------------
// Bank preparation
// 	(1)	PRE can be out of order. Even when ID is same, PRE can be
// issued. 	(2)	ACT can be in order. 	(3)	RD/WR must be in order.
// Push order and pop order must be the same. Pop node (RD/WR cmd) must be head.
// (4) When Ax has priority, bank is prepared.
//-----------------------------------------------------------------------------------------
SPLinkedMUD CScheduler::GetScheduledMUD(CPQ cpQ_AR, CPQ cpQ_AW, int64_t nCycle) {

#ifdef DEBUG
  assert(cpQ_AR != NULL);
  assert(cpQ_AW != NULL);
#endif

  // Check AR or AW priority
  int Quotient = nCycle / CYCLE_COUNT_DIR_CHANGE;
  if (Quotient % 2 == 0) { // Even period
    IsAR_priority = ERESULT_TYPE_YES;
    IsAW_priority = ERESULT_TYPE_NO;
  } else {
    IsAR_priority = ERESULT_TYPE_NO;
    IsAW_priority = ERESULT_TYPE_YES;
  };

  SPLinkedMUD spScan = NULL;
  SPLinkedMUD spTarget = NULL;

  // // 0. PTW top priority
  // #ifdef PTW_TOP_PRIORITY
  // spScan = cpQ_AR->spMUDList_head;
  // while (spScan != NULL) {
  // 	spTarget = spScan;
  //	if (spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_PTW or
  // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_PTW or
  // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_PF or
  // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_PF or
  // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_RPTW  or
  // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_RPTW or
  // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_THIRD_RPTW  ) {
  //
  // 		// Check NOP  FIXME FIXME FIXME
  // 		if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_NOP) {
  // 			spScan = spScan->spNext;
  // 			continue;
  // 		};
  //
  // 		// Check ID head
  // 		if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) {
  // 			return (spTarget);
  // 		};
  // 	};
  // 	spScan = spScan->spNext;
  // };
  // #endif

  // 1. Search AR
  if (IsAR_priority == ERESULT_TYPE_YES) {

#ifdef DEBUG
    assert(IsAW_priority == ERESULT_TYPE_NO);
#endif

    // (0) Time-out urgency
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->nCycle_wait >= CYCLE_TIMEOUT) {

        // Check NOP  FIXME FIXME FIXME
        if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_NOP) {
          spScan = spScan->spNext;
          continue;
        };

        // Check ID head
        if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

// (1) PTW high priority
#ifdef PTW_HIGH_PRIORITY
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      // Check (normal) PTW. (RMM) RPTW. (AT) APTW
      if (spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_PTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_PTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_THIRD_PTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FOURTH_PTW or

          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_PF or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_PF or

          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_APTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_APTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_THIRD_APTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FOURTH_APTW or

          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_RPTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_RPTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_THIRD_RPTW) {

        // Check NOP  FIXME FIXME FIXME
        if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_NOP) {
          spScan = spScan->spNext;
          continue;
        };

        // Check ID head
        if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };
#endif

    // (2) RD cmd (AR Q)
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_RD) {
        // Check ID head
        if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (3) ACT cmd (AR Q)
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_ACT) {
        // Check ID head
        if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };

        // Check if first in target bank
        // If yes, send ACT even it is not ID-head
        if (cpQ_AR->IsThisFirstInBank(spTarget) == ERESULT_TYPE_YES) { // Bank prepare FIXME FIXME Be careful
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (4) PRE cmd (AR Q)
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {

      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_PRE) {

        int nBank = spTarget->upData->cpAR->GetBankNum_MMap();
        int nRow = spTarget->upData->cpAR->GetRowNum_MMap();

        // Check bank prepare
        // If yes, do not send PRE
        if (cpQ_AR->GetMemStatePkt()->IsBankPrepared[nBank] == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        // Check there is any ID head node bank hit
        // If yes, do not send PRE
        if (cpQ_AR->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

#ifdef NO_AGGRESSIVE_PRE
        if (cpQ_AW->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) { // FIXME Be careful. IA. This affect performance
                                                                       // a lot.
          spScan = spScan->spNext;
          continue;
        };
#endif

        // Check ID head
        // if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) { // Bank
        // prepare.

        // Check bank activated row
        int nActivatedRow = cpQ_AR->GetMemStatePkt()->nActivatedRow[nBank];
        if (nRow != nActivatedRow) { // Bank miss
          return (spTarget);
        };
        // };
      };
      spScan = spScan->spNext;
    };

    // (5) WR cmd (AW Q)
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_WR) {
        // Check ID head
        if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (6) ACT cmd (AW Q)
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL and cpQ_AR->IsEmpty() == ERESULT_TYPE_YES) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_ACT) {
        // Check ID head
        if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };

        // Check if first in target bank
        // If yes, send ACT even it is not ID-head
        if (cpQ_AW->IsThisFirstInBank(spTarget) == ERESULT_TYPE_YES) { // Bank prepare FIXME FIXME Be careful
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (7) PRE cmd (AW Q)
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL and cpQ_AR->IsEmpty() == ERESULT_TYPE_YES) {

      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_PRE) {

        int nBank = spTarget->upData->cpAW->GetBankNum_MMap();
        int nRow = spTarget->upData->cpAW->GetRowNum_MMap();

        // Check bank prepare
        // If yes, do not send PRE
        if (cpQ_AW->GetMemStatePkt()->IsBankPrepared[nBank] == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        // Check there is any ID head node bank hit
        // If yes, do not send PRE
        if (cpQ_AW->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        if (cpQ_AR->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        // Check ID head
        // if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) { // Bank
        // prepare.

        // Check bank activated row
        int nActivatedRow = cpQ_AW->GetMemStatePkt()->nActivatedRow[nBank];
        if (nRow != nActivatedRow) { // Bank miss
          return (spTarget);
        };
        // };
      };
      spScan = spScan->spNext;
    };

    // (8) NOP (AR Q)
    // if (cpQ_AR->spMUDList_head != NULL) {
    //	assert (cpQ_AR->spMUDList_head->spMemCmdPkt->eMemCmd ==
    // EMEM_CMD_TYPE_NOP); 	return (cpQ_AR->spMUDList_head);
    // };
    // assert (cpQ_AR->spMUDList_head == NULL);
    // assert (cpQ_AR->IsEmpty() == ERESULT_TYPE_YES);

    // (9) NOP (AW Q)
    // if (cpQ_AW->spMUDList_head != NULL) {
    //	assert (cpQ_AW->spMUDList_head->spMemCmdPkt->eMemCmd ==
    // EMEM_CMD_TYPE_NOP); 	return (cpQ_AW->spMUDList_head);
    // };
  };

  // 2. Search AW
  if (IsAW_priority == ERESULT_TYPE_YES) {
#ifdef DEBUG
    assert(IsAR_priority == ERESULT_TYPE_NO);
#endif

    // (0) Time out urgent
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->nCycle_wait >= CYCLE_TIMEOUT) {

        // Check NOP  FIXME FIXME FIXME
        if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_NOP) {
          spScan = spScan->spNext;
          continue;
        };

        // Check ID head
        if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // // (0) PTW high priority
    // #ifdef PTW_HIGH_PRIORITY
    // spScan = cpQ_AR->spMUDList_head;
    // while (spScan != NULL) {
    // 	spTarget = spScan;
    // 	// Check (normal) PTW or (RMM) RPTW
    // if (spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_PTW or
    // 	spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_PTW  or
    // 	spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_RPTW  or
    // 	spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_RPTW or
    // 	spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_THIRD_RPTW  ) {
    //
    // 		// Check NOP  FIXME FIXME FIXME
    // 		if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_NOP) {
    // 			spScan = spScan->spNext;
    // 			continue;
    // 		};
    //
    // 		// Check ID head
    // 		if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) {
    // 			return (spTarget);
    // 		};
    // 	};
    // 	spScan = spScan->spNext;
    // };
    // #endif

    // (1) WR cmd (AW Q)
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_WR) {
        // Check ID head
        if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (2) ACT cmd (AW Q)
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_ACT) {
        // Check ID head
        if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) { // Bank prepare
          return (spTarget);
        };

        // Check if first in target bank
        // If yes, send ACT even it is not ID-head
        if (cpQ_AW->IsThisFirstInBank(spTarget) == ERESULT_TYPE_YES) { // Bank prepare FIXME FIXME Be careful
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (3) PRE cmd (AW Q)
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL) {

      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_PRE) {
        // Check bank activated row
        int nBank = spTarget->upData->cpAW->GetBankNum_MMap();
        int nRow = spTarget->upData->cpAW->GetRowNum_MMap();

        // Check bank prepare
        // If yes, do not send PRE
        if (cpQ_AW->GetMemStatePkt()->IsBankPrepared[nBank] == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        // Check there is any ID head node bank hit
        // If yes, do not send PRE
        if (cpQ_AW->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

#ifdef NO_AGGRESSIVE_PRE
        if (cpQ_AR->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) { // FIXME Be careful. IA. This affect performance
                                                                       // a lot.
          spScan = spScan->spNext;
          continue;
        };
#endif

        // Check ID head
        // if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) { // Bank
        // prepare.

        // Check bank activated row
        int nActivatedRow = cpQ_AW->GetMemStatePkt()->nActivatedRow[nBank];
        if (nRow != nActivatedRow) { // Bank miss
          return (spTarget);
        };
        // };
      };
      spScan = spScan->spNext;
    };

    // (4) RD cmd (AR Q)
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_RD) {
        // Check ID head
        if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) { // Schedule only ID head
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (5) ACT cmd (AR Q)
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL and cpQ_AW->IsEmpty() == ERESULT_TYPE_YES) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_ACT) {
        // Check ID head
        if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) { // Schedule only ID head
          return (spTarget);
        };

        // Check if first in target bank
        // If yes, send ACT even it is not ID-head
        if (cpQ_AR->IsThisFirstInBank(spTarget) == ERESULT_TYPE_YES) { // Bank prepare FIXME FIXME Be careful
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (6) PRE cmd (AR Q)
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL and cpQ_AW->IsEmpty() == ERESULT_TYPE_YES) {

      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_PRE) {

        int nBank = spTarget->upData->cpAR->GetBankNum_MMap();
        int nRow = spTarget->upData->cpAR->GetRowNum_MMap();

        // Check bank prepare
        // If yes, do not send PRE
        if (cpQ_AR->GetMemStatePkt()->IsBankPrepared[nBank] == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        // Check there is any ID head node bank hit
        // If yes, do not send PRE
        if (cpQ_AR->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        if (cpQ_AW->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        // Check ID head
        // if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) { // Bank
        // prepare.

        // Check bank activated row
        int nActivatedRow = cpQ_AR->GetMemStatePkt()->nActivatedRow[nBank];
        if (nRow != nActivatedRow) { // Bank miss
          return (spTarget);
        };
        // };
      };
      spScan = spScan->spNext;
    };

    // (7) NOP (AW Q)
    // if (cpQ_AW->spMUDList_head != NULL) {
    //	// assert (cpQ_AW->spMUDList_head->spMemCmdPkt->eMemCmd ==
    // EMEM_CMD_TYPE_NOP); 	return (cpQ_AW->spMUDList_head);
    // };
    // assert (cpQ_AW->spMUDList_head == NULL);
    // assert (cpQ_AW->IsEmpty() == ERESULT_TYPE_YES);

    // (8) NOP (AR Q)
    // if (cpQ_AR->spMUDList_head != NULL) {
    //	assert (cpQ_AR->spMUDList_head->spMemCmdPkt->eMemCmd ==
    // EMEM_CMD_TYPE_NOP); 	return (cpQ_AR->spMUDList_head);
    // };
  };

  // 4. NOP cmd
  return (NULL);
};

//-----------------------------------------------------------------------------------------
// Scheduler
//-----------------------------------------------------------------------------------------
// Bank hit first
// 	Aggressive bank preparation
//-----------------------------------------------------------------------------------------
// 1. If AR priority, Check AR first (Tile-out AR -> PTW -> RD AR -> ACT AR ->
// PRE AR -> WR AW -> ACT AW -> PRE AW)
//    If AW priority, Check AW first (Time-out AW ->        WR AW -> ACT AW ->
//    PRE AW -> RD AR -> ACT AR -> PRE AR)
// 2. NOP
//-----------------------------------------------------------------------------------------
// Bank preparation
//  (1)	PRE can be out of order. Even when ID is same, PRE can be issued.
//  (2)	ACT can be in order.
//  (3)	RD/WR must be in order. Push order and pop order must be the same. Pop
//  node (RD/WR cmd) must be head. (4) When Ax has priority, bank is prepared.
//-----------------------------------------------------------------------------------------
SPLinkedMUD CScheduler::GetScheduledMUD_Aggressive(CPQ cpQ_AR, CPQ cpQ_AW, int64_t nCycle) {

#ifdef DEBUG
  assert(cpQ_AR != NULL);
  assert(cpQ_AW != NULL);
#endif

  // Check AR or AW priority
  int Quotient = nCycle / CYCLE_COUNT_DIR_CHANGE;
  if (Quotient % 2 == 0) { // Even period
    IsAR_priority = ERESULT_TYPE_YES;
    IsAW_priority = ERESULT_TYPE_NO;
  } else {
    IsAR_priority = ERESULT_TYPE_NO;
    IsAW_priority = ERESULT_TYPE_YES;
  };

  SPLinkedMUD spScan = NULL;
  SPLinkedMUD spTarget = NULL;

  // // 0. PTW top priority
  // #ifdef PTW_TOP_PRIORITY
  // spScan = cpQ_AR->spMUDList_head;
  // while (spScan != NULL) {
  // 	spTarget = spScan;
  // 	if (spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_PTW   or
  // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_PTW or
  // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_PF or
  // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_PF or
  // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_RPTW or
  // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_RPTW or
  // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_THIRD_RPTW  ) {
  //
  // 		// Check NOP  FIXME FIXME FIXME
  // 		if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_NOP) {
  // 			spScan = spScan->spNext;
  // 			continue;
  // 		};
  //
  // 		// Check ID head
  // 		if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) {
  // 			return (spTarget);
  // 		};
  // 	};
  // 	spScan = spScan->spNext;
  // };
  // #endif

  // 1. Search AR
  if (IsAR_priority == ERESULT_TYPE_YES) {

#ifdef DEBUG
    assert(IsAW_priority == ERESULT_TYPE_NO);
#endif

    // (0) Time-out urgency
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->nCycle_wait >= CYCLE_TIMEOUT) {

        // Check NOP  FIXME FIXME FIXME
        if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_NOP) {
          spScan = spScan->spNext;
          continue;
        };

        // Check ID head
        if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

// (1) PTW high priority
#ifdef PTW_HIGH_PRIORITY
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      // Check (normal) PTW. (RMM) RPTW. (AT) APTW
      if (spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_PTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_PTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_THIRD_PTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FOURTH_PTW or

          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_PF or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_PF or

          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_APTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_APTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_THIRD_APTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FOURTH_APTW or

          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_RPTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_RPTW or
          spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_THIRD_RPTW) {

        // Check NOP  FIXME FIXME FIXME
        if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_NOP) {
          spScan = spScan->spNext;
          continue;
        };

        // Check ID head
        if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };
#endif

    // (2) RD cmd (AR Q)
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_RD) {
        // Check ID head
        if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (3) ACT cmd (AR Q)
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_ACT) {
        // Check ID head
        if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };

        // Check if first in target bank
        // If yes, send ACT even it is not ID-head
        if (cpQ_AR->IsThisFirstInBank(spTarget) == ERESULT_TYPE_YES) { // Bank prepare FIXME FIXME Be careful
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (4) PRE cmd (AR Q)
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {

      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_PRE) {

        int nBank = spTarget->upData->cpAR->GetBankNum_MMap();
        int nRow = spTarget->upData->cpAR->GetRowNum_MMap();

        // Check bank prepare
        // If yes, do not send PRE
        if (cpQ_AR->GetMemStatePkt()->IsBankPrepared[nBank] == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        // Check there is any ID head node bank hit
        // If yes, do not send PRE
        if (cpQ_AR->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        if (cpQ_AW->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) { // FIXME Be careful. Performance. Better remove
          spScan = spScan->spNext;
          continue;
        };

        // Check ID head
        // if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) { // Bank
        // prepare.

        // Check bank activated row
        int nActivatedRow = cpQ_AR->GetMemStatePkt()->nActivatedRow[nBank];
        if (nRow != nActivatedRow) { // Bank miss
          return (spTarget);
        };
        // };
      };
      spScan = spScan->spNext;
    };

    // (5) WR cmd (AW Q)
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_WR) {
        // Check ID head
        if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (6) ACT cmd (AW Q)
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_ACT) {
        // Check ID head
        if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };

        // Check if first in target bank
        // If yes, send ACT even it is not ID-head
        if (cpQ_AW->IsThisFirstInBank(spTarget) == ERESULT_TYPE_YES) { // Bank prepare FIXME FIXME Be careful
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (7) PRE cmd (AW Q)
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL) {

      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_PRE) {

        int nBank = spTarget->upData->cpAW->GetBankNum_MMap();
        int nRow = spTarget->upData->cpAW->GetRowNum_MMap();

        // Check bank prepare
        // If yes, do not send PRE
        if (cpQ_AW->GetMemStatePkt()->IsBankPrepared[nBank] == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        // Check there is any ID head node bank hit
        // If yes, do not send PRE
        if (cpQ_AW->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        if (cpQ_AR->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) { // FIXME Be careful. Performance. Better remove.
          spScan = spScan->spNext;
          continue;
        };

        // Check ID head
        // if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) { // Bank
        // prepare.

        // Check bank activated row
        int nActivatedRow = cpQ_AW->GetMemStatePkt()->nActivatedRow[nBank];
        if (nRow != nActivatedRow) { // Bank miss
          return (spTarget);
        };
        // };
      };
      spScan = spScan->spNext;
    };

    // (8) NOP (AR Q)
    // if (cpQ_AR->spMUDList_head != NULL) {
    //	assert (cpQ_AR->spMUDList_head->spMemCmdPkt->eMemCmd ==
    // EMEM_CMD_TYPE_NOP); 	return (cpQ_AR->spMUDList_head);
    // };
    // assert (cpQ_AR->spMUDList_head == NULL);
    // assert (cpQ_AR->IsEmpty() == ERESULT_TYPE_YES);

    // (9) NOP (AW Q)
    // if (cpQ_AW->spMUDList_head != NULL) {
    //	assert (cpQ_AW->spMUDList_head->spMemCmdPkt->eMemCmd ==
    // EMEM_CMD_TYPE_NOP); 	return (cpQ_AW->spMUDList_head);
    // };
  };

  // 2. Search AW
  if (IsAW_priority == ERESULT_TYPE_YES) {
#ifdef DEBUG
    assert(IsAR_priority == ERESULT_TYPE_NO);
#endif

    // (0) Time out urgent
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->nCycle_wait >= CYCLE_TIMEOUT) {

        // Check NOP  FIXME FIXME FIXME
        if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_NOP) {
          spScan = spScan->spNext;
          continue;
        };

        // Check ID head
        if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // // (0) PTW high priority
    // #ifdef PTW_HIGH_PRIORITY
    // spScan = cpQ_AR->spMUDList_head;
    // while (spScan != NULL) {
    // 	spTarget = spScan;
    // 	// Check (normal) PTW or (RMM) RPTW
    // 	if (spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_PTW   or
    // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_PTW or
    // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_PF or
    // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_PF or
    // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_FIRST_RPTW  or
    // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_SECOND_RPTW or
    // spTarget->upData->cpAR->GetTransType() == ETRANS_TYPE_THIRD_RPTW  ) {
    //
    // 		// Check NOP  FIXME FIXME FIXME
    // 		if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_NOP) {
    // 			spScan = spScan->spNext;
    // 			continue;
    // 		};
    //
    // 		// Check ID head
    // 		if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) {
    // 			return (spTarget);
    // 		};
    // 	};
    // 	spScan = spScan->spNext;
    // };
    // #endif

    // (1) WR cmd (AW Q)
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_WR) {
        // Check ID head
        if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) {
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (2) ACT cmd (AW Q)
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_ACT) {
        // Check ID head
        if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) { // Bank prepare
          return (spTarget);
        };

        // Check if first in target bank
        // If yes, send ACT even it is not ID-head
        if (cpQ_AW->IsThisFirstInBank(spTarget) == ERESULT_TYPE_YES) { // Bank prepare FIXME FIXME Be careful
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (3) PRE cmd (AW Q)
    spScan = cpQ_AW->spMUDList_head;
    while (spScan != NULL) {

      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_PRE) {
        // Check bank activated row
        int nBank = spTarget->upData->cpAW->GetBankNum_MMap();
        int nRow = spTarget->upData->cpAW->GetRowNum_MMap();

        // Check bank prepare
        // If yes, do not send PRE
        if (cpQ_AW->GetMemStatePkt()->IsBankPrepared[nBank] == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        // Check there is any ID head node bank hit
        // If yes, do not send PRE
        if (cpQ_AW->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        if (cpQ_AR->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) { // FIXME Be careful. Performance. Better remove
          spScan = spScan->spNext;
          continue;
        };

        // Check ID head
        // if (spTarget == cpQ_AW->GetIDHeadNode(spTarget->nID)) { // Bank
        // prepare.

        // Check bank activated row
        int nActivatedRow = cpQ_AW->GetMemStatePkt()->nActivatedRow[nBank];
        if (nRow != nActivatedRow) { // Bank miss
          return (spTarget);
        };
        // };
      };
      spScan = spScan->spNext;
    };

    // (4) RD cmd (AR Q)
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_RD) {
        // Check ID head
        if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) { // Schedule only ID head
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (5) ACT cmd (AR Q)
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {
      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_ACT) {
        // Check ID head
        if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) { // Schedule only ID head
          return (spTarget);
        };

        // Check if first in target bank
        // If yes, send ACT even it is not ID-head
        if (cpQ_AR->IsThisFirstInBank(spTarget) == ERESULT_TYPE_YES) { // Bank prepare FIXME FIXME Be careful
          return (spTarget);
        };
      };
      spScan = spScan->spNext;
    };

    // (6) PRE cmd (AR Q)
    spScan = cpQ_AR->spMUDList_head;
    while (spScan != NULL) {

      spTarget = spScan;
      if (spTarget->spMemCmdPkt->eMemCmd == EMEM_CMD_TYPE_PRE) {

        int nBank = spTarget->upData->cpAR->GetBankNum_MMap();
        int nRow = spTarget->upData->cpAR->GetRowNum_MMap();

        // Check bank prepare
        // If yes, do not send PRE
        if (cpQ_AR->GetMemStatePkt()->IsBankPrepared[nBank] == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        // Check there is any ID head node bank hit
        // If yes, do not send PRE
        if (cpQ_AR->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        if (cpQ_AW->IsThereIDHeadBankHit(nBank) == ERESULT_TYPE_YES) {
          spScan = spScan->spNext;
          continue;
        };

        // Check ID head
        // if (spTarget == cpQ_AR->GetIDHeadNode(spTarget->nID)) { // Bank
        // prepare.

        // Check bank activated row
        int nActivatedRow = cpQ_AR->GetMemStatePkt()->nActivatedRow[nBank];
        if (nRow != nActivatedRow) { // Bank miss
          return (spTarget);
        };
        // };
      };
      spScan = spScan->spNext;
    };

    // (7) NOP (AW Q)
    // if (cpQ_AW->spMUDList_head != NULL) {
    //	// assert (cpQ_AW->spMUDList_head->spMemCmdPkt->eMemCmd ==
    // EMEM_CMD_TYPE_NOP); 	return (cpQ_AW->spMUDList_head);
    // };
    // assert (cpQ_AW->spMUDList_head == NULL);
    // assert (cpQ_AW->IsEmpty() == ERESULT_TYPE_YES);

    // (8) NOP (AR Q)
    // if (cpQ_AR->spMUDList_head != NULL) {
    //	assert (cpQ_AR->spMUDList_head->spMemCmdPkt->eMemCmd ==
    // EMEM_CMD_TYPE_NOP); 	return (cpQ_AR->spMUDList_head);
    // };
  };

  // 4. NOP cmd
  return (NULL);
};

// Get mem name
string CScheduler::GetName() { return (this->cName); };

// Update state
// EResultType CScheduler::UpdateState() {
//
//	return (ERESULT_TYPE_SUCCESS);
// };

// Debug
EResultType CScheduler::CheckScheduler() { return (ERESULT_TYPE_SUCCESS); };

EResultType CScheduler::Display() { return (ERESULT_TYPE_SUCCESS); };
