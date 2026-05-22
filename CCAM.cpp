//-----------------------------------------------------------
// Filename     : CCAM.cpp
// Version	: 0.2
// Date         : 25 Feb 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: CAM definition
//-----------------------------------------------------------
// FIXME Under development
//-----------------------------------------------------------
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "CCAM.h"

// Constructor
CCAM::CCAM(string cName, EUDType eUDType, int nMaxNum) {

  // Generate a node later when push
  this->spFIFOList_head = NULL;
  this->spFIFOList_tail = NULL;

  // Initialize
  this->cName = cName;
  this->eUDType = eUDType;
  this->nMaxNum = nMaxNum;
};

// Destructor
CCAM::~CCAM() {

  // Delete all nodes
  SPLinkedFIFO spScan;
  SPLinkedFIFO spTarget;
  spScan = this->spFIFOList_head;
  while (spScan != NULL) {
    spTarget = spScan;
    spScan = spScan->spNext;
    delete (spTarget);
  };

  // Debug
  assert(this->spFIFOList_head == NULL);
  assert(this->spFIFOList_tail == NULL);
};

// Initialize
EResultType CCAM::Reset() {

  SPLinkedFIFO spScan = NULL;
  SPLinkedFIFO spTarget = NULL;

  // Delete all FIFOs
  spScan = this->spFIFOList_head;
  while (spScan != NULL) {
    spTarget = spScan;
    spScan = spScan->spNext;
    delete (spTarget);
  };

  // Initialize
  this->spFIFOList_head = NULL;
  this->spFIFOList_tail = NULL;

  return (ERESULT_TYPE_SUCCESS);
};

//--------------------------------------------
// Push
//	(1) Push only when space avalable
//--------------------------------------------
EResultType CCAM::Push(UPUD upThis, int nID) {
  // Debug
  assert(upThis != NULL);
  assert(this->IsFull() == ERESULT_TYPE_NO);

  // Push
  if (this->spFIFOList_head == NULL) {

    // Generate FIFO. Initialize
    SPLinkedFIFO spLinkedFIFO_new = new SLinkedFIFO;
    spLinkedFIFO_new->cpFIFO = new CFIFO("CAMFIFO", this->eUDType, 16); // FIXME name, FIFO size
    spLinkedFIFO_new->spPrev = NULL;
    spLinkedFIFO_new->spNext = NULL;

    // Push
    spLinkedFIFO_new->cpFIFO->Push(upThis);

    // Maintain list
    this->spFIFOList_head = spLinkedFIFO_new;
    this->spFIFOList_tail = spLinkedFIFO_new;
  } else {
    // Search matching FIFO
    SPLinkedFIFO spScan = this->spFIFOList_head;
    SPLinkedFIFO spTarget = NULL;
    while (spScan != NULL) {
      if (spScan->GetTop()->GetID() == nID) {
        spTarget = spScan;

        // Push
        spTarget->cpFIFO->Push(upThis);
        break;
      };
      spScan = spScan->spNext;
    };

    // No matching FIFO. Generate FIFO.
    if (spTarget == NULL) {

      // Generate FIFO. Initialize
      SPLinkedFIFO spLinkedFIFO_new = new SLinkedFIFO;
      spLinkedFIFO_new->cpFIFO = new CFIFO("CAMFIFO", this->eUDType, 16);
      spLinkedFIFO_new->spPrev = NULL;
      spLinkedFIFO_new->spNext = NULL;

      // Push
      spLinkedFIFO_new->cpFIFO->Push(upThis);

      // Maintain list
      this->spFIFOList_tail->spNext = spLinkedFIFO_new;
      spLinkedFIFO_new->spPrev = this->spFIFOList_tail;
      this->spFIFOList_tail = spLinkedFIFO_new;
    };
  };

  // Increment occupancy
  this->nCurNum++;

  // Debug
  // this->CheckCAM();

  return (ERESULT_TYPE_SUCCESS);
};

//--------------------------------------------
// Pop
//
//--------------------------------------------
UPUD CCAM::Pop(int nID) {
  // Debug
  // this->CheckCAM();

  // Get matching FIFO
  SPLinkedFIFO spScan = this->spFIFOList_head;
  SPLinkedFIFO spTarget = NULL;
  while (spScan != NULL) {
    // Check ID
    if (spScan->GetTop()->GetID() == nID) {
      // Check waiting cycle
      if (spScan->cpFIFO->GetTop()->nLatency == 0) {
        spTarget = spScan;
        break;
      };
    };
    spScan = spScan->spNext;
  };

  if (spTarget == NULL) {
    return (NULL);
  };

  // Debug
  assert(spTarget->cpFIFO->GetTop()->nLatency == 0);

  // Pop
  UPUD upTarget = spTarget->cpFIFO->Pop();

  // Check FIFO size
  if (spTarget->cpFIFO->GetCurNum() == 0) {

    // Decrement occupancy
    this->nCurNum--;

    // Maintain list
    if (this->GetCurNum() == 0) {
      this->spFIFOList_head = NULL;
      this->spFIFOList_tail = NULL;
    } else {
      if (spTarget == this->spFIFOList_head) {
        this->spFIFOList_head = this->spFIFOList_head->spNext;
      } else if (spTarget == this->spFIFOList_tail) {
        this->spFIFOList_tail = spTarget->spPrev;
        spTarget->spPrev->spNext = NULL;
      } else {
        spTarget->spPrev->spNext = spTarget->spNext;
        spTarget->spNext->spPrev = spTarget->spPrev;
      };
    };

    // Delete FIFO
    delete (spTarget);
    delete (spTarget->cpFIFO);
  };

  // Debug
  assert(upTarget != NULL);
  // this->CheckCAM();

  return (upTarget);
};

