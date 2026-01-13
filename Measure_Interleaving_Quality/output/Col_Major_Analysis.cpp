// Col-Major Analysis - Complete Auto-generated bit order configuration
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
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 4, 3, 10, 2, 1, 0, 9, 8, 7, 6, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 2, 12, 11, 1, 0, 10, 9, 8, 7, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 10, 9, 8, 6, 7, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 2, 1, 14, 13, 12, 0, 11, 10, 9, 8, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 2560 && IMG_VERTICAL_SIZE == 320) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 10, 9, 6, 8, 7, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 11, 10, 9, 7, 8, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 3584 && IMG_VERTICAL_SIZE == 448) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 10, 7, 9, 6, 8, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 14, 13, 12, 11, 10, 9, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 7, 10, 6, 9, 8, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 11, 10, 7, 9, 8, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 5632 && IMG_VERTICAL_SIZE == 704) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 10, 6, 8, 9, 7, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 8, 9, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 6656 && IMG_VERTICAL_SIZE == 832) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 8, 10, 6, 7, 9, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 11, 8, 10, 7, 9, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 8, 7, 10, 6, 9, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 8, 7, 6, 5, 4, 3, 2, 1, 0, 17, 16, 15, 14, 13, 12, 11, 10, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 8704 && IMG_VERTICAL_SIZE == 1088) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 8, 7, 11, 6, 10, 9, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 8, 11, 7, 10, 9, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 9728 && IMG_VERTICAL_SIZE == 1216) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 8, 6, 10, 7, 9, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 8, 10, 9, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 6, 8, 10, 9, 7, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 11, 7, 9, 10, 8, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 11776 && IMG_VERTICAL_SIZE == 1472) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 7, 6, 9, 10, 8, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 14, 13, 12, 11, 9, 10, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 12800 && IMG_VERTICAL_SIZE == 1600) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 7, 9, 6, 10, 8, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 9, 11, 7, 8, 10, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 11, 9, 6, 8, 7, 10, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 9, 11, 8, 10, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 14848 && IMG_VERTICAL_SIZE == 1856) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 9, 8, 11, 6, 7, 10, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 5, 4, 3, 2, 1, 0, 14, 13, 12, 9, 8, 11, 7, 10, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 15872 && IMG_VERTICAL_SIZE == 1984) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 0, 13, 12, 9, 8, 7, 11, 6, 10, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 18, 17, 16, 15, 14, 13, 12, 11, 10};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }


// ========== BANK_NUM = 32 ==========
} else if (BANK_NUM == 32) {

    if (IMG_HORIZONTAL_SIZE == 1024 && IMG_VERTICAL_SIZE == 128) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 5, 4, 3, 0, 12, 11, 2, 1, 10, 9, 8, 7, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 2, 0, 14, 13, 12, 1, 11, 10, 9, 8, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 11, 15, 14, 13, 12, 10, 9, 7, 8, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 14, 13, 12, 11, 10, 9, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 5120 && IMG_VERTICAL_SIZE == 640) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 11, 15, 14, 13, 12, 10, 7, 9, 8, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 11, 10, 8, 9, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 7168 && IMG_VERTICAL_SIZE == 896) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 11, 15, 14, 13, 12, 8, 10, 7, 9, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 8, 7, 6, 5, 4, 3, 2, 1, 0, 14, 18, 17, 16, 15, 13, 12, 11, 10, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 8, 15, 14, 13, 12, 11, 7, 10, 9, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 11, 8, 10, 9, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 11264 && IMG_VERTICAL_SIZE == 1408) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 11, 15, 14, 13, 12, 7, 9, 10, 8, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 7, 6, 5, 4, 3, 2, 1, 0, 13, 17, 16, 15, 14, 12, 11, 9, 10, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 13312 && IMG_VERTICAL_SIZE == 1664) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 9, 15, 14, 13, 12, 11, 7, 8, 10, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 9, 11, 8, 10, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 12, 15, 14, 13, 9, 8, 11, 7, 10, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 19, 18, 17, 16, 14, 13, 12, 11, 10};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 17408 && IMG_VERTICAL_SIZE == 2176) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 8, 15, 14, 13, 9, 12, 7, 11, 10, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 9, 16, 15, 14, 13, 12, 8, 11, 10, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 19456 && IMG_VERTICAL_SIZE == 2432) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 9, 15, 14, 13, 12, 7, 11, 8, 10, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 7, 6, 5, 4, 3, 2, 1, 0, 13, 17, 16, 15, 14, 12, 9, 11, 10, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 7, 15, 14, 13, 12, 9, 11, 10, 8, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 8, 10, 11, 9, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 23552 && IMG_VERTICAL_SIZE == 2944) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 12, 15, 14, 13, 8, 7, 10, 11, 9, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 8, 7, 6, 5, 4, 3, 2, 1, 0, 14, 18, 17, 16, 15, 13, 12, 10, 11, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 25600 && IMG_VERTICAL_SIZE == 3200) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 12, 15, 14, 13, 8, 10, 7, 11, 9, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 10, 16, 15, 14, 13, 12, 8, 9, 11, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 12, 15, 14, 13, 10, 7, 9, 8, 11, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 7, 6, 5, 4, 3, 2, 1, 0, 13, 17, 16, 15, 14, 10, 12, 9, 11, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 29696 && IMG_VERTICAL_SIZE == 3712) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 9, 15, 14, 13, 10, 12, 7, 8, 11, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 13, 16, 15, 14, 10, 9, 12, 8, 11, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 31744 && IMG_VERTICAL_SIZE == 3968) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 13, 15, 14, 10, 9, 8, 12, 7, 11, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 20, 19, 18, 17, 15, 14, 13, 12, 11};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }


