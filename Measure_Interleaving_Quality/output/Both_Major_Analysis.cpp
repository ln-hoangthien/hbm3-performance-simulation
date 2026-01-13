// Both-Major Analysis - Complete Auto-generated bit order configuration
// Generated from BipFlip Result analysis - ALL {len(configs)} IMAGE SIZES
// Contains BOTH Near-Optimal and Min-K-Union strategies
//
// Near-Optimal Format: bit_order = [High bits (31..n), Row (reversed), BG (reversed), Bank (reversed), Col (reversed)]
// Min-K-Union Format: bit_order = [High bits (31..n), Row (reversed)]
//
// COMPLETE CONFIGURATION FOR ALL BANK_NUM VALUES (16, 32, 48, 64)

// ========== BANK_NUM = 16 ==========
if (BANK_NUM == 16) {

    if (IMG_HORIZONTAL_SIZE == 512 && IMG_VERTICAL_SIZE == 64) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 4, 9, 8, 3, 2, 7, 1, 6, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 4, 10, 9, 3, 2, 8, 1, 7, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 4, 9, 8, 3, 2, 1, 6, 7, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 6, 5, 4, 11, 10, 3, 2, 9, 1, 8, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 10, 9, 2, 1, 6, 8, 7, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 4, 10, 9, 3, 2, 1, 7, 8, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 10, 9, 7, 2, 1, 6, 8, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 6, 5, 4, 12, 11, 3, 2, 10, 1, 9, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 10, 7, 2, 1, 6, 9, 8, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 3, 11, 10, 2, 1, 7, 9, 8, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 10, 6, 2, 1, 8, 9, 7, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 6, 5, 4, 11, 10, 3, 2, 1, 8, 9, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 10, 8, 2, 1, 6, 7, 9, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 3, 11, 10, 8, 2, 1, 7, 9, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 8, 4, 3, 11, 10, 7, 2, 1, 6, 9, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 8, 7, 6, 5, 4, 13, 12, 3, 2, 11, 1, 10, 0, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 8, 4, 3, 11, 7, 2, 1, 6, 10, 9, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 3, 11, 8, 2, 1, 7, 10, 9, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 2, 11, 8, 6, 1, 10, 7, 9, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 3, 12, 11, 2, 1, 8, 10, 9, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 2, 11, 8, 6, 1, 10, 9, 7, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 3, 11, 7, 2, 1, 9, 10, 8, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 2, 11, 7, 6, 1, 9, 10, 8, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 6, 5, 4, 12, 11, 3, 2, 1, 9, 10, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 2, 11, 9, 7, 1, 6, 10, 8, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 3, 11, 9, 2, 1, 7, 8, 10, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 2, 11, 9, 6, 1, 8, 7, 10, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 3, 12, 11, 9, 2, 1, 8, 10, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 9, 4, 3, 11, 8, 2, 1, 6, 7, 10, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 5, 4, 3, 12, 11, 8, 2, 1, 7, 10, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 8, 4, 3, 12, 11, 7, 2, 1, 6, 10, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 9, 8, 7, 6, 5, 4, 14, 13, 3, 2, 12, 1, 11, 0, 10};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }


// ========== BANK_NUM = 32 ==========
} else if (BANK_NUM == 32) {

    if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 5, 2, 10, 9, 4, 3, 8, 1, 7, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 6, 5, 2, 11, 10, 4, 3, 9, 1, 8, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 9, 11, 10, 3, 2, 1, 7, 8, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 7, 6, 5, 2, 12, 11, 4, 3, 10, 1, 9, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 10, 11, 3, 2, 1, 7, 9, 8, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 10, 12, 11, 3, 2, 1, 8, 9, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 10, 11, 8, 3, 2, 1, 7, 9, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 8, 7, 6, 5, 2, 13, 12, 4, 3, 11, 1, 10, 0, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 8, 2, 1, 7, 10, 9, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 11, 12, 3, 2, 1, 8, 10, 9, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 11, 2, 1, 9, 10, 8, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 4, 11, 13, 12, 3, 2, 1, 9, 10, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 9, 2, 1, 7, 8, 10, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 11, 12, 9, 3, 2, 1, 8, 10, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 9, 8, 2, 1, 7, 10, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 9, 8, 7, 6, 5, 2, 14, 13, 4, 3, 12, 1, 11, 0, 10};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 9, 5, 4, 3, 12, 13, 8, 2, 1, 7, 11, 10, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 3, 12, 13, 9, 2, 1, 8, 11, 10, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 9, 2, 1, 11, 8, 10, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 4, 12, 13, 3, 2, 1, 9, 11, 10, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 9, 12, 7, 2, 1, 11, 10, 8, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 3, 8, 13, 12, 2, 1, 10, 11, 9, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 8, 2, 1, 10, 11, 9, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 8, 7, 6, 5, 4, 12, 14, 13, 3, 2, 1, 10, 11, 0, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 10, 12, 8, 2, 1, 7, 11, 9, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 3, 12, 13, 10, 2, 1, 8, 9, 11, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 10, 2, 1, 9, 8, 11, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 4, 12, 13, 10, 3, 2, 1, 9, 11, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 2, 1, 7, 8, 11, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 3, 12, 13, 10, 9, 2, 1, 8, 11, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 8, 2, 1, 7, 11, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 10, 9, 8, 7, 6, 5, 2, 15, 14, 4, 3, 13, 1, 12, 0, 11};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }


