//------------------------------------------------------------
// Filename	: CCAM.h
// Version	: 0.11
// Date		: 24 Feb 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: CAM type header
//------------------------------------------------------------
// Request tracker in MIU
// Structure	: Queue of FIFO
//------------------------------------------------------------
// Note
// 	(1) FIXME Under development
//------------------------------------------------------------
#ifndef CCAM_H
#define CCAM_H

#include <string>

#include "CFIFO.h"
#include "UD_Bus.h"

using namespace std;

typedef struct tagsLinkedFIFO *SPLinkedFIFO;
typedef struct tagsLinkedFIFO {
  CPFIFO cpFIFO;
  SPLinkedFIFO spPrev;
  SPLinkedFIFO spNext;
} SLinkedFIFO;

//----------------------------
// CAM class
//----------------------------
typedef class CCAM *CPCAM;
class CCAM {

public:
  // 1. Contructor and Destructor
  CCAM(string cName, EUDType eType, int nMaxNum); // FIXME FIFO size
  ~CCAM();

  // 2. Control
  // Set value
  EResultType Push(UPUD upUD, int nKey);
  UPUD Pop(nKey);

  EResultType UpdateState();

  // Get value
  EUDType GetUDType() int GetCurNum();
  int GetMaxNum();
  CPFIFO GetTopFIFO();
  CPFIFO GetFIFO(nKey);
  // UPUD		GetTarget(int nKey, EResultType (*SearchMethod)(CAxPkt
  // cpThis));

  EResultType IsEmpty();
  EResultType IsExist(nKey);
  EResultType IsPushable(nKey);
  // EResultType	IsTheOtherExist(nKey);

  // Control
  EResultType Reset();

  // Debug
  EResultType CheckCAM();
  EResultType Display();

private:
  // Original info
  string cName;
  EUDType eUDType;
  int nMaxNum;

  // List node
  SPLinkedFIFO spFIFOList_head;
  SPLinkedFIFO spFIFOList_tail;
};

#endif
