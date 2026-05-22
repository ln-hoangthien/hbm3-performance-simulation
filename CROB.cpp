//-----------------------------------------------------------
// Filename     : CROB.cpp
// Version	: 0.1
// Date         : 2 Mar 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: ROB definition
//-----------------------------------------------------------
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "CROB.h"

// Constructor
CROB::CROB(string cName, EUDType eUDType, int nMaxNum) {

  // Generate a node later when push
  this->spRUDList_head = NULL;
  this->spRUDList_tail = NULL;

  // Initialize
  this->cName = cName;
  this->eUDType = eUDType;
  this->nMaxNum = nMaxNum;
  this->nCurNum = -1;

  // this->nMaxCycleWait = -1;
};

// Destructor
CROB::~CROB() {

// Debug
#ifndef BACKGROUND_TRAFFIC_ON
  assert(this->GetCurNum() == 0); // RMM, background RPTW maybe on-going
  assert(this->spRUDList_head == NULL);
  assert(this->spRUDList_tail == NULL);
#endif

  // Delete all nodes
  SPLinkedRUD spScan;
  SPLinkedRUD spTarget;
  spScan = this->spRUDList_head;
  while (spScan != NULL) {
    spTarget = spScan;
    spScan = spScan->spNext;
    Delete_UD(spTarget->upData, this->eUDType);
    spTarget->upData = NULL;
    spTarget = NULL;
    // spTarget->spNext = NULL;
  };

  // Debug
  // assert (this->spRUDList_head == NULL);
  // assert (this->spRUDList_tail == NULL);
};

// Initialize
EResultType CROB::Reset() {

  // Delete all nodes
  SPLinkedRUD spScan;
  SPLinkedRUD spTarget;
  spScan = this->spRUDList_head;
  while (spScan != NULL) {
    spTarget = spScan;
    spScan = spScan->spNext;
    Delete_UD(spTarget->upData, this->eUDType);
    delete (spTarget->upData);
    spTarget->upData = NULL;
  };

  // Initialize
  this->spRUDList_head = NULL;
  this->spRUDList_tail = NULL;
  this->nCurNum = 0;
  // this->nMaxCycleWait  = 0;

  return (ERESULT_TYPE_SUCCESS);
};

