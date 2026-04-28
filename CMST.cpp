//-----------------------------------------------------------
// Filename	 : CMST.cpp 
// Version	: 0.79
// Date		 : 18 Nov 2019
// Contact	: JaeYoung.Hur@gmail.com
// Description	: Master definition
//-----------------------------------------------------------
// Note
// (1) int64_t nAddr: Max address is 0x7FFF_FFFF_FFFF_FFFF
//-----------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <string.h>

#include "CMST.h"

// Constructor
CMST::CMST(string cName) {

	// Generate ports 
	this->cpTx_AR = new CTRx(cName + "_Tx_AR", ETRX_TYPE_TX, EPKT_TYPE_AR);
	this->cpTx_AW = new CTRx(cName + "_Tx_AW", ETRX_TYPE_TX, EPKT_TYPE_AW);
	this->cpRx_R  = new CTRx(cName + "_Rx_R",  ETRX_TYPE_RX, EPKT_TYPE_R);
	this->cpTx_W  = new CTRx(cName + "_Tx_W",  ETRX_TYPE_TX, EPKT_TYPE_W);
	this->cpRx_B  = new CTRx(cName + "_Rx_B",  ETRX_TYPE_RX, EPKT_TYPE_B);

	#ifdef CCI_ON
		this->cpRx_AC = new CTRx(cName + "_Rx_AC", ETRX_TYPE_RX, EPKT_TYPE_AC);
		this->cpTx_CR = new CTRx(cName + "_Tx_CR", ETRX_TYPE_TX, EPKT_TYPE_CR);
		this->cpTx_CD = new CTRx(cName + "_Tx_CD", ETRX_TYPE_TX, EPKT_TYPE_CD);
	#endif

	// Generate FIFO
	this->cpFIFO_AR = new CFIFO(cName + "_FIFO_AR", EUD_TYPE_AR, MAX_MO_COUNT);
	this->cpFIFO_AW = new CFIFO(cName + "_FIFO_AW", EUD_TYPE_AW, MAX_MO_COUNT);
	this->cpFIFO_W  = new CFIFO(cName + "_FIFO_W",  EUD_TYPE_W,  MAX_MO_COUNT*4);

	// this->cpFIFO_AR = new CFIFO("MST_FIFO_AR", EUD_TYPE_AR, 10000000);		// Debug. AR_ADDR_GEN_TEST
	// this->cpFIFO_AW = new CFIFO("MST_FIFO_AW", EUD_TYPE_AW, 10000000);

	// Addr generator
	this->cpAddrGen_AR = new CAddrGen(cName + "_AddrGen_AR", ETRANS_DIR_TYPE_READ);
	this->cpAddrGen_AW = new CAddrGen(cName + "_AddrGen_AW", ETRANS_DIR_TYPE_WRITE);

	// Initialize
		this->cName = cName;

		this->nAllTransFinished = -1;
		this->nARTransFinished  = -1;
		this->nAWTransFinished  = -1;

		this->eAllTransFinished = ERESULT_TYPE_UNDEFINED;
		this->eARTransFinished  = ERESULT_TYPE_UNDEFINED;
		this->eAWTransFinished  = ERESULT_TYPE_UNDEFINED;

		this->nCycle_AR_Finished = -1;
		this->nCycle_AW_Finished = -1;

	this->nARTrans = -1;
	this->nAWTrans = -1;
	this->nR = -1;
	this->nB = -1;

	this->nMO_AR = -1;
	this->nMO_AW = -1;

	// Config traffic
	this->nAR_GEN_NUM = -1;
	this->nAW_GEN_NUM = -1;

	this->nAR_START_ADDR = 0;
	this->nAW_START_ADDR = 0;

	this->ScalingFactor = -1;  

	this->AR_ISSUE_MIN_INTERVAL = -1;
	this->AW_ISSUE_MIN_INTERVAL = -1;

	// Trace
	this->Trace_rewind = -1;

	// Maxtrix operation
	this->rowAIndex = -1;
	this->colAIndex = -1;
	this->rowBIndex = -1;
	this->colBIndex = -1;
	this->rowCIndex = -1;
	this->colCIndex = -1;
	this->rowDIndex = -1;
	this->colDIndex = -1;
	this->rowEIndex = -1;
	this->colEIndex = -1;
	this->nStateAddr = ESTATE_ADDR_INI;
	this->nStateAddr_next = ESTATE_ADDR_UND;
	this->nStartAddrA = -1;
	this->nStartAddrB = -1;
	this->nStartAddrC = -1;

};


// Destructor
CMST::~CMST() {

	// Debug
	assert (this->cpTx_AR != NULL);
	assert (this->cpTx_AW != NULL);
	assert (this->cpTx_W  != NULL);
	assert (this->cpRx_R  != NULL);
	assert (this->cpRx_B  != NULL);

	#ifdef CCI_ON
		assert (this->cpRx_AC != NULL);
		assert (this->cpTx_CR != NULL);
		assert (this->cpTx_CD != NULL);
	#endif

	assert (this->cpFIFO_AR != NULL);
	assert (this->cpFIFO_AW != NULL);
	assert (this->cpFIFO_W  != NULL);

	assert (this->cpAddrGen_AR != NULL);
	assert (this->cpAddrGen_AW != NULL);

	delete (this->cpTx_AR);
	delete (this->cpRx_R);
	delete (this->cpTx_AW);
	delete (this->cpTx_W);
	delete (this->cpRx_B);

	delete (this->cpFIFO_AR);
	delete (this->cpFIFO_AW);
	delete (this->cpFIFO_W);

	delete (this->cpAddrGen_AR);
	delete (this->cpAddrGen_AW);

	this->cpTx_AR = NULL;
	this->cpTx_AW = NULL;
	this->cpTx_W  = NULL;
	this->cpRx_R  = NULL;
	this->cpRx_B  = NULL;

	this->cpFIFO_AR = NULL;
	this->cpFIFO_AW = NULL;
	this->cpFIFO_W  = NULL;

	this->cpAddrGen_AR = NULL;
	this->cpAddrGen_AW = NULL;
};


// Initialize
EResultType CMST::Reset() {

	this->cpTx_AR->Reset();
	this->cpTx_AW->Reset();
	this->cpTx_W ->Reset();
	this->cpRx_R ->Reset();
	this->cpRx_B ->Reset();

	#ifdef CCI_ON
		this->cpRx_AC->Reset();
		this->cpTx_CR->Reset();
		this->cpTx_CD->Reset();
	#endif

	this->cpFIFO_AR->Reset();	
	this->cpFIFO_AW->Reset();	
	this->cpFIFO_W ->Reset();	
	#if !defined MATRIX_MULTIPLICATION && !defined MATRIX_MULTIPLICATIONKIJ && !defined MATRIX_TRANSPOSE && !defined MATRIX_CONVOLUTION && !defined MATRIX_SOBEL
	this->cpAddrGen_AR->Reset();
	this->cpAddrGen_AW->Reset();
	#endif	

		this->nAllTransFinished = 0;
		this->nARTransFinished  = 0;
		this->nAWTransFinished  = 0;

		this->eAllTransFinished = ERESULT_TYPE_NO;
		this->eARTransFinished  = ERESULT_TYPE_NO;
		this->eAWTransFinished  = ERESULT_TYPE_NO;

		this->nCycle_AR_Finished = 0;
		this->nCycle_AW_Finished = 0;

	this->nARTrans = 0;
	this->nAWTrans = 0;
	this->nR = 0;
	this->nB = 0;

	this->nMO_AR = 0;
	this->nMO_AW = 0;

	if (this->nAR_GEN_NUM == 0) {
		this->SetARTransFinished(ERESULT_TYPE_YES);	
	};

	if (this->nAW_GEN_NUM == 0) {
		this->SetAWTransFinished(ERESULT_TYPE_YES);	
	};

	this->ScalingFactor = 1;				// No scaling by default  

	this->AR_ISSUE_MIN_INTERVAL = 4;
	this->AW_ISSUE_MIN_INTERVAL = 4;

	// Trace
	this->Trace_rewind = 0;

	return (ERESULT_TYPE_SUCCESS);
};


// Sobel in transaction (based high-end gpu)
EResultType CMST::LoadTransfer_MatrixSobelTransaction(int64_t nCycle){

		// Check trans num user defined
		//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
		if(this->nAWTrans == this->nAW_GEN_NUM) {
				return (ERESULT_TYPE_SUCCESS);
		};

		// Padding
		#ifdef PADDING_ENABLE
				int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
				int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		#else
				int MATRIX_A_COL_PAD = MATRIX_A_COL;
				int MATRIX_C_COL_PAD = MATRIX_C_COL;
		#endif

		if(MATRIX_A_COL_PAD % 16 != 0) assert(0);	   // assum Horizontal size is multiple of 16 (cache line size)
		if(MATRIX_C_COL_PAD % 16 != 0) assert(0);

		CPAxPkt cpAx_new = NULL;
		int64_t nAddr = -1;

		static int nLen		    = -1;
		static int rowOffset	= -2;
		static int waitAR		= 4;
		       int computationDelay = 10;
		static int remainDelay      = 10;

		// Generate
		char cAxPktName[50];

		// Get addr and create transfer package
		switch(this->nStateAddr){
				case(ESTATE_ADDR_INI): {
						if(rowOffset == -2){
								nLen = 3;	   // the horizional size allways is multiple of 16 pixels
								rowAIndex = 0;
								colAIndex = 0;
								rowCIndex = 0;
								colCIndex = 0;
								rowOffset = -1;
						}

						this->nStateAddr_next = ESTATE_ADDR_LDA;
						this->nStateAddr	  = this->nStateAddr_next;
				}
				case(ESTATE_ADDR_LDA): {
						// Check FIFO full
						if((this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO) && (this->nARTrans < this->nAR_GEN_NUM)) {
							//printf("currentState: read A\n");
							if(((rowOffset + rowAIndex) >= 0) && ((rowOffset + rowAIndex) < MATRIX_A_ROW) && (colAIndex < MATRIX_A_COL)){
									nAddr = nStartAddrA + ((rowAIndex + rowOffset) * MATRIX_A_COL_PAD * ELEMENT_SIZE) + (colAIndex * ELEMENT_SIZE);
									//printf("Raddress A: 0x%lx\n", nAddr);

									// create transfer package
									sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
									cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
									cpAx_new->SetPkt(ARID,  nAddr,  nLen);
									cpAx_new->SetSrcName(this->cName);
									cpAx_new->SetVA(nAddr);
									cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
									cpAx_new->SetTransNum(nARTrans + 1);
									// Push AR
									UPUD upAx_new = new UUD;
									upAx_new->cpAR = cpAx_new;
									this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
									//printf("Raddress A: 0x%lx\n", nAddr);
									#ifdef DEBUG_MST
											printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
											//cpAx_new->Display();
											//this->cpFIFO_AR->Display();
											printf("[Cycle %3ld] read address: 0x%lx\n", nCycle, nAddr);
									#endif
									Delete_UD(upAx_new, EUD_TYPE_AR);
									this->nARTrans++;
							}
							rowOffset++;
							if(rowOffset == 2){
									rowOffset = -1;
									colAIndex += (nLen + 1) * 4;
							}
							if(colAIndex > MATRIX_A_COL) assert(0);
							if(colAIndex == MATRIX_A_COL){
									colAIndex = 0;
									rowAIndex++;
							}
						//break;
						}
				}
				case(ESTATE_ADDR_LDB): {}
				case(ESTATE_ADDR_LDC): {}
				case(ESTATE_ADDR_STC): {
						// Check FIFO full
						if((this->cpFIFO_AW->IsFull() == ERESULT_TYPE_NO) && (waitAR <= this->nARTransFinished)) {
							if(--remainDelay){
								this->nStateAddr_next = ESTATE_ADDR_LDA;
								this->nStateAddr	  = this->nStateAddr_next;			// Don't wait, forward to LDA
								return (ERESULT_TYPE_SUCCESS);
							}
							remainDelay = computationDelay;
							if(this->nARTransFinished < this->nAR_GEN_NUM){
								if((rowCIndex == 0) || (rowCIndex == MATRIX_C_ROW - 1)){
									waitAR += 2;
								} else {
									waitAR += 3;
								}
							}
							//printf("currentState: write C\n");
							nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
							//printf("Waddress C: 0x%lx\n", nAddr);

							colCIndex += (nLen + 1) * 4;
							if(colCIndex > MATRIX_C_COL)	assert(0);
							if(colCIndex == MATRIX_C_COL){
									rowCIndex++;
									colCIndex = 0;
							}

							// create transfer package
							sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
							cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
							cpAx_new->SetPkt(AWID,  nAddr,  nLen);
							cpAx_new->SetSrcName(this->cName);
							cpAx_new->SetVA(nAddr);
							cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
							cpAx_new->SetTransNum(nAWTrans + 1);
							if(rowCIndex == MATRIX_C_ROW){
								cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
								printf("Application finish\n");
							}
							// Push AR
							UPUD upAx_new = new UUD;
							upAx_new->cpAR = cpAx_new;
							this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
							//printf("Waddress C: 0x%lx\n", nAddrStart);
							#ifdef DEBUG_MST
								// printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
								//cpAx_new->Display();
								//this->cpFIFO_AW->Display();
								// printf("[Cycle %3ld] store address: 0x%lx\n", nCycle, nAddr);
							#endif
							Delete_UD(upAx_new, EUD_TYPE_AW);
							this->nAWTrans++;
						}
						this->nStateAddr_next = ESTATE_ADDR_LDA;
						this->nStateAddr	  = this->nStateAddr_next;
						break;
				}
				default: {
					assert(0);
				}
		} // end switch

		return (ERESULT_TYPE_SUCCESS);
}


