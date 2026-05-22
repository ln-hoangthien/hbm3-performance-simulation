//------------------------------------------------------------
// Filename	: CTracker.h
// Version	: 0.82
// Date		: 14 Mar 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Priority queue type header
//------------------------------------------------------------
// Cache MO tracker
//------------------------------------------------------------
//
#ifndef CTRACKER_H
#define CTRACKER_H

#include <string>

#include "UD_Bus.h"

using namespace std;

//---------------------------------------
// Cache entry transaction state tracker
//---------------------------------------
typedef enum {
  // MMU, Cache
  ECACHE_STATE_TYPE_HIT,   // Doing service for hit
  ECACHE_STATE_TYPE_PASS,  // NON-CACHEABLE, NON-MMU LOOKUP
  ECACHE_STATE_TYPE_STALL, // Stalled due to (address or ID) dependency. FIXME
                           // Seems no need

  // Cache
  ECACHE_STATE_TYPE_FILL,  // Doing line-fill
  ECACHE_STATE_TYPE_EVICT, // Doing evict

  // MMU
  ECACHE_STATE_TYPE_FIRST_PTW, // Doing first-level "Demand-PTW"
  ECACHE_STATE_TYPE_SECOND_PTW,
  ECACHE_STATE_TYPE_SECOND_PTW_DONE,
  ECACHE_STATE_TYPE_THIRD_PTW,
  ECACHE_STATE_TYPE_FOURTH_PTW,
  ECACHE_STATE_TYPE_FOURTH_PTW_DONE,
  ECACHE_STATE_TYPE_PTW_HIT,

  // MMU
  ECACHE_STATE_TYPE_FIRST_PF, // Doing first-level PF
  ECACHE_STATE_TYPE_SECOND_PF,
  ECACHE_STATE_TYPE_THIRD_PF,

  // MMU RMM
  ECACHE_STATE_TYPE_FIRST_RPTW, // Doing first-level RPTW. RMM
  ECACHE_STATE_TYPE_SECOND_RPTW,
  ECACHE_STATE_TYPE_THIRD_RPTW,

  // MMU AT
  ECACHE_STATE_TYPE_FIRST_APTW, // Doing first-level APTW. AT
  ECACHE_STATE_TYPE_SECOND_APTW,
  ECACHE_STATE_TYPE_SECOND_APTW_DONE,
  ECACHE_STATE_TYPE_THIRD_APTW,
  ECACHE_STATE_TYPE_FOURTH_APTW,
  ECACHE_STATE_TYPE_FOURTH_APTW_DONE, // Done. stalled

  ECACHE_STATE_TYPE_UNDEFINED
} ECacheStateType;

//------------------------------------------
// Convert enum to string
//------------------------------------------
string Convert_eCacheState2string(ECacheStateType eType);

//---------------------------------
// MMU transaction state tracker
//---------------------------------
// typedef enum{
//	EMMU_STATE_TYPE_HIT,		// Doing service for hit
//	EMMU_STATE_TYPE_STALL,		// Stalled due to address or ID
// dependency
//
//	EMMU_STATE_TYPE_FIRST_PTW,	// Doing first-level PTW
//	EMMU_STATE_TYPE_SECOND_PTW,
//	EMMU_STATE_TYPE_THIRD_PTW,
//
//	EMMU_STATE_TYPE_UNDEFINED
// }EMMUStateType;

//---------------------------------
// MMU MO tracker entry
// 	Doubly linked list node (UD queue)
//---------------------------------
typedef struct tagSLinkedCUD *SPLinkedCUD;
typedef struct tagSLinkedCUD {
  UPUD upData;
  EUDType eUDType;

  SPLinkedCUD spPrev;
  SPLinkedCUD spNext;

  ECacheStateType eState;
  int nID; // Transaction ID

  int nCycle_wait; // Waiting time since allocation

} SLinkedCUD;

