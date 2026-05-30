//-----------------------------------------------------------
// Filename     : Memory.cpp
// Version	: 0.7
// Date         : 28 Feb 2018
// Contact	: JaeYoung.Hur@gmail.com
// Description  : Memory global functions
//-----------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>

#include "Memory.h"

string Convert_eMemCmd2string(EMemCmdType eType) {

  switch (eType) {
  case EMEM_CMD_TYPE_ACT:
    return ("ACT");
    break;
  case EMEM_CMD_TYPE_PRE:
    return ("PRE");
    break;
  case EMEM_CMD_TYPE_RD:
    return ("RD");
    break;
  case EMEM_CMD_TYPE_WR:
    return ("WR");
    break;
  case EMEM_CMD_TYPE_NOP:
    return ("NOP");
    break;
  case EMEM_CMD_TYPE_UNDEFINED:
    assert(0);
    return ("EMEM_CMD_TYPE_UNDEFINED");
  default:
    break;
  };
  return (NULL);
};
