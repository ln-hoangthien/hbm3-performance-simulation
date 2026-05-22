//------------------------------------------------------------
// Filename	: CQ.h
// Version	: 0.7
// Date		: 13 Mar 2018
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Queue type header
//------------------------------------------------------------
// Request queue in memory controller
//------------------------------------------------------------
#ifndef CQ_H
#define CQ_H

#include <string>

#include "MUD_Mem.h"
#include "UD_Bus.h"

using namespace std;

//--------------------------------
// Delay cycle in queue
//--------------------------------
#define Q_MIN_WAITING_CYCLE 2

//--------------------------------
// Queue class
//--------------------------------
typedef class CQ *CPQ;
class CQ {

public:
  // 1. Contructorr Destructor
  CQ(string cName, EUDType eType, int nMaxNum);
  ~CQ();

  // 2. Control
  EResultType Reset();

  // Set value
  EResultType Push(UPUD upUD);
  UPUD Pop(uint64_t nKey);
  UPUD Pop();

  EResultType SetMemStateCmdPkt(SPMemStatePkt spThis); // Set state. For all entires, get cmd. Set Cmd

  EResultType UpdateState();

  // Get value
  EUDType GetUDType();
  int GetCurNum();
  int GetMaxNum();
  UPUD GetTop();
  EResultType IsEmpty();
  EResultType IsFull();

  EResultType IsThereIDHeadBankHit(int nBank);       // Check bank hit for any ID head
  EResultType IsThisFirstInBank(SPLinkedMUD spThis); // Check this node is first (in req Q)
                                                     // to access target bank

  // SPMemCmdPkt	GetMemCmdPkt(SPLinkedMUD spThis);		//
  // Generate Cmd for MemState

  EMemCmdType GetMemCmd(SPLinkedMUD spThis); // Generate Cmd for MemState
  SPLinkedMUD GetIDHeadNode(uint64_t nID);

  SPMemStatePkt GetMemStatePkt();

  // Stat
  int GetMaxCycleWait();         // Max waiting cycle among entries
  int GetCycleWait(UPUD upThis); // Waiting cycle

  // Debug
  EResultType CheckQ();
  EResultType Display();

private:
  // Original
  string cName;
  EUDType eUDType;
  int nCurNum;
  int nMaxNum;

  // Control
  SPMemStatePkt spMemStatePkt; // Mem state global

  // Stat
  int nMaxCycleWait; // Max waiting time (among all entries)

public:
  // Node
  SPLinkedMUD spMUDList_head;
  SPLinkedMUD spMUDList_tail;
};

#endif
