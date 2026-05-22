//-----------------------------------------------------------
// FileName	: CBPkt.cpp
// Version	: 0.79
// Date		: 26 Feb 2020
// Contact	: JaeYoung.Hur@gmail.com
// Description	: B Pkt class definition
//-----------------------------------------------------------
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "CBPkt.h"

// Construct
CBPkt::CBPkt(string cName) {

  // Generate and initialize
  this->spPkt = new SBPkt;
  this->spPkt->nID = -1;
  this->cName = cName;
  this->nMemCh = -1;
  this->nCacheCh = -1;
};

// Construct
CBPkt::CBPkt() {

  // Generate and initialize
  this->spPkt = new SBPkt;
  this->spPkt->nID = -1;
  this->cName = "B_pkt";
  this->nMemCh = -1;
  this->nCacheCh = -1;
};

// Destruct
CBPkt::~CBPkt() {

  this->CheckPkt();

  delete (this->spPkt);
  this->spPkt = NULL;
};

// Set name
EResultType CBPkt::SetName(string cName) {

  this->cName = cName;
  return (ERESULT_TYPE_SUCCESS);
};

//// Set B pkt
// EResultType CBPkt::SetPkt(SPBPkt spPkt) {
//
//	// Debug
//	assert (this->spPkt != NULL);
//
//	this->spPkt = spPkt;  // spPkt generated when CBPkt generated
//	return (ERESULT_TYPE_SUCCESS);
// };

// Set B pkt
EResultType CBPkt::SetPkt(int nID) {

#ifdef DEBUG
  assert(this->spPkt != NULL);
#endif

  // Assign
  this->spPkt->nID = nID;

  return (ERESULT_TYPE_SUCCESS);
};

// Set B id
EResultType CBPkt::SetID(int nID) {

#ifdef DEBUG
  assert(this->spPkt != NULL);
#endif

  this->spPkt->nID = nID;
  return (ERESULT_TYPE_SUCCESS);
};

// Set final trans
EResultType CBPkt::SetFinalTrans(EResultType eResult) {

#ifdef DEBUG
  assert(this->spPkt != NULL);
#endif

  this->eFinalTrans = eResult;
  return (ERESULT_TYPE_SUCCESS);
};

// Set mem ch num
EResultType CBPkt::SetMemCh(int nMemCh) {

#ifdef DEBUG
  assert(this->spPkt != NULL);
#endif

  this->nMemCh = nMemCh;
  return (ERESULT_TYPE_SUCCESS);
};

// Set cache ch num
EResultType CBPkt::SetCacheCh(int nCacheCh) {

#ifdef DEBUG
  assert(this->spPkt != NULL);
#endif

  this->nCacheCh = nCacheCh;
  return (ERESULT_TYPE_SUCCESS);
};

// Get B pkt
SPBPkt CBPkt::GetPkt() {

  // this->CheckPkt();
  return (this->spPkt);
};

// Get B pkt name
string CBPkt::GetName() {

  // this->CheckPkt();
  return (this->cName);
};

// Get ID
int CBPkt::GetID() {

  // this->CheckPkt();
  return (this->spPkt->nID);
};

// Check final trans
EResultType CBPkt::IsFinalTrans() {

  // this->CheckPkt();
  return (this->eFinalTrans);
};

// Get mem ch num
int CBPkt::GetMemCh() {

  // this->CheckPkt();
  return (this->nMemCh);
};

// Get cache ch num
int CBPkt::GetCacheCh() {

  // this->CheckPkt();
  return (this->nCacheCh);
};

// Debug
EResultType CBPkt::CheckPkt() {

  assert(this != NULL);
  assert(this->spPkt != NULL);
  assert(this->spPkt->nID >= 0);
  assert(this->spPkt->nID < 1000);

  return (ERESULT_TYPE_SUCCESS);
};

// Debug
EResultType CBPkt::Display() {

  // this->CheckPkt();

  string cFinalTrans = Convert_eResult2string(this->eFinalTrans);

  // printf("---------------------------------------------\n");
  // printf("\t B pkt display\n");
  printf("---------------------------------------------\n");
  printf("\t Name		: \t %s\n", this->cName.c_str());
  printf("\t ID		: \t 0x%x\n", this->spPkt->nID);
  printf("\t FinalTrans	: \t %s\n", cFinalTrans.c_str());
  printf("\t MemCh	: \t %d\n", this->nMemCh);
  printf("\t CacheCh	: \t %d\n", this->nCacheCh);
  printf("---------------------------------------------\n");

  return (ERESULT_TYPE_SUCCESS);
};