EResultType CMST::LoadTransfer_MatrixSobelLocalCache(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_D_COL_PAD = MATRIX_D_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_E_COL_PAD = MATRIX_E_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
		int MATRIX_D_COL_PAD = MATRIX_D_COL;
		int MATRIX_E_COL_PAD = MATRIX_E_COL;
	#endif
	
	/*
		phase 0: Init, Load step-by-step Matrix A from Memory, store step-by-step to Matrix C in Memory
		phase 1: Init, Load step-by-step Matrix A from Memory, store step-by-step to Matrix D in Memory
		phase 2: Init, Load step-by-step Matrix C and D from Memory, store step-by-step to Matrix D in Memory
	*/
	static int phase = 0;

	CPAxPkt cpAx_new = NULL;
	int64_t nAddr = -1;

	static int nConvOpe = 1;	// number of convolution operations
	static int nLen	 = -1;
	static int nLen_pre = -1;
	static int computationDelay = 1; // cycles
	int delayPerComputation = 10;

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			nConvOpe = 1;
			nLen	 = -1;
			nLen_pre = -1;
			rowAIndex = 0;
			colAIndex = 0;
			rowBIndex = -1;
			colBIndex = -1;
			rowCIndex = 0;
			colCIndex = 0;
			rowDIndex = 0;
			colDIndex = 0;
			rowEIndex = 0;
			colEIndex = 0;
			switch(phase){
				case 0: 
				case 1: nStateAddr_next = ESTATE_ADDR_LDA; break;
				case 2: nStateAddr_next = ESTATE_ADDR_LDC; break;
				default: assert(0); break;
			}
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDA){
				return (ERESULT_TYPE_SUCCESS);
			}

			#ifdef DEBUG_MST
				if(phase > 1) assert(0);
			#endif

			nStateAddr = ESTATE_ADDR_LDA;
			//printf("currentState: read A\n");
			if(colBIndex == 2){
				rowBIndex++;
				colBIndex = -1;
			}
			if(rowBIndex == 2){
				colAIndex++;
				rowBIndex = -1;
			}
			if(colAIndex == MATRIX_A_COL){
				rowAIndex++;
				colAIndex = 0;
			}
			nAddr = nStartAddrA + ((rowAIndex + rowBIndex) * MATRIX_A_COL_PAD * ELEMENT_SIZE) + ((colAIndex + colBIndex) * ELEMENT_SIZE);
			//printf("Raddress A: 0x%lx\n", nAddr);

			int remainElement = MATRIX_A_COL - colAIndex - colBIndex;
			if(nLen == - 1){
				if(remainElement <= 4){
					nLen = 0;
				} else if(remainElement <= 8){
					nLen = 1;
				} else if(remainElement <= 12){
					nLen = 2;
				} else if(remainElement > 12){
					nLen = 3;
				} else {
					assert(0);
				}
				nLen_pre = nLen;
			}
			//printf("number of ConvOpe: %d\n", nConvOpe);
			if((colAIndex + colBIndex >= 0) && (rowAIndex + rowBIndex >= 0) && (colAIndex + colBIndex < MATRIX_A_COL) && (rowAIndex + rowBIndex <= MATRIX_A_ROW) && ((colAIndex + colBIndex) % ((nLen + 1) * 4) == 0) && ((nConvOpe % ((nLen + 1) * 4) == 0) || nConvOpe == 1)){
				if(remainElement <= 4){
					nLen = 0;
				} else if(remainElement <= 8){
					nLen = 1;
				} else if(remainElement <= 12){
					nLen = 2;
				} else if(remainElement > 12){
					nLen = 3;
				} else {
					assert(0);
				}
				if(rowAIndex + rowBIndex < MATRIX_A_ROW){
					// create transfer package
					sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
					cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
					cpAx_new->SetPkt(ARID,  nAddr,  nLen);
					cpAx_new->SetSrcName(this->cName);
					cpAx_new->SetVA(nAddr);
					cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
					cpAx_new->SetTransNum(nARTrans + 1);

					// Push AR
					UPUD upAx_new = new UUD;
					upAx_new->cpAR = cpAx_new;
					this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
					#ifdef DEBUG_MST
						printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
						//cpAx_new->Display();
						//this->cpFIFO_AR->Display();
						printf("read address: 0x%lx\n", nAddr);
					#endif
					Delete_UD(upAx_new, EUD_TYPE_AR);
					this->nARTrans++;
				}
				colBIndex++;
				this->nStateAddr_next = (rowBIndex == 1 && colBIndex == 2) ? ( (phase == 0) ? ESTATE_ADDR_STC : ESTATE_ADDR_STD ) : ESTATE_ADDR_LDA;
				if(this->nStateAddr_next == ESTATE_ADDR_STC || this->nStateAddr_next == ESTATE_ADDR_STD){
					nConvOpe++;
				}
				break;
			}
			colBIndex++;
			if(rowBIndex == 1 && colBIndex == 2){
				nConvOpe++;
				if(colAIndex == MATRIX_A_COL - 1){
					this->nStateAddr_next = (phase == 0) ?  ESTATE_ADDR_STC : ESTATE_ADDR_STD;
					this->nStateAddr	  = this->nStateAddr_next;
					computationDelay	  = delayPerComputation * (nLen + 1) * 4;
					nConvOpe = 1;
					nLen	 = -1;
				}
			}
			break;
		}
		case(ESTATE_ADDR_LDB): {}
		case(ESTATE_ADDR_STC): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			if(this->nARTrans != this->nARTransFinished){	   // Wait until done all of R transaction
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STC){
				return (ERESULT_TYPE_SUCCESS);
			}
			if(computationDelay > 0){			// delay due to computation cycles
				computationDelay--;
				return (ERESULT_TYPE_SUCCESS);
			}

			#ifdef DEBUG_MST
				if(phase != 0) assert(0);
			#endif

			//printf("currentState: write C\n");
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			this->nStateAddr_next = ESTATE_ADDR_LDA;
			//printf("Waddress C: 0x%lx\n", nAddr);
			colCIndex += (nLen_pre + 1) * 4;
			if(colCIndex > MATRIX_C_COL) assert(0);
			if(colCIndex == MATRIX_C_COL){
				rowCIndex++;
				colCIndex = 0;
			}

			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  nLen_pre);
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);
			if(rowCIndex == MATRIX_C_ROW){
				//cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
				phase = 1;
				nStateAddr_next = ESTATE_ADDR_INI;
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
				printf("store address: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;
			nLen_pre = nLen; // update nLen_pre

			this->nStateAddr	  = this->nStateAddr_next;  // don't use Write channel so don't need to update state in B channel
			break;
		}
		case(ESTATE_ADDR_STD): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			if(this->nARTrans != this->nARTransFinished){	   // Wait until done all of R transaction
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STD){
				return (ERESULT_TYPE_SUCCESS);
			}
			if(computationDelay > 0){			// delay due to computation cycles
				computationDelay--;
				return (ERESULT_TYPE_SUCCESS);
			}

			#ifdef DEBUG_MST
				if(phase != 1) assert(0);
			#endif

			//printf("currentState: write D\n");
			nAddr = nStartAddrD + (rowDIndex * MATRIX_D_COL_PAD * ELEMENT_SIZE) + (colDIndex * ELEMENT_SIZE);
			this->nStateAddr_next = ESTATE_ADDR_LDA;
			//printf("Waddress D: 0x%lx\n", nAddr);
			colDIndex += (nLen_pre + 1) * 4;
			if(colDIndex > MATRIX_D_COL) assert(0);
			if(colDIndex == MATRIX_D_COL){
				rowDIndex++;
				colDIndex = 0;
			}

			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  nLen_pre);
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);
			if(rowDIndex == MATRIX_D_ROW){
				//cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
				phase = 2;
				nStateAddr_next = ESTATE_ADDR_INI;
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
				printf("store address: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;
			nLen_pre = nLen; // update nLen_pre

			this->nStateAddr	  = this->nStateAddr_next;  // don't use Write channel so don't need to update state in B channel
			break;
		}
		case(ESTATE_ADDR_LDC): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDC){
				return (ERESULT_TYPE_SUCCESS);
			}
			//printf("currentState: read C\n");
			#ifdef DEBUG_MST
				if(phase != 2) assert(0);
			#endif

			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("Raddress C: 0x%lx\n", nAddr);

			int remainElement = MATRIX_C_COL - colCIndex;
			if(nLen == - 1){
				if(remainElement <= 4){
					nLen = 0;
				} else if(remainElement <= 8){
					nLen = 1;
				} else if(remainElement <= 12){
					nLen = 2;
				} else if(remainElement > 12){
					nLen = 3;
				} else {
					assert(0);
				}
			}
			if(colCIndex % ((nLen + 1) * 4) == 0){
				if(remainElement <= 4){
					nLen = 0;
				} else if(remainElement <= 8){
					nLen = 1;
				} else if(remainElement <= 12){
					nLen = 2;
				} else if(remainElement > 12){
					nLen = 3;
				} else {
					assert(0);
				}
				// create transfer package
				sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
				cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
				cpAx_new->SetPkt(ARID,  nAddr,  nLen); 
				cpAx_new->SetSrcName(this->cName);
				cpAx_new->SetVA(nAddr);
				cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
				cpAx_new->SetTransNum(nARTrans + 1);
				//if(rowCIndex == MATRIX_C_ROW){
				//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
				//}

				// Push AR
				UPUD upAx_new = new UUD;
				upAx_new->cpAR = cpAx_new;
				this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
				#ifdef DEBUG_MST
					printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
					//cpAx_new->Display();
					//this->cpFIFO_AR->Display();
					printf("read address: 0x%lx\n", nAddr);
				#endif
				Delete_UD(upAx_new, EUD_TYPE_AR);
				this->nARTrans++;
			}
			colCIndex += (nLen + 1) * 4;
			if(colCIndex > MATRIX_C_COL) assert(0);
			if(colCIndex == MATRIX_C_COL){
				rowCIndex++;
				colCIndex = 0;
			}
			this->nStateAddr_next = ESTATE_ADDR_LDD;
			this->nStateAddr	  = this->nStateAddr_next;  // don't use Write channel so don't need to update state in B channel
			break;
		}
		case(ESTATE_ADDR_LDD): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDD){
				return (ERESULT_TYPE_SUCCESS);
			}
			//printf("currentState: read D\n");
			#ifdef DEBUG_MST
				if(phase != 2) assert(0);
			#endif

			nAddr = nStartAddrD + (rowDIndex * MATRIX_D_COL_PAD * ELEMENT_SIZE) + (colDIndex * ELEMENT_SIZE);
			//printf("Raddress D: 0x%lx\n", nAddr);
			
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  nLen);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowDIndex == MATRIX_D_ROW){					
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
				printf("load address: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			colDIndex += (nLen + 1) * 4;
			if(colDIndex > MATRIX_D_COL) assert(0);
			if(colDIndex == MATRIX_D_COL){
				rowDIndex++;
				colDIndex = 0;
			}
			this->nStateAddr_next = ESTATE_ADDR_STE;
			this->nStateAddr	  = this->nStateAddr_next;
			computationDelay	  = delayPerComputation * (nLen + 1) * 4;
			break;
		}
		case(ESTATE_ADDR_STE): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			if(this->nARTrans != this->nARTransFinished){	   // Wait until done all of R transaction
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STE){
				return (ERESULT_TYPE_SUCCESS);
			}
			if(computationDelay > 0){			// delay due to computation cycles
				computationDelay--;
				return (ERESULT_TYPE_SUCCESS);
			}

			//printf("currentState: write E\n");
			#ifdef DEBUG_MST
				if(phase != 2) assert(0);
			#endif

			nAddr = nStartAddrE + (rowEIndex * MATRIX_E_COL_PAD * ELEMENT_SIZE) + (colEIndex * ELEMENT_SIZE);
			//printf("Waddress E: 0x%lx\n", nAddr);
			

			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  nLen);
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);

			colEIndex += (nLen + 1) * 4;
			if(colEIndex > MATRIX_E_COL) assert(0);
			if(colEIndex == MATRIX_E_COL){
				colEIndex = 0;
				rowEIndex++;
				nLen = -1;
			}
			if(rowEIndex == MATRIX_E_ROW){
				cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
				printf("Application finish\n");
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
				printf("store address: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;
			this->nStateAddr_next = ESTATE_ADDR_LDC;
			this->nStateAddr	  = this->nStateAddr_next;
			break;
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}

EResultType CMST::LoadTransfer_MatrixSobel(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_D_COL_PAD = MATRIX_D_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_E_COL_PAD = MATRIX_E_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
		int MATRIX_D_COL_PAD = MATRIX_D_COL;
		int MATRIX_E_COL_PAD = MATRIX_E_COL;
	#endif
	
	/*
		phase 0: Init, Load step-by-step Matrix A from Memory, store step-by-step to Matrix C in Memory
		phase 1: Init, Load step-by-step Matrix A from Memory, store step-by-step to Matrix D in Memory
		phase 2: Init, Load step-by-step Matrix C and D from Memory, store step-by-step to Matrix D in Memory
	*/
	static int phase = 0;

	CPAxPkt cpAx_new = NULL;
	int64_t nAddr = -1;

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			rowAIndex = 0;
			colAIndex = 0;
			rowBIndex = -1;
			colBIndex = -1;
			rowCIndex = 0;
			colCIndex = 0;
			rowDIndex = 0;
			colDIndex = 0;
			rowEIndex = 0;
			colEIndex = 0;
			switch(phase){
				case 0: 
				case 1: nStateAddr_next = ESTATE_ADDR_LDA; break;
				case 2: nStateAddr_next = ESTATE_ADDR_LDC; break;
				default: assert(0); break;
			}
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDA){
				return (ERESULT_TYPE_SUCCESS);
			}

			#ifdef DEBUG_MST
				if(phase > 1) assert(0);
			#endif

			nStateAddr = ESTATE_ADDR_LDA;
			if(colBIndex == 2){
				rowBIndex++;
				colBIndex = -1;
			}
			if(rowBIndex == 2){
				colAIndex++;
				rowBIndex = -1;
			}
			if(colAIndex == MATRIX_A_COL){
				rowAIndex++;
				colAIndex = 0;
			}
			if((rowAIndex + rowBIndex < 0) || (colAIndex + colBIndex < 0) || (colAIndex + colBIndex >= MATRIX_A_COL) || (rowAIndex + rowBIndex >= MATRIX_A_ROW)){
				colBIndex++;
				nStateAddr_next = (rowBIndex == 1 && colBIndex == 2) ? ( (phase == 0) ? ESTATE_ADDR_STC : ESTATE_ADDR_STD ) : ESTATE_ADDR_LDA;
				return (ERESULT_TYPE_SUCCESS);
			}
			nAddr = nStartAddrA + ((rowAIndex + rowBIndex) * MATRIX_A_COL_PAD * ELEMENT_SIZE) + ((colAIndex + colBIndex) * ELEMENT_SIZE);
			//printf("Raddress A: 0x%lx\n", nAddr);
			colBIndex++;
			nStateAddr_next = (rowBIndex == 1 && colBIndex == 2) ? ( (phase == 0) ? ESTATE_ADDR_STC : ESTATE_ADDR_STD ) : ESTATE_ADDR_LDA;
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  0);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);

			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
				printf("load address: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			break;
		}
		case(ESTATE_ADDR_LDB): {}
		case(ESTATE_ADDR_STC): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}

			// check state
			if(nStateAddr_next != ESTATE_ADDR_STC){
				return (ERESULT_TYPE_SUCCESS);
			}

			#ifdef DEBUG_MST
				if(phase != 0) assert(0);
			#endif

			nStateAddr_next = ESTATE_ADDR_LDA;
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("Waddress C: 0x%lx\n", nAddr);
			colCIndex++;
			if(colCIndex == MATRIX_C_COL){
				rowCIndex++;
				colCIndex = 0;
			}
			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  0);  // 4 bytes data / 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);
			if(rowCIndex == MATRIX_C_ROW){
				//cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
				phase = 1;
				nStateAddr_next = ESTATE_ADDR_INI;
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
				printf("store address: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;
			break;
		}
		case(ESTATE_ADDR_STD): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STD){
				return (ERESULT_TYPE_SUCCESS);
			}

			#ifdef DEBUG_MST
				if(phase != 1) assert(0);
			#endif

			nStateAddr_next = ESTATE_ADDR_LDA;
			nAddr = nStartAddrD + (rowDIndex * MATRIX_D_COL_PAD * ELEMENT_SIZE) + (colDIndex * ELEMENT_SIZE);
			//printf("Waddress D: 0x%lx\n", nAddr);
			colDIndex++;
			if(colDIndex == MATRIX_D_COL){
				rowDIndex++;
				colDIndex = 0;
			}
			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  0);  // 4 bytes data / 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);
			if(rowDIndex == MATRIX_D_ROW){
				//cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
				phase = 2;
				nStateAddr_next = ESTATE_ADDR_INI;
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
				printf("store address: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;
			break;
		}
		case(ESTATE_ADDR_LDC): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDC){
				return (ERESULT_TYPE_SUCCESS);
			}

			#ifdef DEBUG_MST
				if(phase != 2) assert(0);
			#endif

			nStateAddr_next = ESTATE_ADDR_LDD;
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("Raddress C: 0x%lx\n", nAddr);
			colCIndex++;
			if(colCIndex == MATRIX_C_COL){
				rowCIndex++;
				colCIndex = 0;
			}
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  0);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowCIndex == MATRIX_C_ROW){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}

			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
				printf("load address: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			break;
		}
		case(ESTATE_ADDR_LDD): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDD){
				return (ERESULT_TYPE_SUCCESS);
			}

			#ifdef DEBUG_MST
				if(phase != 2) assert(0);
			#endif

			nStateAddr_next = ESTATE_ADDR_STE;
			nAddr = nStartAddrD + (rowDIndex * MATRIX_D_COL_PAD * ELEMENT_SIZE) + (colDIndex * ELEMENT_SIZE);
			//printf("Raddress D: 0x%lx\n", nAddr);
			colDIndex++;
			if(colDIndex == MATRIX_D_COL){
				rowDIndex++;
				colDIndex = 0;
			}
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  0);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowDIndex == MATRIX_D_ROW){					
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
				printf("load address: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			break;
		}
		case(ESTATE_ADDR_STE): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STE){
				return (ERESULT_TYPE_SUCCESS);
			}

			#ifdef DEBUG_MST
				if(phase != 2) assert(0);
			#endif

			nStateAddr_next = ESTATE_ADDR_LDC;
			nAddr = nStartAddrE + (rowEIndex * MATRIX_E_COL_PAD * ELEMENT_SIZE) + (colEIndex * ELEMENT_SIZE);
			//printf("Waddress E: 0x%lx\n", nAddr);
			colEIndex++;
			if(colEIndex == MATRIX_E_COL){
				colEIndex = 0;
				rowEIndex++;
			}
			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  0);  // 4 bytes data / 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);
			if(rowEIndex == MATRIX_E_ROW){
				cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
				printf("store address: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;
			break;
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}

// Convolution in transaction (based high-end gpu)
EResultType CMST::LoadTransfer_MatrixConvolutionTransaction(int64_t nCycle){

		// Check trans num user defined
		//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
		if(this->nAWTrans == this->nAW_GEN_NUM) {
				return (ERESULT_TYPE_SUCCESS);
		};

		// Padding
		#ifdef PADDING_ENABLE
				int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
				int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		#else
				int MATRIX_A_COL_PAD = MATRIX_A_COL;
				int MATRIX_C_COL_PAD = MATRIX_C_COL;
		#endif

		if(MATRIX_A_COL_PAD % 16 != 0) assert(0);	   // assum Horizontal size is multiple of 16 (cache line size)
		if(MATRIX_C_COL_PAD % 16 != 0) assert(0);

		CPAxPkt cpAx_new = NULL;
		int64_t nAddr = -1;

		static int nLen	  = -1;
		static int rowOffset = -2;
		static int waitAR	= 4;

		// Generate
		char cAxPktName[50];

		// Get addr and create transfer package
		switch(this->nStateAddr){
				case(ESTATE_ADDR_INI): {
						if(rowOffset == -2){
								nLen = 3;	   // the horizional size allways is multiple of 16 pixels
								rowAIndex = 0;
								colAIndex = 0;
								rowCIndex = 0;
								colCIndex = 0;
								rowOffset = -1;
						}

						this->nStateAddr_next = ESTATE_ADDR_LDA;
						this->nStateAddr	  = this->nStateAddr_next;
				}
				case(ESTATE_ADDR_LDA): {
						// Check FIFO full
						if((this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO) && (this->nARTrans < this->nAR_GEN_NUM)) {
							//printf("currentState: read A\n");
							if(((rowOffset + rowAIndex) >= 0) && ((rowOffset + rowAIndex) < MATRIX_A_ROW) && (colAIndex < MATRIX_A_COL)){
									nAddr = nStartAddrA + ((rowAIndex + rowOffset) * MATRIX_A_COL_PAD * ELEMENT_SIZE) + (colAIndex * ELEMENT_SIZE);
									//printf("Raddress A: 0x%lx\n", nAddr);

									// create transfer package
									sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
									cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
									cpAx_new->SetPkt(ARID,  nAddr,  nLen);
									cpAx_new->SetSrcName(this->cName);
									cpAx_new->SetVA(nAddr);
									cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
									cpAx_new->SetTransNum(nARTrans + 1);
									// Push AR
									UPUD upAx_new = new UUD;
									upAx_new->cpAR = cpAx_new;
									this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
									//printf("Raddress A: 0x%lx\n", nAddr);
									#ifdef DEBUG_MST
											printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
											//cpAx_new->Display();
											//this->cpFIFO_AR->Display();
											printf("[Cycle %3ld] read address: 0x%lx\n", nCycle, nAddr);
									#endif
									Delete_UD(upAx_new, EUD_TYPE_AR);
									this->nARTrans++;
							}
							rowOffset++;
							if(rowOffset == 2){
									rowOffset = -1;
									colAIndex += (nLen + 1) * 4;
							}
							if(colAIndex > MATRIX_A_COL) assert(0);
							if(colAIndex == MATRIX_A_COL){
									colAIndex = 0;
									rowAIndex++;
							}
							//break;
						}
				}
				case(ESTATE_ADDR_LDB): {}
				case(ESTATE_ADDR_LDC): {}
				case(ESTATE_ADDR_STC): {
						// Check FIFO full
						if((this->cpFIFO_AW->IsFull() == ERESULT_TYPE_NO) && (waitAR <= this->nARTransFinished)) {
							if(this->nARTransFinished < this->nAR_GEN_NUM){
								if((rowCIndex == 0) || (rowCIndex == MATRIX_C_ROW - 1)){
									waitAR += 2;
								} else {
									waitAR += 3;
								}
							}
							//printf("currentState: write C\n");
							nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
							//printf("Waddress C: 0x%lx\n", nAddr);

							colCIndex += (nLen + 1) * 4;
							if(colCIndex > MATRIX_C_COL)	assert(0);
							if(colCIndex == MATRIX_C_COL){
									rowCIndex++;
									colCIndex = 0;
							}

							// create transfer package
							sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
							cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
							cpAx_new->SetPkt(AWID,  nAddr,  nLen);
							cpAx_new->SetSrcName(this->cName);
							cpAx_new->SetVA(nAddr);
							cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
							cpAx_new->SetTransNum(nAWTrans + 1);
							if(rowCIndex == MATRIX_C_ROW){
								cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
								printf("Application finish\n");
							}
							// Push AR
							UPUD upAx_new = new UUD;
							upAx_new->cpAR = cpAx_new;
							this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
							//printf("Waddress C: 0x%lx\n", nAddrStart);
							#ifdef DEBUG_MST
								printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
								//cpAx_new->Display();
								//this->cpFIFO_AW->Display();
								printf("[Cycle %3ld] store address: 0x%lx\n", nCycle, nAddr);
							#endif
							Delete_UD(upAx_new, EUD_TYPE_AW);
							this->nAWTrans++;
						}
						this->nStateAddr_next = ESTATE_ADDR_LDA;
						this->nStateAddr	  = this->nStateAddr_next;
						break;
				}
				default: {
					assert(0);
				}
		} // end switch

		return (ERESULT_TYPE_SUCCESS);
}

// Convolution in address transaction but computation in element-by-element (based simple gpu)
EResultType CMST::LoadTransfer_MatrixConvolutionLocalCache(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
	#endif
	
	CPAxPkt cpAx_new = NULL;
	int64_t nAddr = -1;

	static int nConvOpe = 1;	// number of convolution operations
	static int nLen	 = -1;
	static int nLen_pre = -1;
	static int computationDelay = 1;
	int delayPerComputation = 10;

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			rowAIndex = 0;
			colAIndex = 0;
			rowBIndex = -1;
			colBIndex = -1;
			rowCIndex = 0;
			colCIndex = 0;
			nStateAddr_next = ESTATE_ADDR_LDA;
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDA){
				return (ERESULT_TYPE_SUCCESS);
			}
			nStateAddr = ESTATE_ADDR_LDA;
			//printf("currentState: read A\n");
			if(colBIndex == 2){
				rowBIndex++;
				colBIndex = -1;
			}
			if(rowBIndex == 2){
				colAIndex++;
				rowBIndex = -1;
			}
			if(colAIndex == MATRIX_A_COL){
				rowAIndex++;
				colAIndex = 0;
			}
			nAddr = nStartAddrA + ((rowAIndex + rowBIndex) * MATRIX_A_COL_PAD * ELEMENT_SIZE) + ((colAIndex + colBIndex) * ELEMENT_SIZE);
			//printf("AIndex: (%d, %d)\n", rowAIndex + rowBIndex, colAIndex + colBIndex);
			//printf("Raddress A: 0x%lx\n", nAddr);

			int remainElement = MATRIX_A_COL - colAIndex - colBIndex;
			if(nLen == - 1){
				if(remainElement <= 4){
					nLen = 0;
				} else if(remainElement <= 8){
					nLen = 1;
				} else if(remainElement <= 12){
					nLen = 2;
				} else if(remainElement > 12){
					nLen = 3;
				} else {
					assert(0);
				}
				nLen_pre = nLen;
			}
			//printf("number of ConvOpe: %d\n", nConvOpe);
			if((colAIndex + colBIndex >= 0) && (rowAIndex + rowBIndex >= 0) && (colAIndex + colBIndex < MATRIX_A_COL) && (rowAIndex + rowBIndex <= MATRIX_A_ROW) && ((colAIndex + colBIndex) % ((nLen + 1) * 4) == 0) && (nConvOpe % ((nLen + 1) * 4) == 0 || nConvOpe == 1)){
				//printf("remainElement: %d\n", remainElement);
				if(remainElement <= 4){
					nLen = 0;
				} else if(remainElement <= 8){
					nLen = 1;
				} else if(remainElement <= 12){
					nLen = 2;
				} else if(remainElement > 12){
					nLen = 3;
				} else {
					assert(0);
				}
				//printf("nLen: %d\n", nLen);
				if(rowAIndex + rowBIndex < MATRIX_A_ROW){
					// create transfer package
					sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
					cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
					cpAx_new->SetPkt(ARID,  nAddr,  nLen); 
					cpAx_new->SetSrcName(this->cName);
					cpAx_new->SetVA(nAddr);
					cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
					cpAx_new->SetTransNum(nARTrans + 1);
					//if((rowAIndex == MATRIX_A_ROW - 1) && (colAIndex == MATRIX_A_COL - 1) && (rowBIndex == 0) && (colBIndex == 0)){
					//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
					//}
					// Push AR
					UPUD upAx_new = new UUD;
					upAx_new->cpAR = cpAx_new;
					this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
					//printf("Raddress A: 0x%lx\n", nAddr);
					#ifdef DEBUG_MST
						printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
						//cpAx_new->Display();
						//this->cpFIFO_AR->Display();
						printf("read address: 0x%lx\n", nAddr);
					#endif
					Delete_UD(upAx_new, EUD_TYPE_AR);
					this->nARTrans++;
				}
				colBIndex++;
				this->nStateAddr_next = (rowBIndex == 1 && colBIndex == 2) ? ESTATE_ADDR_STC : ESTATE_ADDR_LDA;
				if(this->nStateAddr_next == ESTATE_ADDR_STC){
					nConvOpe++;
					computationDelay = delayPerComputation * (nLen + 1) * 4;	
				}
				break;
			}

			colBIndex++;
			if(rowBIndex == 1 && colBIndex == 2){
				nConvOpe++;
				if(colAIndex == MATRIX_A_COL - 1){
					this->nStateAddr_next = ESTATE_ADDR_STC;
					this->nStateAddr	  = this->nStateAddr_next;
					computationDelay = delayPerComputation * (nLen + 1) * 4;	
					nConvOpe = 1;
					nLen	 = -1;
				}
				
			}
			break;
		}
		case(ESTATE_ADDR_LDB): {}
		case(ESTATE_ADDR_LDC): {}
		case(ESTATE_ADDR_STC): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			//printf("nARTrans: %d\n", this->nARTrans);
			//printf("nR:	   %d\n", this->nARTransFinished);
			if(this->nARTrans != this->nARTransFinished){		// Wait until done all of R transaction
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STC){
				return (ERESULT_TYPE_SUCCESS);
			}
			if(computationDelay > 0){			// delay due to computation cycles
				computationDelay--;
				return (ERESULT_TYPE_SUCCESS);
			}
			//printf("currentState: write C\n");
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("Waddress C: 0x%lx\n", nAddr);
			colCIndex += (nLen_pre + 1) * 4;
			if(colCIndex > MATRIX_C_COL)	assert(0);
			if(colCIndex == MATRIX_C_COL){
				rowCIndex++;
				colCIndex = 0;
			}

			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  nLen_pre);
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);
			if(rowCIndex == MATRIX_C_ROW){
				cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
				printf("Application finish\n");
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			//printf("Waddress C: 0x%lx\n", nAddrStart);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
				printf("store address: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;
			nLen_pre = nLen; // update nLen_pre

			this->nStateAddr_next = ESTATE_ADDR_LDA;
			this->nStateAddr	  = this->nStateAddr_next;	// don't use Write channel so don't need to update state in B channel
			break;
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}

// Convolution in element-by-element (based cpu)
EResultType CMST::LoadTransfer_MatrixConvolution(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
	#endif
	
	CPAxPkt cpAx_new = NULL;
	int64_t nAddr = -1;

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			rowAIndex = 0;
			colAIndex = 0;
			rowBIndex = -1;
			colBIndex = -1;
			rowCIndex = 0;
			colCIndex = 0;
			nStateAddr_next = ESTATE_ADDR_LDA;
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDA){
				return (ERESULT_TYPE_SUCCESS);
			}
			nStateAddr = ESTATE_ADDR_LDA;
			if(colBIndex == 2){
				rowBIndex++;
				colBIndex = -1;
			}
			if(rowBIndex == 2){
				colAIndex++;
				rowBIndex = -1;
			}
			if(colAIndex == MATRIX_A_COL){
				rowAIndex++;
				colAIndex = 0;
			}
			if((rowAIndex + rowBIndex < 0) || (colAIndex + colBIndex < 0) || (colAIndex + colBIndex >= MATRIX_A_COL) || (rowAIndex + rowBIndex >= MATRIX_A_ROW)){
				colBIndex++;
				nStateAddr_next = (rowBIndex == 1 && colBIndex == 2) ? ESTATE_ADDR_STC : ESTATE_ADDR_LDA;
				return (ERESULT_TYPE_SUCCESS);
			}
			nAddr = nStartAddrA + ((rowAIndex + rowBIndex) * MATRIX_A_COL_PAD * ELEMENT_SIZE) + ((colAIndex + colBIndex) * ELEMENT_SIZE);
			//printf("Raddress A: 0x%lx\n", nAddr);
			colBIndex++;
			nStateAddr_next = (rowBIndex == 1 && colBIndex == 2) ? ESTATE_ADDR_STC : ESTATE_ADDR_LDA;
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  0);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if((rowAIndex == MATRIX_A_ROW - 1) && (colAIndex == MATRIX_A_COL - 1) && (rowBIndex == 0) && (colBIndex == 0)){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			break;
		}
		case(ESTATE_ADDR_LDB): {}
		case(ESTATE_ADDR_LDC): {}
		case(ESTATE_ADDR_STC): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STC){
				return (ERESULT_TYPE_SUCCESS);
			}
			nStateAddr_next = ESTATE_ADDR_LDA;
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("Waddress C: 0x%lx\n", nAddr);
			colCIndex++;
			if(colCIndex == MATRIX_C_COL){
				rowCIndex++;
				colCIndex = 0;
			}
			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  0);  // 4 bytes data / 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);
			if(rowCIndex == MATRIX_C_ROW){
				cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;
			break;
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}

