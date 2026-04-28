//------------------------------------------------------------
// Filename	: CMST.h 
// Version	: 0.78
// Date		: 10 Dec 2018
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Master header 
//------------------------------------------------------------
#ifndef CMST_H
#define CMST_H

#include <string>

#include "Top.h"
#include "UD_Bus.h"
#include "CFIFO.h"
#include "CTRx.h"
#include "CAddrGen.h"

using namespace std;

//-------------------------------------------- 
// FIFO latency 
//-------------------------------------------- 
#define MST_FIFO_LATENCY		0

//DUONGTRAN add for Matrix operation
typedef enum{
    ESTATE_ADDR_INI,
    ESTATE_ADDR_LDA,
    ESTATE_ADDR_LDB,
    ESTATE_ADDR_LDC,
    ESTATE_ADDR_STC,
    ESTATE_ADDR_LDD,
    ESTATE_ADDR_STD,
    ESTATE_ADDR_LDE,
    ESTATE_ADDR_STE,
    ESTATE_ADDR_UND,
}EStateGenAddr;	

//-------------------------------------------- 
// Transaction issue interval 
// 	Master can issue every interval cycles
//-------------------------------------------- 
// #define MST_AR_ISSUE_MIN_INTERVAL	4
// #define MST_AW_ISSUE_MIN_INTERVAL	4


//-------------------------------------------- 
// Master class
//-------------------------------------------- 
typedef class CMST* CPMST;
class CMST{

public:
	// 1. Contructor and Destructor
	CMST(string cName);
	~CMST();
	
	// 2. Control
	// Handler
	EResultType	Do_AR_fwd(int64_t nCycle);
	EResultType	Do_AR_bwd(int64_t nCycle);
	EResultType	Do_AW_fwd(int64_t nCycle);
	EResultType	Do_AW_bwd(int64_t nCycle);
	EResultType	Do_R_fwd(int64_t nCycle);
	EResultType	Do_R_bwd(int64_t nCycle);
	EResultType	Do_W_fwd(int64_t nCycle);
	EResultType	Do_W_bwd(int64_t nCycle);
	EResultType	Do_B_fwd(int64_t nCycle);
	EResultType	Do_B_bwd(int64_t nCycle);

	#ifdef CCI_ON
		EResultType	Do_AC_fwd(int64_t nCycle);
		EResultType	Do_AC_bwd(int64_t nCycle);
		EResultType	Do_CR_fwd(int64_t nCycle);
		EResultType	Do_CR_bwd(int64_t nCycle);
		EResultType	Do_CD_fwd(int64_t nCycle);
		EResultType	Do_CD_bwd(int64_t nCycle);
	#endif

	EResultType	LoadTransfer_MatrixConvolution(int64_t nCycle);  			// DUONGTRAN add
	EResultType	LoadTransfer_MatrixConvolutionLocalCache(int64_t nCycle);
	EResultType	LoadTransfer_MatrixConvolutionTransaction(int64_t nCycle);
	EResultType	LoadTransfer_MatrixTranspose(int64_t nCycle);
	EResultType	LoadTransfer_MatrixTransposeLocalCache(int64_t nCycle);
	EResultType	LoadTransfer_MatrixTransposeTransaction(int64_t nCycle);
	EResultType	LoadTransfer_MatrixMultiplication(int64_t nCycle);
	EResultType	LoadTransfer_MatrixMultiplicationLocalCache(int64_t nCycle);
	EResultType	LoadTransfer_MatrixMultiplicationTransaction(int64_t nCycle);
	EResultType	LoadTransfer_MatrixMultiplicationTileTransaction(int64_t nCycle);
	EResultType	LoadTransfer_MatrixMultiplicationKIJ(int64_t nCycle);
	EResultType	LoadTransfer_MatrixMultiplicationKIJLocalCache(int64_t nCycle);
	EResultType	LoadTransfer_MatrixSobel(int64_t nCycle);
	EResultType	LoadTransfer_MatrixSobelLocalCache(int64_t nCycle);
	EResultType	LoadTransfer_MatrixSobelTransaction(int64_t nCycle);

	EResultType	LoadTransfer_AR_Test(int64_t nCycle);					// Debug
	EResultType	LoadTransfer_AW_Test(int64_t nCycle);

	EResultType LoadTransfer_AR_AXI_Trace(int64_t nCycle);				// Trace AXI FIXME
	EResultType LoadTransfer_AW_AXI_Trace(int64_t nCycle);

	EResultType LoadTransfer_Ax_PIN_Trace(int64_t nCycle, FILE *fp);			// Trace PIN
	EResultType LoadTransfer_Ax_Custom(int64_t nCycle, FILE *fp, int nLen);			// Trace Custom

	EResultType	LoadTransfer_AR(int64_t nCycle, string cAddrMap, string cOperation);	// cAddrMap (LIAM, BFAM, TILE). cOperation (Rotation, Raster_scan, CNN)
	EResultType	LoadTransfer_AW(int64_t nCycle, string cAddrMap, string cOperation);