// Get UD type
EUDType CCAM::GetUDType() { return (this->eUDType); };

// Get current occupancy
int CCAM::GetCurNum() {

  // Check number of nodes
  int nNumFIFO = 0;
  SPLinkedFIFO spScan = this->spFIFOList_head;
  while (spScan != NULL) {
    spScan = spScan->spNext;
    nNumFIFO++;
  };
  return (nNumFIFO);
};

// Get size
int CCAM::GetMaxNum() { return (this->nMaxNum); };

// Get head FIFO
CPFIFO CCAM::GetTopFIFO() {

  // Debug
  // this->CheckCAM();

  // if (this->spFIFOList_head == NULL) { return (NULL); };

  return (this->spFOFOList_head);
};

// Get head FIFO
CPFIFO CCAM::GetFIFO(int nKey) {

  // Debug
  // this->CheckCAM();

  SPLinkedFIFO spScan = this->spFIFOList_head;
  while (spScan != NULL) {

    if (spScan->GetTop()->GetID() == nKey) {
      return (spScan);
    };
    spScan = spScan->spNext;
  };
  return (NULL);
};

// Check CAM empty
EResultType CCAM::IsEmpty() {

  if (this->spFIFOList_head != NULL) {
    return (ERESULT_TYPE_YES);
  };

  // Debug
  assert(this->nCurNum == 0);

  return (ERESULT_TYPE_NO);
};

// Check ID match
EResultType CCAM::IsExist(int nKey) {

  SPLinkedFIFO spScan = this->spFIFOList_head;
  while (spScan != NULL) {
    if (spScan->GetTop()->GetID() == nKey) {
      return (ERESULT_TYPE_YES);
    };
    spScan = spScan->spNext;
  };
  return (ERESULT_TYPE_NO);
};

// Check space available
EResultType CCAM::IsPushable(int nKey) {

  // Check matching FIFO
  SPLinkedFIFO spScan = this->spFIFOList_head;
  while (spScan != NULL) {
    if (spScan->GetTop()->GetID() == nKey) {
      // Check target FIFO space
      if (spScan->GetTop()->IsFull() == ERESULT_TYPE_NO) {
        return (ERESULT_TYPE_YES);
      } else {
        return (ERESULT_TYPE_NO);
      };
    };
    spScan = spScan->spNext;
  };

  // Debug
  assert(this->IsExist(nKey) == ERESULT_TYPE_NO);

  // Check FIFO num
  if (this->GetCurNum() < this->nMaxNum) {
    return (ERESULT_TYPE_YES);
  };

  return (ERESULT_TYPE_NO);
};

//
EResultType CCAM::IsTheOtherExist(int nKey) { return (ERESULT_TYPE_YES); };

EResultType CCAM::UpdateState() {

  // Update state FIFO
  SPLinkedFIFO spScan = this->spFIFOList_head;
  while (spScan != NULL) {
    spScan->cpFIFO->UpdateState();
    spScan = spScan->spNext;
  };

  return (ERESULT_TYPE_SUCCESS);
};

// Debug
EResultType CCAM::CheckCAM() { // FIXME

  // Check NULL
  if (this->IsEmpty() == ERESULT_TYPE_YES) {
    assert(this->spFIFOList_head == NULL);
    assert(this->GetCurNum() == 0);
  } else {
    assert(this->spFIFOList_head != NULL);
    assert(this->spFIFOList_head->GetTop() != NULL);
    assert(this->GetCurNum() > 0);
  };

  // Check occupancy
  int nCurOccupancy = 0;
  SPLinkedUD spScan = this->spUDList_head;
  while (spScan != NULL) {
    spScan = spScan->spNext;
    nCurOccupancy++;
  };
  assert(nCurOccupancy >= 0);
  assert(nCurOccupancy <= this->nMaxNum);

  return (ERESULT_TYPE_SUCCESS);
};

// Debug
EResultType CCAM::Display() {

  string cUDType;
  cUDType = Convert_eUDType2string(this->eUDType);

  printf("---------------------------------------------\n");
  printf("\t CAM display\n");
  printf("---------------------------------------------\n");
  printf("\t Name    : \t %s\n", this->cName.c_str());
  printf("\t eUDType : \t %s\n", cUDType.c_str());
  printf("\t nMaxNum : \t %d\n", this->nMaxNum);
  printf("---------------------------------------------\n");

  SPLinkedFIFO spScan = this->spFIFOList_head;
  while (spScan != NULL) {
    spScan->Display();
    spScan = spScan->spNext;
  };

  return (ERESULT_TYPE_SUCCESS);
};
