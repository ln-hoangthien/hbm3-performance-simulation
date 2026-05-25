# How to compile
# make <COALESCING> <SCENARIO> <BLOCKSIZE_INIT> <IMG_HORIZONTAL_SIZE> <IMG_VERTICAL_SIZE> <MAINFILE> <NUM_TOTAL_PAGES> <TILE_SIZE> <SEED_INIT>
##############################################################################

COALESCING 			= $(word 1, $(MAKECMDGOALS))

SCENARIO_NO_ := $(subst _,,$(SCENARIO))
MAINFILE_NOCPP := $(subst .cpp,,$(MAINFILE))
MAINFILE_NOCPP := $(subst main,,$(MAINFILE_NOCPP))

MY_RUN = $(COALESCING)_$(MAINFILE_NOCPP)_$(SCENARIO_NO_)_$(AR)_$(AW)_$(BANK_NUM)_$(BLOCKSIZE_INIT)_$(IMG_HORIZONTAL_SIZE)_$(IMG_VERTICAL_SIZE)_$(TILE_SIZE)

# compile BSS BCT
BBS_BCT:
	g++ -g -Wall -fno-stack-protector -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"SINGLE_FETCH" -D"BBS" -D"MAX_ORDER=10" -D"BCT_ENABLE" -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile BBS with AT
BBS_ANCH:
	g++ -g -Wall -fno-stack-protector -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"SINGLE_FETCH" -D"BBS" -D"MAX_ORDER=10" -D"AT_ENABLE" -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)" -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile BBS with PCAD
BBS_PCAD:
	g++ -g -Wall -fno-stack-protector -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)"  -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"SINGLE_FETCH" -D"BBS" -D"MAX_ORDER=10" -D"PCAD"  -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile BSS Tradition
BBS_TRAD:
	g++ -O3 -Wall -march=native -fno-stack-protector -DDEBUG $(CCI_FLAGS) -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"BANK_FLIP_PLUS" -D"SINGLE_FETCH" -D"BBS" -D"MAX_ORDER=10" -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CACPkt.cpp ./CCDPkt.cpp ./CCRPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile BSS Tradition
BBS_TRAD_DEBUG:
	g++ -g -Wall -march=native -fno-stack-protector -DDEBUG -DDEBUG_SLV -DDEBUG_BUS -DDEBUG_MST $(CCI_FLAGS) -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"BANK_FLIP_PLUS" -D"SINGLE_FETCH" -D"BBS" -D"MAX_ORDER=10" -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CACPkt.cpp ./CCDPkt.cpp ./CCRPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile BSS Tradition
BBS_TRAD_SELECTED:
	g++ -O3 -Wall -march=native -fno-stack-protector -DAUTO_BGFAM_ENABLE -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"BANK_FLIP_PLUS" -D"SINGLE_FETCH" -D"BBS" -D"MAX_ORDER=10" -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CACPkt.cpp ./CCDPkt.cpp ./CCRPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile BSS Tradition
BBS_TRAD_w_REFRESH:
	g++ -O3 -Wall -march=native -fno-stack-protector -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"BANK_FLIP_PLUS" -D"REFRESH" -D"PRECHARGE" -D"SINGLE_FETCH" -D"BBS" -D"MAX_ORDER=10" -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CACPkt.cpp ./CCDPkt.cpp ./CCRPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile BSS Tradition
BBS_TRAD_BANK_SHUFFLE_PLUS:
	g++ -O3 -Wall -march=native -fno-stack-protector -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"BANK_SHUFFLE_PLUS" -D"SINGLE_FETCH" -D"BBS" -D"MAX_ORDER=10" -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile BSS Tradition
BBS_TRAD_BANK_SHUFFLE_MINUS:
	g++ -O3 -Wall -march=native -fno-stack-protector -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"BANK_SHUFFLE_MINUS" -D"SINGLE_FETCH" -D"BBS" -D"MAX_ORDER=10" -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile BSS NAPOT
BBS_NAPOT:
	g++ -g -Wall -fno-stack-protector -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"SINGLE_FETCH" -D"NAPOT" -D"BBS" -D"MAX_ORDER=10" -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile BSS CAMB
