//------------------------------------------------------------
// Filename	: CROB.h
// Version	: 0.1
// Date		: 2 Mar 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Queue type header
//------------------------------------------------------------
// Superset of FIFO
// Same as FIFO except:
// 	(1) Out-of-order pop
// 	(2) Double linked list
// 	(3) Memory channel info
//------------------------------------------------------------
#ifndef CROB_H
#define CROB_H

#include <string>

#include "UD_Bus.h"

using namespace std;

//-------------------------------
// Linked list node (for UD Queue. ROB)
//-------------------------------
typedef struct tagSLinkedRUD *SPLinkedRUD;
typedef struct tagSLinkedRUD {
  UPUD upData;
  int nID;
  int nLatency;

  int nMemCh; // Memory channel (0,1,2,..)
  // EResultType	IsHead_Resp;				// only need in
  // R,B

  int nCycle_wait; // Waiting cycles since allocation
  SPLinkedRUD spPrev;
  SPLinkedRUD spNext;
} SLinkedRUD;

//----------------------------
// ROB class
//----------------------------
typedef class CROB *CPROB;
class CROB {

public:
  // 1. Contructor. Destructor
  CROB(string cName, EUDType eType, int nMaxNum);
  ~CROB();

  // 2. Control
  // Control
  EResultType Reset();

  // Set value
  EResultType Push(UPUD upUD, int nLatency);
  EResultType Push(UPUD upUD);
  EResultType Push(UPUD upUD, int nMemCh, int nLatency);

  UPUD Pop();         // In order
  UPUD Pop(int nKey); // Out of order
  UPUD Pop(int nKey, int nMemCh);

  // EResultType	SetHead_Resp(int nID, int nMemCh); // ROB_R,B

  EResultType UpdateState();

  // Get value
  string GetName();
  EUDType GetUDType();
  int GetCurNum();
  int GetMaxNum();
  UPUD GetTop();
  UPUD GetTarget(int nID);       // ROB
  int GetMemCh_ReqHead(int nID); // ROB_Ax

  SPLinkedRUD GetReqNode_ROB(int nID, int nMemch);
  // SPLinkedRUD	GetResp_poppable_ROB(CPROB cpResp, CPROB cpReq);
  SPLinkedRUD GetResp_poppable_ROB(CPROB cpReq);

  EResultType IsEmpty();
  EResultType IsFull();

  // Stat
  // int		GetMaxCycleWait();
  // // Max waiting cycle among entries int		GetCycleWait(UPUD
  // upThis);				// Waiting cycle

  // Debug
  EResultType CheckROB();
  EResultType Display();

private:
  // Original info
  string cName;
  EUDType eUDType;
  int nCurNum;
  int nMaxNum;

  // Stat
  // int		nMaxCycleWait;
  // // Max waiting time (among all entries)

  // Node
  SPLinkedRUD spRUDList_head;
  SPLinkedRUD spRUDList_tail;
};

#endif
