//------------------------------------------------------------
// FileName	: CBPkt.h
// Version	: 0.80
// DATE 	: 15 Nov 2022
// Contact	: JaeYoung.Hur@gmail.com
// Description	: B packet class header
//------------------------------------------------------------
// Note
// 1. When member added, modify "Copy_BPkt" function in "UD_Bus.cpp"
//------------------------------------------------------------
#ifndef CBPKT_H
#define CBPKT_H

#include "Top.h"
#include <stdio.h>
#include <string>

using namespace std;

// B pkt
typedef struct tagSBPkt *SPBPkt;
typedef struct tagSBPkt {
  int nID;
} SBPkt;

// B pkt class
typedef class CBPkt *CPBPkt;
class CBPkt {

public:
  // 1. Contructor and Destructor
  CBPkt(string cName);
  CBPkt();
  ~CBPkt();

  // 2. Control
  // Set value
  // EResultType	SetPkt(SPBPkt spPkt_new);
  EResultType SetPkt(int nID);
  EResultType SetName(string cName);
  EResultType SetID(int nID);
  EResultType SetFinalTrans(EResultType eResult);
  EResultType SetMemCh(int nMemCh);
  EResultType SetCacheCh(int nCacheCh);

  // Get value
  SPBPkt GetPkt();
  string GetName();
  int GetID();
  EResultType IsFinalTrans();
  int GetMemCh();
  int GetCacheCh();

  // Debug
  EResultType Display();
  EResultType CheckPkt();

private:
  // Original pkt info
  SPBPkt spPkt;

  // Control info
  string cName;
  EResultType eFinalTrans;
  int nMemCh;
  int nCacheCh;
};

#endif
