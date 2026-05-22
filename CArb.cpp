//-----------------------------------------------------------
// FileName	: CArb.cpp
// Version	: 0.72
// Date		: 2 Mar 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Arbiter class definition
//-----------------------------------------------------------
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "CArb.h"

// Construct
CArb::CArb(string cName, EArbType eArbType, int nCandidateNum) {

  // Generate and initialize
  this->cName = cName;
  this->eArbType = eArbType;
  this->nCandidateNum = nCandidateNum;
  this->nPrevResult = -1;
  this->IsPrevResult_RLAST = ERESULT_TYPE_UNDEFINED;
};

// Construct
CArb::CArb(string cName, int nCandidateNum) {

  // Generate and initialize
  this->cName = cName;

#if defined BUS_FIXED_PRIORITY
  this->eArbType = EARB_TYPE_FixedPriority;
#elif defined BUS_ROUND_ROBIN
  this->eArbType = EARB_TYPE_RR;
#else
  this->eArbType = EARB_TYPE_UNDEFINED;
#endif

  this->nCandidateNum = nCandidateNum;
  this->nPrevResult = -1;
  this->IsPrevResult_RLAST = ERESULT_TYPE_UNDEFINED;
};

// Destruct
CArb::~CArb(){

};

// Reset
EResultType CArb::Reset() {

  this->nPrevResult = -1;
  this->IsPrevResult_RLAST = ERESULT_TYPE_NO;

  return (ERESULT_TYPE_SUCCESS);
};

// Update state
EResultType CArb::UpdateState() { return (ERESULT_TYPE_SUCCESS); };

// Get arbitration
int CArb::Arbitration(int nCandidateList[]) {

#ifdef DEBUG
  assert(nCandidateList != NULL);
#endif

  int nArbResult = -1;
#if defined BUS_ROUND_ROBIN
  nArbResult = this->Arbitration_RR(nCandidateList);
#elif defined BUS_FIXED_PRIORITY
  nArbResult = this->Arbitration_Fixed(nCandidateList);
#endif

  // Debug
  // this->CheckArb();

  return (nArbResult);
};

// Get arbitration
int CArb::Arbitration_RR(int nCandidateList[]) {

  // Debug
  // this->CheckArb();

  // Start point search
  int nSearchStartPoint = -1;
  if (this->nPrevResult == this->nCandidateNum - 1) {
    nSearchStartPoint = 0;
  } else {
    nSearchStartPoint = this->nPrevResult + 1;
  };

  // Search first
  for (int i = nSearchStartPoint; i < this->nCandidateNum; i++) {
    if (nCandidateList[i] == 1) { // valid
      this->nPrevResult = i;
      return (i);
    };
  };

  // Search second
  if (nSearchStartPoint != 0) {

    for (int i = 0; i < nSearchStartPoint; i++) {
      if (nCandidateList[i] == 1) {
        this->nPrevResult = i;
        return (i);
      };
    };
  };

  // Nothing arbitrated
  return (-1);
};

//---------------------------------------------------
// Get arbitration
// 	R transaction level
// 	Once R is arbitrated, give high priority until RLAST
// 	FIXME Under develop
//---------------------------------------------------
int CArb::Arbitration_RR(int nCandidateList[], EResultType IsLast[]) {

  // Debug
  // this->CheckArb();

  // Start point search
  int nSearchStartPoint = -1;
  if (this->nPrevResult == this->nCandidateNum - 1 and this->IsPrevResult_RLAST == ERESULT_TYPE_YES) {
    nSearchStartPoint = 0;
  } else if (this->nPrevResult < this->nCandidateNum - 1 and this->IsPrevResult_RLAST == ERESULT_TYPE_YES) {
    nSearchStartPoint = this->nPrevResult + 1;
  } else {
    nSearchStartPoint = this->nPrevResult;
  };

  // Search first
  for (int i = nSearchStartPoint; i < this->nCandidateNum; i++) {
    if (nCandidateList[i] == 1) { // valid

      this->nPrevResult = i;
      this->IsPrevResult_RLAST = IsLast[i];

      return (i);
    };
  };

  // Search second
  if (nSearchStartPoint != 0) {

    for (int i = 0; i < nSearchStartPoint; i++) {
      if (nCandidateList[i] == 1) {

        this->nPrevResult = i;
        this->IsPrevResult_RLAST = IsLast[i];

        return (i);
      };
    };
  };

  // Nothing arbitrated
  return (-1);
};

// Get arbitration
int CArb::Arbitration_Fixed(int nCandidateList[]) {

  // Debug
  // this->CheckArb();

  for (int i = 0; i < nCandidateNum; i++) {

    if (nCandidateList[i] == 1) { // valid
      this->nPrevResult = i;
      return (i);
    };
  };

  // Nothing arbitrated
  return (-1);
};

// Get name
string CArb::GetName() {

  // Debug
  // this->CheckArb();
  return (this->cName);
};

// Get the latest arbiter result
int CArb::GetArbResult() {

  // Debug
  // this->CheckArb();
  return (this->nPrevResult);
};

// Debug
EResultType CArb::CheckArb() {

  assert(nPrevResult >= -1);
  assert(nPrevResult < this->nCandidateNum);
  return (ERESULT_TYPE_SUCCESS);
};

// Debug
EResultType CArb::Display() {

  // this->CheckArb();
  return (ERESULT_TYPE_SUCCESS);
};
