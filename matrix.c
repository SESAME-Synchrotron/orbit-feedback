#include <stdio.h>
#include <stdlib.h>
#include "mkl.h"

int main()
{
	int m = 3;
	MKL_INT n = 3;
	MKL_INT o = 3;
	double matrix[9] = {0.7922, 0.0357, 0.6787, 0.9595, 0.8491, 0.7577, 0.6557, 0.934, 0.7431};
	double work[9];
	lapack_int pivot[3];
	int x;
	LAPACKE_dgetrf(LAPACK_ROW_MAJOR, 3, 3, matrix, 3, pivot);
	LAPACKE_dgetri(LAPACK_ROW_MAJOR, 3, matrix, 3, pivot);

	int i = 0;
	for(i = 0; i < 9; i++)
		printf("%f\n", matrix[i]);

	return 0;
}
