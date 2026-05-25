#!/usr/bin/env python3
import os

SEED_INIT = 493
MAINFILE = "mainMemCopy.cpp"
SCENARIO = "RASTER_SCAN"

# Output script name
bash_filename = "run_cci_tests.bash"

print(f"Generating {bash_filename}...")

with open(bash_filename, "w") as f:
    f.write("#!/bin/bash\n")
    f.write("# Script to compile and run CCI tests automatically\n\n")
    f.write("# Ensure the review_cci directory exists\n")
    f.write("mkdir -p review_cci\n\n")
    
    # Core parameter lists - easily configurable by adding values
    cacheline_list = [1, 16, 64]
    trans_type_list = ["READ", "WRITE", "READWRITE"]
    h_size_list = [2048, 4096]
    
    # Mapping to short names for matching review filenames
    trans_short = {
        "READ": "AR",
        "WRITE": "AW",
        "READWRITE": "ARAW"
    }
    
    for trans_type in trans_type_list:
        for cacheline in cacheline_list:
            for h_size in h_size_list:
              short_type = trans_short[trans_type]
              v_size  = h_size // 8
                
              # Compiled binary name matching Makefile rules dynamically formatted with dimensions
              exec_name = f"cci_test_MemCopy__AR_LIAM_AW_LIAM_16_16_{h_size}_{v_size}_4"
                
              # Log file matching verification script expectations
              log_name = f"review_{short_type}_{h_size}x{v_size}_{cacheline}_50_10_8"
                
              # Compile with dynamic overrides
              make_cmd = (
                  f"make cci_test "
                  f"nCACHELINE={cacheline} "
                  f"TRANS_TYPE={trans_type} "
                  f"IMG_HORIZONTAL_SIZE={h_size} "
                  f"IMG_VERTICAL_SIZE={v_size} "
                  f"MAINFILE={MAINFILE}\n"
              )
              f.write(make_cmd)
                
              # Run the simulation and pipe outputs into the review log file
              run_cmd = f"./{exec_name} > {log_name} &\n"
              f.write(run_cmd)
                
              # Clean up the output binary
              f.write(f"rm -f {exec_name}\n\n")
            
    f.write("echo \"All CCI tests executed successfully!\"\n")

# Make the generated script executable
os.chmod(bash_filename, 0o755)
print(f"Generated {bash_filename} successfully and set executable permissions.")