//---------------------------------
// Priority queue class
//---------------------------------
typedef class CTracker *CPTracker;
class CTracker {

public:
  // 1. Contructor and Destructor
  CTracker(string cName, int nMaxNum);
  ~CTracker();

  // 2. Control
  EResultType Reset();

  // Set value
  EResultType Push(UPUD upUD, EUDType eUDType, ECacheStateType eState);
  UPUD Pop(int nID, EUDType eUDType);
  UPUD Pop(int nID, EUDType eUDType, ECacheStateType eState);
  UPUD Pop(SPLinkedCUD spNode);
  UPUD Pop();

  EResultType SetStateNode(int nID, EUDType eUDTYpe,
                           ECacheStateType eNewState); // (1) Search head node matching ID,
                                                       // UDType. (2) Update new state
  EResultType SetStateNode(int nID, EUDType eUDTYpe, ECacheStateType eOldState,
                           ECacheStateType eNewState); // (1) Search head node matching ID, UDType,
                                                       // state. (2) Update new state

  EResultType UpdateState();

  // Get value
  int GetCurNum();
  int GetMaxNum();
  UPUD GetTop();
  EResultType IsEmpty();
  EResultType IsFull();

  SPLinkedCUD GetNode(int nID, EUDType eUDType, ECacheStateType eState);
  SPLinkedCUD GetNode(ECacheStateType eState);   // Search first node matching state
  SPLinkedCUD GetNode(int nID, EUDType eUDType); //                   matching ID and UDType
  SPLinkedCUD GetNode(int nID,
                      ECacheStateType eState); //                   matching ID and state
  SPLinkedCUD GetNode(int nID);                //                   matching ID
  SPLinkedCUD GetNode_anyPTW();                //                   doing any PTW. APTW. RPTW.
  SPLinkedCUD GetNode_PTW();                   //                   doing PTW

  SPLinkedCUD GetNode_APTW_AT(); // Search first node doing APTW. AT.
  SPLinkedCUD GetNode_Pair_APTW_VA_AT(int64_t nVA,
                                      EUDType eUDType); //                   doing APTW (for VA) matching UDType
  SPLinkedCUD GetNode_Pair_PTW_VA_AT(int64_t nVA,
                                     EUDType eUDType); //                   doing PTW  (for VA) matching UDType
  // SPLinkedCUD	GetNode_APTW_AT();
  // //                   doing any PTW. AT

  EResultType IsIDMatch(int nID, EUDType eUDTYpe); // Check any node matching ID and UDType
  // EResultType	IsAddrMatch(int64_t nAddr);
  // //                matching address. FIXME need?
  EResultType IsAnyPTW_Match_VPN(int64_t nVA);  //                doing PTW matching VPN.
  EResultType IsAddrMatch_Cache(int64_t nAddr); //                matching address cache-line

  EResultType IsAnyLineFill(); // Check any node doing line-fill
  EResultType IsSameID_LineFill(int nID,
                                EUDType eUDType); //                matching ID and UDType
                                                  //                is doing line-fill

  EResultType IsIDMatchPTW(int nID, EUDType eUDTYpe); // Check any node matching ID and
                                                      // UDType. Check the node is doing PTW
  EResultType IsUDMatchPTW(EUDType eUDTYpe);          //                matching UDType.        Check
                                                      //                the node is doing PTW
  EResultType IsAnyPTW();                             //                doing PTW, APTW

  EResultType IsRPTW_sameVPN_Match_RMM(int64_t nVPN_aligned); // Check any node doing RPTW matching Addr. RMM

  // Stat
  int GetMaxCycleWait(); // Max waiting cycle among entries

  // Debug
  EResultType CheckTracker();
  EResultType Display();

private:
  // Original info
  string cName;
  int nCurNum;
  int nMaxNum;

  // Control info

  // Stat
  int nMaxCycleWait; // Max waiting time (among all entries)

public:
  // Node
  SPLinkedCUD spCUDList_head;
  SPLinkedCUD spCUDList_tail;
};

#endif