EResultType CMST::LoadTransfer_MatrixTransposeTransaction(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
	#endif
	
	CPAxPkt cpAx_new = NULL;
	int64_t nAddr = -1;
	
	static int nLen           = -1;
    static int blockSizeEle   = -1;
    static int rowAIndexBlock = -1;
    static int rowCIndexBlock = -1;
	static int waitAR         = -1;

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			nLen	       = 3;
			blockSizeEle   = (nLen + 1) * 4;
			rowAIndexBlock = 0;
			rowCIndexBlock = 0;
			waitAR         = blockSizeEle;
			rowAIndex = 0;
			colAIndex = 0;
			rowCIndex = 0;
			colCIndex = 0;
			this->nStateAddr_next = ESTATE_ADDR_LDA;
			this->nStateAddr      = this->nStateAddr_next;
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if ((this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO) && (this->nARTrans < this->nAR_GEN_NUM)) {
				nAddr = nStartAddrA + (rowAIndex * MATRIX_A_COL_PAD * ELEMENT_SIZE) + (colAIndex * ELEMENT_SIZE);
				//printf("Raddress A: 0x%lx\n", nAddr);
				//printf("remainElement: %d\n", remainElement);
				// create transfer package
				sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
				cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
				cpAx_new->SetPkt(ARID,  nAddr,  nLen);
				cpAx_new->SetSrcName(this->cName);
				cpAx_new->SetVA(nAddr);
				cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
				cpAx_new->SetTransNum(nARTrans + 1);
				rowAIndex++;
				if(rowAIndex == rowAIndexBlock + blockSizeEle){
					rowAIndex = rowAIndexBlock;
					colAIndex += blockSizeEle;
					if(colAIndex == MATRIX_A_COL){
						rowAIndexBlock += blockSizeEle;
						rowAIndex      = rowAIndexBlock;
						colAIndex      = 0;
					}
				}
				if(rowAIndexBlock == MATRIX_A_ROW){
					cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
				}
				
				// Push AR
				UPUD upAx_new = new UUD;
				upAx_new->cpAR = cpAx_new;
				this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
				#ifdef DEBUG_MST
					printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
					//cpAx_new->Display();
					//this->cpFIFO_AR->Display();
					printf("Cycle %10ld, read address A: 0x%lx\n", nCycle, nAddr);
				#endif
				Delete_UD(upAx_new, EUD_TYPE_AR);
				this->nARTrans++;
				//break;
			}
			this->nStateAddr_next = ESTATE_ADDR_STC;
			this->nStateAddr      = this->nStateAddr_next;
		}
		case(ESTATE_ADDR_LDB): {}
		case(ESTATE_ADDR_LDC): {}
		case(ESTATE_ADDR_STC): {
			// Check FIFO full
			if ((this->cpFIFO_AW->IsFull() == ERESULT_TYPE_NO) && (waitAR <= this->nARTransFinished)) {

				nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
				//printf("Waddress C: 0x%lx\n", nAddr);
				// create transfer package
				sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
				cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
				cpAx_new->SetPkt(AWID,  nAddr,  0);  // 4 bytes data / 8 bytes data: 1 BURST
				cpAx_new->SetSrcName(this->cName);
				cpAx_new->SetVA(nAddr);
				cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
				cpAx_new->SetTransNum(nAWTrans + 1);
				rowCIndex++;
				if(rowCIndex == rowCIndexBlock + blockSizeEle){
					waitAR += blockSizeEle;
					rowCIndexBlock += blockSizeEle;
					if(rowCIndex == MATRIX_C_ROW){
						colCIndex      += blockSizeEle;
						rowCIndexBlock  = 0;
						rowCIndex       = 0;
					}
				}
				if(colCIndex == MATRIX_C_COL){
					cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
					printf("Application finish\n");
				}

				// Push AR
				UPUD upAx_new = new UUD;
				upAx_new->cpAR = cpAx_new;
				this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
				#ifdef DEBUG_MST
					printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
					//cpAx_new->Display();
					//this->cpFIFO_AW->Display();
					printf("Cycle %10ld, write address C: 0x%lx\n", nCycle, nAddr);
				#endif
				Delete_UD(upAx_new, EUD_TYPE_AW);
				this->nAWTrans++;
			}
			this->nStateAddr_next = ESTATE_ADDR_LDA;
			this->nStateAddr      = this->nStateAddr_next;
			break;
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}

EResultType CMST::LoadTransfer_MatrixTransposeLocalCache(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
	#endif
	
	CPAxPkt cpAx_new = NULL;
	int64_t nAddr = -1;
	
	static int nLen = -1;
	static int remainElement = MATRIX_A_COL * MATRIX_A_ROW;

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			nLen	  = -1;
			rowAIndex = 0;
			colAIndex = 0;
			rowCIndex = 0;
			colCIndex = 0;
			nStateAddr_next = ESTATE_ADDR_LDA;
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDA){
				return (ERESULT_TYPE_SUCCESS);
			}
			nStateAddr = ESTATE_ADDR_LDA;
			
			nAddr = nStartAddrA + (rowAIndex * MATRIX_A_COL_PAD * ELEMENT_SIZE) + (colAIndex * ELEMENT_SIZE);
			//printf("Raddress A: 0x%lx\n", nAddr);
			//printf("remainElement: %d\n", remainElement);
			if(remainElement <= 4){
				nLen = 0;
			} else if(remainElement <= 8){
				nLen = 1;
			} else if(remainElement <= 12){
				nLen = 2;
			} else if(remainElement > 12){
				nLen = 3;
			} else {
				assert(0);
			}
			remainElement -= (nLen + 1) * 4;
			while((rowAIndex * MATRIX_A_COL + colAIndex) != (MATRIX_A_ROW * MATRIX_A_COL - remainElement)){
				colAIndex++;
				if(colAIndex == MATRIX_A_COL){
					colAIndex = 0;
					rowAIndex++;
				}
			}
		
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  nLen);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if((rowAIndex == MATRIX_A_ROW - 1) && (colAIndex == MATRIX_A_COL)){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
				printf("read address A: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			this->nStateAddr_next = ESTATE_ADDR_STC;
			this->nStateAddr	  = this->nStateAddr_next;
			break;
		}
		case(ESTATE_ADDR_LDB): {}
		case(ESTATE_ADDR_LDC): {}
		case(ESTATE_ADDR_STC): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			if(this->nARTrans != this->nARTransFinished){			// Wait until done all of R Transaction
				return (ERESULT_TYPE_SUCCESS);
			}
			//nStateAddr = ESTATE_ADDR_STC;  //fix issue due to ignore some states
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STC){
				return (ERESULT_TYPE_SUCCESS);
			}
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("Waddress C: 0x%lx\n", nAddr);
			rowCIndex++;
			if(rowCIndex == MATRIX_C_ROW){
				colCIndex++;
				rowCIndex = 0;
			}
			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  0);  // 4 bytes data / 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);
			if(colCIndex == MATRIX_C_COL){
				cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
				printf("Application finish\n");
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
				printf("write address C: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;
			this->nStateAddr_next = (colCIndex * MATRIX_C_ROW + rowCIndex + remainElement == MATRIX_C_ROW * MATRIX_C_COL) ? ESTATE_ADDR_LDA : ESTATE_ADDR_STC;
			this->nStateAddr	  = this->nStateAddr_next;
			break;
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}

EResultType CMST::LoadTransfer_MatrixTranspose(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
	#endif
	
	CPAxPkt cpAx_new = NULL;
	int64_t nAddr = -1;

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			rowAIndex = 0;
			colAIndex = 0;
			rowCIndex = 0;
			colCIndex = 0;
			nStateAddr_next = ESTATE_ADDR_LDA;
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDA){
				return (ERESULT_TYPE_SUCCESS);
			}
			nStateAddr = ESTATE_ADDR_LDA;
			nStateAddr_next = ESTATE_ADDR_STC;
			if(colAIndex == MATRIX_A_COL){
				rowAIndex++;
				colAIndex = 0;
			}
			nAddr = nStartAddrA + (rowAIndex * MATRIX_A_COL_PAD * ELEMENT_SIZE) + (colAIndex * ELEMENT_SIZE);
			//printf("Raddress A: 0x%lx\n", nAddr);
			colAIndex++;
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  0);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if((rowAIndex == MATRIX_A_ROW - 1) && (colAIndex == MATRIX_A_COL)){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			break;
		}
		case(ESTATE_ADDR_LDB): {}
		case(ESTATE_ADDR_LDC): {}
		case(ESTATE_ADDR_STC): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			//nStateAddr = ESTATE_ADDR_STC;  //fix issue due to ignore some states
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STC){
				return (ERESULT_TYPE_SUCCESS);
			}
			nStateAddr_next = ESTATE_ADDR_LDA;
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("Waddress C: 0x%lx\n", nAddr);
			rowCIndex++;
			if(rowCIndex == MATRIX_C_ROW){
				colCIndex++;
				rowCIndex = 0;
			}
			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  0);  // 4 bytes data / 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);
			if(colCIndex == MATRIX_C_COL){
				cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;
			break;
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}

