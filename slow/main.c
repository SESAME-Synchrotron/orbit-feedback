#include "sofb.h"

int main()
{
	double value;

	int i = 0;
	int alpha = 1;
	int beta = 0;
	int n = 1;

	Rmat = (double*) mkl_malloc(BPM_COUNT * CORRECTOR_COUNT * sizeof(double), 64);
	gold_orbit_x = (double*) mkl_malloc(BPM_COUNT * sizeof(double), 64);
	gold_orbit_y = (double*) mkl_malloc(BPM_COUNT * sizeof(double), 64);
	orbit_x  = (double*) mkl_malloc(BPM_COUNT * sizeof(double), 64);
	orbit_y  = (double*) mkl_malloc(BPM_COUNT * sizeof(double), 64);
	hc_load  = (double*) mkl_malloc(CORRECTOR_COUNT * sizeof(double), 64);
	vc_load  = (double*) mkl_malloc(CORRECTOR_COUNT * sizeof(double), 64);
	hc_delta = (double*) mkl_malloc(CORRECTOR_COUNT * sizeof(double), 64);
	vc_delta = (double*) mkl_malloc(CORRECTOR_COUNT * sizeof(double), 64);
	if (orbit_x == NULL      || orbit_y == NULL ||
		gold_orbit_x == NULL || gold_orbit_y == NULL ||
		hc_load == NULL      || vc_load == NULL ||
		hc_delta == NULL     || vc_delta == NULL || Rmat == NULL)
	{
		printf("Unable to initialize data vectors.\n");
		return -1;
	}

	sofb_init_epics();
	sofb_init_params();

    // BPM
	sofb_read_position_x(1, 1, &value);
	printf("Value: %.3f\n", value);
	
	sofb_read_positions();
	value = orbit_x[0];
	printf("Value: %.3f\n", value);

    // Correctors
    sofb_read_current_h(16, 2, &value);
    printf("H2C16 Current Value %.3f\n", value);
    sofb_read_current_h(7, 1, &value);
    printf("H1C7 Current Value %.3f\n", value);

    sofb_read_current_v(16, 2, &value);
    printf("V2C16 Current Value %.3f\n", value);
    sofb_read_current_v(7, 1, &value);
    printf("V1C7 Current Value %.3f\n", value);

    sofb_read_currents();
    printf("H1C1 Current Value %.3f\n", hc_load[0]);
    printf("V1C1 Current Value %.3f\n", vc_load[0]);

    // RF
	sofb_read_rf(&value);
	printf("RF: %f\n", value);

	// Main loop.
	while(0)
	{
		sofb_read_positions();
		sofb_read_currents();

		cblas_daxpy(BPM_COUNT, -1, gold_orbit_x, 0, orbit_x, 0);
		cblas_daxpy(BPM_COUNT, -1, gold_orbit_y, 0, orbit_y, 0);

		cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, BPM_COUNT, n, CORRECTOR_COUNT, alpha, Rmat, CORRECTOR_COUNT, orbit_x, n, beta, hc_delta, n);
		cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, BPM_COUNT, n, CORRECTOR_COUNT, alpha, Rmat, CORRECTOR_COUNT, orbit_y, n, beta, vc_delta, n);

		for(i = 0; i < CORRECTOR_COUNT; i++)
		{
			hc_load[i] += hc_delta[i];
			vc_load[i] += vc_delta[i];
			ca_put(DBR_DOUBLE, hc_reference_id[i], hc_load + i);
			ca_put(DBR_DOUBLE, vc_reference_id[i], vc_load + i);
		}
		ca_pend_io(IO_TIMEOUT);

		sleep(1 / sampling_frequency);
	}

	return 0;
}


