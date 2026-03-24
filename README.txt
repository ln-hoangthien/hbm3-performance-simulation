-----------------------------------------------------------
Memory system simulator 
-----------------------------------------------------------
Version		: 0.1
Contact		: lnhthien@stu.jejunu.ac.kr, jaeyoung.hur@jejunu.ac.kr
Date		: 24 March 2025 
-----------------------------------------------------------

-----------------------------------------------------------
Descriptions 
	1. Simulation of memory subsystem in SoC
	2. Performance model to explore micro-architectures
	3. Cycle level simulation
	4. Transaction-level (ARM architecture, AXI protocol)
	5. HBM3 performance model
-----------------------------------------------------------
Notes 
	- The simulation is tested with g++ (gcc version 9.4.0) in Linux (5.15.90.1-microsoft-standard-WSL2) 
	- We did not experience warnings or errors. For any support, please contact lnhthien@stu.jejunu.ac.kr
-----------------------------------------------------------
Scenarios
	- Image
		+ Display
		+ Blending
		+ Preview
		+ Scaling
	- Matrix
		+ Transpose
		+ Multiplition
		+ Convolution
		+ Sobel
-----------------------------------------------------------
How to run
	To run the simulation, type below command:
	step 1:	Generate the testlist: python gentestcase.py
	step 2:	./getTLBCoalescingReport_<bank_number>.bash
	step 3: check the log file: check_BBS_TRAD_<configuration>
	
-----------------------------------------------------------