EResultType CMST::LoadTransfer_MatrixMultiplicationTileTransaction(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};


	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_B_COL_PAD = MATRIX_B_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_B_COL_PAD = MATRIX_B_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
	#endif
	
	CPAxPkt cpAx_new          = NULL;
	int64_t nAddr             = -1;	
	static int nLen           = -1;
    static int rowAIndexBlock = -1;
    static int rowBIndexBlock = -1;
    static int rowCIndexBlock = -1;
	static int waitAR         = -1;
	//static bool newColBlockB  = false;
	static bool keepALocal    = false;
	static bool activeSTC     = false;
	static int  nextTransA    = -1;
	static int  nextTransB    = -1;
	static int  nextTransC    = -1;
	
	//printf("DUONGTRAN address this->nARTransFinished: %d ; waitAR: %d\n", this->nARTransFinished, waitAR);

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			if(TILE_SIZE == 4){
				nLen = 0;
			} else if(TILE_SIZE == 8){
				nLen = 1;
			} else if(TILE_SIZE >= 16){
				nLen = 3;
			} else{
				assert(0);
			}
			//waitAR         = MATRIX_A_COL_PAD * TILE_SIZE / 2;
			waitAR         = MATRIX_A_COL_PAD * 2 * (int)(ceil(TILE_SIZE / 16.0f));
			rowAIndexBlock = TILE_SIZE;
			rowBIndexBlock = TILE_SIZE;
			rowCIndexBlock = TILE_SIZE;
			rowAIndex      = 0;
			colAIndex      = 0;
			rowBIndex      = 0;
			colBIndex      = 0;
			rowCIndex      = 0;
			colCIndex      = 0;
			nextTransA	   = 0;
			nextTransB     = 0;
			nextTransC     = 0;
			this->nStateAddr_next = ESTATE_ADDR_LDA;
			this->nStateAddr      = this->nStateAddr_next;
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if ((this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO) && (this->nARTrans < this->nAR_GEN_NUM) && (keepALocal == false)) {
				//printf("address ---- nStartAddrA: %lx, rowAIndex: %d, colAIndex: %d\n", nStartAddrA, rowAIndex, colAIndex);
				nAddr = nStartAddrA + (rowAIndex * MATRIX_A_COL_PAD * ELEMENT_SIZE) + ((colAIndex + (nextTransA << 4)) * ELEMENT_SIZE);
				// create transfer package
				sprintf(cAxPktName, "AR%d_%s",this->nARTrans + 1, this->cName.c_str());
				cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
				cpAx_new->SetPkt(ARID,  nAddr,  nLen);
				cpAx_new->SetSrcName(this->cName);
				cpAx_new->SetVA(nAddr);
				cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
				cpAx_new->SetTransNum(nARTrans + 1);
				if(nextTransA < (TILE_SIZE >> 5)){
					nextTransA++;
				//}
				//if(nextTransA < (TILE_SIZE - (nLen + 1))){
				//	nextTransA += (nLen + 1);
				} else{
					nextTransA  = 0;
					rowAIndex++;
				}
				if(rowAIndex == rowAIndexBlock){
					rowAIndex       = rowAIndexBlock - TILE_SIZE;
					colAIndex      += TILE_SIZE;
					this->nStateAddr_next = ESTATE_ADDR_LDB;
					this->nStateAddr      = this->nStateAddr_next;		// FIXME Duong add this line to ignore updating at B_bwd and R_bwd
				}
				if(colAIndex >= MATRIX_A_COL_PAD){
					keepALocal      = true;
					rowAIndex       = rowAIndexBlock;
					rowAIndexBlock += TILE_SIZE;
					colAIndex       = 0;
				}
				
				// Push AR
				UPUD upAx_new = new UUD;
				upAx_new->cpAR = cpAx_new;
				this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
				#ifdef DEBUG_MST
					printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
					//cpAx_new->Display();
					//this->cpFIFO_AR->Display();
					printf("Cycle %10ld, read address A: 0x%lx with %d byte\n", nCycle, nAddr, (nLen + 1) * 16);
				#endif
				Delete_UD(upAx_new, EUD_TYPE_AR);
				this->nARTrans++;
			}
			break;
		}
		case(ESTATE_ADDR_LDB): {
			// Check FIFO full
			if(this->nARTrans + 1 == this->nAR_GEN_NUM){
				activeSTC = true;
			}
			if(this->nARTrans == this->nAR_GEN_NUM){
				activeSTC = true;
			}
			if ((this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO) && (this->nARTrans < this->nAR_GEN_NUM)) {
				nAddr = nStartAddrB + (rowBIndex * MATRIX_B_COL_PAD * ELEMENT_SIZE) + ((colBIndex + (nextTransB << 4 )) * ELEMENT_SIZE);
				// create transfer package
				sprintf(cAxPktName, "AR%d_%s", this->nARTrans + 1, this->cName.c_str());
				cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
				cpAx_new->SetPkt(ARID,  nAddr,  nLen);
				cpAx_new->SetSrcName(this->cName);
				cpAx_new->SetVA(nAddr);
				cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
				cpAx_new->SetTransNum(nARTrans + 1);
				if(nextTransB < (TILE_SIZE >> 5)){
					nextTransB++;
				} else{
					nextTransB  = 0;
					rowBIndex++;
				}
				if(rowBIndex == rowBIndexBlock){
					if(rowBIndexBlock >= MATRIX_B_ROW){
						rowBIndex      = 0;
						rowBIndexBlock = 0;
						colBIndex     += TILE_SIZE;
						this->nStateAddr_next = ESTATE_ADDR_STC;
					} else {
						if(keepALocal == false){
							this->nStateAddr_next = ESTATE_ADDR_LDA;
						}
					}
					this->nStateAddr   = this->nStateAddr_next;      // FIXME Duong add this line to ignore updating at B_bwd and R_bwd
					rowBIndexBlock    += TILE_SIZE;
				}

				if(colBIndex > MATRIX_B_COL_PAD){
					colBIndex = 0;
				}
				
				// Push AR
				UPUD upAx_new = new UUD;
				upAx_new->cpAR = cpAx_new;
				this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
				#ifdef DEBUG_MST
					printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
					//cpAx_new->Display();
					//this->cpFIFO_AR->Display();
					printf("Cycle %10ld, read address B: 0x%lx with %d byte\n", nCycle, nAddr, (nLen + 1) * 16);
				#endif
				Delete_UD(upAx_new, EUD_TYPE_AR);
				this->nARTrans++;
			}
			if(activeSTC == false){
				break;
			}
		}
		case(ESTATE_ADDR_LDC): {}
		case(ESTATE_ADDR_STC): {
			activeSTC = true;
			//if ((this->cpFIFO_AW->IsFull() == ERESULT_TYPE_NO) && (waitAR <= this->nARTransFinished)) {
			if ((this->cpFIFO_AW->IsFull() == ERESULT_TYPE_NO) && (this->nARTransFinished >= waitAR)) {
				nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + ((colCIndex + (nextTransC << 4)) * ELEMENT_SIZE);
				//printf("Waddress C: 0x%lx\n", nAddr);
				// create transfer package
				sprintf(cAxPktName, "AW%d_%s", this->nAWTrans + 1, this->cName.c_str());
				cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
				cpAx_new->SetPkt(AWID,  nAddr,  nLen); 
				cpAx_new->SetSrcName(this->cName);
				cpAx_new->SetVA(nAddr);
				cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
				cpAx_new->SetTransNum(nAWTrans + 1);
				if(nextTransC < (TILE_SIZE >> 5)){
					nextTransC++;
				} else{
					nextTransC  = 0;
					rowCIndex++;
				}
				if((rowCIndex == MATRIX_C_ROW) && (colCIndex == MATRIX_C_COL_PAD - TILE_SIZE)){
					cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
					printf("Application finish\n");
				}
				if(rowCIndex == rowCIndexBlock){
					colCIndex      += TILE_SIZE;
					if(colCIndex >= MATRIX_C_COL_PAD){
						keepALocal  = false;
						//rowCIndex  += TILE_SIZE;
						colCIndex   = 0;
						this->nStateAddr_next = ESTATE_ADDR_LDA;
						rowCIndexBlock       += TILE_SIZE;
						waitAR += (MATRIX_A_COL_PAD * 2 * (int)(ceil(TILE_SIZE / 16.0f)));
					} else{
						this->nStateAddr_next = ESTATE_ADDR_LDB;
						rowCIndex             = rowCIndexBlock - TILE_SIZE;
						waitAR += (MATRIX_A_COL_PAD * (int)(ceil(TILE_SIZE / 16.0f)));
					}
					this->nStateAddr   = this->nStateAddr_next;      // FIXME Duong add this line to ignore updating at B_bwd and R_bwd
					activeSTC   = false;
				}

				// Push AR
				UPUD upAx_new = new UUD;
				upAx_new->cpAR = cpAx_new;
				this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
				#ifdef DEBUG_MST
					printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
					//cpAx_new->Display();
					//this->cpFIFO_AW->Display();
					printf("Cycle %10ld, write address C: 0x%lx with %d byte\n", nCycle, nAddr, (nLen + 1) * 16);
				#endif
				Delete_UD(upAx_new, EUD_TYPE_AW);
				this->nAWTrans++;
				break;
			}
			this->nStateAddr  =  ESTATE_ADDR_LDB;
			break;
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}
EResultType CMST::LoadTransfer_MatrixMultiplicationTransaction(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_B_COL_PAD = MATRIX_B_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_B_COL_PAD = MATRIX_B_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
	#endif
	
	CPAxPkt cpAx_new = NULL;
	int64_t nAddr = -1;
	
	static int nLen           = -1;
    static int rowBIndexBlock = -1;
	static int waitAR         = -1;
	static bool newColBlockB = false;
	static int runLDA         = -1;
	static int runLDB         = -1;

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			nLen	       = 3;
			waitAR         = MATRIX_C_ROW + (int)(ceil(MATRIX_C_ROW /((nLen + 1) * 4.0)));
			runLDA		   = (int)(ceil(MATRIX_A_COL /((nLen + 1) * 4.0)));
			runLDB		   = MATRIX_B_COL;
			rowBIndexBlock = 0;
			rowAIndex = 0;
			colAIndex = 0;
			rowBIndex = 0;
			colBIndex = 0;
			rowCIndex = 0;
			colCIndex = 0;
			this->nStateAddr_next = ESTATE_ADDR_LDA;
			this->nStateAddr      = this->nStateAddr_next;
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if ((this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO) && (this->nARTrans < this->nAR_GEN_NUM) && (runLDA > 0)) {
				runLDA--;
				nAddr = nStartAddrA + (rowAIndex * MATRIX_A_COL_PAD * ELEMENT_SIZE) + (colAIndex * ELEMENT_SIZE);
				// create transfer package
				sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
				cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
				cpAx_new->SetPkt(ARID,  nAddr,  nLen);
				cpAx_new->SetSrcName(this->cName);
				cpAx_new->SetVA(nAddr);
				cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
				cpAx_new->SetTransNum(nARTrans + 1);
				colAIndex += ((nLen + 1) * 4);
				if(colAIndex == MATRIX_A_COL){
					colAIndex      = 0;
					rowAIndex++;
					//this->nStateAddr_next = ESTATE_ADDR_LDB;
					//this->nStateAddr      = this->nStateAddr_next;
				}
				if(rowAIndex == MATRIX_A_ROW){
					newColBlockB = true;
					rowAIndex = 0;
				}
				
				// Push AR
				UPUD upAx_new = new UUD;
				upAx_new->cpAR = cpAx_new;
				this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
				#ifdef DEBUG_MST
					printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
					//cpAx_new->Display();
					//this->cpFIFO_AR->Display();
					printf("Cycle %10ld, read address A: 0x%lx\n", nCycle, nAddr);
				#endif
				Delete_UD(upAx_new, EUD_TYPE_AR);
				this->nARTrans++;
			}
			//break;
		}
		case(ESTATE_ADDR_LDB): {
			// Check FIFO full
			if ((this->cpFIFO_AR->IsFull() == ERESULT_TYPE_NO) && (this->nARTrans < this->nAR_GEN_NUM)) {
				runLDB--;
				if(runLDB == 0){
					runLDA = (int)(ceil(MATRIX_A_COL /((nLen + 1) * 4.0)));
					runLDB = MATRIX_B_COL;
				}
				nAddr = nStartAddrB + (rowBIndex * MATRIX_B_COL_PAD * ELEMENT_SIZE) + (colBIndex * ELEMENT_SIZE);
				// create transfer package
				sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
				cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
				cpAx_new->SetPkt(ARID,  nAddr,  nLen);
				cpAx_new->SetSrcName(this->cName);
				cpAx_new->SetVA(nAddr);
				cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
				cpAx_new->SetTransNum(nARTrans + 1);
				rowBIndex++;
				if(rowBIndex == rowBIndexBlock + (nLen + 1) * 4){
					rowBIndexBlock += (nLen + 1) * 4;
					if(rowBIndex == MATRIX_B_ROW){
						rowBIndexBlock  = 0;
						rowBIndex       = 0;
						//this->nStateAddr_next = ESTATE_ADDR_STC;
						//this->nStateAddr      = this->nStateAddr_next;
						if(newColBlockB){
							colBIndex      += ((nLen + 1) * 4);
							newColBlockB = false;
						}
					}

				}

				if(colBIndex == MATRIX_B_COL){
					cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
				}
				
				// Push AR
				UPUD upAx_new = new UUD;
				upAx_new->cpAR = cpAx_new;
				this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
				#ifdef DEBUG_MST
					printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
					//cpAx_new->Display();
					//this->cpFIFO_AR->Display();
					printf("Cycle %10ld, read address B: 0x%lx\n", nCycle, nAddr);
				#endif
				Delete_UD(upAx_new, EUD_TYPE_AR);
				this->nARTrans++;
			}
			//break;
		}
		case(ESTATE_ADDR_LDC): {}
		case(ESTATE_ADDR_STC): {
			// Check FIFO full
			if ((this->cpFIFO_AW->IsFull() == ERESULT_TYPE_NO) && (waitAR <= this->nARTransFinished)) {

				nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
				//printf("Waddress C: 0x%lx\n", nAddr);
				// create transfer package
				sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
				cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
				cpAx_new->SetPkt(AWID,  nAddr,  nLen); 
				cpAx_new->SetSrcName(this->cName);
				cpAx_new->SetVA(nAddr);
				cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
				cpAx_new->SetTransNum(nAWTrans + 1);
				rowCIndex++;
				waitAR += MATRIX_C_ROW + (int)(ceil(MATRIX_C_ROW /((nLen + 1) * 4.0)));
				if(rowCIndex == MATRIX_C_ROW){
					colCIndex      += (nLen + 1) * 4;
					rowCIndex       = 0;
				}
				if(colCIndex == MATRIX_C_COL){
					cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
					printf("Application finish\n");
				}

				// Push AR
				UPUD upAx_new = new UUD;
				upAx_new->cpAR = cpAx_new;
				this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
				#ifdef DEBUG_MST
					printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
					//cpAx_new->Display();
					//this->cpFIFO_AW->Display();
					printf("Cycle %10ld, write address C: 0x%lx\n", nCycle, nAddr);
				#endif
				Delete_UD(upAx_new, EUD_TYPE_AW);
				this->nAWTrans++;
			}
			this->nStateAddr_next = ESTATE_ADDR_LDA;
			this->nStateAddr      = this->nStateAddr_next;
			break;
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}


EResultType CMST::LoadTransfer_MatrixMultiplicationLocalCache(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_B_COL_PAD = MATRIX_B_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_B_COL_PAD = MATRIX_B_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
	#endif
	
	CPAxPkt cpAx_new = NULL;
	int64_t nAddr	= -1;
	static int nLenC = -1;
	static int computationDelay = 1;
	int delayPerComputation = 10;

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			rowAIndex = 0;
			colAIndex = 0;
			rowBIndex = 0;
			colBIndex = 0;
			rowCIndex = 0;
			colCIndex = 0;
			nStateAddr_next = ESTATE_ADDR_LDA;
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			if(this->nARTrans != this->nARTransFinished){		// Wait until done all of R transaction
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDA){
				return (ERESULT_TYPE_SUCCESS);
			}
			nStateAddr = ESTATE_ADDR_LDA;
			//printf("current state: read A\n");
			nAddr = nStartAddrA + (rowAIndex * MATRIX_A_COL_PAD * ELEMENT_SIZE) + (colAIndex * ELEMENT_SIZE);
			//printf("R: 0x%08lx\n", nAddr);
			int remainElement = MATRIX_A_COL - colAIndex;
			//printf("remainElement: %d\n", remainElement);
			int nLen = -1;
			if(remainElement <= 4){
				nLen = 0;
			} else if(remainElement <= 8){
				nLen = 1;
			} else if(remainElement <= 12){
				nLen = 2;
			} else if(remainElement > 12){
				nLen = 3;
			} else {
				assert(0);
			}
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  nLen);
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowAIndex == MATRIX_A_ROW){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
				printf("load address A: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;

			colAIndex += (nLen + 1) * 4;
			if(colAIndex > MATRIX_A_COL) assert(0);
			if(colAIndex == MATRIX_A_COL){
				colAIndex = 0;
			}
			this->nStateAddr_next = ESTATE_ADDR_LDB;
			this->nStateAddr	  = this->nStateAddr_next;
			break;
		}
		case(ESTATE_ADDR_LDB): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDB){
				return (ERESULT_TYPE_SUCCESS);
			}
			//printf("current state: read B\n");
			nAddr = nStartAddrB + (rowBIndex * MATRIX_B_COL_PAD * ELEMENT_SIZE) + (colBIndex * ELEMENT_SIZE);
			//printf("R: 0x%08lx\n", nAddr);
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  0);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowAIndex == MATRIX_A_ROW){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
				printf("load address B: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;

			rowBIndex++;
			if(rowBIndex > MATRIX_B_ROW) assert(0);
			if(rowBIndex == MATRIX_B_ROW){
				rowBIndex = 0;
				colBIndex++;
				if(colBIndex == MATRIX_B_COL){
					rowBIndex = 0;
					colBIndex = 0;
				}
				this->nStateAddr_next = ESTATE_ADDR_STC;
				computationDelay = delayPerComputation * (nLenC + 1) * 4;
			} else {
				if(rowBIndex == colAIndex){
					this->nStateAddr_next = ESTATE_ADDR_LDA;
				} else {
					this->nStateAddr_next = ESTATE_ADDR_LDB;
				}
			}
			this->nStateAddr	  = this->nStateAddr_next;
			break;
		}
		case(ESTATE_ADDR_LDC): {
		}
		case(ESTATE_ADDR_STC): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			if(this->nARTrans != this->nARTransFinished){		// Wait until done all of R transaction
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STC){
				return (ERESULT_TYPE_SUCCESS);
			}
			if(computationDelay > 0){			// delay due to computation cycles
				computationDelay--;
				return (ERESULT_TYPE_SUCCESS);
			}
			//printf("current state: store C\n");
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("W: 0x%08lx\n", nAddr);
			//if(colAIndex == MATRIX_A_COL){ 
			//	colCIndex++;
			//}
			int remainElement = MATRIX_C_COL - colCIndex;
			//printf("remainElement: %d\n", remainElement);
			if(nLenC == -1){
				if(remainElement <= 4){
					nLenC = 0;
				} else if(remainElement <= 8){
					nLenC = 1;
				} else if(remainElement <= 12){
					nLenC = 2;
				} else if(remainElement > 12){
					nLenC = 3;
				} else {
					assert(0);
				}
			}
			colCIndex++;
			if(colCIndex % ((nLenC + 1) * 4) == 0){
				remainElement = MATRIX_C_COL - colCIndex;
				//printf("remainElement: %d\n", remainElement);
			
				// create transfer package
				sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
				cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
				int64_t nAddrStart = nAddr - (nLenC + 1) * 16 + 4;
				cpAx_new->SetPkt(AWID,  nAddrStart,  nLenC);  // 4 bytes data / 8 bytes data: 1 BURST
				cpAx_new->SetSrcName(this->cName);
				cpAx_new->SetVA(nAddr);
				cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
				cpAx_new->SetTransNum(nAWTrans + 1);
				if(rowCIndex == MATRIX_C_ROW){
					cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
				}
				// Push AR
				UPUD upAx_new = new UUD;
				upAx_new->cpAR = cpAx_new;
				this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
				#ifdef DEBUG_MST
					printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
					//cpAx_new->Display();
					//this->cpFIFO_AW->Display();
					printf("write address C: 0x%lx\n", nAddrStart);
				#endif
				Delete_UD(upAx_new, EUD_TYPE_AW);
				this->nAWTrans++;
				if(remainElement <= 4){
					nLenC = 0;
				} else if(remainElement <= 8){
					nLenC = 1;
				} else if(remainElement <= 12){
					nLenC = 2;
				} else if(remainElement > 12){
					nLenC = 3;
				} else {
					assert(0);
				}
			}
			if(colCIndex == MATRIX_C_COL){
				rowAIndex++;
				rowCIndex++;
				colCIndex = 0;
				nLenC = -1;
				#ifdef DEBUG_MST
					if(rowCIndex == MATRIX_C_ROW){
						printf("Application finish!!!\n");
					}
				#endif
			}
			this->nStateAddr_next = ESTATE_ADDR_LDA;
			this->nStateAddr	  = this->nStateAddr_next;
			break;
		}
		case(ESTATE_ADDR_UND): {
			assert(0);
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}

EResultType CMST::LoadTransfer_MatrixMultiplication(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_B_COL_PAD = MATRIX_B_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_B_COL_PAD = MATRIX_B_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
	#endif
	
	CPAxPkt cpAx_new = NULL;
	int64_t nAddr = -1;

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			rowAIndex = 0;
			colAIndex = 0;
			rowBIndex = 0;
			colBIndex = 0;
			rowCIndex = 0;
			colCIndex = 0;
			nStateAddr_next = ESTATE_ADDR_LDA;
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDA){
				return (ERESULT_TYPE_SUCCESS);
			}
			nStateAddr = ESTATE_ADDR_LDA;
			if(colAIndex == MATRIX_A_COL){
				if(rowBIndex == MATRIX_B_ROW && colBIndex == MATRIX_B_COL - 1){
					rowAIndex++;
				}
				colAIndex = 0;
			}
			nStateAddr_next = ESTATE_ADDR_LDB;
			nAddr = nStartAddrA + (rowAIndex * MATRIX_A_COL_PAD * ELEMENT_SIZE) + (colAIndex * ELEMENT_SIZE);
			//printf("R: 0x%08lx\n", nAddr);
			colAIndex++;
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  0);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowAIndex == MATRIX_A_ROW){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			break;
		}
		case(ESTATE_ADDR_LDB): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDB){
				return (ERESULT_TYPE_SUCCESS);
			}
			if(rowBIndex == MATRIX_B_ROW && colBIndex == MATRIX_B_COL - 1){
				rowBIndex = 0;
				colBIndex = 0;
			} else if(rowBIndex == MATRIX_B_ROW){
				if(colBIndex == MATRIX_B_COL - 1){
					
				} else {
					colBIndex++;
					rowBIndex = 0;
				}
			}
			nAddr = nStartAddrB + (rowBIndex * MATRIX_B_COL_PAD * ELEMENT_SIZE) + (colBIndex * ELEMENT_SIZE);
			//printf("R: 0x%08lx\n", nAddr);
			rowBIndex++;
			nStateAddr_next = (rowBIndex == MATRIX_B_ROW) ? ESTATE_ADDR_STC : ESTATE_ADDR_LDA;
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  0);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowAIndex == MATRIX_A_ROW){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			break;
		}
		case(ESTATE_ADDR_LDC): {
		}
		case(ESTATE_ADDR_STC): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STC){
				return (ERESULT_TYPE_SUCCESS);
			}
			if(colAIndex < MATRIX_A_COL){
				return (ERESULT_TYPE_SUCCESS);
			}
			nStateAddr_next = ESTATE_ADDR_LDA;
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("W: 0x%08lx\n", nAddr);
			//if(colAIndex == MATRIX_A_COL){ 
			//	colCIndex++;
			//}
			colCIndex++;
			if(colCIndex == MATRIX_C_COL){
				rowCIndex++;
				colCIndex = 0;
			}
			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  0);  // 4 bytes data / 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);
			if(rowCIndex == MATRIX_C_ROW){
				cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;
			break;
		}
		case(ESTATE_ADDR_UND): {
			assert(0);
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}

EResultType CMST::LoadTransfer_MatrixMultiplicationKIJLocalCache(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_B_COL_PAD = MATRIX_B_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_B_COL_PAD = MATRIX_B_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
	#endif
	
	CPAxPkt cpAx_new = NULL;
	int64_t nAddr = -1;
	static int nLen = -1;
	static int computationDelay = 1;
	int delayPerComputation = 10;

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			rowAIndex = 0;
			colAIndex = 0;
			rowBIndex = 0;
			colBIndex = 0;
			rowCIndex = 0;
			colCIndex = 0;
			nStateAddr_next = ESTATE_ADDR_LDA;
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDA){
				return (ERESULT_TYPE_SUCCESS);
			}
			//printf("current state: Load A\n");
			nStateAddr = ESTATE_ADDR_LDA;
			if(rowAIndex == MATRIX_A_ROW){
				colAIndex++;
				rowAIndex = 0;
			}
			nAddr = nStartAddrA + (rowAIndex * MATRIX_A_COL_PAD * ELEMENT_SIZE) + (colAIndex * ELEMENT_SIZE);
			//printf("R 0x%08lx\n", nAddr);
			//printf("ReadA Address: 0x%lx\n", nAddr);
			rowAIndex++;
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  0);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowAIndex == MATRIX_A_ROW){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
				printf("read address A: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			this->nStateAddr_next = ESTATE_ADDR_LDB;
			this->nStateAddr	  = this->nStateAddr_next;
			break;
		}
		case(ESTATE_ADDR_LDB): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDB){
				return (ERESULT_TYPE_SUCCESS);
			}
			//printf("current state: Load B\n");
			nAddr = nStartAddrB + (rowBIndex * MATRIX_B_COL_PAD * ELEMENT_SIZE) + (colBIndex * ELEMENT_SIZE);
			//printf("R 0x%08lx\n", nAddr);
			//printf("ReadB Address: 0x%lx\n", nAddr);
			int remainElement = MATRIX_B_COL - colBIndex;
			//printf("remainElement: %d\n", remainElement);
			if(remainElement <= 4){
				nLen = 0;
			} else if(remainElement <= 8){
				nLen = 1;
			} else if(remainElement <= 12){
				nLen = 2;
			} else if(remainElement > 12){
				nLen = 3;
			} else {
				assert(0);
			}
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  nLen);
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowAIndex == MATRIX_A_ROW){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
				printf("read address B: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;

			colBIndex += (nLen + 1) * 4;
			if(rowAIndex == MATRIX_A_ROW && colBIndex == MATRIX_B_COL){
				rowBIndex++;
			}
			if(colBIndex > MATRIX_B_COL) assert(0);
			if(colBIndex == MATRIX_B_COL){
				colBIndex = 0;
			}
			this->nStateAddr_next = ESTATE_ADDR_LDC;
			this->nStateAddr	  = this->nStateAddr_next;
			break;
		}
		case(ESTATE_ADDR_LDC): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDC){
				return (ERESULT_TYPE_SUCCESS);
			}
			//printf("current state: Load C\n");
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("R 0x%08lx\n", nAddr);
			//printf("ReadC Address: 0x%lx\n", nAddr);
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  nLen);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowAIndex == MATRIX_A_ROW){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
				printf("read address C: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			this->nStateAddr_next = ESTATE_ADDR_STC;
			this->nStateAddr	  = this->nStateAddr_next;
			computationDelay = delayPerComputation * (nLen + 1) * 4;
			break;
		}
		case(ESTATE_ADDR_STC): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			if(this->nARTrans != this->nARTransFinished){		// Wait until done all of R transaction
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STC){
				return (ERESULT_TYPE_SUCCESS);
			}
			if(computationDelay > 0){			// delay due to computation cycles
				computationDelay--;
				return (ERESULT_TYPE_SUCCESS);
			}
			static int countStoreC = 0; //Duong DEBUG
			//printf("current state: Store C\n");
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("W 0x%08lx\n", nAddr);
			//printf("WriteC Address: 0x%lx\n", nAddr);
			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  nLen);  // 4 bytes data / 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);

			colCIndex += (nLen + 1) * 4;
			if(colCIndex > MATRIX_C_COL) assert(0);
			if(colCIndex == MATRIX_C_COL){
				rowCIndex++;
				if(rowCIndex == MATRIX_C_ROW){
					//cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
					countStoreC++;
					if(countStoreC == MATRIX_B_ROW) printf("Application finish\n");
					rowCIndex = 0;
				}
				colCIndex = 0;
				this->nStateAddr_next = ESTATE_ADDR_LDA;
			} else {
				this->nStateAddr_next = ESTATE_ADDR_LDB;
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
				printf("write address C: 0x%lx\n", nAddr);
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;

			this->nStateAddr = this->nStateAddr_next;
			break;
		}
		case(ESTATE_ADDR_UND): {
			assert(0);
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}

