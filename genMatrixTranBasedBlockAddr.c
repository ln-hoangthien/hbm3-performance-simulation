#include <stdio.h>

#define ROWS 640
#define COLS 640
#define BLOCK_SIZE 16

void transpose(int A[ROWS][COLS], int B[COLS][ROWS], int n, int m) {
  for (int ii = 0; ii < n; ii += BLOCK_SIZE) {
    for (int jj = 0; jj < m; jj += BLOCK_SIZE) {
      for (int i = ii; i < ii + BLOCK_SIZE && i < n; ++i) {
        printf("read A address:  %p\n", &A[i]);
        // for (int j = jj; j < jj + BLOCK_SIZE && j < m; ++j) {
        //     B[j][i] = A[i][j];
        //	printf("read A address:  %p\n", &A[i][j]);
        //	printf("store B address: %p\n", &B[j][i]);
        //}
      }
      for (int i = ii; i < ii + BLOCK_SIZE && i < n; ++i) {
        printf("store B address: %p\n", &B[jj] + i);
      }
    }
  }
}

int main() {
  int A[ROWS][COLS];
  int B[COLS][ROWS];

  // Initialize the matrix
  for (int i = 0; i < ROWS; ++i) {
    for (int j = 0; j < COLS; ++j) {
      A[i][j] = i + j;
    }
  }

  // Transpose the matrix
  transpose(A, B, ROWS, COLS);

  // Print the transposed matrix
  for (int i = 0; i < COLS; ++i) {
    for (int j = 0; j < ROWS; ++j) {
      printf("%d ", B[i][j]);
    }
    printf("\n");
  }

  return 0;
}
