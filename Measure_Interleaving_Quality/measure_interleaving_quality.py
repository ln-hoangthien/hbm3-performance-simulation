#!/usr/bin/python3

import math
import argparse
import numpy as np
from numba import njit
from typing import Tuple

import matplotlib
matplotlib.use('Agg')

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib.colors as mcolors
from mpl_toolkits.mplot3d import Axes3D 
import matplotlib.patches as mpatches
import sys
import plotly.graph_objects as go

# Default Image Configuration
banks_per_group = 4
num_cols = 32
num_rows = 32768
col_bits = math.ceil(math.log2(num_cols))
row_bits = math.ceil(math.log2(num_rows))
bank_bits = 4
bytes_per_pixel = 4
page_size = 2048
pixels_per_col = 64 // bytes_per_pixel
pixels_per_page = page_size // bytes_per_pixel

@njit
def calculate_pixel_values(img_v: int, img_hb: int, num_banks: int) -> Tuple[np.ndarray, np.ndarray]:
    """Vectorized calculation of super page IDs and bank IDs for all pixels."""
    total_transactions = math.ceil(img_hb  / 16) * img_v
    transactions = np.arange(total_transactions, dtype=np.uint32)
    
    div_col = transactions // num_cols
    ri = (div_col // num_banks).astype(np.uint64)
    bi = (div_col % num_banks).astype(np.uint8)
    ci = (transactions % num_cols).astype(np.uint8)
    
    return ri, bi, ci, transactions


def generate_bankgroup_interleaving_matrix_liam(img_hb: int) -> Tuple[np.ndarray, np.ndarray]:
    """Generate a bankgroup interleaving matrix for LIAM using vectorized operations."""
    row_ids, bank_ids, col_ids, _ = calculate_pixel_values(img_v, img_hb, num_banks)
    return row_ids, bank_ids, col_ids


def generate_bankgroup_interleaving_matrix_bfam(img_hb: int) -> Tuple[np.ndarray, np.ndarray]:
    """Generate a bankgroup interleaving matrix for BFAM."""
    row_ids, bank_ids, col_ids, _ = calculate_pixel_values(img_v, img_hb, num_banks)

    gspn = row_ids // k
    mask = (gspn % 2 == 1)
    bank_ids = np.where(mask, (bank_ids + num_banks // 2) % num_banks, bank_ids)

    return row_ids, bank_ids, col_ids


def generate_bankgroup_interleaving_matrix_bgfam(num_bankgroups: int, img_hb: int) -> Tuple[np.ndarray, np.ndarray]:
    """Generate a bankgroup interleaving matrix for BGFAM."""
    row_ids, bank_ids, col_ids, _ = calculate_pixel_values(img_v, img_hb, num_banks)

    gspn = row_ids // k
    bank_ids = (bank_ids + (gspn % num_bankgroups) * banks_per_group) % num_banks

    return row_ids, bank_ids, col_ids

def get_bit(value, position):
    """
    Returns the bit (0 or 1) at the given position.
    Position 0 is the Least Significant Bit (rightmost).
    """
    return (value >> position) & 1

def cal_row_jump(curr_col_offset: int, img_width_pixels: int, bit_num: int):
    """Calculate row jump based on current column offset."""
    # Each transaction is 16 pixels
    previous_bit = 0
    jump_value = 0
    for i in range(1, bit_num + 1):
        curr_bit = bit_num - i
        curr_col_offset_bit  = get_bit(curr_col_offset, curr_bit)
        img_width_pixels_bit = get_bit(img_width_pixels, curr_bit)
        power_of_two = ((curr_col_offset_bit ^ 1) & img_width_pixels_bit) & (previous_bit ^ 1)
        jump_value += (power_of_two << curr_bit)
        previous_bit |= power_of_two
        #print(f"curr_col_offset = {curr_col_offset} / img_width_pixels = {img_width_pixels}: Bit Position {curr_bit}: curr_col_offset_bit={curr_col_offset_bit}, img_width_pixels_bit={img_width_pixels_bit}, previous_bit={previous_bit}, power_of_two={power_of_two}, jump_value={jump_value}")
    #print(f"curr_col_offset = {curr_col_offset} : Final row_jump = {jump_value}")
    return jump_value

#def cal_row_accumulate(row_jump: np.ndarray, img_width_trans: int):
#    """Calculate row jump based on current column offset."""
#    # Each transaction is 16 pixels
#    accumulate_value = np.zeros_like(row_jump)
#    for i in range(0, len(row_jump)):
#        accumulate_value[i] = (accumulate_value[i-1] | row_jump[i]) if (i % img_width_trans != 0) else row_jump[i]
#    return accumulate_value

def cal_backward_forward_col(row_jump: np.ndarray, img_width_trans: int):
    row_jump_backward = ~row_jump + 1 # 2-nd complement - include current row_jump position
    row_jump_forward = ~(row_jump_backward) # Not include current row_jump position
    row_jump_backward = row_jump_backward & ~row_jump # 2-nd complement - Noinclude current row_jump position

    return (img_width_trans & row_jump_backward), (img_width_trans & row_jump_forward)

def cal_ratio_optimal(a: np.ndarray, b: np.ndarray):
    """Calculate ratio x/y compared to a/b."""
    a_lsb = np.log2(np.maximum(a, 1)).astype(int)
    #a_lsb = np.log2(np.maximum(a & (~a + 1), 1)).astype(int)
    b_lsb = np.log2(np.maximum(b & (~b + 1), 1)).astype(int)

    x = a >> np.minimum(a_lsb, b_lsb).astype(int)
    y = b >> np.minimum(a_lsb, b_lsb).astype(int)

    return x, y

def cal_ratio(a: np.ndarray, b: np.ndarray):
    """Calculate ratio x/y compared to a/b."""
    #a_lsb = np.log2(np.maximum(a, 1)).astype(int)
    a_lsb = np.log2(np.maximum(a & (~a + 1), 1)).astype(int)
    b_lsb = np.log2(np.maximum(b & (~b + 1), 1)).astype(int)

    print(f"a[32] = {a[32]} - a[48] = {a[48]} - a_lsb[32] = {a_lsb[32]} - a_lsb[48] = {a_lsb[48]} - b_lsb = {b_lsb}")

    x = a >> np.minimum(a_lsb, b_lsb).astype(int)
    y = b >> np.minimum(a_lsb, b_lsb).astype(int)

    return x, y


def bit_count(n):
    """
    Method 2: The 'Hardware Simulation' way.
    Uses a loop to shift bits, similar to a linear scan in hardware.
    """
    return bin(n & 0xFFFFFFFF).count('1')

def generate_bankgroup_interleaving_matrix_proposal(num_bankgroups: int, img_width_pixels: int, img_height_pixels: int) -> Tuple[np.ndarray, np.ndarray]:
    """Generate a bankgroup interleaving matrix for Proposal (fully vectorized)."""
    _, _, _, transactions = calculate_pixel_values(img_v, img_width_pixels, num_banks)
    
    # Pre-calculate constants
    image_width_trans_all   = img_width_pixels  // pixels_per_col # one transaction = 16 pixels
    curr_col_offset         = transactions      %  image_width_trans_all
    curr_row_offset         = transactions      // image_width_trans_all

    #print(f"image_width_trans_all = {image_width_trans_all}")
    
    if (math.log2(num_cols).is_integer()):
        num_cols_log = int(math.log2(num_cols))
    else:
        print(f"Error: num_cols ({num_cols}) is not a power-of-2 number.")
        exit(1)

    if (math.log2(banks_per_group).is_integer()):
        banks_per_group_log = int(math.log2(banks_per_group))
    else:
        print(f"Error: banks_per_group ({banks_per_group}) is not a power-of-2 number.")
        exit(1) 

    block_size_trans        = num_banks << num_cols_log
    block_size_trans_log    = int(math.log2(block_size_trans))

    # Row Jump Pre-calculation
    # Row Jump always be a power-of-2 number. Can these numbers be get log2() easily?
    row_jump                        = np.array([cal_row_jump(curr_col_offset[i], image_width_trans_all, 64) for i in range(len(curr_col_offset))]) # Proposed a new circuit where hardware overhead should be low.
    row_jump_log                    = np.log2(np.maximum(row_jump, 1)).astype(int) # Implements in Verilog as a n-to-2^n Decoder. 2^n * 16 >= Image-Width-in-Pixels. With m = 11, the Max Image-Width-in-Pixels could be up to 32k pixels.

    #num_row_per_allbanks            = np.maximum(block_size_trans // row_jump,  1).astype(int)
    #num_row_per_allbanks_log        = np.log2(num_row_per_allbanks).astype(int)

    # Bank Pre-calculation
    #num_bank_per_row        = (row_jump  >> num_cols_log)

    num_row_per_banks       = (num_cols >> row_jump_log)
    num_row_per_banks       = np.log2(np.maximum(num_row_per_banks,     1)).astype(int) # Implements in Verilog as a n-bit Decoder. 2^n * 16 >= Image-Width-in-Pixels. With m = 11, the Max Image-Width-in-Pixels could be up to 32k pixels.

    num_row_per_allbankgroup        = num_bankgroups << num_row_per_banks
    num_row_per_allbankgroup_log    = np.log2(np.maximum(num_row_per_allbankgroup, 1)).astype(int)

    row_jump_backward, row_jump_forward = cal_backward_forward_col(row_jump, image_width_trans_all)

    row_jump_backward = np.where(row_jump_backward == 0, 1, row_jump_backward) # Sofware hack to avoid div-by-zero
    curr_col_offset_row_bank = np.where(row_jump_backward == 1, curr_col_offset, curr_col_offset % row_jump_backward)

    n_sp_pages, n_rows            = cal_ratio_optimal(row_jump, block_size_trans) # Only for non-power-of-2 num_banks
    n_sp_pages_log                = np.log2(n_sp_pages).astype(int)
    n_rows_log                    = np.log2(n_rows).astype(int)

    #n_sp_pages_forward, n_rows_forward  = cal_ratio(row_jump_forward, block_size_trans) # Only for non-power-of-2 num_banks
    #n_sp_pages_forward                 = np.maximum(n_sp_pages_forward, 1).astype(int) 
    #n_sp_pages_forward_log              = np.log2(np.maximum(n_sp_pages_forward, 1)).astype(int)
    #n_rows_forward_log                  = np.log2(np.maximum(n_rows_forward, 1)).astype(int)

    # Row: Reduction calculation
    # X * pages = Y * rows => X = Y * pages // block
    # 64 = 2 banks => 1 row 2 banks, => 8 rows, 16 banks => 1 page = 8 row => 368 rows = (368 * 1 // 8) pages
    high_bit_num = np.array([bit_count(row_jump_forward[i]) for i in range(len(row_jump_forward))])
    if (math.log2(num_banks).is_integer()):
        row_utilization         = row_jump_forward - (((row_jump_forward * (img_height_pixels & (block_size_trans - 1))) >> block_size_trans_log) + high_bit_num)
        #row_utilization         = ((row_jump_forward * (block_size_trans - (img_height_pixels & (block_size_trans - 1))))  >> block_size_trans_log) + high_bit_num
    else:
        row_utilization         = row_jump_forward - (((row_jump_forward * (img_height_pixels % block_size_trans     ))  // block_size_trans    ) + high_bit_num)
        #row_utilization         = ((row_jump_forward * (block_size_trans - (img_height_pixels % block_size_trans))) // block_size_trans) + high_bit_num
    # Row: Pre calculation
    row_inc_firstblock      = row_jump_forward
    #row_linear              = np.zeros_like(curr_row_offset)
    if (math.log2(num_banks).is_integer()):
        row_reduce_location         = img_height_pixels >> block_size_trans_log
        row_reducion                = ((curr_row_offset >> block_size_trans_log) == row_reduce_location) * row_utilization
        #row_linear                   = np.where(row_jump_backward == 1, ((curr_row_offset << row_jump_log) + curr_col_offset)                       >> block_size_trans_log, 
        #                                                                ((curr_row_offset << row_jump_log) + (curr_col_offset % row_jump_backward)) >> block_size_trans_log) # Sofware hack to avoid div-by-zero
        row_verical_linear          = (curr_row_offset >> n_rows_log << n_sp_pages_log)
        row_horizonal_linear        = (curr_col_offset_row_bank >> num_cols_log) >> np.log2(np.maximum((row_jump >> num_cols_log) >> n_sp_pages_log, 1)).astype(int)  # Sofware hack to avoid div-by-zero
        row_inc_block               = (curr_row_offset >> block_size_trans_log) * (image_width_trans_all & ~row_jump)
    else:
        row_reduce_location         = img_height_pixels // block_size_trans
        row_reducion                = ((curr_row_offset // block_size_trans) == row_reduce_location) * row_utilization
        #row_linear                   = np.where(row_jump_backward == 1, ((curr_row_offset << row_jump_log) + curr_col_offset)                       // block_size_trans, 
        #                                                                ((curr_row_offset << row_jump_log) + (curr_col_offset % row_jump_backward)) // block_size_trans) # Sofware hack to avoid div-by-zero
        row_verical_linear          = ((curr_row_offset // n_rows) * n_sp_pages)
        row_horizonal_linear        = ((curr_col_offset_row_bank >> num_cols_log) // np.maximum(((row_jump >> num_cols_log) // n_sp_pages), 1)) # Sofware hack to avoid div-by-zero
        row_inc_block               = (curr_row_offset // block_size_trans) * (image_width_trans_all & ~row_jump)

    # Bank: Pre calculation
    bank_inc_block                  = np.where(row_jump_backward == 1, 0, (curr_col_offset // row_jump_backward))
    bank_linear                    = ((curr_row_offset >> num_row_per_banks) % num_bankgroups) << banks_per_group_log

    bank_inc_img_bankgroup          = np.zeros_like(curr_row_offset)
    if (math.log2(num_banks).is_integer()):
        bank_inc_img_bankgroup     = ( curr_row_offset >> num_row_per_allbankgroup_log ) & (banks_per_group - 1)
        bank_inc_col               = ((curr_col_offset_row_bank >> num_cols_log) * np.maximum((n_rows // num_bankgroups), 1)) & (banks_per_group - 1)
        bank_inc_col_bg            = (curr_col_offset_row_bank >> num_cols_log >> banks_per_group_log) << np.log2(n_rows << banks_per_group_log).astype(int)
    else:
        bank_inc_img_bankgroup    = ( curr_row_offset // num_row_per_allbankgroup ) & (banks_per_group - 1)
        bank_inc_col              = ((curr_col_offset_row_bank >> num_cols_log) * np.maximum((n_rows // num_bankgroups), 1)) & (banks_per_group - 1)
        bank_inc_col_bg           = (curr_col_offset_row_bank >> num_cols_log >> banks_per_group_log) * (n_rows << banks_per_group_log)

    # Calculate new address
    new_row                 = (row_verical_linear + row_horizonal_linear + row_inc_firstblock + row_inc_block - row_reducion) % num_rows
    #new_bank                = (bank_linear + bank_linear_overflow + bank_inc_img_bankgroup + bank_inc_col + bank_inc_col_bg + bank_inc_block) % num_banks
    new_bank                = (bank_linear + bank_inc_img_bankgroup + bank_inc_col + bank_inc_col_bg + bank_inc_block) % num_banks
    new_col                 = (curr_col_offset + (curr_row_offset << row_jump_log)) % num_cols

    #for i in range(len(new_row)):
    #    print(f"({new_row[i]},{new_bank[i]},{new_col[i]}) - row_jump[{i}]: {row_jump[i]} - curr_col_offset[{i}]: {curr_col_offset[i]} - curr_row_offset[{i}]: {curr_row_offset[i]} - row_reducion[{i}]: {row_reducion[i]} - row_jump_forward[{i}]: {row_jump_forward[i]} - rơw_inc_firstblock[{i}]: {row_inc_firstblock[i]} - row_utilization[{i}]: {row_utilization[i]}")
    #    print(f"({new_row[i]},{new_bank[i]},{new_col[i]}) - row_jump[{i}]: {row_jump[i]} - curr_col_offset[{i}]: {curr_col_offset[i]} - curr_row_offset[{i}]: {curr_row_offset[i]} - bank_linear[{i}]: {bank_linear[i]} - bank_inc_col[{i}]: {bank_inc_col[i]} - bank_inc_col_bg[{i}]: {bank_inc_col_bg[i]} - n_rows[{i}]: {n_rows[i]} - n_sp_pages[{i}]: {n_sp_pages[i]} - num_row_per_banks[{i}]: {num_row_per_banks[i]}")
    #    print(f"({new_row[i]},{new_bank[i]},{new_col[i]}) - row_linear[{i}]: {row_linear[i]} - bank_linear[{i}]: {bank_linear[i]} - curr_col_offset[{i}]: {curr_col_offset[i]} - curr_row_offset[{i}]: {curr_row_offset[i]} - row_inc_firstblock[{i}]: {row_inc_firstblock[i]} - row_inc_block[{i}]: {row_inc_block[i]} - row_jump_backward[{i}]: {row_jump_backward[i]} - row_jump_forward[{i}]: {row_jump_forward[i]}")
              
    return new_row, new_bank, new_col

@njit
def qualifying_matrix_optimized(row_ids: np.ndarray, bank_ids: np.ndarray,
                                img_v: int, img_hb: int, 
                                num_banks: int, banks_per_group: int) -> float:
    """Optimized quality calculation using numba for speed."""
    total_quality = 0.0

    row_ids = row_ids.reshape((img_v, img_hb))
    bank_ids = bank_ids.reshape((img_v, img_hb))

    # USE THE PASSED ARGUMENTS (img_v, img_hb) INSTEAD OF GLOBALS
    for row in range(img_v):
        for col in range(img_hb):
        #for col in range(1):
            each_quality = 0
            
            for per_bg in range(1, num_banks):
                # Right - Different Bank
                if col + per_bg < img_hb and bank_ids[row, col] != bank_ids[row, col + per_bg]:
                    bg1 = bank_ids[row, col] // banks_per_group
                    bg2 = bank_ids[row, col + per_bg] // banks_per_group
                    each_quality += 2 if bg1 != bg2 else 1
                # Right - Same Bank
                if col + per_bg < img_hb and bank_ids[row, col] == bank_ids[row, col + per_bg]:
                    each_quality += 2 if row_ids[row, col] == row_ids[row, col + per_bg] else -1
                    if row_ids[row, col] != row_ids[row, col + per_bg]:
                        break
            
            for per_bg in range(1, num_banks):
                # Left - Different Bank
                if col - per_bg >= 0 and bank_ids[row, col] != bank_ids[row, col - per_bg]:
                    bg1 = bank_ids[row, col] // banks_per_group
                    bg2 = bank_ids[row, col - per_bg] // banks_per_group
                    each_quality += 2 if bg1 != bg2 else 1
                # Left - Same Bank
                if col - per_bg >= 0 and bank_ids[row, col] == bank_ids[row, col - per_bg]:
                    each_quality += 2 if row_ids[row, col] == row_ids[row, col - per_bg] else -1
                    if row_ids[row, col] != row_ids[row, col - per_bg]:
                        break
            
            for per_bg in range(1, num_banks):
                nr = row + (col + per_bg * img_hb) // img_hb
                nc = (col + per_bg * img_hb) % img_hb
                # Down - Different Bank
                if nr < img_v and bank_ids[row, col] != bank_ids[nr, nc]:
                    bg1 = bank_ids[row, col] // banks_per_group
                    bg2 = bank_ids[nr, nc] // banks_per_group
                    each_quality += 2 if bg1 != bg2 else 1
                # Down - Same Bank
                if nr < img_v and bank_ids[row, col] == bank_ids[nr, nc]:
                    each_quality += 2 if row_ids[row, col] == row_ids[nr, nc] else -1
                    if row_ids[row, col] != row_ids[nr, nc]:
                        break
            
            for per_bg in range(1, num_banks):
                if col - per_bg * img_hb >= 0:
                    nr = row - (col - per_bg * img_hb) // img_hb
                    nc = (col - per_bg * img_hb) % img_hb
                    # Up - Different Bank
                    if nr >= 0 and bank_ids[row, col] != bank_ids[nr, nc]:
                        bg1 = bank_ids[row, col] // banks_per_group
                        bg2 = bank_ids[nr, nc] // banks_per_group
                        each_quality += 2 if bg1 != bg2 else 1
                    # Up - Same Bank
                    if nr >= 0 and bank_ids[row, col] == bank_ids[nr, nc]:
                        each_quality += 2 if row_ids[row, col] == row_ids[nr, nc] else -1
                        if row_ids[row, col] != row_ids[nr, nc]:
                            break

            total_quality += each_quality

    return total_quality / (img_v * img_hb) / num_banks

def check_and_print_overlaps(matrix_tuple: Tuple[np.ndarray, np.ndarray]) -> bool:
    """
    Checks for overlaps and prints the specific (Row, Bank) pairs that collide.
    """
    row_ids, bank_ids, col_ids = matrix_tuple
    
    # 1. Flatten and cast to integer to ensure clean comparison
    flat_rows = row_ids.flatten().astype(np.int64)
    flat_banks = bank_ids.flatten().astype(np.int64)
    flat_cols = col_ids.flatten().astype(np.int64)
    
    # 2. Stack them to create pairs: [[r1, b1], [r2, b2], ...]
    pairs = np.stack((flat_rows, flat_banks, flat_cols), axis=1)
    
    # 3. Find unique pairs and count their occurrences
    # axis=0 looks at the "rows" of our pairs list (the coordinates)
    unique_pairs, counts = np.unique(pairs, axis=0, return_counts=True)
    
    # 4. Identify indices where count is greater than 1
    duplicate_indices = np.where(counts > 1)[0]
    
    if len(duplicate_indices) > 0:
        print(f"\n[!] OVERLAP DETECTED: {len(duplicate_indices)} unique addresses have collisions.")
        print(f"{'Row ID':<10} | {'Bank ID':<10} | {'Col ID':<10} | {'Count'}")
        print("-" * 35)
        
        # Print the duplicates
        for idx in duplicate_indices:
            row_val = unique_pairs[idx][0]
            bank_val = unique_pairs[idx][1]
            col_val = unique_pairs[idx][2]
            count = counts[idx]
            print(f"{row_val:<10} | {bank_val:<10} | {col_val:<10} | {count}")
            
        return True # Overlaps exist
    else:
        print("\n[OK] No overlaps found. All addresses are unique.")
        return False # No overlaps

def map_proposal_to_physical_memory(matrix_tuple: Tuple[np.ndarray, np.ndarray], 
                                    num_banks: int, 
                                    num_rows: int) -> np.ndarray:
    """
    Maps logical (Row, Bank) pairs to physical memory [num_banks, num_rows].
    Includes safety checks to prevent crashes from invalid Bank/Row IDs.
    """
    row_ids_logical, bank_ids_logical, col_ids_logical = matrix_tuple

    # 1. Initialize Map
    physical_memory_map = np.zeros((num_banks, num_rows, num_cols), dtype=np.int32)

    # 2. Flatten for iteration
    r_flat = row_ids_logical.flatten().astype(np.int64)
    b_flat = bank_ids_logical.flatten().astype(np.int64)
    c_flat = col_ids_logical.flatten().astype(np.int64)

    # 3. Safety Check: Filter Invalid IDs
    # This prevents the "IndexError" you encountered if the proposal algorithm 
    # generates a Bank ID >= num_banks.
    valid_mask = (r_flat >= 0) & (r_flat < num_rows) & \
                 (b_flat >= 0) & (b_flat < num_banks) & \
                 (c_flat >= 0) & (c_flat < num_cols)
    
    total_pixels = len(r_flat)
    valid_pixels = np.sum(valid_mask)
    
    if valid_pixels < total_pixels:
        print(f"Warning: {total_pixels - valid_pixels} pixels were out of bounds (Invalid Bank or Row ID) and were skipped.")

    # Apply mask
    r_flat = r_flat[valid_mask]
    b_flat = b_flat[valid_mask]
    c_flat = c_flat[valid_mask]

    # 4. Populate Map
    # np.add.at handles collisions correctly by incrementing the counter
    np.add.at(physical_memory_map, (b_flat, r_flat, c_flat), 1)

    return physical_memory_map

import math
import numpy as np
from numba import njit

@njit
def get_col_major_conflicts(row_ids: np.ndarray, bank_ids: np.ndarray, 
                            img_v: int, img_hb_trans: int, num_banks: int) -> int:
    """
    Counts page conflicts (Row Buffer Misses) assuming a Column-Major access pattern.
    
    In a column-major scan, we access: (0,0), (1,0), (2,0)... then (0,1), (1,1)...
    We must track the state of every bank's row buffer because the access jumps 
    between banks constantly.
    """
    # 1. Initialize Row Buffer State for all banks
    # -1 indicates the bank is closed/idle
    conflicts = 0
    
    # 2. Iterate in Column-Major Order
    # Outer Loop: Columns (Transactions)
    # Inner Loop: Rows (Vertical height)
    for c in range(img_hb_trans):
        for r in range(img_v):
            # Calculate the flat index in the Row-Major arrays
            # The arrays from LIAM/BFAM are stored as [Row 0, Row 1, ...]

            if r == 0:
                flat_idx     = r * img_hb_trans + c 
                flat_idx_pre = (img_v - 1) * img_hb_trans + (c - 1) if c > 0 else (img_v - 1) * img_hb_trans + (img_hb_trans - 1)
            else:
                flat_idx     = r * img_hb_trans + c
                flat_idx_pre = (r - 1) * img_hb_trans + c if r > 0 else -1
            
            conflicts += 1 if ((bank_ids[flat_idx] == bank_ids[flat_idx_pre]) and (row_ids[flat_idx] != row_ids[flat_idx_pre])) else 0

    return conflicts

def save_heatmap_3D(physical_map: np.ndarray, filename: str = "heatmap_3d.png", img_hb=0, img_v=0):
    """
    Saves a 3D Voxel plot showing exact memory usage.
    Input physical_map shape: (num_banks, num_rows, num_cols)
    
    Target Plot Mapping:
    X-axis: Banks
    Y-axis: Columns
    Z-axis: Rows (Height)
    """
    print("Preparing 3D data...")

    # --- 1. CRITICAL FIX: Transpose Axes ---
    # Current shape: (Bank, Row, Col) -> (0, 1, 2)
    # Target shape for (x, y, z): (Bank, Col, Row) -> (0, 2, 1)
    # We swap axis 1 and 2 so that Rows become the Z-axis (height).
    plot_data = physical_map.transpose(0, 2, 1)
    
    # Get new dimensions based on the transposed data
    num_banks_x, num_cols_y, num_rows_z = plot_data.shape

    # --- 2. Setup Font ---
    plt.rcParams['font.family'] = 'serif'
    plt.rcParams['font.serif'] = ['Times New Roman'] + plt.rcParams['font.serif']

    # --- 3. Prepare Data for Voxels ---
    # A) Where to draw
    filled_voxels = plot_data > 0

    # B) What color to draw
    voxel_colors = np.empty(plot_data.shape, dtype=object)
    
    # Allocations (Green, Semi-transparent)
    voxel_colors[plot_data == 1] = '#00FF0080' 
    # Collisions (Red, Solid)
    voxel_colors[plot_data > 1] = '#FF0000FF' 

    # --- 4. Setup 3D Plot ---
    fig = plt.figure(figsize=(20, 24)) # Adjusted figsize for better vertical proportion
    ax = fig.add_subplot(111, projection='3d')

    print("Rendering 3D Voxels (this might take a moment)...")
    
    # Plot voxels
    ax.voxels(filled_voxels, facecolors=voxel_colors, edgecolors='k', linewidth=0.1)

    # --- 5. Labels and Formatting ---
    ax.set_xlabel(f'Bank ID (0-{num_banks_x-1})', fontsize=12, labelpad=10)
    ax.set_ylabel(f'Column ID (0-{num_cols_y-1})', fontsize=12, labelpad=10)
    ax.set_zlabel(f'Row ID (0-{num_rows_z-1})', fontsize=12, labelpad=10)

    # Set limits
    ax.set_xlim(0, num_banks_x)
    ax.set_ylim(0, num_cols_y)
    ax.set_zlim(0, num_rows_z)

    # --- 6. Aspect Ratio Fix ---
    # Memory arrays are tall (Row) and narrow (Bank/Col). 
    # We scale the visual aspect ratio so the plot isn't a thin needle or flat pancake.
    # We force X and Y to look square-ish, and Z to be 1.5x taller.
    ax.set_box_aspect((1, 1, 1.5)) 

    # Titles
    plt.suptitle("3D Physical Memory Layout & Collisions", fontsize=16)
    plt.title(f"Image Size: {img_hb}x{img_v} | Red Cubes = Column Collision", fontsize=14)

    # View angle (Isometric-ish)
    ax.view_init(elev=25, azim=-45)

    # --- 7. Legend ---
    legend_elements = [
        mpatches.Patch(color='#00FF0080', label='Allocation'),
        mpatches.Patch(color='#FF0000FF', label='Collision'),
    ]
    # Move legend to a cleaner spot
    ax.legend(handles=legend_elements, loc='upper left', bbox_to_anchor=(0.0, 0.9))

    # --- 8. Save ---
    print(f"Saving 3D plot to '{filename}'...")
    plt.savefig(filename, dpi=150, bbox_inches='tight')
    plt.close()
    print("Done.")

def save_heatmap_3D_fast(physical_map: np.ndarray, filename: str = "heatmap_3d.png", img_hb=0, img_v=0):
    """
    Saves a 3D Scatter plot showing exact memory usage.
    Uses ax.scatter instead of ax.voxels for significantly faster performance.
    """
    print("Preparing 3D Scatter data...")

    # 1. Setup Font
    plt.rcParams['font.family'] = 'serif'
    plt.rcParams['font.serif'] = ['Times New Roman'] + plt.rcParams['font.serif']

    # --- KEY CHANGE: Convert Grid to Coordinates ---
    # physical_map shape is (num_banks, num_rows, num_cols)
    # np.nonzero returns a tuple of arrays (indices) for all non-zero elements
    # These become our X, Y, Z coordinates
    banks, rows, cols = np.nonzero(physical_map)

    # If the map is empty, handle it safely
    if len(banks) == 0:
        print("Warning: Map is empty. Nothing to plot.")
        return

    # 2. Determine Colors based on values at those coordinates
    # Get the value (count) at each occupied coordinate
    values = physical_map[banks, rows, cols]

    # Create a color array matching the length of our points
    # 'lime' for single allocation (1), 'red' for collisions (>1)
    # Using a list comprehension or numpy where is fine here
    colors = np.where(values > 1, 'red', 'lime')
    
    # 3. Setup 3D Plot
    fig = plt.figure(figsize=(20, 20))
    ax = fig.add_subplot(111, projection='3d')

    print("Rendering 3D Scatter...")
    
    # 4. Plot using Scatter
    # We map physical_map dimensions to axes:
    # Axis 0 (Banks) -> X
    # Axis 2 (Cols)  -> Y (Swapped to match your original visual preference if needed, otherwise Y=Rows)
    # Let's match your original labels: X=Banks, Y=Cols, Z=Rows
    
    # Note: In your original code:
    # ax.set_xlabel('Bank') -> dim 0
    # ax.set_ylabel('Column') -> dim 2
    # ax.set_zlabel('Row') -> dim 1
    # So we pass (banks, cols, rows) to match that specific visual layout
    ax.scatter(banks, cols, rows, c=colors, marker='s', s=100, edgecolors='k', linewidth=0.3, alpha=0.6)

    # --- 5. Labels and Formatting ---
    num_banks, num_rows, num_cols = physical_map.shape
    
    ax.set_xlabel(f'Bank ID (0-{num_banks-1})', fontsize=16, labelpad=10)
    ax.set_ylabel(f'Column ID (0-{num_cols-1})', fontsize=16, labelpad=10)
    ax.set_zlabel(f'Row ID (0-{num_rows-1})', fontsize=16, labelpad=10)

    ax.set_xlim(0, num_banks)
    ax.set_ylim(0, num_cols)
    ax.set_zlim(0, num_rows)

    plt.title(f"3D Physical Memory Layout & Collisions\nImage Size: {img_hb}x{img_v}", fontsize=20)

    ax.view_init(elev=30, azim=-60)

    # --- 6. Custom Legend ---
    legend_elements = [
        mpatches.Patch(color='lime', label='Allocation'),
        mpatches.Patch(color='red', label='Collision'),
    ]
    plt.legend(handles=legend_elements, loc='upper left', bbox_to_anchor=(0.0, 1.0), 
               title="Status", frameon=True, fontsize=16, title_fontsize=20)

    print(f"Saving 3D plot to '{filename}'...")
    plt.savefig(filename, dpi=600, bbox_inches='tight')
    plt.close()
    print("Done.")

def save_heatmap_2D(physical_map: np.ndarray, filename: str = "heatmap_legend.png", 
                    img_hb=0, img_v=0, quotient=0):
    """
    Saves a 2D heatmap by projecting the 3D memory map.
    
    Logic:
      - Red:   If ANY column at this (Bank, Row) has a collision (count > 1).
      - Green: If NO collision, but data exists (count > 0).
      - Black: Empty.
    """
    # Set Font
    plt.rcParams['font.family'] = 'serif'
    plt.rcParams['font.serif'] = ['Times New Roman'] + plt.rcParams['font.serif']

    num_banks, num_rows, num_cols = physical_map.shape
    
    # --- 1. Data Projection (3D -> 2D) ---
    # We need to flatten the 'Columns' dimension (axis 2).
    
    # A) Max Projection: Finds the highest count in the column stack.
    #    If max > 1, it means at least one column address collided.
    max_val_map = np.max(physical_map, axis=2)
    
    # B) Sum Projection: Checks if anything is written there at all.
    sum_val_map = np.sum(physical_map, axis=2)
    
    # Initialize 2D display map with 0 (Black/Empty)
    display_map = np.zeros((num_banks, num_rows))
    
    # Apply Logic:
    # 1. Mark Allocation (Green/1) where sum > 0
    display_map[sum_val_map > 0] = 1
    
    # 2. Mark Collision (Red/2) where max > 1 (Overrides Green)
    display_map[max_val_map > 1] = 2
    
    # --- 2. Define Colors ---
    # 0 -> Black, 1 -> Lime, 2 -> Red
    colors = ['black', 'lime', 'red']
    cmap = mcolors.ListedColormap(colors)

    # --- 3. Plotting ---
    plt.figure(figsize=(12, 12))
    
    # [cite_start]Transpose (.T) to put Banks on X-axis and Rows on Y-axis [cite: 31]
    # vmin=0, vmax=2 ensures strict mapping to our 3 colors
    plt.imshow(display_map.T, cmap=cmap, vmin=0, vmax=2, aspect='auto', interpolation='nearest')
    
    # --- 4. Decoration ---
    plt.suptitle(f"Physical Memory Layout (2D Projection)\nBanks: {num_banks}, Image Size: {img_hb}x{img_v}", fontsize=14)
    plt.xlabel(f"Bank ID (0 - {num_banks-1})", fontsize=12)
    plt.ylabel(f"Row ID (0 - {num_rows-1})", fontsize=12)
    
    # Often memory maps look better with Row 0 at the bottom, but 'imshow' puts 0 at top.
    # Use origin='lower' in imshow or invert_yaxis:
    plt.gca().invert_yaxis() 

    # --- 5. Custom Legend ---
    legend_elements = [
        mpatches.Patch(color='black', label='Empty'),
        mpatches.Patch(color='lime',  label='Allocation'),
        mpatches.Patch(color='red',   label='Collision')
    ]
    
    # Legend placement
    plt.legend(handles=legend_elements, loc='upper left', bbox_to_anchor=(1.02, 1), 
               title="Status", fontsize=12, title_fontsize=14, frameon=True)
    
    plt.tight_layout()

    # --- 6. Save ---
    print(f"Saving 2D heatmap to '{filename}'...")
    plt.savefig(filename, dpi=150, bbox_inches='tight')
    plt.close()
    print("Done.")

def qualifying_matrix(matrix_tuple: Tuple[np.ndarray, np.ndarray]) -> float:
    row_ids, bank_ids, _ = matrix_tuple
    return qualifying_matrix_optimized(row_ids, bank_ids, img_v, math.ceil(img_hb / 16), num_banks, banks_per_group)
    #return qualifying_matrix_optimized(row_ids, bank_ids, img_v, math.ceil(img_hb / 16), 2, banks_per_group)

def analyze_memory_fragmentation(matrix_tuple: Tuple[np.ndarray, np.ndarray], num_banks: int, num_cols: int) -> float:
    """
    Analyzes the percentage of empty space (fragmentation) within the active memory region.
    
    The 'Active Region' is the total physical memory block required to store the image, 
    defined from Row 0 up to the maximum Row ID accessed by the proposal.
    
    Formula: 
        Total Capacity = (Max Row ID + 1) * Num Banks * Num Cols
        Empty Space %  = (1 - (Unique Occupied Cells / Total Capacity)) * 100
    """
    row_ids, bank_ids, col_ids = matrix_tuple

    # 1. Flatten arrays to process all transactions
    # Flatten logic similar to check_and_print_overlaps
    flat_rows = row_ids.flatten().astype(np.int64)
    flat_banks = bank_ids.flatten().astype(np.int64)
    flat_cols = col_ids.flatten().astype(np.int64)

    if len(flat_rows) == 0:
        print("[Analysis] No transactions found (Empty Input).")
        return 100.0

    # 2. Determine the boundary of allocated memory (The "Max Value")
    # We assume the memory footprint extends to the highest row index used.
    max_row_accessed = np.max(flat_rows)
    max_bank_accessed = np.max(flat_banks)
    max_col_accessed = np.max(flat_cols)
    
    # Total physical cells available in the region [0 ... max_row_accessed]
    # Rows are 0-indexed, so count is max_row + 1
    total_capacity_slots = (max_row_accessed + 1) * (max_bank_accessed + 1) * (max_col_accessed + 1)

    # 3. Determine actual occupancy
    # We use np.unique to count unique (Row, Bank, Col) coordinates.
    # This ensures that if 'collisions' occur (overlapping writes), we don't 
    # count the same physical cell twice as 'occupied'.
    # Using axis=0 to look at coordinate pairs
    coordinates = np.stack((flat_rows, flat_banks, flat_cols), axis=1)
    unique_coordinates = np.unique(coordinates, axis=0)
    occupied_slots = len(unique_coordinates)

    # 4. Calculate Empty Space
    empty_slots = total_capacity_slots - occupied_slots
    empty_percentage = (empty_slots / total_capacity_slots) * 100.0

    # 5. Output Report
    print(f"\n[Memory Fragmentation Analysis]")
    print(f"{'Metric':<25} | {'Value'}")
    print("-" * 40)
    print(f"{'Max Row Index':<25} | {max_row_accessed}")
    print(f"{'Active Banks':<25} | {max_bank_accessed}")
    print(f"{'Cols per Bank':<25} | {max_col_accessed}")
    print("-" * 40)
    print(f"{'Total Window Capacity':<25} | {total_capacity_slots:,} slots")
    print(f"{'Occupied Slots':<25} | {occupied_slots:,} slots")
    print(f"{'Empty/Wasted Slots':<25} | {empty_slots:,} slots")
    print("-" * 40)
    print(f"{'Empty Space %':<25} | {empty_percentage:.4f}%")

    return empty_percentage

def print_matrix_as_image_layout(matrix_tuple: Tuple[np.ndarray, np.ndarray], img_hb: int, img_v: int):
    """
    Reshapes the 1D transaction arrays into 2D grids matching the image structure
    and prints them. Handles float-to-int conversion safely.
    """
    row_ids, bank_ids, col_ids = matrix_tuple

    # --- FIX: Explicitly cast to integer types ---
    # This resolves the "Unknown format code 'd'" error
    row_ids = row_ids.astype(int)
    bank_ids = bank_ids.astype(int)
    col_ids = col_ids.astype(int)
    # ---------------------------------------------

    # 1. Determine dimensions
    reshaped_rows = row_ids.reshape((img_v, -1))
    reshaped_banks = bank_ids.reshape((img_v, -1))
    reshaped_cols = col_ids.reshape((img_v, -1))

    rows, cols = reshaped_rows.shape
    print(f"\n[Visualizing Memory Layout as Image: {rows} rows x {cols} transaction-cols]")

    ## 2. Print Bank IDs Grid
    #print("\n--- Bank ID Layout (img_v x img_hb//16) ---")
    #for r in range(rows):
    #    # Now that reshaped_banks contains ints, :2d will work perfectly
    #    line = " ".join(f"{val:2d}" for val in reshaped_banks[r])
    #    print(f"Row {r:3d}: [ {line} ]")

    # 3. Print Combined Layout (Row, Bank, Col)
    print("\n--- Combined Layout (Row, Bank, Col) ---")
    for r in range(rows):
        line_items = []
        #for c in range(cols):
        for c in range(0, cols, 32):
            item = f"({reshaped_rows[r,c]},{reshaped_banks[r,c]},{reshaped_cols[r,c]})"
            line_items.append(item)
        print(f"Row {r:3d}: " + "  ".join(line_items))

def final_result():
    key = f"{num_banks}_{img_hb}_{img_v}_{tile_size}"
    schemes = [
        ("LIAM", generate_bankgroup_interleaving_matrix_liam),
        ("BFAM", generate_bankgroup_interleaving_matrix_bfam),
        ("BGFAM", generate_bankgroup_interleaving_matrix_bgfam),
        ("Proposal", generate_bankgroup_interleaving_matrix_proposal),
    ]

    phy_map         = generate_bankgroup_interleaving_matrix_proposal(num_bankgroups, img_hb, img_v)
    phy_map_liam    = generate_bankgroup_interleaving_matrix_liam(img_hb)
    phy_map_bfam    = generate_bankgroup_interleaving_matrix_bfam(img_hb)
    phy_map_bgfam   = generate_bankgroup_interleaving_matrix_bgfam(num_bankgroups, img_hb)

    liam_rows, liam_banks, _ = generate_bankgroup_interleaving_matrix_liam(img_hb)
    
    liam_conflicts = get_col_major_conflicts(
        liam_rows.flatten(), 
        liam_banks.flatten(), 
        img_v, 
        (img_hb // 16), 
        num_banks
    )
    print(f"LIAM Page Conflicts: {liam_conflicts:,}")

    # 2. Analyze BFAM
    # (Assuming generate_bankgroup_interleaving_matrix_bfam is available in your scope)
    print("Generating BFAM...")
    bfam_rows, bfam_banks, _ = generate_bankgroup_interleaving_matrix_bfam(img_hb)
    
    bfam_conflicts = get_col_major_conflicts(
        bfam_rows.flatten(), 
        bfam_banks.flatten(), 
        img_v, 
        (img_hb // 16), 
        num_banks
    )
    print(f"BFAM Page Conflicts: {bfam_conflicts:,}")

    print("Matrix Quality  | Proposal\t| LIAM\t| BFAM\t| BGFAM")
    print(f"Quality Results | {qualifying_matrix(phy_map):.2f}\t\t| {qualifying_matrix(phy_map_liam):.2f}\t| {qualifying_matrix(phy_map_bfam):.2f}\t| {qualifying_matrix(phy_map_bgfam):.2f}")

    #print_matrix_as_image_layout(phy_map_liam, img_hb, img_v)
    #print_matrix(generate_bankgroup_interleaving_matrix_bfam(num_bankgroups, img_hb))
    #print_matrix(generate_bankgroup_interleaving_matrix_bgfam(num_bankgroups, img_hb))
    #print_matrix_as_image_layout(phy_map, img_hb, img_v)

    # Check for Overlaps
    check_and_print_overlaps(phy_map)

    analyze_memory_fragmentation(phy_map, num_banks, num_cols)

    # Check for HeatMap
    #phy_map = map_proposal_to_physical_memory(generate_bankgroup_interleaving_matrix_proposal(num_bankgroups, img_hb, img_v, tile_size), num_banks=num_banks, num_rows=img_v)
    #save_heatmap_3D(phy_map, filename=f"./heatmap_3d/heatmap_{key}.png", img_hb=img_hb, img_v=img_v)
    #save_heatmap_2D(phy_map, filename=f"./heatmap_2d/heatmap_{key}.png", img_hb=img_hb, img_v=img_v)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Script qualifying interleaving quality.")
    parser.add_argument("--num_banks", required=True, type=int)
    parser.add_argument("--img_hb",    required=True, type=int)
    parser.add_argument("--img_v",     required=True, type=int)
    parser.add_argument("--tile_size", default=0,     type=int)
    args = parser.parse_args()

    #if not (16 <= args.num_banks <= 64):
    #    print("Error: The number of banks must be from 16 to 64.")
    #    exit(1)
    if not ((args.num_banks /banks_per_group).is_integer()):
        print(f"Error: The number of banks ({args.num_banks}) must be a multiple of banks per group ({banks_per_group}).")
        exit(1) 

    num_banks = args.num_banks
    img_hb     = args.img_hb
    img_v      = args.img_v
    tile_size  = args.tile_size

    quotient     = (img_hb * 4.0)    / (num_banks * page_size)

    #num_bankgroups          = (num_banks // banks_per_group) if (num_banks != 48) else 8
    num_bankgroups          = (num_banks // banks_per_group)
    super_page_size         = page_size * num_banks
    k                       = 1 if (round((img_hb * bytes_per_pixel) / super_page_size) == 0) else round((img_hb * bytes_per_pixel) / super_page_size)
    print(f"num_banks: {num_banks}, img_hb: {img_hb}, img_v: {img_v}, num_bankgroups: {num_bankgroups}, super_page_size: {super_page_size}, k: {k}")

    final_result()
