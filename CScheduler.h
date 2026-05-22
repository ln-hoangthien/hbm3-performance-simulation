//------------------------------------------------------------
// FileName	: CScheduler.h
// Version	: 0.71
// DATE 	: 4 Feb 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: MC scheduler
//------------------------------------------------------------
#ifndef CScheduler_H
#define CScheduler_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "CQ.h"
#include "MUD_Mem.h"
#include "Memory.h"
#include "Top.h"
#include "UD_Bus.h"

using namespace std;

//-------------------------------------
// PTW top priority
//-------------------------------------
#define PTW_HIGH_PRIORITY

//-------------------------------------
// Scheduling
//-------------------------------------
// #define FIRST_COME_FIRST_SERVE
#define BANK_HIT_FIRST

//-------------------------------------
// Aggressive PRE cmd
//-------------------------------------
#define NO_AGGRESSIVE_PRE

//-------------------------------------
// Cmd scheduler
//-------------------------------------
typedef class CScheduler *CPScheduler;
class CScheduler {

public:
  // 1. Contructor and Destructor
  CScheduler(string cName);
  ~CScheduler();

  // 2. Control
  EResultType Reset();

  // Get value
  string GetName();

  // SPLinkedMUD	GetScheduledMUD(CPQ cpQ);
  // // Scheduler SPLinkedMUD	GetScheduledMUD_ReadFirst(CPQ cpQ_AR, CPQ
  // cpQ_AW);
  SPLinkedMUD GetScheduledMUD_FIFO(CPQ cpQ_AR, CPQ cpQ_AW, int64_t nCycle);
  SPLinkedMUD GetScheduledMUD_Aggressive(CPQ cpQ_AR, CPQ cpQ_AW,
                                         int64_t nCycle); // Aggressive bank preparation
  SPLinkedMUD GetScheduledMUD(CPQ cpQ_AR, CPQ cpQ_AW,
                              int64_t nCycle); // Priority bank praparation

  // Control
  // EResultType	UpdateState();

  // Debug
  EResultType CheckScheduler();
  EResultType Display();

private:
  // Original
  string cName;

  // Control
  // CPQ		cpQ_AR;
  // CPQ		cpQ_AW;

  EResultType IsAR_priority; // Schedule AR priority
  EResultType IsAW_priority;
};

#endif
