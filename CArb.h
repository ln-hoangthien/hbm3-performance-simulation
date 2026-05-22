//------------------------------------------------------------
// FileName	: CArb.h
// Version	: 0.72
// DATE 	: 2 Mar 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Arbiter header
//------------------------------------------------------------
#ifndef CARB_H
#define CARB_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "Top.h"

using namespace std;

//-----------------------------
// Arbiter
//-----------------------------
typedef class CArb *CPArb;
class CArb {

public:
  // 1. Contructor and Destructor
  CArb(string cName, EArbType eArbType, int nCandidateNum);
  CArb(string cName, int nCandidateNum);
  ~CArb();

  // 2. Control
  // Set value
  EResultType Reset();
  EResultType UpdateState();

  // Get value
  string GetName();

  int Arbitration(int nCandidateList[]);
  int Arbitration_RR(int nCandidateList[]);
  int Arbitration_RR(int nCandidateList[],
                     EResultType IsLast[]); // R transaction level.
  int Arbitration_Fixed(int nCandidateList[]);

  int GetArbResult();

  // Debug
  EResultType CheckArb();
  EResultType Display();

private:
  // Control info
  string cName;
  EArbType eArbType;              // RR, Fixed
  int nCandidateNum;              // Number of arbiter candidates
  int nPrevResult;                // Previous arbitrated result
  EResultType IsPrevResult_RLAST; // Previous arbitrated result RLAST
};

#endif
