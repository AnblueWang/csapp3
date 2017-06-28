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
    int n1 = N/4;
    int m1 = 64/n1;
    int tmp;
    int flag = 0;
    int ii,jj;
    for (int n = 0; n < N/n1; ++n)
    {
        for (int m = 0; m < M/m1; ++m)
        {
            for (int i = n*n1; i < (n+1)*n1; ++i)
            {
                flag = 0;
                for (int j = m*m1; j < (m+1)*m1; ++j)
                {
                    if((i*M+j)/8%32 == (j*N+i)/8%32) {
                        ii = i;
                        jj = j;
                        tmp = A[i][j];
                        flag = 1;
                        continue;
                     }
                    B[j][i] = A[i][j];
                }
                if(flag)
                B[jj][ii]=tmp;
            }
        }
        for (int i = n*n1; i < (n+1)*n1; ++i)
        {
            for (int j = (M/m1)*m1; j < M; ++j)
            {
                B[j][i] = A[i][j];
            }
        }
    }

    for (int m = 0; m < M/m1; ++m) {
        for (int i = (N/n1)*n1; i < N; ++i)
            {
                for (int j = m*m1; j < (m+1)*m1; ++j)
                {
                    B[j][i] = A[i][j];
                }
            }
    }
    for (int i = (N/n1)*n1; i < N; ++i)
        {
            for (int j = (M/m1)*m1; j < M; ++j)
            {
                B[j][i] = A[i][j];
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
char transpose_1_desc[] = "Test 1";
void transpose_1(int M, int N, int A[N][M], int B[M][N]) {
    int n1 = 8;
    int m1 = 4;
    int tmp;
    int flag = 0;
    int ii,jj;
    for (int n = 0; n < N/n1; ++n)
    {
        for (int m = 0; m < M/m1; ++m)
        {
            for (int i = n*n1; i < (n+1)*n1; ++i)
            {
                flag = 0;
                for (int j = m*m1; j < (m+1)*m1; ++j)
                {
                    if((i*64+j)/8%32 == (j*64+i)/8%32) {
                        ii = i;
                        jj = j;
                        tmp = A[i][j];
                        flag = 1;
                        continue;
                     }
                    B[j][i] = A[i][j];
                }
                if(flag)
                B[jj][ii]=tmp;
            }
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

    registerTransFunction(transpose_1, transpose_1_desc);

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