EResultType CMST::LoadTransfer_MatrixMultiplicationKIJ(int64_t nCycle){

	// Check trans num user defined
	//if((this->nARTrans == this->nAR_GEN_NUM) && (this->nAWTrans == this->nAW_GEN_NUM)) {
	if(this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Padding
	#ifdef PADDING_ENABLE
		int MATRIX_A_COL_PAD = MATRIX_A_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_B_COL_PAD = MATRIX_B_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
		int MATRIX_C_COL_PAD = MATRIX_C_COL + MAX_TRANS_SIZE / ELEMENT_SIZE;
	#else
		int MATRIX_A_COL_PAD = MATRIX_A_COL;
		int MATRIX_B_COL_PAD = MATRIX_B_COL;
		int MATRIX_C_COL_PAD = MATRIX_C_COL;
	#endif
	
	CPAxPkt cpAx_new = NULL;
	int64_t nAddr = -1;

	// Generate
	char cAxPktName[50];
	
	// Get addr and create transfer package
	switch(this->nStateAddr){
		case(ESTATE_ADDR_INI): {
			rowAIndex = 0;
			colAIndex = 0;
			rowBIndex = 0;
			colBIndex = 0;
			rowCIndex = 0;
			colCIndex = 0;
			nStateAddr_next = ESTATE_ADDR_LDA;
		}
		case(ESTATE_ADDR_LDA): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDA){
				return (ERESULT_TYPE_SUCCESS);
			}
			nStateAddr = ESTATE_ADDR_LDA;
			if(rowAIndex == MATRIX_A_ROW){
				colAIndex++;
				rowAIndex = 0;
			}
			nStateAddr_next = ESTATE_ADDR_LDB;
			nAddr = nStartAddrA + (rowAIndex * MATRIX_A_COL_PAD * ELEMENT_SIZE) + (colAIndex * ELEMENT_SIZE);
			//printf("R 0x%08lx\n", nAddr);
			//printf("ReadA Address: 0x%lx\n", nAddr);
			rowAIndex++;
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  0);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowAIndex == MATRIX_A_ROW){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			break;
		}
		case(ESTATE_ADDR_LDB): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDB){
				return (ERESULT_TYPE_SUCCESS);
			}
			nAddr = nStartAddrB + (rowBIndex * MATRIX_B_COL_PAD * ELEMENT_SIZE) + (colBIndex * ELEMENT_SIZE);
			//printf("R 0x%08lx\n", nAddr);
			//printf("ReadB Address: 0x%lx\n", nAddr);
			colBIndex++;
			if(rowAIndex == MATRIX_A_ROW && colBIndex == MATRIX_B_COL){
				rowBIndex++;
			}
			if(colBIndex == MATRIX_B_COL){
				colBIndex = 0;
			}
			nStateAddr_next = ESTATE_ADDR_LDC;
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  0);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowAIndex == MATRIX_A_ROW){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			break;
		}
		case(ESTATE_ADDR_LDC): {
			// Check FIFO full
			if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_LDC){
				return (ERESULT_TYPE_SUCCESS);
			}
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("R 0x%08lx\n", nAddr);
			//printf("ReadC Address: 0x%lx\n", nAddr);
			nStateAddr_next = ESTATE_ADDR_STC;
			// create transfer package
			sprintf(cAxPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_READ);
			cpAx_new->SetPkt(ARID,  nAddr,  0);  // 4 bytes or 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nARTrans + 1);
			//if(rowAIndex == MATRIX_A_ROW){
			//	cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			//}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AR->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AR->Display();
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AR);
			this->nARTrans++;
			break;
		}
		case(ESTATE_ADDR_STC): {
			// Check FIFO full
			if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
				return (ERESULT_TYPE_SUCCESS);
			}
			// check state
			if(nStateAddr_next != ESTATE_ADDR_STC){
				return (ERESULT_TYPE_SUCCESS);
			}
			nAddr = nStartAddrC + (rowCIndex * MATRIX_C_COL_PAD * ELEMENT_SIZE) + (colCIndex * ELEMENT_SIZE);
			//printf("W 0x%08lx\n", nAddr);
			//printf("WriteC Address: 0x%lx\n", nAddr);
			colCIndex++;
			if(colCIndex == MATRIX_C_COL){
				rowCIndex++;
				if(rowCIndex == MATRIX_C_ROW)
					rowCIndex = 0;
				colCIndex = 0;
				nStateAddr_next = ESTATE_ADDR_LDA;
			} else {
				nStateAddr_next = ESTATE_ADDR_LDB;
			}
			// create transfer package
			sprintf(cAxPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
			cpAx_new = new CAxPkt(cAxPktName, ETRANS_DIR_TYPE_WRITE);
			cpAx_new->SetPkt(AWID,  nAddr,  0);  // 4 bytes data / 8 bytes data: 1 BURST
			cpAx_new->SetSrcName(this->cName);
			cpAx_new->SetVA(nAddr);
			cpAx_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAx_new->SetTransNum(nAWTrans + 1);
			if(rowCIndex == MATRIX_C_ROW){
				cpAx_new->SetFinalTrans(ERESULT_TYPE_YES);
			}
			// Push AR
			UPUD upAx_new = new UUD;
			upAx_new->cpAR = cpAx_new;
			this->cpFIFO_AW->Push(upAx_new, MST_FIFO_LATENCY);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAxPktName);
				//cpAx_new->Display();
				//this->cpFIFO_AW->Display();
			#endif
			Delete_UD(upAx_new, EUD_TYPE_AW);
			this->nAWTrans++;
			break;
		}
		case(ESTATE_ADDR_UND): {
			assert(0);
		}
		default: {
				assert(0);
		}
	} // end switch	

	return (ERESULT_TYPE_SUCCESS);
}

//-------------------------------------------------------------------	
// Trace master PIN 
// 	Generate Ax and FIFO push
//-------------------------------------------------------------------	
EResultType CMST::LoadTransfer_Ax_PIN_Trace(int64_t nCycle, FILE *fp) {

	// Check trace finished
	if (feof(fp)) {
		return (ERESULT_TYPE_SUCCESS);
	};

	int	 nID   = 1;
	int64_t nAddr = -1;
	int	 nLen  = 3;

	// char cLine[BUFSIZ]; // BUFSIZ (8192) defined in stdio.h
	char cLine[25]; 
	char RW;
	// int  nLine  = 0;

	// DUONGTRAN add to fix issue FIXME
	// Check FIFO full 
	if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES || this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};
	//if (this->Trace_rewind == 1) {
	//	this->Trace_rewind = 0;
	//	// rewind(fp);			// Re-start from 1st line
	//	// fseek (fp, -13, SEEK_CUR);	// Offset -13 from current cursor
	//	fgets(cLine, sizeof(cLine), fp);
	//	fseek (fp, -12, SEEK_CUR);	// Offset -12 from current cursor. PIN_trc
	//};

	//---------------------------------------------	
	// Trace format: LineNum  RW  AxAddr
	//---------------------------------------------	
	while (fgets(cLine, sizeof(cLine), fp) != NULL) { // cLine entire line.

		// Check content
		if (cLine[0] == '\n') continue; 
		// printf("%s", cLine);
	
		// Get value	
		// sscanf(cLine, "%d %c %lx",  &nLine, &RW, &nAddr);
		// printf("%d: %c %32lx \n", nLine, RW,  nAddr);
		sscanf(cLine, "%c %lx",  &RW, &nAddr);
		//DUONGTRAN debug
		//printf("DUONGTRAN DEBUG: Type: %c, Addr: 0x%lx \n", RW, nAddr);

		// Ax
		if (RW == 'R') { // AR

			// Check all trans finished
			if (this->nARTrans == this->nAR_GEN_NUM) {
				return (ERESULT_TYPE_SUCCESS);
			};

			//// Check FIFO full   DUONGTRAN comment FIXME
			//if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
			//	this->Trace_rewind = 1;
			//	return (ERESULT_TYPE_SUCCESS);
			//};

			// Check line
			// if (nLine <= this->nARTrans) continue; // FIXME need?

			// Generate AR trans
			CPAxPkt cpAR_new = NULL;
			UPUD	upAR_new = NULL;

			char cARPktName[50];
			sprintf(cARPktName, "AR%d_%s", this->nARTrans+1, this->cName.c_str());
			cpAR_new = new CAxPkt(cARPktName, ETRANS_DIR_TYPE_READ);
			
			//				ID,	Addr,   Len
			cpAR_new->SetPkt(nID,   nAddr,  nLen);
			// printf("[Cycle %3ld: %s.LoadTransfer_AR_PIN_Trace] %s generated.\n", nCycle, this->cName.c_str(), cARPktName);
			cpAR_new->SetSrcName(this->cName);
			cpAR_new->SetTransNum(this->nARTrans+1);
			cpAR_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAR_new->SetVA(nAddr);
			// cpAR_new->Display();
			
			// Push
			upAR_new = new UUD;
			upAR_new->cpAR = cpAR_new;
			this->cpFIFO_AR->Push(upAR_new, 0);

			#ifdef DEBUG_MST	
			// this->cpFIFO_AR->CheckFIFO();
			// printf("R %d\t 0x%lx \n", this->nARTrans+1, nAddr);
			
			// printf("[Cycle %3ld: %s.LoadTransfer_Ax_PIN_Trace] (%s) push FIFO_AR.\n", nCycle, this->cName.c_str(), cARPktName);
			// this->cpFIFO_AR->Display();
			#endif
		
			// Maintain	
			Delete_UD(upAR_new, EUD_TYPE_AR); // Be careful to check upAR_new, upAR_new->cpAR, upAR_new->cpAR->spPkt deleted correctly. We can manually delete these.

			// Stat
			this->nARTrans++;

			// Finished this cycle
			break;
		}
		else if (RW == 'W') { // AW

			// Check all trans finished
			if (this->nAWTrans == this->nAW_GEN_NUM) {
				return (ERESULT_TYPE_SUCCESS);
			};

			//// Check FIFO full  DUONGTRAN comment FIXME
			//if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
			//	this->Trace_rewind = 1;
			//	return (ERESULT_TYPE_SUCCESS);
			//};

			// Check line
			// if (nLine <= this->nAWTrans) continue; // FIXME need?

			// Generate AW trans
			CPAxPkt cpAW_new = NULL;
			UPUD	upAW_new = NULL;

			char cAWPktName[50];
			sprintf(cAWPktName, "AW%d_%s", this->nAWTrans+1, this->cName.c_str());
			cpAW_new = new CAxPkt(cAWPktName, ETRANS_DIR_TYPE_WRITE);
			
			//			   ID,	Addr,   Len
			cpAW_new->SetPkt(nID,   nAddr,  nLen);
			// printf("[Cycle %3ld: %s.LoadTransfer_AW_PIN_Trace] %s generated.\n", nCycle, this->cName.c_str(), cAWPktName);
			cpAW_new->SetSrcName(this->cName);
			cpAW_new->SetTransNum(this->nAWTrans+1);
			cpAW_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAW_new->SetVA(nAddr);
			// cpAW_new->Display();
			
			// Push
			upAW_new = new UUD;
			upAW_new->cpAW = cpAW_new;
			this->cpFIFO_AW->Push(upAW_new, 0);

			#ifdef DEBUG_MST
			// this->cpFIFO_AW->CheckFIFO();
			// printf("W %d\t 0x%lx \n", this->nAWTrans+1, nAddr);
			
			// printf("[Cycle %3ld: %s.LoadTransfer_Ax_PIN_Trace] (%s) push FIFO_AW.\n", nCycle, this->cName.c_str(), cAWPktName);
			// this->cpFIFO_AW->Display();
			#endif
		
			// Maintain	
			Delete_UD(upAW_new, EUD_TYPE_AW);

			// Stat
			this->nAWTrans++;

			// Finished this cycle
			break;
		}
		else {
			assert (0);
		};

		// Trace
		this->Trace_rewind = 0; // read next line
	};
	
	return (ERESULT_TYPE_SUCCESS);
};

EResultType CMST::LoadTransfer_Ax_Custom(int64_t nCycle, FILE *fp, int nLen) {

	assert(BURST_SIZE == 8);  // DOUBLE datatype

	// Check trace finished
	if (feof(fp)) {
		return (ERESULT_TYPE_SUCCESS);
	};

	int	 nID   = 1;
	int64_t nAddr = -1;

	// char cLine[BUFSIZ]; // BUFSIZ (8192) defined in stdio.h
	char cLine[25]; 
	char RW;
	// int  nLine  = 0;

	// DUONGTRAN add to fix issue FIXME
	// Check FIFO full 
	if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES || this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};

	//---------------------------------------------	
	// Trace format: LineNum  RW  AxAddr
	//---------------------------------------------	
	while (fgets(cLine, sizeof(cLine), fp) != NULL) { // cLine entire line.

		// Check content
		if (cLine[0] == '\n') continue; 
		// printf("%s", cLine);
	
		// Get value	
		// sscanf(cLine, "%d %c %lx",  &nLine, &RW, &nAddr);
		// printf("%d: %c %32lx \n", nLine, RW,  nAddr);
		sscanf(cLine, "%c %lx",  &RW, &nAddr);
		//DUONGTRAN debug
		//printf("DUONGTRAN DEBUG: Type: %c, Addr: 0x%lx \n", RW, nAddr);

		// Ax
		if (RW == 'R') { // AR

			// Check all trans finished
			if (this->nARTrans == this->nAR_GEN_NUM) {
				return (ERESULT_TYPE_SUCCESS);
			};

			// Generate AR trans
			CPAxPkt cpAR_new = NULL;
			UPUD	upAR_new = NULL;

			char cARPktName[50];
			sprintf(cARPktName, "AR%d_%s", this->nARTrans+1, this->cName.c_str());
			cpAR_new = new CAxPkt(cARPktName, ETRANS_DIR_TYPE_READ);
			
			//				ID,	Addr,   Len
			cpAR_new->SetPkt(nID,   nAddr,  nLen);
			// printf("[Cycle %3ld: %s.LoadTransfer_AR_PIN_Trace] %s generated.\n", nCycle, this->cName.c_str(), cARPktName);
			cpAR_new->SetSrcName(this->cName);
			cpAR_new->SetTransNum(this->nARTrans+1);
			cpAR_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAR_new->SetVA(nAddr);
			// cpAR_new->Display();
			
			// Push
			upAR_new = new UUD;
			upAR_new->cpAR = cpAR_new;
			this->cpFIFO_AR->Push(upAR_new, 0);

			#ifdef DEBUG_MST	
			// this->cpFIFO_AR->CheckFIFO();
			// printf("R %d\t 0x%lx \n", this->nARTrans+1, nAddr);
			
			// printf("[Cycle %3ld: %s.LoadTransfer_Ax_PIN_Trace] (%s) push FIFO_AR.\n", nCycle, this->cName.c_str(), cARPktName);
			// this->cpFIFO_AR->Display();
			#endif
		
			// Maintain	
			Delete_UD(upAR_new, EUD_TYPE_AR); // Be careful to check upAR_new, upAR_new->cpAR, upAR_new->cpAR->spPkt deleted correctly. We can manually delete these.

			// Stat
			this->nARTrans++;

			// Finished this cycle
			break;
		}
		else if (RW == 'W') { // AW

			// Check all trans finished
			if (this->nAWTrans == this->nAW_GEN_NUM) {
				return (ERESULT_TYPE_SUCCESS);
			};

			// Check line
			// if (nLine <= this->nAWTrans) continue; // FIXME need?

			// Generate AW trans
			CPAxPkt cpAW_new = NULL;
			UPUD	upAW_new = NULL;

			char cAWPktName[50];
			sprintf(cAWPktName, "AW%d_%s", this->nAWTrans+1, this->cName.c_str());
			cpAW_new = new CAxPkt(cAWPktName, ETRANS_DIR_TYPE_WRITE);
			
			//			   ID,	Addr,   Len
			cpAW_new->SetPkt(nID,   nAddr,  nLen);
			// printf("[Cycle %3ld: %s.LoadTransfer_AW_PIN_Trace] %s generated.\n", nCycle, this->cName.c_str(), cAWPktName);
			cpAW_new->SetSrcName(this->cName);
			cpAW_new->SetTransNum(this->nAWTrans+1);
			cpAW_new->SetTransType(ETRANS_TYPE_NORMAL);
			cpAW_new->SetVA(nAddr);
			// cpAW_new->Display();
			
			// Push
			upAW_new = new UUD;
			upAW_new->cpAW = cpAW_new;
			this->cpFIFO_AW->Push(upAW_new, 0);

			#ifdef DEBUG_MST
			// this->cpFIFO_AW->CheckFIFO();
			// printf("W %d\t 0x%lx \n", this->nAWTrans+1, nAddr);
			
			// printf("[Cycle %3ld: %s.LoadTransfer_Ax_PIN_Trace] (%s) push FIFO_AW.\n", nCycle, this->cName.c_str(), cAWPktName);
			// this->cpFIFO_AW->Display();
			#endif
		
			// Maintain	
			Delete_UD(upAW_new, EUD_TYPE_AW);

			// Stat
			this->nAWTrans++;

			// Finished this cycle
			break;
		}
		else {
			assert (0);
		};
	};
	
	return (ERESULT_TYPE_SUCCESS);
};

