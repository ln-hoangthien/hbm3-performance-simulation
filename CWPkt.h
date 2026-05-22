//------------------------------------------------------------
// FileName	: CWPkt.h
// Version	: 0.77
// DATE 	: 10 Mar 2019
// Contact	: JaeYoung.Hur@gmail.com
// Description	: W packet class header
//------------------------------------------------------------
// Note
// 1. When member added, modify Copy_WPkt function in UD_Bus.cpp
//------------------------------------------------------------
#ifndef CWPKT_H
#define CWPKT_H

#include "Top.h"
#include <stdio.h>
#include <string>

using namespace std;

//------------------------------
// W pkt
//------------------------------
typedef struct tagSWPkt *SPWPkt;
typedef struct tagSWPkt {
  int nID;
  int nData;
  int nLast;
} SWPkt;

//------------------------------
// W pkt class
//------------------------------
typedef class CWPkt *CPWPkt;
class CWPkt {

public:
  // 1. Contructor and Destructor
  CWPkt(string cName);
  CWPkt();
  ~CWPkt();

  // 2. Control
  // Set value
  // EResultType	SetPkt(SPWPkt spPkt_new);
  EResultType SetPkt(int nID, int nData, int nLast);
  EResultType SetName(string cName);
  EResultType SetID(int nID);
  EResultType SetData(int nData);
  EResultType SetLast(EResultType eResult);

  EResultType SetTransType(ETransType eType);
  EResultType SetSrcName(string cName);

  // Get value
  SPWPkt GetPkt();
  string GetName();
  int GetID();
  int GetData();
  EResultType IsLast();
  ETransType GetTransType();
  string GetSrcName();

  // Debug
  EResultType Display();
  EResultType CheckPkt();

private:
  // Original pkt info
  SPWPkt spPkt;

  // Control info
  string cName;
  ETransType eTransType; // Normal, evict
  string cSrcName;       // Who generates pkt. Master, cache, MMU
};

#endif
