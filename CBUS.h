//------------------------------------------------------------
// Filename	: CBUS.h 
// Version	: 0.78
// Date		: 20 Aug 2018
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Bus header 
//------------------------------------------------------------
#ifndef CBUS_H
#define CBUS_H

#include <string>
#include <math.h>

#include "Top.h"
#include "UD_Bus.h"
#include "CFIFO.h"
#include "CTRx.h"
#include "CArb.h"

using namespace std;


typedef class CBUS* CPBUS;
class CBUS{

public:
	// 1. Contructor. Destructor
	CBUS(string cName, int NUM_PORT);
	~CBUS();
	
	// 2. Control
	// Handler
	EResultType	Do_AR_fwd(int64_t nCycle);
	EResultType	Do_AR_bwd(int64_t nCycle);
	EResultType	Do_AW_fwd(int64_t nCycle);
	EResultType	Do_AW_bwd(int64_t nCycle);
	#ifdef CCI_ON
		EResultType	Do_AC_fwd(int64_t nCycle);
		EResultType	Do_AC_bwd(int64_t nCycle);
		EResultType	Do_CR_fwd(int64_t nCycle);
		EResultType	Do_CR_bwd(int64_t nCycle);
		EResultType	Do_W_snoop_fwd(int64_t nCycle);
		EResultType	Do_CD_fwd(int64_t nCycle);
		EResultType	Do_CD_bwd(int64_t nCycle);
	#endif
	EResultType	Do_R_fwd(int64_t nCycle);
	EResultType	Do_R_bwd(int64_t nCycle);
	EResultType	Do_W_fwd(int64_t nCycle);
	EResultType	Do_W_bwd(int64_t nCycle);
	EResultType	Do_B_fwd(int64_t nCycle);
	EResultType	Do_B_bwd(int64_t nCycle);

	// Set value
	EResultType	Increase_MO_AR();
	EResultType	Decrease_MO_AR();
	EResultType	Increase_MO_AW();
	EResultType	Decrease_MO_AW();

	// Get value
	int		GetMO_AR(); 	
	int		GetMO_AW(); 

	int		GetPortNum(int nID); 

	// Control
	EResultType	UpdateState(int64_t nCycle);
	EResultType	Reset();
	EResultType	PrintStat(int64_t nCycle, FILE *fp);

	// Debug
	EResultType	CheckLink();

	// Port slave interface
	CPTRx*		cpRx_AR;				// [NUM_PORT]
	CPTRx*		cpTx_R;
	CPTRx*		cpRx_AW;
	CPTRx*		cpRx_W;
	CPTRx*		cpTx_B;

	// Port master interface
	CPTRx		cpTx_AR;
	CPTRx		cpRx_R;
	CPTRx		cpTx_AW;
	CPTRx		cpTx_W;
	CPTRx		cpRx_B;

	#ifdef CCI_ON
		// CCI specific interface
		CPTRx* 		cpTx_AC; // [NUM_PORT]

		CPTRx*		cpRx_CD; // [NUM_PORT]
		CPTRx*		cpRx_CR; // [NUM_PORT]
	#endif

private:
    // Original
	string		cName;

	// Control
	int		NUM_PORT;				// Num port
	int		BIT_PORT;				// Bits port
	
	int		nMO_AR;					// MO count AR
	int		nMO_AW;	

	// Stat
	int*		nAR_SI;				// [NUM_PORT]
	int*		nAW_SI;
	int*		nR_SI ;
	int*		nB_SI ;

	// Arbiter
	CPArb		cpArb_AR;
	CPArb		cpArb_AW;

	// FIFO
	// CPFIFO	cpFIFO_AR;
	// CPFIFO	cpFIFO_R;
	CPFIFO		cpFIFO_AW;
	// CPFIFO	cpFIFO_W;
	// CPFIFO	cpFIFO_B;

	#ifdef CCI_ON
		CPFIFO*		cpFIFO_MstAW; // [NUM_PORT]
		CPFIFO*		cpFIFO_MstAR; // [NUM_PORT]

		//=======================================================
		// Purpose:
		// 		cpFIFO_SnoopData: Storing the returned data from snooped master. The data can be issue to the main memory for WR operation or send-back to the initiating Master.
		// 		cpFIFO_SnoopResp: Storing the returned response from snooped master.
		// 		cpFIFO_ActiveSnoopAx: Storing the ID of returned response from snooped master.
		// 		cpFIFO_ActiveSnoopAC: Storing the ACSnoop types of returned data. This is used to determine the destination of the returned response and data.
		//=======================================================
		CPFIFO		cpFIFO_SnoopData;
		CPFIFO*		cpFIFO_SnoopResp;
		CPFIFO		cpFIFO_ActiveSnoopAx;
		CPFIFO		cpFIFO_ActiveSnoopAC;
		int*		nSnoopedMaster;
		bool* 		bArb; // Round-robin for CCI snoop
		std::vector<int> m_outstandingMemARID;
		std::vector<int> m_outstandingMemAWID;
	#endif
};

#endif

