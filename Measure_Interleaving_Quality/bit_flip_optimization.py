#!/usr/bin/python3

import math
import argparse
import numpy as np
from itertools import combinations
from numba import njit, prange

# --- Configuration ---
banks_per_group = 4
col_num = 32
row_bits = 13
col_bits = 5
bank_bits = 0
bytes_per_pixel = 4 
page_size = 2048

# ==========================================
# 1. Trace Generation (Vectorized & Memory Optimized)
# ==========================================
def calculate_pixel_values(img_v: int, img_hb: int, num_banks: int):
    total_transactions = img_hb * img_v
    transactions = np.arange(total_transactions, dtype=np.uint32)
    
    div_col = transactions // col_num
    ri = (div_col // num_banks).astype(np.uint16)
    bi = (div_col % num_banks).astype(np.uint8)
    ci = (transactions % col_num).astype(np.uint8)
    
    return ri, bi, ci

#def reconstruct_address(r, b, c):
#    """
#    Helper to apply bit-shifting logic identical to source lines 4-6.
#    """
#    shift_bank = col_bits # col_bits = 5 
#    # Calculate row shift based on max bank index 
#    shift_row = int(math.log2(max(b) + 1)) + col_bits 
#    
#    if shift_row < (bank_bits + col_bits): 
#        shift_row = bank_bits + col_bits 
#
#    # Reconstruct Physical Address 
#    full_address = (r.astype(np.uint32) << shift_row) | \
#                   (b.astype(np.uint32) << shift_bank) | \
#                   c.astype(np.uint32)
#                   
#    return full_address
@njit(fastmath=True)
def reconstruct_address(r, b, c):
    """Numba-accelerated address reconstruction."""
    n = len(r)
    shift_bank = col_bits
    # Calculate row shift based on max bank index 
    shift_row = int(math.log2(max(b) + 1)) + col_bits 
    res = np.empty(n, dtype=np.uint32)
    for i in range(n):
        res[i] = (np.uint32(r[i]) << shift_row) | (np.uint32(b[i]) << shift_bank) | np.uint32(c[i])
    return res

def get_flat_trace(ri, bi, ci, img_v, img_hb, row_major=False, col_major=False):
    if row_major:
        # 1. Row-Major Trace
        r = ri
        b = bi
        c = ci

    if col_major:
        # Generate Column-Major indices
        r_p = ri.reshape(img_v, img_hb).flatten(order='F')
        b_p = bi.reshape(img_v, img_hb).flatten(order='F')
        c_p = ci.reshape(img_v, img_hb).flatten(order='F')
        
        r = np.concatenate((r, r_p)) if row_major else r_p
        b = np.concatenate((b, b_p)) if row_major else b_p
        c = np.concatenate((c, c_p)) if row_major else c_p

    # Reconstruct Physical Address
    shift_bank = col_bits
    shift_row = int(math.log2(max(b) + 1)) + col_bits 
    if shift_row < (bank_bits + col_bits): 
        shift_row = bank_bits + col_bits 

    full_address = (r.astype(np.uint32) << shift_row) | \
                   (b.astype(np.uint32) << shift_bank) | \
                   c.astype(np.uint32)
                   
    return full_address

#def get_tile_trace(ri, bi, ci, img_v, img_hb, tile_h, tile_w, include_bank_col=False):
#    """
#    Generates a trace for memory accessed in tiles.
#    Pads the image with 0 if dimensions are not divisible by tile size.
#    """
#    # Calculate padding needed to make dimensions divisible by tile size
#    pad_v = (tile_h - (img_v % tile_h)) % tile_h
#    pad_hb = (tile_w - (img_hb % tile_w)) % tile_w
#
#    # Reshape to 2D for padding, then apply padding (constant 0)
#    r_2d = ri.reshape(img_v, img_hb)
#    r_padded = np.pad(r_2d, ((0, pad_v), (0, pad_hb)), mode='constant', constant_values=0)
#    
#    new_v, new_hb = r_padded.shape
#
#    # Reshape into 4D: (num_tiles_v, tile_h, num_tiles_hb, tile_w)
#    r_view = r_padded.reshape(new_v // tile_h, tile_h, new_hb // tile_w, tile_w)
#    r = r_view.transpose(0, 2, 1, 3).flatten().astype(np.uint32)
#
#    if not include_bank_col:
#        return r
#
#    # Repeat for Bank and Column indices if full address is needed
#    b_2d = bi.reshape(img_v, img_hb)
#    c_2d = ci.reshape(img_v, img_hb)
#    
#    b_padded = np.pad(b_2d, ((0, pad_v), (0, pad_hb)), mode='constant', constant_values=0)
#    c_padded = np.pad(c_2d, ((0, pad_v), (0, pad_hb)), mode='constant', constant_values=0)
#
#    b_view = b_padded.reshape(new_v // tile_h, tile_h, new_hb // tile_w, tile_w)
#    c_view = c_padded.reshape(new_v // tile_h, tile_h, new_hb // tile_w, tile_w)
#    
#    b = b_view.transpose(0, 2, 1, 3).flatten().astype(np.uint32)
#    c = c_view.transpose(0, 2, 1, 3).flatten().astype(np.uint32)
#
#    return reconstruct_address(r, b, c)

@njit(parallel=True, fastmath=True)
def get_tile_trace(ri, bi, ci, img_v, img_hb, tile_h, tile_w):
    """
    Generates a tiled trace with address reconstruction matching get_flat_trace.
    """
    # 1. Calculate padded dimensions 
    new_v = ((img_v + tile_h - 1) // tile_h) * tile_h
    new_hb = ((img_hb + tile_w - 1) // tile_w) * tile_w
    total_elements = new_v * new_hb
    
    # 2. Setup Address Reconstruction Parameters
    shift_bank = col_bits # Fixed at 5
    # Calculate row shift based on max bank index
    max_b = 0
    for val in bi:
        if val > max_b: max_b = val
    shift_row = int(math.log2(max_b + 1)) + col_bits 
    
    if shift_row < (bank_bits + col_bits): 
        shift_row = bank_bits + col_bits 

    full_address = np.empty(total_elements, dtype=np.uint32)
    
    # 3. Tiled Access Pattern
    for tv in prange(new_v // tile_h):
        for thb in range(new_hb // tile_w):
            for i in range(tile_h):
                for j in range(tile_w):
                    curr_v = tv * tile_h + i
                    curr_hb = thb * tile_w + j
                    
                    # Target index in the output trace
                    out_idx = (tv * (new_hb // tile_w) * tile_h * tile_w) + \
                              (thb * tile_h * tile_w) + (i * tile_w) + j
                    
                    if curr_v < img_v and curr_hb < img_hb:
                        # Map back to the original index in ri, bi, ci 
                        flat_idx = curr_v * img_hb + curr_hb
                        
                        # Reconstruct Physical Address
                        r_val = np.uint32(ri[flat_idx])
                        b_val = np.uint32(bi[flat_idx])
                        c_val = np.uint32(ci[flat_idx])
                        
                        full_address[out_idx] = (r_val << shift_row) | (b_val << shift_bank) | c_val
                    else:
                        full_address[out_idx] = 0 # Padding 
    return full_address

def get_strided_trace(ri, bi, ci, stride):
    """
    Generates a trace by picking every Nth pixel (stride).
    """
    # Select every 'stride' element from the flattened arrays
    r = ri[::stride]
    b = bi[::stride]
    c = ci[::stride]

    return reconstruct_address(r, b, c)

def get_diff_histogram(trace):
    diffs = trace[:-1] ^ trace[1:]
    diffs = diffs[diffs != 0]
    return np.unique(diffs, return_counts=True)

# ==========================================
# 2. Fast Numba Kernels
# ==========================================

@njit(fastmath=True)
def calc_union_size_numba(unique_diffs, counts, mask):
    total = 0
    for i in range(len(unique_diffs)):
        if (unique_diffs[i] & mask) != 0:
            total += counts[i]
    return total

@njit(fastmath=True)
def _calc_block_score(block):
    """
    Helper: Sorts and counts uniques in a block.
    Extracted to prevent Numba 'unexpected cycle' errors.
    """
    block.sort()
    length = len(block)
    if length == 0:
        return 0
    unique_count = 1
    for j in range(1, length):
        if block[j] != block[j-1]:
            unique_count += 1
    return length - unique_count

@njit(parallel=True, fastmath=True)
def find_best_mask_batch(trace, masks, window_size):
    """
    Parallel Batch Solver:
    Evaluates ALL masks in parallel. Each thread takes a mask 
    and scans the trace to compute its score.
    """
    n_masks = len(masks)
    n_trace = len(trace)
    num_blocks = n_trace // window_size
    
    if num_blocks == 0:
        return 0, 0
    
    scores = np.zeros(n_masks, dtype=np.uint64)
    
    # Parallelize over the MASKS (Search Space)
    for m in prange(n_masks):
        mask = masks[m]
        total_score = 0
        
        # Scan trace sequentially for this mask
        # (This is safe inside a parallel loop)
        for i in range(num_blocks):
            start = i * window_size
            end = start + window_size
            
            # Extract and mask
            block = trace[start:end] & mask
            
            # Calculate score
            total_score += _calc_block_score(block)
            
        scores[m] = total_score
        
    # Find best result
    best_idx = 0
    min_score = scores[0]
    for m in range(1, n_masks):
        if scores[m] < min_score:
            min_score = scores[m]
            best_idx = m
            
    return best_idx, min_score

# ==========================================
# 3. Solvers
# ==========================================

def solve_min_k_union(unique_diffs, counts, num_bits, k):
    min_size = float('inf')
    best_combo = None
    bit_masks = np.array([1 << i for i in range(num_bits)], dtype=np.uint32)
    
    for combo in combinations(range(num_bits), k):
        mask = 0
        for bit in combo:
            mask |= bit_masks[bit]
        current_size = calc_union_size_numba(unique_diffs, counts, mask)
        if current_size < min_size:
            min_size = current_size
            best_combo = combo
    return best_combo, min_size

def get_high_entropy_bits(unique_diffs, counts, total_bits):
    entropy = []
    for bit in range(total_bits):
        mask = 1 << bit
        flips = calc_union_size_numba(unique_diffs, counts, mask)
        entropy.append((bit, flips))
    entropy.sort(key=lambda x: x[1], reverse=True)
    return [x[0] for x in entropy]

def solve_near_optimal_mapping(trace, num_bg, num_bk, num_col, total_width):
    n_total = num_bg + num_bk + num_col
    
    # 1. Entropy Pre-calc
    udiffs, ucounts = get_diff_histogram(trace)
    
    #print(f"Selecting top {n_total} high-entropy bits...")
    pool = get_high_entropy_bits(udiffs, ucounts, total_width)[:n_total]
    remaining = set(pool)
    mapping = {}
    
    # 2. Assign Columns (Highest Entropy)
    #print("Assigning Columns (Max Entropy)...")
    col_bits = [b for b in pool if b in remaining][:num_col]
    mapping['Column'] = col_bits
    remaining -= set(col_bits)
    
    # 3. Assign Bank Groups (Min Repetitive) - PARALLELIZED
    if num_bg > 0:
        #print("Optimizing Bank Groups (Min Repetitive)...")
        bg_win = 1 << num_bg
        
        # A. Generate ALL combinations
        combos = list(combinations(remaining, num_bg))
        masks_list = []
        for combo in combos:
            mask = 0
            for b in combo: mask |= (1 << b)
            masks_list.append(mask)
        masks_array = np.array(masks_list, dtype=np.uint32)
        
        # B. Solve all in parallel
        best_idx, _ = find_best_mask_batch(trace, masks_array, bg_win)
        
        best_combo = combos[best_idx]
        mapping['BankGroup'] = list(best_combo)
        remaining -= set(best_combo)
    else:
        mapping['BankGroup'] = []
        
    # 4. Assign Banks (Min Repetitive) - PARALLELIZED
    if num_bk > 0:
        #print("Optimizing Banks (Min Repetitive)...")
        bk_win = 1 << num_bk
        
        # A. Generate ALL combinations (from remaining bits)
        combos = list(combinations(remaining, num_bk))
        masks_list = []
        for combo in combos:
            mask = 0
            for b in combo: mask |= (1 << b)
            masks_list.append(mask)
        masks_array = np.array(masks_list, dtype=np.uint32)
        
        # B. Solve all in parallel
        best_idx, _ = find_best_mask_batch(trace, masks_array, bk_win)
        
        best_combo = combos[best_idx]
        mapping['Bank'] = list(best_combo)
        remaining -= set(best_combo)
    else:
        mapping['Bank'] = []
    
    # 5. Assign Rows
    all_bits = set(range(total_width))
    mapped_bits = set(mapping['Column'] + mapping['BankGroup'] + mapping['Bank'])
    mapping['Row'] = list(all_bits - mapped_bits)
        
    return mapping

# ==========================================
# Main
# ==========================================
def final_result(args):
    num_banks = args.num_banks
    img_hb = args.img_hb
    img_v = args.img_v

    img_hb = img_hb * bytes_per_pixel // 64 # 64-byte per transaction
    
    # Configuration for patterns
    # These will be iterated through for Tile and Strided modes
    t_sizes = [16, 32, 64, 128]
    stride_vals = [16, 32, 64, 128]
    
    # 1. Generate base indices 
    ri, bi, ci = calculate_pixel_values(img_v, img_hb, num_banks)
    #for r_idx in range(0, len(ri), 512):
    #    print(f"ri[{r_idx}] = {ri[r_idx]}")
    #print(f"bi = {bi}")
    #print(f"ci = {ci}")
    
    total_bank_bits = int(math.log2(num_banks))
    n_bk = 4
    n_bg = total_bank_bits - n_bk


    # 2. Define patterns to evaluate
    patterns = ["Row-Major", "Col-Major", "Both-Major", "Tile", "Strided"]
    #patterns = ["Tile", "Strided"]
    #patterns = ["Row-Major", "Col-Major", "Both-Major"]

    for name in patterns:
        print(f"\n{'='*20} {name} Analysis {'='*20}")

        # Determine which values to iterate over for this pattern
        current_iterations = [None] # Default for non-parameterized patterns
        if name == "Tile":
            current_iterations = t_sizes
        elif name == "Strided":
            current_iterations = stride_vals

        for val in current_iterations:
            # --- A. Generate Trace (Row Only for Min-K-Union, Full for Mapping) ---
            if name == "Row-Major":
                full_trace = get_flat_trace(ri, bi, ci, img_v, img_hb, row_major=True, col_major=False)
            elif name == "Col-Major":
                full_trace = get_flat_trace(ri, bi, ci, img_v, img_hb, row_major=False, col_major=True)
            elif name == "Both-Major":
                full_trace = get_flat_trace(ri, bi, ci, img_v, img_hb, row_major=True, col_major=True)
            elif name == "Tile":
                base_trace = get_flat_trace(ri, bi, ci, img_v, img_hb, row_major=True, col_major=False)
                tiled_trace = get_tile_trace(ri, bi, ci, img_v, img_hb, val, val)
                full_trace = np.concatenate((base_trace, tiled_trace))
            elif name == "Strided":
                base_trace = get_flat_trace(ri, bi, ci, img_v, img_hb, row_major=True, col_major=False)
                strided_trace = get_strided_trace(ri, bi, ci, val)
                full_trace = np.concatenate((base_trace, strided_trace))

            # --- B. Solve Min-K-Union (Requires Row Trace Only) ---
            udiffs, ucounts = get_diff_histogram(full_trace)
            best_bits, misses = solve_min_k_union(udiffs, ucounts, (row_bits + col_bits + total_bank_bits), row_bits)
            
            # --- C. Solve Near-Optimal Mapping (Requires Full Trace) ---
            mapping = solve_near_optimal_mapping(
                full_trace, 
                num_bg=n_bg, 
                num_bk=n_bk, 
                num_col=col_bits, 
                total_width=(row_bits + col_bits + total_bank_bits)
            )

            # --- D. Unified Print Logic ---
            param_str = f"[{val}]" if val is not None else ""
            print(f"\n--- Sub-Pattern: {name}{param_str} ---")
            print(f"Min-K-Union:\tRow Bits={best_bits} - Misses={misses}")
            print(f"Near-Optimal:\tCol={mapping['Column']} - BG={mapping['BankGroup']} - Bank={mapping['Bank']} - Row={mapping['Row']}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--num_banks", required=True, type=int)
    parser.add_argument("--img_hb",    required=True, type=int)
    parser.add_argument("--img_v",     required=True, type=int)
    args = parser.parse_args()
    
    print("=" * 150)
    print(f"num_banks: {args.num_banks}, img_hb: {args.img_hb}, img_v: {args.img_v}")
    final_result(args)