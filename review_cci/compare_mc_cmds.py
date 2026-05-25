#!/usr/bin/env python3
import re
import sys

def parse_log(filename, target_cmd):
    """
    Parses the log file and extracts memory controller commands matching target_cmd (WR or RD).
    Extracts Cycle, Transaction Name, Bank, and Row.
    """
    entries = []
    # Regex to capture: MC cmd <cmd> (<tx>)(Bank <bank>, Row <row>)
    pattern = re.compile(
        r"\[Cycle\s+(\d+):\s+.*?\]\s+MC cmd\s+" + re.escape(target_cmd) + r"\s+\((.*?)\)\(Bank\s+(\d+),\s+Row\s+(0x[0-9a-fA-F]+)\)"
    )
    
    try:
        with open(filename, 'r') as f:
            for line in f:
                match = pattern.search(line)
                if match:
                    cycle = int(match.group(1))
                    tx = match.group(2)
                    bank = int(match.group(3))
                    row = match.group(4)
                    entries.append({
                        'cycle': cycle,
                        'tx': tx,
                        'bank': bank,
                        'row': row
                    })
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found.")
        sys.exit(1)
        
    return entries

def display_comparison(file1, file2, entries1, entries2, cmd_type):
    print(f"=== Sequential Comparison for MC cmd {cmd_type} ===")
    
    max_len = max(len(entries1), len(entries2))
    
    # Header format
    header_fmt = "{:<5} | {:<32} | {:<32} | {:<8}"
    col_header_fmt = "{:<5} | {:<6} {:<11} {:<4} {:<6} | {:<6} {:<11} {:<4} {:<6} | {:<8}"
    divider = "=" * 92
    
    print(header_fmt.format("Index", file1[:32], file2[:32], "Status"))
    print(col_header_fmt.format("", "Cycle", "Tx", "Bank", "Row", "Cycle", "Tx", "Bank", "Row", ""))
    print(divider)
    
    row_fmt = "{:<5} | {:<6} {:<11} B:{:<2}  {:<6} | {:<6} {:<11} B:{:<2}  {:<6} | {:<8}"
    
    match_count = 0
    mismatch_count = 0
    unmatched_count = 0
    
    for i in range(max_len):
        e1_cycle, e1_tx, e1_bank, e1_row = "", "", "", ""
        e2_cycle, e2_tx, e2_bank, e2_row = "", "", "", ""
        
        has_e1 = i < len(entries1)
        has_e2 = i < len(entries2)
        
        if has_e1:
            e1_cycle = entries1[i]['cycle']
            e1_tx = entries1[i]['tx']
            e1_bank = entries1[i]['bank']
            e1_row = entries1[i]['row']
            
        if has_e2:
            e2_cycle = entries2[i]['cycle']
            e2_tx = entries2[i]['tx']
            e2_bank = entries2[i]['bank']
            e2_row = entries2[i]['row']
            
        # Determine match status based on Bank and Row address
        if has_e1 and has_e2:
            if entries1[i]['bank'] == entries2[i]['bank'] and entries1[i]['row'] == entries2[i]['row']:
                status = "MATCH"
                match_count += 1
            else:
                status = "MISMATCH"
                mismatch_count += 1
        else:
            status = "UNMATCHED"
            unmatched_count += 1
            
        print(row_fmt.format(
            i + 1, 
            e1_cycle, e1_tx, e1_bank, e1_row,
            e2_cycle, e2_tx, e2_bank, e2_row,
            status
        ))
        
    print(divider)
    print(f"Summary for {cmd_type}:")
    print(f"  - Total Matches    : {match_count}")
    print(f"  - Total Mismatches : {mismatch_count}")
    print(f"  - Total Unmatched  : {unmatched_count}")
    print(f"  - Total Commands   : {file1} = {len(entries1)}, {file2} = {len(entries2)}\n")

def main():
    file1 = "review_AW_1_50_10_8"
    file2 = "review_AW_64_50_10_8"
    
    # Check if a target command type is specified via command line arguments
    target_cmds = ["WR", "RD"]
    if len(sys.argv) > 1:
        arg = sys.argv[1].upper()
        if arg in ["WR", "RD"]:
            target_cmds = [arg]
        else:
            print(f"Invalid argument '{sys.argv[1]}'. Usage: ./compare_mc_cmds.py [WR|RD]")
            sys.exit(1)
            
    for cmd in target_cmds:
        entries1 = parse_log(file1, cmd)
        entries2 = parse_log(file2, cmd)
        display_comparison(file1, file2, entries1, entries2, cmd)

if __name__ == "__main__":
    main()