//-------------------------------------------------------------	
// Trace master AXI
//	Generate AR and FIFO push
//	FIXME When FIFO size limited, line number lost
//	FIXME fopen, fclose should be global
//-------------------------------------------------------------	
EResultType CMST::LoadTransfer_AR_AXI_Trace(int64_t nCycle) {

	// Check FIFO full 
	if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};

	if (this->nARTrans == this->nAR_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};
	
	CPAxPkt cpAR_new = NULL;
	UPUD	upAR_new = NULL;
	
	char const *FileName = "AXI_trace.trc";
	FILE *fp = NULL;
	char cLine[BUFSIZ]; // BUFSIZ (8192) is defined in stdio.h
	char nLine = 0;
	
	fp = fopen(FileName, "r");
	if (!fp) {
		printf("Couldn't open file %s for reading.\n", FileName);
		return (ERESULT_TYPE_FAIL);
	}
	// printf("Opened file %s for reading.\n\n", FileName);
	
	int nCycleDelay  = -1;
	int AxID   = -1;
	int AxADDR = -1;
	int AxLEN  = -1;
	
	// printf("--------------------------------\n");
	// printf("Line: Cycle AxID   AxADDR AxLEN \n");
	// printf("--------------------------------\n");

		while (fgets(cLine, sizeof(cLine), fp) != NULL) { // Entire line

		// First six lines are comments
		if (nLine < 6) {
			nLine++;
			continue;
		};
		
		nLine++;
		
		// printf("%4d: %s", nLine, cLine); // Newline is in cLine
		
		sscanf(cLine, "%d %d %d %d", &nCycleDelay, &AxID, &AxADDR, &AxLEN);
		// printf("%4d: %4d %6d %8x   %d\n", nLine-6, nCycleDelay, AxID, ARADDR, ARLEN);
		
		// Generate AR transaction
		char cARPktName[50];
		sprintf(cARPktName, "AR%d_%s", nLine-6, this->cName.c_str());
		cpAR_new = new CAxPkt(cARPktName, ETRANS_DIR_TYPE_READ);
		
		// AxID, AxADDR, AxLEN
		int	 nID   = AxID;
		int64_t nAddr = AxADDR;
		int	 nLen  = AxLEN;
		
		//			   ID,	Addr,   Len
		cpAR_new->SetPkt(nID,   nAddr,  nLen);
		// printf("[Cycle %3ld: %s.LoadTransfer_AR_Trace] %s generated.\n", nCycle, this->cName.c_str(), cARPktName);
		cpAR_new->SetSrcName(this->cName);
		cpAR_new->SetTransNum(nLine-6);
		cpAR_new->SetTransType(ETRANS_TYPE_NORMAL);
		cpAR_new->SetVA(nAddr);
		cpAR_new->Display();
		
		// Push
		upAR_new = new UUD;
		upAR_new->cpAR = cpAR_new;
		this->cpFIFO_AR->Push(upAR_new, nCycleDelay);

		#ifdef DEBUG_MST 
		// this->cpFIFO_AR->CheckFIFO();
		printf("[Cycle %3ld: %s.LoadTransfer_AR_AXI_Trace] (%s) push FIFO_AR.\n", nCycle, this->cName.c_str(), cARPktName);
		this->cpFIFO_AR->Display();
		#endif
	
		// Maintain	
		Delete_UD(upAR_new, EUD_TYPE_AR);
		
		// Stat
		this->nARTrans++;
	};
	
	// AR_NUM
	this->Set_nAR_GEN_NUM(nLine-6);
	
	// Stat
	// printf("\n Number of AR = %d\n", nLine-6);
	
	return (ERESULT_TYPE_SUCCESS);
};


//-------------------------------------------------------------	
// Generate AR. FIFO push
//-------------------------------------------------------------	
EResultType CMST::LoadTransfer_AR_Test(int64_t nCycle) {

	// Check master finished
	if (this->cpAddrGen_AR->IsFinalTrans() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Check trans num user defined
	if (this->nARTrans == this->nAR_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};
	
	// Check FIFO full 
	if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};

	CPAxPkt cpAR_new = NULL;
	UPUD	upAR_new = NULL;

	int64_t nAddr = this->nAR_START_ADDR - AR_ADDR_INC;
	// int64_t nAddr = this->nAR_START_ADDR - 0x20 ;			// Test

	for (int nIndex =0; nIndex <(this->nAR_GEN_NUM); nIndex++) {

		// Generate AR transaction
		char cARPktName[50];
		sprintf(cARPktName, "AR%d_%s", nIndex+1, this->cName.c_str());
		cpAR_new = new CAxPkt(cARPktName, ETRANS_DIR_TYPE_READ);
		
		// AxID
		int nID = ARID;
		
		#ifdef ARID_RANDOM
		nID = (rand() % 16);
		#endif
		
		// AxADDR
		nAddr += AR_ADDR_INC;
		// nAddr -= AR_ADDR_INC;	// Debug 
		// nAddr += 0x20;		// Debug 
		
		// // Random generation
		// #ifdef AR_ADDR_RANDOM	
		// // nAddr  = ( ( (rand() % 0x7FFF) >> 6) << 6 ); // Addr signed int type.
		// nAddr  = ( ( (rand() % 0x7FFFFFFF) >> 6) << 6 ); // Addr signed int type.
		// // nAddr  = ( ( (rand() % 0xFF) >> 6) << 6 );  // Addr signed int type  // Test
		// #endif
		
		//			   ID,	Addr,   Len
		cpAR_new->SetPkt(nID,   nAddr,  MAX_BURST_LENGTH-1);
		// printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cARPktName);
		cpAR_new->SetSrcName(this->cName);
		cpAR_new->SetTransNum(nIndex+1);
		cpAR_new->SetTransType(ETRANS_TYPE_NORMAL);
		cpAR_new->SetVA(nAddr);
		cpAR_new->Display();		
		
		// Push
		upAR_new = new UUD;
		upAR_new->cpAR = cpAR_new;
		this->cpFIFO_AR->Push(upAR_new, MST_FIFO_LATENCY);

		#ifdef DEBUG_MST	
		// this->cpFIFO_AR->CheckFIFO();
		// printf("[Cycle %3ld: %s.LoadTransfer_AR] (%s) push FIFO_AR.\n", nCycle, this->cName.c_str(), cARPktName);
		// this->cpFIFO_AR->Display();
		#endif
	
		// Maintain	
		Delete_UD(upAR_new, EUD_TYPE_AR);
		
		// Stat
		this->nARTrans++;
	};
	return (ERESULT_TYPE_SUCCESS);
};


//-------------------------------------------------------------	
// Generate AW and FIFO push
//-------------------------------------------------------------	
EResultType CMST::LoadTransfer_AW_Test(int64_t nCycle) {

	// Check master finished
	if (this->cpAddrGen_AW->IsFinalTrans() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Check trans num user defined
	if (this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};
	
	// Check FIFO full 
	if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// if (this->cpFIFO_W->IsFull() == ERESULT_TYPE_YES) {
	// 	return (ERESULT_TYPE_SUCCESS);
	// };

	int64_t nAddr = this->nAW_START_ADDR - AW_ADDR_INC;
	// int64_t nAddr = this->nAW_START_ADDR - 0x20;		// Test

	for (int nIndex =0; nIndex <(this->nAW_GEN_NUM); nIndex++) {

		// Generate AW transaction
		char cAWPktName[50];
		sprintf(cAWPktName, "AW%d_%s", nIndex+1, this->cName.c_str());

		#ifdef DEBUG_MST
		// printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAWPktName);
		#endif
		
		CPAxPkt cpAW_new = new CAxPkt(cAWPktName, ETRANS_DIR_TYPE_WRITE);
		
		// AxID
		int nAWID = AWID;
		
		#ifdef AWID_RANDOM
		nAWID = (rand() % 16);
		#endif
		
		// AxADDR
		nAddr += AW_ADDR_INC;
		// nAddr -= AW_ADDR_INC;	// Debug 
		// nAddr += 0x20;		// Debug 
		
		// Random generation
		#ifdef AW_ADDR_RANDOM	
		nAddr  = ( ( (rand() % 0x7FFFFFFF) >> 6) << 6 );  // Addr signed int type
		//nAddr  = ( ( (rand() % 0xFF) >> 6) << 6 );  // Addr signed int type  // Test
		#endif
		
		//			   ID,	Addr,   Len
		cpAW_new->SetPkt(nAWID, nAddr,  MAX_BURST_LENGTH-1);
		cpAW_new->SetSrcName(this->cName);
		cpAW_new->SetTransNum(nIndex+1);
		cpAW_new->SetVA(nAddr);
		cpAW_new->SetTransType(ETRANS_TYPE_NORMAL);
		// cpAW_new->Display();		
		
		// Push
		UPUD upAW_new = new UUD;
		upAW_new->cpAW = cpAW_new;
		this->cpFIFO_AW->Push(upAW_new, MST_FIFO_LATENCY);

		#ifdef DEBUG_MST	
		// printf("[Cycle %3ld: %s.LoadTransfer_AW] (%s) push FIFO_AW.\n", nCycle, this->cName.c_str(), cAWPktName);
		// this->cpFIFO_AW->Display();
		// this->cpFIFO_AW->CheckFIFO();
		#endif
	
		#ifdef USE_W_CHANNEL
		// Get W info
		int nBurstNum = cpAW_new->GetLen();
		int nID	   = cpAW_new->GetID();
		#endif

		// Maintain AW
		Delete_UD(upAW_new, EUD_TYPE_AW);
	
		#ifdef USE_W_CHANNEL	
		for (int i=0; i<=nBurstNum; i++) {
		
			char cWPktName[50];
			sprintf(cWPktName, "W%d_%s",this->nAWTrans+1, this->cName.c_str());
			CPWPkt  cpW_new = new CWPkt(cWPktName);
			cpW_new->SetID(nID);
			cpW_new->SetData(i+1);
			cpW_new->SetSrcName(this->cName);
			cpW_new->SetTransType(ETRANS_TYPE_NORMAL);

			if (i == nBurstNum) {
					cpW_new->SetLast(ERESULT_TYPE_YES);
			} 
			else {
					cpW_new->SetLast(ERESULT_TYPE_NO);
			};
			
			// Push W
			UPUD upW_new = new UUD;
			upW_new->cpW = cpW_new;
			this->cpFIFO_W->Push(upW_new, MST_FIFO_LATENCY);
			
			#ifdef DEBUG_MST	
			// upW_new->cpW->CheckPkt();
			// this->cpFIFO_W->CheckFIFO();
			// printf("[Cycle %3ld: %s.LoadTransfer_AW] (%s) push FIFO_W.\n", nCycle, this->cName.c_str(), cWPktName);
			// this->cpFIFO_W->Display();
			#endif

			// Maintain	 
			Delete_UD(upW_new, EUD_TYPE_W);
		};
		#endif

		// Stat
		this->nAWTrans++;
	};
	return (ERESULT_TYPE_SUCCESS);
};


//-------------------------------------------------------------	
// 1. Generate AR 
// 2. Push FIFO
//-------------------------------------------------------------	
// 	AddrMap		: LIAM, BFAM, TILE
// 	Operation	: RASTER_SCAN, ROTATION, CNN, RANDOM
//-------------------------------------------------------------	
EResultType CMST::LoadTransfer_AR(int64_t nCycle, string cAddrMap, string cOperation) {
	
	// Check master finished
	if (this->cpAddrGen_AR->IsFinalTrans() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Check trans num user defined
	if (this->nARTrans == this->nAR_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};
	
	// Check FIFO full 
	if (this->cpFIFO_AR->IsFull() == ERESULT_TYPE_YES) {
		printf("[Cycle %3ld: %s.LoadTransfer_AR] cpFIFO_AR full.\n", nCycle, this->cName.c_str());
		return (ERESULT_TYPE_SUCCESS);
	};
	
	// if (this->cpFIFO_W->IsFull() == ERESULT_TYPE_YES) {
	// 	return (ERESULT_TYPE_SUCCESS);
	// };
	
	CPAxPkt cpAR_new = NULL;
	
	// Generate
	char cARPktName[50];
	sprintf(cARPktName, "AR%d_%s",this->nARTrans+1, this->cName.c_str());
	// printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cARPktName);
	cpAR_new = new CAxPkt(cARPktName, ETRANS_DIR_TYPE_READ);

	// Get addr	 
	this->Set_AR_AddrMap(cAddrMap);
	int64_t nAddr = this->cpAddrGen_AR->GetAddr(cAddrMap);

	if (cOperation == "RANDOM") {	
		nAddr  = ( ( (rand() % 0x7FFFFFFF) >> 6) << 6 );  		// Addr signed int type
	};	

	#ifdef DEBUG
	// Check addr 
	if (nAddr == -1) {
		assert (0);							// No addr generated
		return (ERESULT_TYPE_SUCCESS);
	};
	#endif

	// Get tile num
	int nTileNum = this->cpAddrGen_AR->GetTileNum();

	// Get len
	int nNumPixelTrans = this->cpAddrGen_AR->GetNumPixelTrans(); 
	int nTransSizeByte = nNumPixelTrans * BYTE_PER_PIXEL;
	int nLen = -1;
	if	    (nTransSizeByte > (BURST_SIZE * 3))	nLen = 3; 
	else if (nTransSizeByte > (BURST_SIZE * 2))	nLen = 2; 
	else if (nTransSizeByte > (BURST_SIZE * 1))	nLen = 1; 
	else if (nTransSizeByte > (BURST_SIZE * 0))	nLen = 0; 
	else	assert (0);

	// Set pkt
	//			   ID,	Addr,   Len
	cpAR_new->SetPkt(ARID,  nAddr,  nLen);
	// cpAR_new->SetPkt(ARID,  nAddr,  MAX_BURST_LENGTH-1);
	cpAR_new->SetSrcName(this->cName);
	cpAR_new->SetTransNum(nARTrans + 1);
	cpAR_new->SetVA(nAddr);
	cpAR_new->SetTileNum(nTileNum);
	cpAR_new->SetTransType(ETRANS_TYPE_NORMAL);
	#ifdef DEBUG_MST
	printf("[Cycle %3ld: %s.LoadTransfer_AR] %s generated.\n", nCycle, this->cName.c_str(), cARPktName);
	// cpAR_new->Display();
	#endif
	
	// Check final trans application
	EResultType eFinalTrans = this->cpAddrGen_AR->IsFinalTrans();
	if (eFinalTrans == ERESULT_TYPE_YES) {
		cpAR_new->SetFinalTrans(ERESULT_TYPE_YES);
	};
	
	// Push AR
	UPUD upAR_new = new UUD;
	upAR_new->cpAR = cpAR_new;
	this->cpFIFO_AR->Push(upAR_new, MST_FIFO_LATENCY);

	#ifdef DEBUG_MST
	printf("[Cycle %3ld: %s.LoadTransfer_AR] (%s) push FIFO_AR.\n", nCycle, this->cName.c_str(), cARPktName);
	// this->cpFIFO_AR->Display();
	// this->cpFIFO_AR->CheckFIFO();
	#endif
	
	// Maintain AR
	Delete_UD(upAR_new, EUD_TYPE_AR);  // Check upAR_new correctly delete

	// Stat
	this->nARTrans++;
	
	return (ERESULT_TYPE_SUCCESS);
};


//-------------------------------------------------------------	
// 1. Generate AW
// 2. Push FIFO
//-------------------------------------------------------------	
EResultType CMST::LoadTransfer_AW(int64_t nCycle, string cAddrMap, string cOperation) { // AddrMap (LIAM, BFAM, TILE), Operation (RASTER_SCAN, ROTATION, RANDOM)
	
	// Check master finished
	if (this->cpAddrGen_AW->IsFinalTrans() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_SUCCESS);
	};

	if (this->nAWTrans == this->nAW_GEN_NUM) {
		return (ERESULT_TYPE_SUCCESS);
	};
	
	// Check FIFO full 
	if (this->cpFIFO_AW->IsFull() == ERESULT_TYPE_YES) {
		printf("[Cycle %3ld: %s.LoadTransfer_AW] cpFIFO_AW full.\n", nCycle, this->cName.c_str());
		return (ERESULT_TYPE_SUCCESS);
	};
	
	CPAxPkt cpAW_new = NULL;
	
	// Generate
	char cAWPktName[50];
	sprintf(cAWPktName, "AW%d_%s",this->nAWTrans+1, this->cName.c_str());
	#ifdef DEBUG_MST
		printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated.\n", nCycle, this->cName.c_str(), cAWPktName);
	#endif
	cpAW_new = new CAxPkt(cAWPktName, ETRANS_DIR_TYPE_WRITE);
	
	// Get addr
	this->Set_AW_AddrMap(cAddrMap);
	int64_t nAddr = this->cpAddrGen_AW->GetAddr(cAddrMap);
	
	if (cOperation == "RANDOM") {	
		nAddr  = ( ( (rand() % 0x7FFFFFFF) >> 6) << 6 );  			// Addr signed int type
	};	

	#ifdef DEBUG
	// Check addr 
	if (nAddr == -1) {								// No addr generated
		assert (0);
		return (ERESULT_TYPE_SUCCESS);
	};
	#endif

	// Get tile num
	int nTileNum = this->cpAddrGen_AW->GetTileNum();

	// Get len
	int nNumPixelTrans = this->cpAddrGen_AW->GetNumPixelTrans(); 
	int nTransSizeByte = nNumPixelTrans * BYTE_PER_PIXEL;
	int nLen = -1;
	if	  (nTransSizeByte > (BURST_SIZE * 3))	nLen = 3; 
	else if (nTransSizeByte > (BURST_SIZE * 2))	nLen = 2; 
	else if (nTransSizeByte > (BURST_SIZE * 1))	nLen = 1; 
	else if (nTransSizeByte > (BURST_SIZE * 0))	nLen = 0; 
	else	assert (0);

	//			   ID,	Addr,  Len
	cpAW_new->SetPkt(AWID,  nAddr, nLen);
	// cpAW_new->SetPkt(AWID,  nAddr,  MAX_BURST_LENGTH-1);
	cpAW_new->SetSrcName(this->cName);
	cpAW_new->SetTransNum(nAWTrans + 1);
	cpAW_new->SetVA(nAddr);
	cpAW_new->SetTileNum(nTileNum);
	cpAW_new->SetTransType(ETRANS_TYPE_NORMAL);

	#ifdef DEBUG_MST
	// printf("[Cycle %3ld: %s.LoadTransfer_AW] %s generated - IsFinalTrans = %s.\n", nCycle, this->cName.c_str(), cAWPktName, Convert_eResult2string(cpAW_new->IsFinalTrans()).c_str());
	// cpAW_new->Display();
	#endif
	
	// Check final trans application
	EResultType eFinalTrans = this->cpAddrGen_AW->IsFinalTrans();

	if (eFinalTrans == ERESULT_TYPE_YES) {
		cpAW_new->SetFinalTrans(ERESULT_TYPE_YES);
	};
	
	// Push AW
	UPUD upAW_new = new UUD;
	upAW_new->cpAW = cpAW_new;
	this->cpFIFO_AW->Push(upAW_new, MST_FIFO_LATENCY);

	#ifdef DEBUG_MST
	printf("[Cycle %3ld: %s.LoadTransfer_AW] (%s) push FIFO_AW - IsFinalTrans = %s.\n", nCycle, this->cName.c_str(), cAWPktName, Convert_eResult2string(cpAW_new->IsFinalTrans()).c_str());
	// this->cpFIFO_AW->Display();
	// this->cpFIFO_AW->CheckFIFO();
	#endif
	
	#ifdef USE_W_CHANNEL
	// Get info for W
	int nBurstNum = cpAW_new->GetLen();
	int nID	   = cpAW_new->GetID();
	#endif
	
	// Maintain AW
	Delete_UD(upAW_new, EUD_TYPE_AW);

	#ifdef USE_W_CHANNEL	
	for (int i=0; i<=nBurstNum; i++) {
		
		char cWPktName[50];
		sprintf(cWPktName, "W%d_%s",this->nAWTrans+1, this->cName.c_str());

		CPWPkt  cpW_new = new CWPkt(cWPktName);
		cpW_new->SetID(nID);
		cpW_new->SetData(i+1);
		cpW_new->SetSrcName(this->cName);
		cpW_new->SetTransType(ETRANS_TYPE_NORMAL);

		if (i == nBurstNum) {
				cpW_new->SetLast(ERESULT_TYPE_YES);
		}
		else {
				cpW_new->SetLast(ERESULT_TYPE_NO);
		};
		
		// Push W
		UPUD upW_new = new UUD;
		upW_new->cpW = cpW_new;
		this->cpFIFO_W->Push(upW_new, MST_FIFO_LATENCY);
	
		#ifdef DEBUG_MST	
		// upW_new->cpW->CheckPkt();
		// this->cpFIFO_W->CheckFIFO();
		// printf("[Cycle %3ld: %s.LoadTransfer_AW] (%s) push FIFO_W.\n", nCycle, this->cName.c_str(), cWPktName);
		// this->cpFIFO_W->Display();
		// this->cpFIFO_W->CheckFIFO();
		#endif
		
		// Maintain	 
		Delete_UD(upW_new, EUD_TYPE_W);
	};
	#endif
	
	// Stat
	this->nAWTrans++;
	
	return (ERESULT_TYPE_SUCCESS);
};


//------------------------------------
// AR valid
//------------------------------------
EResultType CMST::Do_AR_fwd(int64_t nCycle) {

	// Check issue interval
	if (nCycle % this->AR_ISSUE_MIN_INTERVAL != 0) {
		  return (ERESULT_TYPE_SUCCESS);  
	};

	// Check Tx valid 
	if (this->cpTx_AR->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};

	// Check FIFO
	if (this->cpFIFO_AR->IsEmpty() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};
	// this->cpFIFO_AR->CheckFIFO();

	// Check MO
	if (this->GetMO_AR() >= MAX_MO_COUNT) {
		return (ERESULT_TYPE_FAIL);
	};

	// Pop
	UPUD upAR_new = this->cpFIFO_AR->Pop();

	#ifdef DEBUG
	if (upAR_new == NULL) {
		assert (0);
		return (ERESULT_TYPE_SUCCESS);
	};
	#endif

	#ifdef DEBUG_MST
	assert (upAR_new != NULL);
	// this->cpFIFO_AR->CheckFIFO();
	printf("[Cycle %3ld: %s.Do_AR_fwd] (%s) popped FIFO_AR.\n", nCycle, this->cName.c_str(), upAR_new->cpAR->GetName().c_str());
	// this->cpFIFO_AR->Display();
	#endif

	// Put Tx
	CPAxPkt cpAR_new = upAR_new->cpAR;

	#if defined(CCI_ON) && defined(CCI_TESTING) // Setting the snoop value.
	// The testing commands skip these number 4, 5, 6, 10, 14, 15
	int SnoopType = nCycle & 0xF;; // Take 4 LSB.
	
	if ((SnoopType == 4) ||
	    (SnoopType == 5) ||	
	    (SnoopType == 6) ||	
	    (SnoopType == 10) ||
		(SnoopType == 14) ||
		(SnoopType == 15)
	) {
		SnoopType = 0;
	}
	cpAR_new->SetSnoop(SnoopType);
	#endif

	this->cpTx_AR->PutAx(cpAR_new);

	#ifdef DEBUG_MST
	printf("[Cycle %3ld: %s.Do_AR_fwd] (%s) put Tx_AR.\n", nCycle, this->cName.c_str(), upAR_new->cpAR->GetName().c_str());
	#endif

	// Maintain
	Delete_UD(upAR_new, EUD_TYPE_AR); // FIXME FIXME Check: upAR_new itself deleted

	return (ERESULT_TYPE_SUCCESS);
};


//-----------------------------------------
// AW valid
//-----------------------------------------
EResultType CMST::Do_AW_fwd(int64_t nCycle) {

	// Check issue interval
	// if ((nCycle+1) % MST_AW_ISSUE_MIN_INTERVAL != 0) { // // Every interval cycles. 5,9,13,...
	if (nCycle % AW_ISSUE_MIN_INTERVAL != 0) { // Every interval cycles. 4,8,12,... 
		  return (ERESULT_TYPE_SUCCESS);  
	};

	// Check Tx valid 
	if (this->cpTx_AW->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};

	// Check FIFO
	if (this->cpFIFO_AW->IsEmpty() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};

	// Check MO
	if (this->GetMO_AW() >= MAX_MO_COUNT) {
		return (ERESULT_TYPE_FAIL);
	};
	// this->cpFIFO_AW->CheckFIFO();

	// Pop
	UPUD upAW_new = this->cpFIFO_AW->Pop();

	if (upAW_new == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	};


	#ifdef DEBUG_MST
	assert (upAW_new != NULL);
	// this->cpFIFO_AW->CheckFIFO();
	// printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) popped FIFO_AW.\n", nCycle, this->cName.c_str(), upAW_new->cpAW->GetName().c_str());
	// this->cpFIFO_AW->Display();
	#endif

	// Put Tx
	CPAxPkt cpAW_new = upAW_new->cpAW;

	#if defined(CCI_ON) && defined(CCI_TESTING) // Setting the snoop value.
	// The testing commands skip these number 4, 5, 6, 10, 14, 15
	int SnoopType = nCycle & 0x7;; // Take 3 LSB.
	
	if (SnoopType > 5) {
		SnoopType = 0;
	}

	cpAW_new->SetSnoop(SnoopType);
	#endif

	this->cpTx_AW->PutAx(cpAW_new);

	#ifdef DEBUG_MST
	printf("[Cycle %3ld: %s.Do_AW_fwd] (%s) put Tx_AW.\n", nCycle, this->cName.c_str(), upAW_new->cpAW->GetName().c_str());
	// upAW_new->cpAW->Display();
	// cpAW_new->CheckPkt();
	#endif

	// Maintain
	Delete_UD(upAW_new, EUD_TYPE_AW);

	return (ERESULT_TYPE_SUCCESS);
};


//------------------------------------------
// W valid
//------------------------------------------
EResultType CMST::Do_W_fwd(int64_t nCycle) {

	// Check Tx valid 
	if (this->cpTx_W->IsBusy() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};

	// Check FIFO
	if (this->cpFIFO_W->IsEmpty() == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_FAIL);
	};
	// this->cpFIFO_W->CheckFIFO();

	// Pop
	UPUD upW_new = this->cpFIFO_W->Pop();
	if (upW_new == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	};

	#ifdef DEBUG_MST
	assert (upW_new != NULL);
	// this->cpFIFO_W->CheckFIFO();
	// printf("[Cycle %3ld: %s.Do_W_fwd] (%s) popped FIFO_W.\n", nCycle, this->cName.c_str(), upW_new->cpW->GetName().c_str());
	// this->cpFIFO_W->Display();
	#endif

	// Put Tx
	CPWPkt cpW_new = upW_new->cpW;
	this->cpTx_W->PutW(cpW_new);

	#ifdef DEBUG_MST
	printf("[Cycle %3ld: %s.Do_W_fwd] (%s) put Tx_W.\n", nCycle, this->cName.c_str(), upW_new->cpW->GetName().c_str());
	// upW_new->cpW->Display()
	// cpW_new->CheckPkt();
	#endif

	// Maintain
	Delete_UD(upW_new, EUD_TYPE_W);

	return (ERESULT_TYPE_SUCCESS);
};


