#include <stdio.h>

void main() {
  int WIDTH = 10;
  int HIGH = 10;
  int KERNEL_SIZE = 3; // always odd

  int A[WIDTH][HIGH];
  int K[KERNEL_SIZE][KERNEL_SIZE];
  int R[WIDTH][HIGH];
  int startA = 0;
  int startR = startA + (WIDTH * HIGH * 8) * 2;

  for (int i = 0; i < WIDTH; i++) {
    for (int j = 0; j < HIGH; j++) {
      for (int m = -KERNEL_SIZE / 2; m < KERNEL_SIZE / 2 + 1; m++) {
        for (int n = -KERNEL_SIZE / 2; n < KERNEL_SIZE / 2 + 1; n++) {
          if (m + i >= WIDTH || n + j >= HIGH || m + i < 0 || n + j < 0)
            continue;
          // printf("(%d,%d) ", m+i, n+j);
          printf("0x%x\n", (m + i) * HIGH * 8 + 8 * (n + j));
        }
      }

      printf("0x%x\n", startR + 8 * i * HIGH + 8 * j);
    }
  }
}
