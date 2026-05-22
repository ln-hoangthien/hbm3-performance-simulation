//-----------------------------------------------------------
// FileName	: CWPkt.cpp
// Version	: 0.78
// Date		: 17 Nov 2019
// Contact	: JaeYoung.Hur@gmail.com
// Description	: W Pkt class definition
//-----------------------------------------------------------
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "CWPkt.h"

// Construct
CWPkt::CWPkt(string cName) {

  // Generate and initialize
  this->spPkt = new SWPkt;
  this->spPkt->nID = -1;
  this->spPkt->nData = -1;
  this->spPkt->nLast = -1;

  this->cName = cName;
  this->cSrcName = "Src";
  this->eTransType = ETRANS_TYPE_UNDEFINED;
};

// Construct
CWPkt::CWPkt() {

  // Generate and initialize
  this->spPkt = new SWPkt;
  this->spPkt->nID = -1;
  this->spPkt->nData = -1;
  this->spPkt->nLast = -1;

  this->cName = "W_pkt";
  this->cSrcName = "Src";
  this->eTransType = ETRANS_TYPE_UNDEFINED;
};

// Destruct
CWPkt::~CWPkt() {

  this->CheckPkt();

  delete (this->spPkt);
  this->spPkt = NULL;
};

// Set name
EResultType CWPkt::SetName(string cName) {

  this->cName = cName;
  return (ERESULT_TYPE_SUCCESS);
};

//// Set W pkt
// EResultType CWPkt::SetPkt(SPWPkt spPkt) {
//
//	// Debug
//	assert (this->spPkt != NULL);
//
//	this->spPkt = spPkt;  // spPkt generated outside
//	return (ERESULT_TYPE_SUCCESS);
// };

// Set W pkt
EResultType CWPkt::SetPkt(int nID, int nData, int nLast) {

#ifdef DEBUG
  assert(this->spPkt != NULL);
#endif

  // Assign
  this->spPkt->nID = nID;
  this->spPkt->nData = nData;
  this->spPkt->nLast = nLast;

  return (ERESULT_TYPE_SUCCESS);
};

// Set W id
EResultType CWPkt::SetID(int nID) {

#ifdef DEBUG
  assert(this->spPkt != NULL);
#endif

  this->spPkt->nID = nID;
  return (ERESULT_TYPE_SUCCESS);
};

// Set W data
EResultType CWPkt::SetData(int nData) {

#ifdef DEBUG
  assert(this->spPkt != NULL);
#endif

  this->spPkt->nData = nData;
  return (ERESULT_TYPE_SUCCESS);
};

// Set W last
EResultType CWPkt::SetLast(EResultType eLast) {

#ifdef DEBUG
  assert(this->spPkt != NULL);
#endif

  if (eLast == ERESULT_TYPE_YES) {
    this->spPkt->nLast = 1;
  } else if (eLast == ERESULT_TYPE_NO) {
    this->spPkt->nLast = 0;
  } else {
    assert(0);
  };
  return (ERESULT_TYPE_SUCCESS);
};

// Set transaction type (normal, PTW, evict, line fill)
EResultType CWPkt::SetTransType(ETransType eType) {

#ifdef DEBUG
  assert(this->spPkt != NULL);
#endif

  this->eTransType = eType;
  return (ERESULT_TYPE_SUCCESS);
};

// Set src name
EResultType CWPkt::SetSrcName(string cName) {

#ifdef DEBUG
  assert(this->spPkt != NULL);
#endif

  this->cSrcName = cName;
  return (ERESULT_TYPE_SUCCESS);
};

// Get W pkt
SPWPkt CWPkt::GetPkt() {

  this->CheckPkt();
  return (this->spPkt);
};

// Get W pkt name
string CWPkt::GetName() {

  this->CheckPkt();
  return (this->cName);
};

// Get ID
int CWPkt::GetID() {

  this->CheckPkt();
  return (this->spPkt->nID);
};

// Get data
int CWPkt::GetData() {

  this->CheckPkt();
  return (this->spPkt->nData);
};

// Get trans type
ETransType CWPkt::GetTransType() {

  this->CheckPkt();
  return (this->eTransType);
};

// Get src name
string CWPkt::GetSrcName() {

  this->CheckPkt();
  return (this->cSrcName);
};

// Check W last
EResultType CWPkt::IsLast() {

  this->CheckPkt();

  if (this->spPkt->nLast == 1) {
    return (ERESULT_TYPE_YES);
  } else if (this->spPkt->nLast == 0) {
    return (ERESULT_TYPE_NO);
  } else {
    assert(0);
  };

  return (ERESULT_TYPE_NO);
};

// Debug
EResultType CWPkt::CheckPkt() {

  assert(this != NULL);
  assert(this->spPkt != NULL);
  assert(this->spPkt->nLast == 1 or this->spPkt->nLast == 0);
  assert(this->spPkt->nID >= 0);
  assert(this->spPkt->nID < 1000);
  assert(this->spPkt->nData >= 0);
  // assert (this->spPkt->nData <  16);

  return (ERESULT_TYPE_SUCCESS);
};

// Debug
EResultType CWPkt::Display() {

  this->CheckPkt();

  // printf("---------------------------------------------\n");
  // printf("\t W pkt display\n");
  printf("---------------------------------------------\n");
  printf("\t Name	: \t %s\n", this->cName.c_str());
  printf("\t ID	: \t 0x%x\n", this->spPkt->nID);
  printf("\t DATA	: \t 0x%x\n", this->spPkt->nData);
  printf("\t Last	: \t 0x%x\n", this->spPkt->nLast);
  printf("---------------------------------------------\n");

  return (ERESULT_TYPE_SUCCESS);
};