// ========== BANK_NUM = 48 ==========
} else if (BANK_NUM == 48) {

    if (IMG_HORIZONTAL_SIZE == 1536 && IMG_VERTICAL_SIZE == 192) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 4, 3, 2, 1, 10, 13, 12, 11, 0, 9, 8, 6, 7, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 3072 && IMG_VERTICAL_SIZE == 384) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 11, 15, 14, 13, 12, 10, 9, 7, 8, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 4608 && IMG_VERTICAL_SIZE == 576) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 10, 14, 13, 12, 11, 7, 9, 6, 8, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 11, 10, 8, 9, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 7680 && IMG_VERTICAL_SIZE == 960) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 11, 14, 13, 12, 8, 7, 10, 6, 9, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 9216 && IMG_VERTICAL_SIZE == 1152) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 11, 15, 14, 13, 12, 8, 10, 7, 9, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 10752 && IMG_VERTICAL_SIZE == 1344) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 10, 14, 13, 12, 11, 6, 8, 9, 7, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 7, 6, 5, 4, 3, 2, 1, 0, 13, 17, 16, 15, 14, 12, 11, 10, 9, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 13824 && IMG_VERTICAL_SIZE == 1728) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 6, 14, 13, 12, 11, 10, 8, 9, 7, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 15360 && IMG_VERTICAL_SIZE == 1920) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 8, 15, 14, 13, 12, 11, 7, 10, 9, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 16896 && IMG_VERTICAL_SIZE == 2112) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 7, 14, 13, 12, 8, 11, 6, 9, 10, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 11, 9, 8, 10, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 19968 && IMG_VERTICAL_SIZE == 2496) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 14, 13, 12, 7, 11, 6, 10, 8, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 21504 && IMG_VERTICAL_SIZE == 2688) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 9, 15, 14, 13, 12, 11, 7, 10, 8, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 23040 && IMG_VERTICAL_SIZE == 2880) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 8, 14, 13, 12, 9, 11, 6, 10, 7, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 19, 18, 17, 16, 14, 13, 12, 11, 10};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 26112 && IMG_VERTICAL_SIZE == 3264) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 8, 14, 13, 12, 9, 6, 11, 10, 7, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 27648 && IMG_VERTICAL_SIZE == 3456) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 12, 15, 14, 13, 9, 7, 11, 10, 8, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 29184 && IMG_VERTICAL_SIZE == 3648) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 14, 13, 12, 7, 6, 11, 10, 8, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 12, 16, 15, 14, 13, 8, 9, 11, 10, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 32256 && IMG_VERTICAL_SIZE == 4032) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 12, 14, 13, 8, 7, 6, 9, 11, 10, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 33792 && IMG_VERTICAL_SIZE == 4224) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 12, 15, 14, 13, 8, 7, 10, 9, 11, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 35328 && IMG_VERTICAL_SIZE == 4416) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 10, 14, 13, 12, 6, 8, 9, 11, 7, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 7, 6, 5, 4, 3, 2, 1, 0, 13, 17, 16, 15, 14, 12, 10, 9, 11, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 38400 && IMG_VERTICAL_SIZE == 4800) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 6, 14, 13, 12, 10, 8, 9, 7, 11, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 39936 && IMG_VERTICAL_SIZE == 4992) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 10, 15, 14, 13, 8, 12, 7, 9, 11, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 41472 && IMG_VERTICAL_SIZE == 5184) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 10, 14, 13, 8, 7, 12, 6, 9, 11, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 6, 5, 4, 3, 2, 1, 0, 10, 16, 15, 14, 13, 12, 9, 8, 11, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 44544 && IMG_VERTICAL_SIZE == 5568) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 9, 14, 13, 10, 7, 12, 6, 8, 11, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 46080 && IMG_VERTICAL_SIZE == 5760) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 5, 4, 3, 2, 1, 0, 9, 15, 14, 13, 10, 12, 7, 8, 11, 6};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 47616 && IMG_VERTICAL_SIZE == 5952) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 4, 3, 2, 1, 0, 8, 14, 13, 10, 9, 12, 6, 7, 11, 5};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 20, 19, 18, 17, 15, 14, 13, 12, 11};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }


