#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define MIN(x, y) ((x > y) ? x : y)
#define M(mA, x, y) (mA[(x) * N + (y)])

int main() {
	long int N = 0;
	scanf("%ld", &N);
	long int B = 0;
	scanf("%ld", &B);
	srand(time(0));
	printf("N = %d, size = %lg KB\n", N, N * (N / 1024.));
	double predicted_misses = ((double)N)*N*N;
	if (B <= 8)
		predicted_misses /= B;
	else
		predicted_misses /= 8;
	printf("B = %d, predicted misses = %lg M\n", B, predicted_misses/1000000.);

	/*
	 * for B = N, B > 8 formula shall turn to N*N*N/8
	 * (B*B/8 cache lines per block)        = N*N/8
	 * (N/B) blocks for B = N elements in C = 1
	 * (N*N/B) B-length lines in C          = N
	 * then
	 * for (B >  8) predicted misses are (N*N*N/8),
	 * for (B <= 8) predicted misses are (N*N*N/B)
	 */

	volatile double* mA = (double*)malloc(N * N * sizeof(mA[0]));
	volatile double* mB = (double*)malloc(N * N * sizeof(mB[0]));
	volatile double* C = (double*)malloc(N * N * sizeof(C[0]));

	for (int i = 0; i < (N * N); i++)
		mA[i] = rand();

	for (int i = 0; i < (N * N); i++)
		mB[i] = rand();

	for (int i = 0; i < (N * N); i++)
		C[i] = rand();

	int x, y, k, block_y, block_k, next_y, next_k = 0;

	for(block_y = 0; block_y < N; block_y += B)
		for(block_k = 0; block_k < N; block_k += B)
			for(x = 0; x < N; x++) {
				next_y = MIN(block_y + B, N);
				next_k = MIN(block_k + B, N);

				for(y = block_y; y < next_y; y++)
					for(k = block_k; k < next_k; k++) {
						M(C, x, y) += M(mA, y, k) * M(mB, x, k);
						// mA - column-major, mB - row-major
					}
			}
}

