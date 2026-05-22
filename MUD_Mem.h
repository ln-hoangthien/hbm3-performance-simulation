//------------------------------------------------------------
// Filename	: MUD_Mem.h
// Version	: 0.7
// Date		: 2 Mar 2018
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Unified data type header
//------------------------------------------------------------
// Request queue entry in memory controller
// This is a global type for memory access
//------------------------------------------------------------
#ifndef MUD_MEM_H
#define MUD_MEM_H

#include "Memory.h"
#include "Top.h"
#include "UD_Bus.h"

//----------------------------
// Linked list node (for UD queue)
//----------------------------
typedef struct tagSLinkedMUD *SPLinkedMUD;
typedef struct tagSLinkedMUD {
  UPUD upData;
  EUDType eUDType;
  SPLinkedMUD spPrev;
  SPLinkedMUD spNext;

  SPMemCmdPkt spMemCmdPkt; // Scheduler Q entry
  uint64_t nID;            // Transaction ID

  uint64_t nCycle_wait; // Waiting time since allocation
  // EResultType	IsBankPrepare;		// Scheduler. MC send ACT to
  // prepare bank
} SLinkedMUD;

#endif