// ========== BANK_NUM = 64 ==========
} else if (BANK_NUM == 64) {

    if (IMG_HORIZONTAL_SIZE == 2048 && IMG_VERTICAL_SIZE == 256) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 6, 5, 4, 3, 1, 0, 14, 13, 12, 2, 11, 10, 9, 8, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 4096 && IMG_VERTICAL_SIZE == 512) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 14, 13, 12, 11, 10, 9, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 6144 && IMG_VERTICAL_SIZE == 768) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 12, 17, 16, 15, 14, 11, 10, 8, 9, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 8192 && IMG_VERTICAL_SIZE == 1024) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 8, 7, 6, 5, 4, 3, 2, 1, 14, 0, 18, 17, 16, 15, 13, 12, 11, 10, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 10240 && IMG_VERTICAL_SIZE == 1280) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 12, 17, 16, 15, 14, 11, 8, 10, 9, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 12288 && IMG_VERTICAL_SIZE == 1536) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 13, 18, 17, 16, 15, 12, 11, 9, 10, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 14336 && IMG_VERTICAL_SIZE == 1792) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 12, 17, 16, 15, 14, 9, 11, 8, 10, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 16384 && IMG_VERTICAL_SIZE == 2048) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 20, 19, 18, 17, 14, 13, 12, 11, 10};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 18432 && IMG_VERTICAL_SIZE == 2304) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 9, 17, 16, 15, 14, 12, 8, 11, 10, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 20480 && IMG_VERTICAL_SIZE == 2560) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 13, 18, 17, 16, 15, 12, 9, 11, 10, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 22528 && IMG_VERTICAL_SIZE == 2816) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 12, 17, 16, 15, 14, 8, 10, 11, 9, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 24576 && IMG_VERTICAL_SIZE == 3072) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 19, 18, 17, 16, 13, 12, 10, 11, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 26624 && IMG_VERTICAL_SIZE == 3328) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 10, 17, 16, 15, 14, 12, 8, 9, 11, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 28672 && IMG_VERTICAL_SIZE == 3584) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 13, 18, 17, 16, 15, 10, 12, 9, 11, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 30720 && IMG_VERTICAL_SIZE == 3840) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 10, 17, 16, 15, 14, 9, 12, 8, 11, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 32768 && IMG_VERTICAL_SIZE == 4096) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 17, 16, 21, 20, 19, 18, 15, 14, 13, 12, 11};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 34816 && IMG_VERTICAL_SIZE == 4352) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 14, 9, 17, 16, 15, 10, 13, 8, 12, 11, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 36864 && IMG_VERTICAL_SIZE == 4608) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 10, 18, 17, 16, 15, 13, 9, 12, 11, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 38912 && IMG_VERTICAL_SIZE == 4864) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 10, 17, 16, 15, 14, 8, 12, 9, 11, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 40960 && IMG_VERTICAL_SIZE == 5120) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 19, 18, 17, 16, 13, 10, 12, 11, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 43008 && IMG_VERTICAL_SIZE == 5376) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 8, 17, 16, 15, 14, 10, 12, 11, 9, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 45056 && IMG_VERTICAL_SIZE == 5632) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 13, 18, 17, 16, 15, 9, 11, 12, 10, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 47104 && IMG_VERTICAL_SIZE == 5888) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 9, 17, 16, 15, 14, 8, 11, 12, 10, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 49152 && IMG_VERTICAL_SIZE == 6144) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 15, 20, 19, 18, 17, 14, 13, 11, 12, 10};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 51200 && IMG_VERTICAL_SIZE == 6400) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 9, 17, 16, 15, 14, 11, 8, 12, 10, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 53248 && IMG_VERTICAL_SIZE == 6656) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 11, 18, 17, 16, 15, 13, 9, 10, 12, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 55296 && IMG_VERTICAL_SIZE == 6912) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 13, 11, 17, 16, 15, 14, 8, 10, 9, 12, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 57344 && IMG_VERTICAL_SIZE == 7168) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 19, 18, 17, 16, 11, 13, 10, 12, 9};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 59392 && IMG_VERTICAL_SIZE == 7424) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 14, 10, 17, 16, 15, 11, 13, 8, 9, 12, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 61440 && IMG_VERTICAL_SIZE == 7680) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 7, 6, 5, 4, 3, 2, 1, 0, 14, 11, 18, 17, 16, 15, 10, 13, 9, 12, 8};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 63488 && IMG_VERTICAL_SIZE == 7936) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 6, 5, 4, 3, 2, 1, 0, 14, 10, 17, 16, 15, 11, 9, 13, 8, 12, 7};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
    if (IMG_HORIZONTAL_SIZE == 65536 && IMG_VERTICAL_SIZE == 8192) {
        // Near-Optimal
        bit_order = {31, 30, 29, 28, 27, 26, 25, 24, 23, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 18, 17, 22, 21, 20, 19, 16, 15, 14, 13, 12};
        // Min-K-Union (alternative strategy)
        // bit_order_min_k_union = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
}