//-----------------------------------------
// AR ready
//-----------------------------------------
EResultType CMST::Do_AR_bwd(int64_t nCycle) {

	// Check Tx valid 
	CPAxPkt cpAR = this->cpTx_AR->GetAx();
	if (cpAR == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Check Tx ready
	EResultType eAcceptResult = this->cpTx_AR->GetAcceptResult();

	if (eAcceptResult == ERESULT_TYPE_ACCEPT) {

		#ifdef DEBUG_MST
		string cARPktName = cpAR->GetName();
		printf("[Cycle %3ld: %s.Do_AR_bwd] %s handshake Tx_AR.\n", nCycle, this->cName.c_str(), cARPktName.c_str());
		// cpAR->Display();
		#endif

		// Stat 
		// this->Increase_MO_AR(); // FIXME: AR transaction accepted but not issued yet. Increase MO_AR when AR issued (Do_AR_fwd)

	};
	return (ERESULT_TYPE_SUCCESS);
};


//-------------------------------------------
// AW ready
//-------------------------------------------
EResultType CMST::Do_AW_bwd(int64_t nCycle) {

	// Check Tx valid 
	CPAxPkt cpAW = this->cpTx_AW->GetAx();

	if (cpAW == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Check Tx ready
	EResultType eAcceptResult = this->cpTx_AW->GetAcceptResult();

	if (eAcceptResult == ERESULT_TYPE_ACCEPT) {

		#ifdef DEBUG_MST
		string cAWPktName = cpAW->GetName();
		 printf("[Cycle %3ld: %s.Do_AW_bwd] %s handshake Tx_AW.\n", nCycle, this->cName.c_str(), cAWPktName.c_str());
		// cpAW->Display();
		#endif

		// Stat 
		// this->Increase_MO_AW();
	};
	return (ERESULT_TYPE_SUCCESS);
};


//-----------------------------------------
// W ready
//-----------------------------------------
EResultType CMST::Do_W_bwd(int64_t nCycle) {

	// Check Tx valid 
	CPWPkt cpW = this->cpTx_W->GetW();
	if (cpW == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	};

	// Check Tx ready
	EResultType eAcceptResult = this->cpTx_W->GetAcceptResult();

	if (eAcceptResult == ERESULT_TYPE_ACCEPT) {
		string cWPktName = cpW->GetName();

		#ifdef DEBUG_MST
		if (cpW->IsLast() == ERESULT_TYPE_YES) {
			printf("[Cycle %3ld: %s.Do_W_bwd] (%s) WLAST handshake Tx_W.\n", nCycle, this->cName.c_str(), cWPktName.c_str());
		} 
		else {
			printf("[Cycle %3ld: %s.Do_W_bwd] (%s) handshake Tx_W.\n", nCycle, this->cName.c_str(), cWPktName.c_str());
		};
		// cpW->Display();
		#endif
	};

	return (ERESULT_TYPE_SUCCESS);
};


//------------------------------------------
// R Valid
//------------------------------------------
EResultType CMST::Do_R_fwd(int64_t nCycle) {
	// Check Rx valid 
	// if (this->cpRx_R->IsBusy() == ERESULT_TYPE_YES) {
	//	return (ERESULT_TYPE_SUCCESS);
	// };

	// Check remote-Tx valid
	CPRPkt cpR = this->cpRx_R->GetPair()->GetR();

	if (cpR == NULL) {
			return (ERESULT_TYPE_SUCCESS);
	};

	// Debug
	// cpR->CheckPkt();
	string cPktName = cpR->GetName();

	// Put Rx
	this->cpRx_R->PutR(cpR);

	#ifdef DEBUG_MST
	printf("[Cycle %3ld: %s.Do_R_fwd] (%s) put Rx_R.\n", nCycle, this->cName.c_str(), cPktName.c_str());
	// cpR->Display();
	#endif
	
	return (ERESULT_TYPE_SUCCESS);
};


//-------------------------------------------
// B Valid
//-------------------------------------------
EResultType CMST::Do_B_fwd(int64_t nCycle) {

		// Check Rx valid 
		// if (this->cpRx_B->IsBusy() == ERESULT_TYPE_YES) {
	//	return (ERESULT_TYPE_SUCCESS);
		// };

		// Check remote-Tx valid
		CPBPkt cpB = this->cpRx_B->GetPair()->GetB();
		if (cpB == NULL) {
				return (ERESULT_TYPE_SUCCESS);
		};

	// Debug
	// cpB->CheckPkt();
	string cPktName = cpB->GetName();

	// Put Rx 
	this->cpRx_B->PutB(cpB);

	#ifdef DEBUG_MST
	// printf("[Cycle %3ld: %s.Do_B_fwd] (%s) Put Rx_B.\n", nCycle, this->cName.c_str(), cPktName.c_str());
	// cpB->Display();
	#endif
		
		return (ERESULT_TYPE_SUCCESS);
};


//------------------------------------------
// R ready
//------------------------------------------
EResultType CMST::Do_R_bwd(int64_t nCycle) {

	// Check remote-Tx valid
	// CPRPkt cpR = this->cpRx_R->GetPair()->GetR();
	// if (cpR == NULL) {
	//	return (ERESULT_TYPE_SUCCESS);
	//};

	// Check Rx valid
	CPRPkt cpR = this->cpRx_R->GetR();
	if (cpR == NULL) {
		//printf("cpR is NULL\n");
		return (ERESULT_TYPE_SUCCESS);
	};

		// Set ready
	this->cpRx_R->SetAcceptResult(ERESULT_TYPE_ACCEPT);

	string cRPktName = cpR->GetName();
	// Debug 
	// cpR->CheckPkt();
	// cpR->Display();

	// Stat
	this->nR++;

	if (cpR->IsLast() == ERESULT_TYPE_YES) {

		#ifdef DEBUG_MST
		printf("[Cycle %3ld: %s.Do_R_bwd] (%s) RLAST handshake Rx_R.\n", nCycle, this->cName.c_str(), cRPktName.c_str());
		#endif

		// Stat  
		// this->Decrease_MO_AR();	

		this->nAllTransFinished++;
		this->nARTransFinished++;
	
		#ifdef AR_ADDR_GEN_TEST
		if (this->nARTransFinished == this->nAR_GEN_NUM) {
			this->SetARTransFinished(ERESULT_TYPE_YES);
			this->Set_nCycle_AR_Finished(nCycle);
			#ifdef DEBUG_MST
			printf("[Cycle %3ld: %s.Do_R_bwd] All read transactions finished Rx_R.\n", nCycle, this->cName.c_str());
			#endif
		};

		if (this->nAllTransFinished == this->nAR_GEN_NUM + this->nAW_GEN_NUM) {
			this->SetAllTransFinished(ERESULT_TYPE_YES);
			#ifdef DEBUG_MST
			printf("[Cycle %3ld: %s.Do_R_bwd] All transactions finished Rx_R.\n", nCycle, this->cName.c_str());
			#endif
		};
		#endif

		// Check sim finish	
		// if (this->nARTransFinished == this->nAR_GEN_NUM) {	// Debug 
		if (cpR->IsFinalTrans() == ERESULT_TYPE_YES) {
			this->SetARTransFinished(ERESULT_TYPE_YES);
			this->Set_nCycle_AR_Finished(nCycle);
			#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.Do_R_bwd] All read transactions finished Rx_R.\n", nCycle, this->cName.c_str());
			 #endif
		};
		
		// if (this->nAllTransFinished >= (this->nAR_GEN_NUM + this->nAW_GEN_NUM)) {	// FIXME Debug 
		if (this->IsARTransFinished() == ERESULT_TYPE_YES and this->IsAWTransFinished() == ERESULT_TYPE_YES) {	// FIXME
				this->SetAllTransFinished(ERESULT_TYPE_YES);
				#ifdef DEBUG_MST
				printf("[Cycle %3ld: %s.Do_R_bwd] All transactions finished Rx_R.\n", nCycle, this->cName.c_str());
				#endif
		};

		//DUONGTRAN add for Matrix operation
		//printf("State_pre: %d\n", this->nStateAddr);
		//printf("State_cur: %d\n", this->nStateAddr_next);
		this->nStateAddr = this->nStateAddr_next;

	} 
	else {
		#ifdef DEBUG_MST
		printf("[Cycle %3ld: %s.Do_R_bwd] (%s) handshake Rx_R.\n", nCycle, this->cName.c_str(), cRPktName.c_str());
		#endif
	};
	
		return (ERESULT_TYPE_SUCCESS);
};


//------------------------------------------
// B ready
//------------------------------------------
EResultType CMST::Do_B_bwd(int64_t nCycle) {

	// Check remote-Tx valid
	// CPBPkt cpB = this->cpRx_B->GetPair()->GetB();
	// if (cpB == NULL) {
	//	return (ERESULT_TYPE_SUCCESS);
	// };
	
	// Check Rx valid 
	CPBPkt cpB = this->cpRx_B->GetB();
	if (cpB == NULL) {
		return (ERESULT_TYPE_SUCCESS);
	};
	
	// Set ready
	this->cpRx_B->SetAcceptResult(ERESULT_TYPE_ACCEPT);

	string cBPktName = cpB->GetName();

	#ifdef DEBUG_MST
		printf("[Cycle %3ld: %s.Do_B_bwd] (%s) handshake Rx_B.\n", nCycle, this->cName.c_str(), cBPktName.c_str());
	// cpB->CheckPkt();
	// cpB->Display();
	#endif
	
	// Stat 
	//this->Decrease_MO_AW();	
	this->nB++;
	
	// Count finished transactions	
	this->nAllTransFinished++;
	this->nAWTransFinished++;

	#ifdef AW_ADDR_GEN_TEST
	if (this->nAWTransFinished == this->nAW_GEN_NUM) {
			this->SetAWTransFinished(ERESULT_TYPE_YES);
		this->Set_nCycle_AW_Finished(nCycle);	
			#ifdef DEBUG_MST
			printf("[Cycle %3ld: %s.Do_B_bwd] All write transactions finished Rx_R.\n", nCycle, this->cName.c_str());
			#endif
	};

	if (this->nAllTransFinished == this->nAR_GEN_NUM + this->nAW_GEN_NUM) {
			this->SetAllTransFinished(ERESULT_TYPE_YES);
			#ifdef DEBUG_MST
			printf("[Cycle %3ld: %s.Do_B_bwd] All transactions finished Rx_R.\n", nCycle, this->cName.c_str());
			#endif
	};
	#endif

	// Check sim finish	
	// if (this->nAWTransFinished == this->nAW_GEN_NUM) { // Debug 
	if (cpB->IsFinalTrans() == ERESULT_TYPE_YES) {
		this->SetAWTransFinished(ERESULT_TYPE_YES);	
		this->Set_nCycle_AW_Finished(nCycle);	
		#ifdef DEBUG_MST
		printf("[Cycle %3ld: %s.Do_B_bwd] All write transactions finished Rx_B.\n", nCycle, this->cName.c_str());
		#endif
	};
	
	// if (this->nAllTransFinished >= (this->nAR_GEN_NUM + this->nAW_GEN_NUM)) {	// FIXME Debug 
	if (this->IsARTransFinished() == ERESULT_TYPE_YES and this->IsAWTransFinished() == ERESULT_TYPE_YES) {	// FIXME
		this->SetAllTransFinished(ERESULT_TYPE_YES);	
		#ifdef DEBUG_MST
		printf("[Cycle %3ld: %s.Do_B_bwd] All transactions finished Rx_B.\n", nCycle, this->cName.c_str());
		#endif
	};

	//DUONGTRAN add for Matrix operation
	
	this->nStateAddr = this->nStateAddr_next;

	return (ERESULT_TYPE_SUCCESS);
};

#ifdef CCI_ON
	// Purpose: Getting the transaction from remote ports and put it into local ports.
	EResultType	CMST::Do_AC_fwd(int64_t nCycle) {

		if (this->cpTx_CR->IsBusy() == ERESULT_TYPE_YES) {
			printf("[Cycle %3ld: %s.Do_AC_fwd] cpTx_CR is busy.\n", nCycle, this->cName.c_str());
			return (ERESULT_TYPE_SUCCESS); // Cannot response through CR channel.
		}

		if (simDataTransfer && (this->cpTx_CD->IsBusy() == ERESULT_TYPE_YES)) {
			printf("[Cycle %3ld: %s.Do_AC_fwd] cpTx_CD is busy.\n", nCycle, this->cName.c_str());
			return (ERESULT_TYPE_SUCCESS); // Cannot response through CD channel.
		}

		if (simDataTransfer && ((simCDlen != MAX_BURST_LENGTH) && (simCDlen > 0))) {
			printf("[Cycle %3ld: %s.Do_AC_fwd] CDLen = %d.\n", nCycle, this->cName.c_str(), simCDlen);
			simCDlen++;
			return (ERESULT_TYPE_SUCCESS); // Do not issuing enough data.
		}

		simCDlen = 0; // restarting counting.

		// 1. Get the snoop transactions from AC port.
		// Check remote-Tx valid
		CPACPkt cpAC = this->cpRx_AC->GetPair()->GetAC();

		if (cpAC == NULL) {
			return (ERESULT_TYPE_SUCCESS);
		};

		// Put Rx
		this->cpRx_AC->PutAC(cpAC);
	
		#ifdef DEBUG_MST
		string cPktName = cpAC->GetName();
		printf("[Cycle %3ld: %s.Do_AC_fwd] (%s) put Rx_AC.\n", nCycle, this->cName.c_str(), cPktName.c_str());
		// cpR->Display();
		#endif
	
		return (ERESULT_TYPE_SUCCESS);
	};

	// Purpose: Set the acceptance results of the local port.
	EResultType	CMST::Do_AC_bwd(int64_t nCycle) {
		
		if (this->cpRx_AC->IsBusy() == ERESULT_TYPE_NO) {
			return (ERESULT_TYPE_SUCCESS); // Cannot response through CR channel.
		}

		CPACPkt cpAC = this->cpRx_AC->GetAC();

		if (cpAC == NULL) {
			//printf("[Cycle %3ld: %s.Do_AC_bwd] cpAC is NULL.\n", nCycle, this->cName.c_str());
			return (ERESULT_TYPE_SUCCESS);
		};

		string cACPktName = cpAC->GetName();
		
		// Set ready
		this->cpRx_AC->FlushPkt();
		this->cpRx_AC->SetAcceptResult(ERESULT_TYPE_ACCEPT);

		#ifdef DEBUG_MST
		printf("[Cycle %3ld: %s.Do_AC_bwd] %s handshake Rx_AC.\n", nCycle, this->cName.c_str(), cACPktName.c_str());
		#endif

		return (ERESULT_TYPE_SUCCESS);
	}

	// Purpose: Decode and issue the CR transactions to TX_CR port.
	EResultType	CMST::Do_CR_fwd(int64_t nCycle){
		bool passDirty = false;
		// Check Rx valid
		CPACPkt cpAC = this->cpRx_AC->GetAC();

		if (cpAC == NULL) {
			return (ERESULT_TYPE_SUCCESS);
		};

		#ifdef CCI_TESTING
			if ((nCycle % 500) == 0) { // For testing only
				simDataTransfer = !simDataTransfer; // Flip the data transfer.
			}

			if ((cpAC->GetSnoop() == 0b1001) ||
				(cpAC->GetSnoop() == 0b1000)
			) {
				if (simDataTransfer) passDirty = true;
			}

			if ((cpAC->GetSnoop() == 0b0010) ||
				(cpAC->GetSnoop() == 0b0011)
			) {
				simDataTransfer = false;
			}

		#endif

		int rresponse = 0 + (passDirty << 2) + simDataTransfer; // FIXME: Response decode shoulds be determine by the MASTER not a fixed value.

		if (this->cpTx_CR->IsBusy() == ERESULT_TYPE_YES) {
			return (ERESULT_TYPE_SUCCESS);
		}

		// Debug
		// cpR->CheckPkt();
		string cPktName = cpAC->GetName();
		char cCRPktName[50];

		// Put Rx
		// 2. Handle the Snoop (Snoop should not be blocked by any transactions).
		CPCRPkt cpCR_new = new CCRPkt();

		sprintf(cCRPktName, "CR_%s", cPktName.c_str());
		cpCR_new->SetName(cCRPktName);
		cpCR_new->SetResp(rresponse);

		this->cpTx_CR->PutCR(cpCR_new);

		#ifdef DEBUG_MST
		printf("[Cycle %3ld: %s.Do_CR_fwd] %s/%s handshake Tx_CR: Snoop = %x, DataTransfer = %x, passDirty = %x.\n", nCycle, this->cName.c_str(), cCRPktName, cpAC->GetName().c_str(), cpAC->GetSnoop(), simDataTransfer, passDirty);
		#endif

		return (ERESULT_TYPE_SUCCESS);
	};

	// Purpose: Print the handsake
	EResultType	CMST::Do_CR_bwd(int64_t nCycle){
		// Check Tx valid 
		CPCRPkt cpCR = this->cpTx_CR->GetCR();

		if (cpCR == NULL) {
			return (ERESULT_TYPE_SUCCESS);
		};

		printf("[Cycle %3ld: %s.Do_CR_bwd] (%s) 1. handshake cpTx_CR.\n", nCycle, this->cName.c_str(), cpCR->GetName().c_str());

		// Check Tx ready
		EResultType eAcceptResult = this->cpTx_CR->GetAcceptResult();

		if (eAcceptResult == ERESULT_TYPE_ACCEPT) {
			string cCRPktName = cpCR->GetName();

			#ifdef DEBUG_MST
			printf("[Cycle %3ld: %s.Do_CR_bwd] (%s) 2. handshake cpTx_CR.\n", nCycle, this->cName.c_str(), cCRPktName.c_str());
			#endif
		};

		return (ERESULT_TYPE_SUCCESS);
	};

	// Purpose: Decode and issue the CR transactions to TX_CD port.
	EResultType	CMST::Do_CD_fwd(int64_t nCycle){

		// Check Rx valid
		CPACPkt cpAC = this->cpRx_AC->GetAC();

		if (cpAC == NULL) {
			return (ERESULT_TYPE_SUCCESS);
		};

		if (this->cpTx_CD->IsBusy() == ERESULT_TYPE_YES) {
			return (ERESULT_TYPE_SUCCESS);
		}

		if (!simDataTransfer) {
			return (ERESULT_TYPE_SUCCESS);
		}

		// Debug
		// cpR->CheckPkt();
		string cPktName = cpAC->GetName();
		char cCDPktName[50];

		// Put Rx
		// 2. Handle the Snoop (Snoop should not be blocked by any transactions).
		CPCDPkt cpCD_new = new CCDPkt();

		sprintf(cCDPktName, "CD_%s", cPktName.c_str());
		cpCD_new->SetName(cCDPktName);
		cpCD_new->SetData(0);

		if (simCDlen == MAX_BURST_LENGTH) {
			cpCD_new->SetLast(ERESULT_TYPE_YES);
		} else {
			cpCD_new->SetLast(ERESULT_TYPE_NO);
		}

		this->cpTx_CD->PutCD(cpCD_new);

		#ifdef DEBUG_MST
		printf("[Cycle %3ld: %s.Do_CD_fwd] %s handshake Tx_CD - IsLast  = %d.\n", nCycle, this->cName.c_str(), cCDPktName, (cpCD_new->IsLast() == ERESULT_TYPE_YES));
		#endif

		return (ERESULT_TYPE_SUCCESS);
	};

	// Purpose: Print the handsake
	EResultType	CMST::Do_CD_bwd(int64_t nCycle){
		// Check Tx valid 
		CPCDPkt cpCD = this->cpTx_CD->GetCD();
		
		if (cpCD == NULL) {
			return (ERESULT_TYPE_SUCCESS);
		};

		// Check Tx ready
		EResultType eAcceptResult = this->cpTx_CD->GetAcceptResult();

		if (eAcceptResult == ERESULT_TYPE_ACCEPT) {
			string cCDPktName = cpCD->GetName();

			#ifdef DEBUG_MST
			if (cpCD->IsLast() == ERESULT_TYPE_YES) {
				printf("[Cycle %3ld: %s.Do_CD_bwd] (%s) WLAST handshake cpTx_CD.\n", nCycle, this->cName.c_str(), cCDPktName.c_str());
			} 
			else {
				printf("[Cycle %3ld: %s.Do_CD_bwd] (%s) handshake cpTx_CD.\n", nCycle, this->cName.c_str(), cCDPktName.c_str());
			};
			// cpW->Display();
			#endif
		};

		return (ERESULT_TYPE_SUCCESS);
	};
#endif


// Debug
EResultType CMST::CheckLink() {

	assert (this->cpTx_AR != NULL);
	assert (this->cpTx_AW != NULL);
	assert (this->cpTx_W  != NULL);
	assert (this->cpRx_R  != NULL);
	assert (this->cpRx_B  != NULL);
	#ifdef CCI_ON
		assert (this->cpRx_AC  != NULL);
		assert (this->cpTx_CR  != NULL);
		assert (this->cpTx_CD  != NULL);
	#endif
	assert (this->cpTx_AR->GetPair() != NULL);
	assert (this->cpTx_AW->GetPair() != NULL);
	assert (this->cpTx_W->GetPair()  != NULL);
	assert (this->cpRx_R ->GetPair() != NULL);
	assert (this->cpRx_B ->GetPair() != NULL);
	assert (this->cpTx_AR->GetTRxType() != this->cpTx_AR->GetPair()->GetTRxType());
	assert (this->cpTx_AW->GetTRxType() != this->cpTx_AW->GetPair()->GetTRxType());
	assert (this->cpTx_W ->GetTRxType() != this->cpTx_W ->GetPair()->GetTRxType());
	assert (this->cpRx_R ->GetTRxType() != this->cpRx_R ->GetPair()->GetTRxType());
	assert (this->cpRx_B ->GetTRxType() != this->cpRx_B ->GetPair()->GetTRxType());
	assert (this->cpTx_AR->GetPktType() == this->cpTx_AR->GetPair()->GetPktType());
	assert (this->cpTx_AW->GetPktType() == this->cpTx_AW->GetPair()->GetPktType());
	assert (this->cpTx_W ->GetPktType() == this->cpTx_W ->GetPair()->GetPktType());
	assert (this->cpRx_R ->GetPktType() == this->cpRx_R ->GetPair()->GetPktType());
	assert (this->cpRx_B ->GetPktType() == this->cpRx_B ->GetPair()->GetPktType());
	assert (this->cpTx_AR->GetPair()->GetPair()== this->cpTx_AR);
	assert (this->cpTx_AW->GetPair()->GetPair()== this->cpTx_AW);
	assert (this->cpTx_W ->GetPair()->GetPair()== this->cpTx_W);
	assert (this->cpRx_R ->GetPair()->GetPair()== this->cpRx_R);
	assert (this->cpRx_B ->GetPair()->GetPair()== this->cpRx_B);

		return (ERESULT_TYPE_SUCCESS);
};


// Set all transactions finished
EResultType CMST::SetAllTransFinished(EResultType eResult) {  
	this->eAllTransFinished = eResult;
		return (ERESULT_TYPE_SUCCESS);
};


// Set AR transactions finished
EResultType CMST::SetARTransFinished(EResultType eResult) {  
	this->eARTransFinished = eResult;
		return (ERESULT_TYPE_SUCCESS);
};


// Set AW transactions finished
EResultType CMST::SetAWTransFinished(EResultType eResult) {  

	this->eAWTransFinished = eResult;
		return (ERESULT_TYPE_SUCCESS);
};


// Check all transactions finished
EResultType CMST::IsAllTransFinished() {  

	if (this->eAllTransFinished == ERESULT_TYPE_YES) {
			return (ERESULT_TYPE_YES);
	};
		return (ERESULT_TYPE_NO);
};


// Check AR transactions finished
EResultType CMST::IsARTransFinished() {  
	if (this->eARTransFinished == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_YES);
	};
		return (ERESULT_TYPE_NO);
};


// Check AW transactions finished
EResultType CMST::IsAWTransFinished() {  
	if (this->eAWTransFinished == ERESULT_TYPE_YES) {
		return (ERESULT_TYPE_YES);
	};
		return (ERESULT_TYPE_NO);
};


// Set cycle AR finished 
EResultType CMST::Set_nCycle_AR_Finished(int64_t nCycle) {  

	this->nCycle_AR_Finished = nCycle;
	   	return (ERESULT_TYPE_YES);
};


// Set cycle AW finished 
EResultType CMST::Set_nCycle_AW_Finished(int64_t nCycle) {  

	this->nCycle_AW_Finished = nCycle;
	   	return (ERESULT_TYPE_YES);
};


// Set number total AR 
EResultType CMST::Set_nAR_GEN_NUM(int nNum) {  

	this->nAR_GEN_NUM = nNum;
	this->cpAddrGen_AR->SetNumTotalTrans(nNum);
	   	return (ERESULT_TYPE_YES);
};


// Set number total AW 
EResultType CMST::Set_nAW_GEN_NUM(int nNum) {  

	this->nAW_GEN_NUM = nNum;
	this->cpAddrGen_AW->SetNumTotalTrans(nNum);
	   	return (ERESULT_TYPE_YES);
};


// Set image horizontal size scaling factor 
EResultType CMST::Set_ScalingFactor(float Num) {

	this->ScalingFactor = Num;
	this->cpAddrGen_AR->Set_ScalingFactor(Num);
	   	return (ERESULT_TYPE_YES);
};


// Set issue rate 
EResultType CMST::Set_AR_ISSUE_MIN_INTERVAL(int nNum) {

	this->AR_ISSUE_MIN_INTERVAL = nNum;
	   	return (ERESULT_TYPE_YES);
};


// Set issue rate 
EResultType CMST::Set_AW_ISSUE_MIN_INTERVAL(int nNum) {

	this->AW_ISSUE_MIN_INTERVAL = nNum;
	   	return (ERESULT_TYPE_YES);
};


// Set start address AR 
EResultType CMST::Set_nAR_START_ADDR(int64_t nAddr) {  

	this->nAR_START_ADDR = nAddr;  // LoadTransfer_AR_Test()
	this->nStartAddrA = nAddr;	   //DUONGTRAN add for Matrix operation
	this->cpAddrGen_AR->SetStartAddr(nAddr);
	return (ERESULT_TYPE_YES);
};

// Set start address AR2
EResultType CMST::Set_nAR_START_ADDR2(int64_t nAddr) {  
	this->nStartAddrB = nAddr;
	return (ERESULT_TYPE_YES);
};


// Set start address AW 
EResultType CMST::Set_nAW_START_ADDR(int64_t nAddr) {  

	this->nAW_START_ADDR = nAddr;  // LoadTransfer_AW_Test()
	this->nStartAddrC = nAddr;	 //DUONGTRAN add for Matrix operation
	this->cpAddrGen_AW->SetStartAddr(nAddr);
	   	return (ERESULT_TYPE_YES);
};

// Set start address AW2
EResultType CMST::Set_nAW_START_ADDR2(int64_t nAddr) {  
	this->nStartAddrD = nAddr;	 //DUONGTRAN add for Matrix operation
	return (ERESULT_TYPE_YES);
};

// Set start address AW3
EResultType CMST::Set_nAW_START_ADDR3(int64_t nAddr) {  
	this->nStartAddrE = nAddr;	 //DUONGTRAN add for Matrix operation
	return (ERESULT_TYPE_YES);
};

// Set address map AR 
EResultType CMST::Set_AR_AddrMap(string cAddrMap) {

	this->cpAddrGen_AR->SetAddrMap(cAddrMap);
	   	return (ERESULT_TYPE_YES);
};


// Set address map AW 
EResultType CMST::Set_AW_AddrMap(string cAddrMap) {

	this->cpAddrGen_AW->SetAddrMap(cAddrMap);
	   	return (ERESULT_TYPE_YES);
};


// Set operation AR 
EResultType CMST::Set_AR_Operation(string cOperation) {

	this->cpAddrGen_AR->SetOperation(cOperation);
	   	return (ERESULT_TYPE_YES);
};


// Set operation AW 
EResultType CMST::Set_AW_Operation(string cOperation) {

	this->cpAddrGen_AW->SetOperation(cOperation);
	   	return (ERESULT_TYPE_YES);
};


// Get cycle AR finished 
int64_t CMST::Get_nCycle_AR_Finished() {  

	return (this->nCycle_AR_Finished);
};


// Get cycle AW finished 
int64_t CMST::Get_nCycle_AW_Finished() {  

	return (this->nCycle_AW_Finished);
};


// Increase AR MO count 
EResultType CMST::Increase_MO_AR() {  

	this->nMO_AR++;
	printf("Increase_MO_AR: nMO_AR = %d.\n", nMO_AR);
	return (ERESULT_TYPE_SUCCESS);
};


// Decrease AR MO count 
EResultType CMST::Decrease_MO_AR() {  

	this->nMO_AR--;
	printf("Decrease_MO_AR: nMO_AR = %d.\n", nMO_AR);

	#ifdef DEBUG
	assert (this->nMO_AR >= 0);
	#endif

		return (ERESULT_TYPE_SUCCESS);
};


// Increase AW MO count 
EResultType CMST::Increase_MO_AW() {  

	this->nMO_AW++;
	printf("Increase_MO_AW: nMO_AW = %d.\n", nMO_AW);
	return (ERESULT_TYPE_SUCCESS);
};


// Decrease AW MO count 
EResultType CMST::Decrease_MO_AW() {  

	this->nMO_AW--;
	printf("Decrease_MO_AW: nMO_AW = %d.\n", nMO_AW);
	#ifdef DEBUG
	assert (this->nMO_AW >= 0);
	#endif

	return (ERESULT_TYPE_SUCCESS);
};


// Get AR MO count
int CMST::GetMO_AR() {

	#ifdef DEBUG
	assert (this->nMO_AR >= 0);
	#endif

	return (this->nMO_AR);
};


// Get AW MO count
int CMST::GetMO_AW() {

	#ifdef DEBUG
	assert (this->nMO_AW >= 0);
	#endif

	return (this->nMO_AW);
};


// Update state
EResultType CMST::UpdateState(int64_t nCycle) {  
	
	this->cpTx_AR   ->UpdateState();
	this->cpTx_AW   ->UpdateState();
	// this->cpTx_W ->UpdateState();
	this->cpRx_R	->UpdateState();
	this->cpRx_B	->UpdateState();

	#ifdef CCI_ON
		this->cpRx_AC	->UpdateState();
		this->cpTx_CR	->UpdateState();
		this->cpTx_CD	->UpdateState();
	#endif

	#if !defined MATRIX_MULTIPLICATION && !defined MATRIX_MULTIPLICATIONKIJ && !defined MATRIX_TRANSPOSE && !defined MATRIX_CONVOLUTION && !defined MATRIX_SOBEL
		this->cpAddrGen_AR->UpdateState();
		this->cpAddrGen_AW->UpdateState();
	#endif
	
	// this->cpFIFO_AR->UpdateState(); // nLatency 0
	// this->cpFIFO_AW->UpdateState();
	// this->cpFIFO_W ->UpdateState();

		return (ERESULT_TYPE_SUCCESS);
}; 


// Stat 
EResultType CMST::PrintStat(int64_t nCycle, FILE *fp) {
	
	printf("--------------------------------------------------------\n");
	printf("\t Name: %s\n",	this->cName.c_str());
	printf("--------------------------------------------------------\n");
	printf("\t Number AR					  : %d\n",	this->nARTrans);
	printf("\t Number AW					  : %d\n",	this->nAWTrans);
	printf("\t Number R					   : %d\n",	this->nR);
	printf("\t Number B					   : %d\n",	this->nB);

	// printf("\t Max allowed AR MO		   : %d\n",	MAX_MO_COUNT);
	// printf("\t Max allowed AW MO		   : %d\n\n",	MAX_MO_COUNT);

	printf("\t Cycle to finish read		   : %ld\n",	this->nCycle_AR_Finished);
	printf("\t Cycle to finish write		  : %ld\n\n",	this->nCycle_AW_Finished);

	#ifdef METRIC_ANALYSIS_ENABLE
	printf("\t (%s) Avg tile bank metric AR   : %1.2f\n",	this->cName.c_str(), (float)(this->cpAddrGen_AR->GetTileBankMetric())   / (IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE / (TILEH * TILEV) ));
	printf("\t (%s) Avg tile bank metric AW   : %1.2f\n\n",	this->cName.c_str(), (float)(this->cpAddrGen_AW->GetTileBankMetric())   / (IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE / (TILEH * TILEV) ));

	printf("\t (%s) Avg linear bank metric AR : %1.2f\n",	this->cName.c_str(), (float)(this->cpAddrGen_AR->GetLinearBankMetric()) / (IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE / PIXELS_PER_TRANS ));
	printf("\t (%s) Avg linear bank metric AW : %1.2f\n\n",	this->cName.c_str(), (float)(this->cpAddrGen_AW->GetLinearBankMetric()) / (IMG_HORIZONTAL_SIZE * IMG_VERTICAL_SIZE / PIXELS_PER_TRANS ));
	#endif

	// printf("\t Address map AR			  : %s\n",	this->cpAddrGen_AR->GetAddrMap().c_str());
	// printf("\t Address map AW			  : %s\n\n",	this->cpAddrGen_AW->GetAddrMap().c_str());

	printf("\t Operation AR				   : %s\n",	this->cpAddrGen_AR->GetOperation().c_str());
	printf("\t Operation AW				   : %s\n\n",	this->cpAddrGen_AW->GetOperation().c_str());
	
	printf("\t Utilization R channel		  : %1.2f\n",	(float)(this->nR)/nCycle_AR_Finished);
	printf("\t Utilization W channel		  : %1.2f\n\n", (float)(MAX_BURST_LENGTH * this->nAWTrans)/nCycle_AW_Finished);
	
	// printf("--------------------------------------------------------\n");

	// Debug
	assert (this->nMO_AR == 0);
	assert (this->nMO_AW == 0);
	assert (this->nARTransFinished  == this->nARTrans);
	assert (this->nAWTransFinished  == this->nAWTrans);
	assert (this->nAllTransFinished == this->nARTransFinished + this->nAWTransFinished);
	assert (this->nAllTransFinished == this->nARTrans + this->nAWTrans);

	if (this->nAR_GEN_NUM > 0) {
		assert (this->IsARTransFinished()  == ERESULT_TYPE_YES);
		// assert (this->IsAllTransFinished() == ERESULT_TYPE_YES);	// FIXME
	};
	if (this->nAW_GEN_NUM > 0) {
		assert (this->IsAWTransFinished()  == ERESULT_TYPE_YES);
		// assert (this->IsAllTransFinished() == ERESULT_TYPE_YES);	// FIXME
	};

	assert (this->cpFIFO_AR->GetCurNum() == 0);
	assert (this->cpFIFO_AW->GetCurNum() == 0);
	assert (this->cpFIFO_W->GetCurNum()  == 0);


	//--------------------------------------------------------------	
	// FILE out
	//--------------------------------------------------------------	
	#ifdef FILE_OUT
	fprintf(fp, "--------------------------------------------------------\n");
	fprintf(fp, "\t Name : %s\n",				 this->cName.c_str());
	fprintf(fp, "--------------------------------------------------------\n");
	fprintf(fp, "\t Number AR				   : %d\n",	 this->nARTrans);
	fprintf(fp, "\t Number AW				   : %d\n",	 this->nAWTrans);

	fprintf(fp, "\t Max allowed AR MO		   : %d\n",	 MAX_MO_COUNT);
	fprintf(fp, "\t Max allowed AW MO		   : %d\n\n",	 MAX_MO_COUNT);

	fprintf(fp, "\t Cycle to finish read		: %ld\n",	 this->nCycle_AR_Finished);
	fprintf(fp, "\t Cycle to finish write	   : %ld\n",	 this->nCycle_AW_Finished);
	
	// fprintf(fp, "\t Utilization R channel	: %1.2f\n",	 (float)(this->nR)/nCycle);
	// fprintf(fp, "\t Utilization W channel	: %1.2f\n\n",(float)(this->nW)/nCycle);
	
	// fprintf(fp, "--------------------------------------------------------\n");
	#endif

	return (ERESULT_TYPE_SUCCESS);
};