// ========== BANK_NUM = 48 ==========
} else if (BANK_NUM == 48) {

    if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 8, 10, 9, 3, 2, 1, 6, 7, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 9, 11, 10, 3, 2, 1, 7, 8, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 9, 10, 7, 3, 2, 1, 6, 8, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 10, 12, 11, 3, 2, 1, 9, 8, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 8, 7, 2, 1, 6, 9, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 5, 4, 10, 11, 8, 3, 2, 1, 7, 9, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 6, 11, 10, 2, 1, 8, 9, 7, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 4, 2, 13, 12, 11, 3, 1, 10, 9, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 10, 11, 6, 2, 1, 8, 9, 7, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 8, 2, 1, 7, 10, 9, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 4, 3, 11, 12, 7, 2, 1, 6, 9, 10, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 4, 11, 12, 3, 2, 1, 9, 8, 10, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 4, 3, 11, 9, 7, 2, 1, 6, 10, 8, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 11, 12, 9, 2, 1, 7, 10, 8, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 4, 3, 11, 12, 8, 2, 1, 6, 10, 7, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 9, 8, 7, 6, 5, 2, 14, 13, 4, 3, 12, 1, 11, 10, 0};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 9, 4, 3, 6, 12, 8, 2, 1, 11, 10, 7, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 9, 2, 1, 11, 10, 8, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 6, 12, 9, 7, 1, 11, 10, 8, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 3, 9, 13, 12, 2, 1, 8, 11, 10, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 4, 3, 6, 12, 7, 2, 1, 9, 11, 10, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 7, 12, 8, 2, 1, 10, 9, 11, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 8, 12, 10, 6, 1, 9, 11, 7, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 4, 12, 13, 3, 2, 1, 10, 9, 11, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 4, 3, 2, 8, 12, 10, 6, 1, 9, 7, 11, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 12, 10, 8, 2, 1, 7, 9, 11, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 4, 3, 12, 10, 7, 2, 1, 6, 9, 11, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 3, 12, 13, 10, 2, 1, 9, 8, 11, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 10, 4, 3, 12, 9, 7, 2, 1, 6, 8, 11, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 5, 4, 3, 12, 13, 9, 2, 1, 7, 8, 11, 0, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 10, 9, 4, 3, 12, 13, 8, 2, 1, 6, 7, 11, 0, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 10, 9, 8, 7, 6, 5, 2, 15, 14, 4, 3, 13, 1, 12, 0, 11};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }


// ========== BANK_NUM = 64 ==========
} else if (BANK_NUM == 64) {

    if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 3, 2, 12, 11, 10, 4, 9, 1, 8, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 3, 2, 13, 12, 11, 4, 10, 1, 9, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 6, 5, 10, 2, 12, 11, 4, 3, 1, 8, 9, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 8, 7, 6, 5, 3, 2, 14, 13, 12, 4, 11, 1, 10, 0, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 11, 1, 13, 12, 3, 2, 8, 10, 9, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 7, 6, 5, 11, 2, 13, 12, 4, 3, 1, 9, 10, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 11, 9, 13, 12, 3, 2, 1, 8, 10, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 9, 8, 7, 6, 5, 3, 2, 15, 14, 13, 4, 12, 1, 11, 0, 10};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 12, 1, 13, 9, 3, 2, 8, 11, 10, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 6, 5, 4, 12, 1, 14, 13, 3, 2, 9, 11, 10, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 12, 8, 13, 3, 2, 1, 10, 11, 9, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 8, 7, 6, 5, 12, 2, 14, 13, 4, 3, 1, 10, 11, 0, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 12, 10, 13, 3, 2, 1, 8, 9, 11, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 6, 5, 4, 12, 10, 14, 13, 3, 2, 1, 9, 11, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 6, 5, 4, 12, 9, 13, 10, 3, 2, 1, 8, 11, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 10, 9, 8, 7, 6, 5, 3, 2, 16, 15, 14, 4, 13, 1, 12, 0, 11};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 13, 1, 14, 10, 9, 2, 8, 12, 11, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 6, 5, 4, 13, 1, 14, 10, 3, 2, 9, 12, 11, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 10, 8, 14, 13, 2, 1, 12, 9, 11, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 8, 7, 6, 5, 4, 13, 1, 15, 14, 3, 2, 10, 12, 11, 0, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 13, 10, 14, 8, 2, 1, 12, 11, 9, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 6, 5, 4, 13, 9, 14, 3, 2, 1, 11, 12, 10, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 13, 8, 14, 9, 2, 1, 11, 12, 10, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 9, 8, 7, 6, 5, 13, 2, 15, 14, 4, 3, 1, 11, 12, 0, 10};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 13, 11, 14, 9, 2, 1, 8, 12, 10, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 6, 5, 4, 13, 11, 14, 3, 2, 1, 9, 10, 12, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 13, 8, 14, 11, 2, 1, 10, 9, 12, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 8, 7, 6, 5, 4, 13, 11, 15, 14, 3, 2, 1, 10, 12, 0, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 13, 10, 14, 11, 2, 1, 8, 9, 12, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 7, 6, 5, 4, 13, 10, 14, 11, 3, 2, 1, 9, 12, 0, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 11, 6, 5, 4, 13, 9, 14, 10, 3, 2, 1, 8, 12, 0, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 11, 10, 9, 8, 7, 6, 5, 3, 2, 17, 16, 15, 4, 14, 1, 13, 0, 12};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
}