// Push
EResultType CROB::Push(UPUD upThis, int nLatency) {

#ifdef DEBUG
  assert(upThis != NULL);
  assert(nLatency >= 0);
  assert(this->GetCurNum() < this->GetMaxNum());
#endif

  // Generate node. Initialize
  UPUD upUD_new = Copy_UD(upThis, this->eUDType); // Generate new up and cp
  SPLinkedRUD spLinkedRUD_new = new SLinkedRUD;
  spLinkedRUD_new->upData = upUD_new;
  spLinkedRUD_new->nLatency = nLatency;
  spLinkedRUD_new->nCycle_wait = 0;
  spLinkedRUD_new->spPrev = NULL; // ROB
  spLinkedRUD_new->spNext = NULL;
  if (this->eUDType == EUD_TYPE_AR) {
    spLinkedRUD_new->nID = upThis->cpAR->GetID();
  } else if (this->eUDType == EUD_TYPE_AW) {
    spLinkedRUD_new->nID = upThis->cpAW->GetID();
  } else if (this->eUDType == EUD_TYPE_W) {
    spLinkedRUD_new->nID = upThis->cpW->GetID();
  } else if (this->eUDType == EUD_TYPE_R) {
    spLinkedRUD_new->nID = upThis->cpR->GetID();
  } else if (this->eUDType == EUD_TYPE_B) {
    spLinkedRUD_new->nID = upThis->cpB->GetID();
  };

  // Push
  if (this->spRUDList_head == NULL) {

#ifdef DEBUG
    assert(this->spRUDList_tail == NULL);
#endif

    this->spRUDList_head = spLinkedRUD_new;
    this->spRUDList_tail = spLinkedRUD_new;
  } else {

#ifdef DEBUG
    assert(this->GetCurNum() > 0);
    assert(this->spRUDList_tail != NULL);
#endif

    this->spRUDList_tail->spNext = spLinkedRUD_new;
    spLinkedRUD_new->spPrev = this->spRUDList_tail; // ROB
    this->spRUDList_tail = spLinkedRUD_new;
  };

  // Increment occupancy
  this->nCurNum++;

#ifdef DEBUG
  assert(upUD_new != NULL);
// this->CheckROB();
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// Push
EResultType CROB::Push(UPUD upThis, int nMemCh, int nLatency) {

#ifdef DEBUG
  assert(upThis != NULL);
  assert(nLatency >= 0);
  assert(this->GetCurNum() < this->GetMaxNum());
#endif

  // Generate node. Set.
  UPUD upUD_new = Copy_UD(upThis, this->eUDType); // Generate new up and cp
  SPLinkedRUD spLinkedRUD_new = new SLinkedRUD;
  spLinkedRUD_new->upData = upUD_new;
  spLinkedRUD_new->nMemCh = nMemCh;
  spLinkedRUD_new->nLatency = nLatency;
  spLinkedRUD_new->nCycle_wait = 0;
  spLinkedRUD_new->spPrev = NULL; // ROB
  spLinkedRUD_new->spNext = NULL;
  if (this->eUDType == EUD_TYPE_AR) {
    spLinkedRUD_new->nID = upThis->cpAR->GetID();
  } else if (this->eUDType == EUD_TYPE_AW) {
    spLinkedRUD_new->nID = upThis->cpAW->GetID();
  } else if (this->eUDType == EUD_TYPE_W) {
    spLinkedRUD_new->nID = upThis->cpW->GetID();
  } else if (this->eUDType == EUD_TYPE_R) {
    spLinkedRUD_new->nID = upThis->cpR->GetID();
  } else if (this->eUDType == EUD_TYPE_B) {
    spLinkedRUD_new->nID = upThis->cpB->GetID();
  };

  // Push
  if (this->spRUDList_head == NULL) {

#ifdef DEBUG
    assert(this->spRUDList_tail == NULL);
#endif

    this->spRUDList_head = spLinkedRUD_new;
    this->spRUDList_tail = spLinkedRUD_new;
  } else {

#ifdef DEBUG
    assert(this->GetCurNum() > 0);
    assert(this->spRUDList_tail != NULL);
#endif

    this->spRUDList_tail->spNext = spLinkedRUD_new;
    spLinkedRUD_new->spPrev = this->spRUDList_tail; // ROB
    this->spRUDList_tail = spLinkedRUD_new;
  };

  // Increment occupancy
  this->nCurNum++;

#ifdef DEBUG
  assert(upUD_new != NULL);
// this->CheckROB();
#endif

  return (ERESULT_TYPE_SUCCESS);
};

// Push in zero cycle
EResultType CROB::Push(UPUD upThis) {

#ifdef DEBUG
  assert(upThis != NULL);
#endif

  // Push
  this->Push(upThis, 0);
  ;

  // Debug
  // this->CheckROB();

  return (ERESULT_TYPE_SUCCESS);
};

// Pop
UPUD CROB::Pop() {

  // Debug
  // this->CheckROB();

  // Get head
  SPLinkedRUD spTarget = this->spRUDList_head;

  UPUD upTarget = this->spRUDList_head->upData;

  // Check latency
  if (spTarget->nLatency > 0) { // Pop only when entry latency 0
    return (NULL);
  };

#ifdef DEBUG
  assert(spTarget->nLatency == 0);
#endif

  // Decrement occupancy
  this->nCurNum--;

  // Pop head
  if (this->nCurNum == 0) {
    this->spRUDList_head = NULL;
    this->spRUDList_tail = NULL;
  } else {
    this->spRUDList_head = this->spRUDList_head->spNext;
  };

  // Maintain
  delete (spTarget);
  spTarget = NULL;

#ifdef DEBUG
  assert(upTarget != NULL);
// this->CheckROB();
#endif

  return (upTarget); // Delete outside
};

// Pop node first matching ID
UPUD CROB::Pop(int nID) {

  // Debug
  // this->CheckROB();

  // Get target node
  SPLinkedRUD spScan = NULL;
  SPLinkedRUD spTarget = NULL;
  spScan = this->spRUDList_head;

  while (spScan != NULL) {
    if (spScan->nID == nID) {
      spTarget = spScan;
      break;
    };
    spScan = spScan->spNext;
  };

  // Not matched
  if (spTarget == NULL) {
    return (NULL);
  };

  UPUD upTarget = spTarget->upData;

#ifdef DEBUG
  assert(spTarget != NULL);
  assert(upTarget != NULL);
#endif

  // Decrement occupancy
  this->nCurNum--;

  // Pop target
  if (this->nCurNum == 0) {
    this->spRUDList_head = NULL;
    this->spRUDList_tail = NULL;
  } else {
#ifdef DEBUG
    assert(this->nCurNum >= 1);
    assert(this->spRUDList_head != NULL);
    assert(this->spRUDList_tail != NULL);
#endif

    if (spTarget == this->spRUDList_head) {
      this->spRUDList_head = this->spRUDList_head->spNext;
    } else if (spTarget == this->spRUDList_tail) {
      this->spRUDList_tail = spTarget->spPrev;
      spTarget->spPrev->spNext = NULL;
    } else {
      spTarget->spPrev->spNext = spTarget->spNext;
      spTarget->spNext->spPrev = spTarget->spPrev;
    };
  };

  // Maintain
  delete (spTarget);
  spTarget = NULL;

#ifdef DEBUG
  assert(upTarget != NULL);
// this->CheckROB();
#endif

  return (upTarget);
};

// Pop node first matching ID and MemCh
UPUD CROB::Pop(int nID, int nMemCh) {

  // Debug
  // this->CheckROB();

  // Get target node
  SPLinkedRUD spTarget = NULL;
  SPLinkedRUD spScan = this->spRUDList_head;

  while (spScan != NULL) {
    if (spScan->nID == nID and spScan->nMemCh == nMemCh) {
      spTarget = spScan;
      break;
    };
    spScan = spScan->spNext;
  };

  // Not matched
  if (spTarget == NULL) {
    return (NULL);
  };

  UPUD upTarget = spTarget->upData;

#ifdef DEBUG
  assert(spTarget != NULL);
  assert(upTarget != NULL);
#endif

  // Decrement occupancy
  this->nCurNum--;

  // Pop target
  if (this->nCurNum == 0) {
    this->spRUDList_head = NULL;
    this->spRUDList_tail = NULL;
  } else {
#ifdef DEBUG
    assert(this->nCurNum >= 1);
    assert(this->spRUDList_head != NULL);
    assert(this->spRUDList_tail != NULL);
#endif

    if (spTarget == this->spRUDList_head) {
      this->spRUDList_head = this->spRUDList_head->spNext;
    } else if (spTarget == this->spRUDList_tail) {
      this->spRUDList_tail = spTarget->spPrev;
      spTarget->spPrev->spNext = NULL;
    } else {
      spTarget->spPrev->spNext = spTarget->spNext;
      spTarget->spNext->spPrev = spTarget->spPrev;
    };
  };

  // Maintain
  delete (spTarget);
  spTarget = NULL;

#ifdef DEBUG
  assert(upTarget != NULL);
// this->CheckROB();
#endif

  return (upTarget);
};

// // Find node matching ID and MemCh in ROB_R,B.
// // Set "IsHead_Resp".
// EResultType CROB::SetHead_Resp(int nID, int nMemCh) {
//
//	SPLinkedRUD spScan = this->spRUDList_head;
//	while (spScan != NULL) {
//
//		#ifdef DEBUG
//		assert (spScan->upData != NULL);
//		assert (spScan->nLatency >= 0);
//		#endif
//
//		if (spScan->nID == nID and spScan->nMemCh = nMemCh) {
//			spScan->IsHead_Resp = ERESULT_TYPE_YES;
//			return (ERESULT_TYPE_SUCCESS);
//		};
//
//		spScan = spScan->spNext;
//	};
//	return (ERESULT_TYPE_FAIL);
// };

// Get name
string CROB::GetName() { return (this->cName); };

// Get UD type
EUDType CROB::GetUDType() { return (this->eUDType); };

// Get current occupancy
int CROB::GetCurNum() { return (this->nCurNum); };

// Get size
int CROB::GetMaxNum() { return (this->nMaxNum); };

// Get head data
UPUD CROB::GetTop() {

  // Debug
  // this->CheckROB();

  if (this->spRUDList_head == NULL) {

#ifdef DEBUG
    assert(this->GetCurNum() == 0);
#endif

    return (NULL);
  };

  return (this->spRUDList_head->upData);
};

// Get first data matching ID
UPUD CROB::GetTarget(int nID) {

  SPLinkedRUD spScan = this->spRUDList_head;
  while (spScan != NULL) {

#ifdef DEBUG
    assert(spScan->upData != NULL);
    assert(spScan->nLatency >= 0);
#endif

    if (spScan->nID == nID) {
      return (spScan->upData);
    };

    spScan = spScan->spNext;
  };
  return (NULL);
};

// Get channel num req head in ROB_Ax
int CROB::GetMemCh_ReqHead(int nID) {

  SPLinkedRUD spScan = this->spRUDList_head;
  while (spScan != NULL) {

#ifdef DEBUG
    assert(spScan->upData != NULL);
    assert(spScan->nLatency >= 0);
#endif

    if (spScan->nID == nID) {
      return (spScan->nMemCh);
    };

    spScan = spScan->spNext;
  };
  assert(0);
  return (-1);
};

// Get R, B poppable from ROB_R, B
// 	Check memory ch matching Ax head
SPLinkedRUD CROB::GetResp_poppable_ROB(CPROB cpReq) {

  SPLinkedRUD spScan = this->spRUDList_head;
  while (spScan != NULL) {

#ifdef DEBUG
    assert(spScan->upData != NULL);
    assert(spScan->nLatency >= 0);
#endif

    // Check (1) ID head (2) memory ch (3) Latency
    int MemCh_ReqHead = cpReq->GetMemCh_ReqHead(spScan->nID);
    if (MemCh_ReqHead == spScan->nMemCh and spScan->nLatency == 0) {
      return (spScan);
    };

    spScan = spScan->spNext;
  };

  return (NULL);
};

// Get req head node in ROB_Ax matching ID, ch
SPLinkedRUD CROB::GetReqNode_ROB(int nID, int nMemCh) {

  SPLinkedRUD spScan = this->spRUDList_head;
  while (spScan != NULL) {

#ifdef DEBUG
    assert(spScan->upData != NULL);
    assert(spScan->nLatency >= 0);
#endif

    if (spScan->nID == nID and spScan->nMemCh == nMemCh) {
      return (spScan);
    };

    spScan = spScan->spNext;
  };
  return (NULL);
};

// Check ROB empty
EResultType CROB::IsEmpty() {

  if (this->nCurNum == 0) {

#ifdef DEBUG
    assert(this->spRUDList_head == NULL);
    assert(this->spRUDList_tail == NULL);
#endif

    return (ERESULT_TYPE_YES);
  };

#ifdef DEBUG
  assert(this->spRUDList_head != NULL);
  assert(this->spRUDList_tail != NULL);
#endif

  return (ERESULT_TYPE_NO);
};

// Check ROB full
EResultType CROB::IsFull() {

  if (this->nCurNum == this->nMaxNum) {
    return (ERESULT_TYPE_YES);
  };

  return (ERESULT_TYPE_NO);
};

// // Get max alloc cycles
// int CROB::GetMaxCycleWait() {
//
//	return (this->nMaxCycleWait);
// };

// Update state
EResultType CROB::UpdateState() {

  // Debug
  // this->CheckROB();

  // Update entry wating time since allocation
  // int nMaxCycleWait = 0;
  SPLinkedRUD spScan = this->spRUDList_head;
  while (spScan != NULL) {

#ifdef DEBUG
    assert(spScan->upData != NULL);
    assert(spScan->nLatency >= 0);
#endif

    // Condition pop
    if (spScan->nLatency > 0) {
      spScan->nLatency--;
    };

    // Increment allocation time
    spScan->nCycle_wait++;

    // Get max allocation cycles
    // if (nMaxCycleWait < spScan->nCycle_wait) {
    //	nMaxCycleWait = spScan->nCycle_wait;
    // }

    spScan = spScan->spNext;
  };

  // this->nMaxCycleWait = nMaxCycleWait;

  return (ERESULT_TYPE_SUCCESS);
};

// Debug
EResultType CROB::CheckROB() {

  // Debug
  assert(this->nCurNum >= 0);
  assert(this->nCurNum <= this->nMaxNum);

  // Check NULL
  if (this->nCurNum == 0) {
    assert(this->spRUDList_head == NULL);
    assert(this->spRUDList_tail == NULL);
    assert(this->IsEmpty() == ERESULT_TYPE_YES);
    assert(this->GetCurNum() == 0);
  } else {
    assert(this->spRUDList_head != NULL);
    assert(this->spRUDList_tail != NULL);
    assert(this->IsEmpty() == ERESULT_TYPE_NO);
    assert(this->GetCurNum() > 0);
  };

  // Check occupancy
  int nCurOccupancy = 0;
  SPLinkedRUD spScan = this->spRUDList_head;
  while (spScan != NULL) {
    spScan = spScan->spNext;
    nCurOccupancy++;
  };
  assert(nCurOccupancy == this->nCurNum);

  // Check full
  if (nCurOccupancy == this->nMaxNum) {
    assert(this->IsFull() == ERESULT_TYPE_YES);
  } else {
    assert(this->IsFull() == ERESULT_TYPE_NO);
  };

  return (ERESULT_TYPE_SUCCESS);
};

// Debug
EResultType CROB::Display() {

  string cUDType = Convert_eUDType2string(this->eUDType);

  printf("---------------------------------------------\n");
  printf("\t ROB display\n");
  printf("---------------------------------------------\n");
  printf("\t Name         : \t %s\n", this->cName.c_str());
  printf("\t eUDType      : \t %s\n", cUDType.c_str());
  printf("\t nCurNum      : \t %d\n", this->nCurNum);
  printf("\t nMaxNum      : \t %d\n", this->nMaxNum);
  // printf("\t nMaxCycleWait: \t %d\n", this->nMaxCycleWait);
  printf("---------------------------------------------\n");

  SPLinkedRUD spScan = this->spRUDList_head;
  while (spScan != NULL) {
    Display_UD(spScan->upData, this->eUDType);
    // printf("\t nLatency : \t %d\n", spScan->nLatency);
    printf("\t nMemCh      : \t %d\n", spScan->nMemCh);
    printf("\t nCycle_wait : \t %d\n", spScan->nCycle_wait);
    spScan = spScan->spNext;
  };

  return (ERESULT_TYPE_SUCCESS);
};