	// Set value
	// EResultType	Set_TransGen_ON();
	// EResultType	Set_TransGen_OFF();
	EResultType	SetAllTransFinished(EResultType eResult);
	EResultType	SetARTransFinished(EResultType eResult);
	EResultType	SetAWTransFinished(EResultType eResult);

	EResultType	Set_nCycle_AR_Finished(int64_t nCycle);
	EResultType	Set_nCycle_AW_Finished(int64_t nCycle);
	int64_t		Get_nCycle_AR_Finished();
	int64_t		Get_nCycle_AW_Finished();

	EResultType	Increase_MO_AR();
	EResultType	Decrease_MO_AR();
	EResultType	Increase_MO_AW();
	EResultType	Decrease_MO_AW();

	EResultType	Set_nAR_GEN_NUM(int nNum);						// Total number AR
	EResultType	Set_nAW_GEN_NUM(int nNum);
	EResultType	Set_nAR_START_ADDR(int64_t nAddr);					// Start address AR 
	EResultType	Set_nAR_START_ADDR2(int64_t nAddr);					// Start address AR2
	EResultType	Set_nAW_START_ADDR(int64_t nAddr);
	EResultType	Set_nAW_START_ADDR2(int64_t nAddr);
	EResultType	Set_nAW_START_ADDR3(int64_t nAddr);

	EResultType	Set_AR_AddrMap(string cAddrMap);					// LIAM, BFAM, TILE 
	EResultType	Set_AW_AddrMap(string cAddrMap);
	EResultType	Set_AR_Operation(string cOperation);					// RASTER_SCAN, ROTATION 
	EResultType	Set_AW_Operation(string cOperation);

	EResultType	Set_ScalingFactor(float Num);						// Image scaling factor 

	EResultType	Set_AR_ISSUE_MIN_INTERVAL(int nNum);					// Issue interval cycles 
	EResultType	Set_AW_ISSUE_MIN_INTERVAL(int nNum); 

	// Get value
	EResultType	IsAllTransFinished();
	EResultType	IsARTransFinished();
	EResultType	IsAWTransFinished();

	int64_t		GetLinearAddr(); 
	int			GetMO_AR(); 	
	int			GetMO_AW(); 

	// Stat
	EResultType	PrintStat(int64_t nCycle, FILE *fp);

	// Control
	EResultType	UpdateState(int64_t nCycle);
	EResultType	Reset();

	// Debug
	EResultType	CheckLink();

	// Port 
	CPTRx		cpTx_AR;
	CPTRx		cpRx_R;
	CPTRx		cpTx_AW;
	CPTRx		cpTx_W;
	CPTRx		cpRx_B;
	#ifdef CCI_ON
		CPTRx	cpRx_AC;
		CPTRx	cpTx_CR;
		CPTRx	cpTx_CD;
	#endif

	// DUONGTRAN add for Matrix operation
	int				rowAIndex;
	int				colAIndex;
	int				rowBIndex;
	int				colBIndex;
	int				rowCIndex;
	int				colCIndex;
	int				rowDIndex;
	int				colDIndex;
	int				rowEIndex;
	int				colEIndex;
	int64_t			nStartAddrA;
	int64_t			nStartAddrB;
	int64_t			nStartAddrC;
	int64_t			nStartAddrD;
	int64_t			nStartAddrE;
	EStateGenAddr	nStateAddr;
	EStateGenAddr 	nStateAddr_next;

private:
        // Original
	string		cName;

	// Control
	int			nAllTransFinished;
	EResultType	eAllTransFinished;
	int			nARTransFinished;
	EResultType	eARTransFinished;
	int			nAWTransFinished;
	EResultType	eAWTransFinished;
	
	int			nARTrans;								// Number of AR
	int			nAWTrans; 
	int			nR; 
	int			nB; 

	int64_t		nCycle_AR_Finished;							// When all trans finish
	int64_t		nCycle_AW_Finished;

	int			nMO_AR;									// MO count AR
	int			nMO_AW;

	// FIFO
	CPFIFO		cpFIFO_AR;
	// CPFIFO	cpFIFO_R;
	CPFIFO		cpFIFO_AW;
	CPFIFO		cpFIFO_W;
	// CPFIFO	cpFIFO_B;

	// Address generator
	CPAddrGen	cpAddrGen_AR;
	CPAddrGen	cpAddrGen_AW;

	// Config traffic
	int			nAR_GEN_NUM;								// Total number AR
	int			nAW_GEN_NUM;

	int64_t		nAR_START_ADDR;								// Start addr AR
	int64_t		nAW_START_ADDR;

	float		ScalingFactor;								// Image size scale

	int			AR_ISSUE_MIN_INTERVAL;						// Issue interval cycles
	int			AW_ISSUE_MIN_INTERVAL;

	// Trace
	int			Trace_rewind;								// Back to previous line
	int			simCDlen = 0;								// FIXME: just a temopary variable. should remove and update the real behavior.
	bool 		simDataTransfer = false;					// FIXME: just a temopary variable. should remove and update the real behavior.
};

#endif

