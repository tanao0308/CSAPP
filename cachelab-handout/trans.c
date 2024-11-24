/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if(M == 32) {
        int i, j, k, a, b, c, d, e, f, g, h;
        for(i=0;i<N;i+=8) {
            for(j=0;j<N;j+=8) {
                for(k=0;k<8;++k) {
                    a = A[i+k][j+0];
                    b = A[i+k][j+1];
                    c = A[i+k][j+2];
                    d = A[i+k][j+3];
                    e = A[i+k][j+4];
                    f = A[i+k][j+5];
                    g = A[i+k][j+6];
                    h = A[i+k][j+7];
                    B[j+0][i+k]=a;
                    B[j+1][i+k]=b;
                    B[j+2][i+k]=c;
                    B[j+3][i+k]=d;
                    B[j+4][i+k]=e;
                    B[j+5][i+k]=f;
                    B[j+6][i+k]=g;
                    B[j+7][i+k]=h;
                }
            }
        }
    } else if(M == 64) {
        int i, j, i1, j1, k, a, b, c, d, e, f, g, h;
        for(i=0;i<N;i+=32) {
            for(j=0;j<N;j+=32) {
                for(i1=0;i1<32;i1+=8) {
                    for(j1=0;j1<32;j1+=8) {
                        for(k=0;k<8;++k) {
                            a = A[i+i1+k][j+j1+0];
                            b = A[i+i1+k][j+j1+1];
                            c = A[i+i1+k][j+j1+2];
                            d = A[i+i1+k][j+j1+3];
                            e = A[i+i1+k][j+j1+4];
                            f = A[i+i1+k][j+j1+5];
                            g = A[i+i1+k][j+j1+6];
                            h = A[i+i1+k][j+j1+7];
                            B[j+j1+0][i+i1+k]=a;
                            B[j+j1+1][i+i1+k]=b;
                            B[j+j1+2][i+i1+k]=c;
                            B[j+j1+3][i+i1+k]=d;
                            B[j+j1+4][i+i1+k]=e;
                            B[j+j1+5][i+i1+k]=f;
                            B[j+j1+6][i+i1+k]=g;
                            B[j+j1+7][i+i1+k]=h;
                        }
                    }
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