BBS_CAMB:
	g++ -g -Wall -fno-stack-protector -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"QUARTER_FETCH" -D"CAMB_ENABLE" -D"BBS" -D"MAX_ORDER=10" -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile BSS RCPT
BBS_RCPT:
	g++ -g -Wall -fno-stack-protector -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"QUARTER_FETCH" -D"RCPT_ENABLE" -D"BBS" -D"MAX_ORDER=10" -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile BSS cRCPT
BBS_cRCPT:
	g++ -g -Wall -fno-stack-protector -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"QUARTER_FETCH" -D"cRCPT_ENABLE" -D"BBS" -D"MAX_ORDER=10" -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile RBS with PCAD
RBS_PCAD:
	g++ -g -Wall -fno-stack-protector -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"SINGLE_FETCH" -D"RBS" -D"MAX_ORDER=10" -D"PCAD"  -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# compile PBS with PCAD
PBS_PCAD:
	g++ -g -Wall -fno-stack-protector -DDEBUG -D"$(AR)" -D"$(AW)" -D"BANK_NUM=$(BANK_NUM)" -D"SEED_INIT=$(SEED_INIT)" -D"TILE_SIZE=$(TILE_SIZE)" -D"$(SCENARIO)" -D"NUM_TOTAL_PAGES=$(NUM_TOTAL_PAGES)" -D"BLOCKSIZE_INIT = $(BLOCKSIZE_INIT)" -D"SINGLE_FETCH" -D"PBS" -D"PCAD" -D"MAX_ORDER=10"  -D"Cache_OFF" -D"IMG_HORIZONTAL_SIZE = $(IMG_HORIZONTAL_SIZE)" -D"IMG_VERTICAL_SIZE = $(IMG_VERTICAL_SIZE)"  -D"BYTE_PER_PIXEL = 4" -o $(MY_RUN) ./Top.cpp ./Memory.cpp ./CAxPkt.cpp ./CRPkt.cpp ./CWPkt.cpp ./CBPkt.cpp ./UD_Bus.cpp ./CFIFO.cpp ./CTRx.cpp ./CQ.cpp ./CBank.cpp ./CMem.cpp ./CAddrGen.cpp ./CMST.cpp ./CTracker.cpp ./CROB.cpp ./CMIU.cpp ./CMMU.cpp ./CCache.cpp ./CBUS.cpp ./CArb.cpp ./CScheduler.cpp ./CSLV.cpp ./$(MAINFILE) ./Buddy/BuddyTop.cpp ./Buddy/CQueue.cpp ./Buddy/CFreeList.cpp ./Buddy/CBlockList.cpp ./Buddy/CBlockArray.cpp ./Buddy/CPageArray.cpp ./Buddy/CBuddy.cpp

# custom target for BFAM Image Display configuration
cci_test cci_test_debug: AR=AR_LIAM
cci_test cci_test_debug: AW=AW_LIAM
cci_test cci_test_debug: BANK_NUM=16
cci_test cci_test_debug: IMG_HORIZONTAL_SIZE=4096
cci_test cci_test_debug: IMG_VERTICAL_SIZE=256
cci_test cci_test_debug: MAINFILE=mainMemCopy.cpp
cci_test cci_test_debug: SCENARIO?=RASTER_SCAN
cci_test cci_test_debug: NUM_TOTAL_PAGES?=32
cci_test cci_test_debug: BLOCKSIZE_INIT?=16
cci_test cci_test_debug: TILE_SIZE?=4
cci_test cci_test_debug: SEED_INIT?=493
cci_test cci_test_debug: nCACHELINE?=64
cci_test cci_test_debug: TRANS_TYPE?=WRITE
cci_test cci_test_debug: CCI_TRANS_FLAGS=$(if $(filter $(TRANS_TYPE),READ),-D"CCI_READ_ONLY",$(if $(filter $(TRANS_TYPE),READWRITE),-D"CCI_READ_WRITE",-D"CCI_WRITE_ONLY"))
cci_test cci_test_debug: CCI_FLAGS=-D"CCI_CACHELINE=$(nCACHELINE)" $(CCI_TRANS_FLAGS)

cci_test: BBS_TRAD
cci_test_debug: BBS_TRAD_DEBUG
