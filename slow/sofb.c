#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <cadef.h>

#define LIBERA_COUNT	12
#define BPM_COUNT		48
#define CELL_COUNT      16
#define CORRECTOR_COUNT 32
#define IO_TIMEOUT		1

#define SOFB_OK			0
#define SOFB_ERROR_CA	1

chid x_positions_id[ BPM_COUNT ];
chid y_positions_id[ BPM_COUNT ];
chid hc_reference_id[ CORRECTOR_COUNT ];
chid vc_reference_id[ CORRECTOR_COUNT ];
chid hc_load_id[ CORRECTOR_COUNT ];
chid vc_load_id[ CORRECTOR_COUNT ];
chid rf_frequency_id;
chid set_rf_id;
double x_positions[ BPM_COUNT ];
double y_positions[ BPM_COUNT ];
double hc_load[ CORRECTOR_COUNT ];
double vc_load[ CORRECTOR_COUNT ];
double* orm;

static int initialize_epics();
static int read_positions();
static int read_position_x(int libera, int bpm, double* value);
static int read_position_y(int libera, int bpm, double* value);
static int read_currents();
static int read_h_current(int cell, int index, double* value);
static int read_v_current(int cell, int index, double* value);
static int read_rf(double* value);
static int set_rf(double value);

int main()
{
	double value;

	initialize_epics();

    // BPM
	read_position_x(1, 1, &value);
	printf("Value: %.3f\n", value);
	
	read_positions();
	value = x_positions[0];
	printf("Value: %.3f\n", value);

    // Correctors
    read_h_current(16, 2, &value);
    printf("H2C16 Current Value %.3f\n", value);
    read_h_current(7, 1, &value);
    printf("H1C7 Current Value %.3f\n", value);

    read_v_current(16, 2, &value);
    printf("V2C16 Current Value %.3f\n", value);
    read_v_current(7, 1, &value);
    printf("V1C7 Current Value %.3f\n", value);

    read_currents();
    printf("H1C1 Current Value %.3f\n", hc_load[0]);
    printf("V1C1 Current Value %.3f\n", vc_load[0]);

    // RF
	read_rf(&value);
	printf("RF: %f\n", value);

	orm = (double*) mkl_alloc(BPM_COUNT * CORRECTOR_COUNT * sizeof(double), 64);
	if(orm == NULL) {
		// TODO: Error.
	}
	// Read and init the orm pointer;
	
	return 0;
}

int initialize_epics()
{
	int i, j, status;
	char pv_name[50];

	ca_task_initialize();
	for(i = 0; i < LIBERA_COUNT; i++)
	{
		for(j = 0; j < 4; j++)
		{
			memset(pv_name, 0, sizeof(pv_name));
			snprintf(pv_name, sizeof(pv_name), "libera%d:bpm%d:sa.X", i + 1, j + 1);
			ca_search(pv_name, &x_positions_id[i * 4 + j]);
			
			memset(pv_name, 0, sizeof(pv_name));
			snprintf(pv_name, sizeof(pv_name), "libera%d:bpm%d:sa.Y", i + 1, j + 1);
			ca_search(pv_name, &y_positions_id[i * 4 + j]);
		}
	}

    for (i = 0; i < CELL_COUNT; i++)
    {
        for (j = 0; j < 2; j++)
        {
            memset(pv_name, 0, sizeof(pv_name));
            snprintf(pv_name, sizeof(pv_name), "SRC%02d-PS-HC%d:setReference", i + 1, j + 1);
            ca_search(pv_name, &hc_reference_id[i * 2 + j]);

            memset(pv_name, 0, sizeof(pv_name));
            snprintf(pv_name, sizeof(pv_name), "SRC%02d-PS-VC%d:setReference", i + 1, j + 1);
            ca_search(pv_name, &vc_reference_id[i * 2 + j]);

            memset(pv_name, 0, sizeof(pv_name));
            snprintf(pv_name, sizeof(pv_name), "SRC%02d-PS-HC%d:getIload", i + 1, j + 1);
            ca_search(pv_name, &hc_load_id[i * 2 + j]);

            memset(pv_name, 0, sizeof(pv_name));
            snprintf(pv_name, sizeof(pv_name), "SRC%02d-PS-VC%d:getIload", i + 1, j + 1);
            ca_search(pv_name, &vc_load_id[i * 2 + j]);
        }
    }

	memset(pv_name, 0, sizeof(pv_name));
	snprintf(pv_name, sizeof(pv_name), "BO-RF-SGN1:getFrequency");
	ca_search(pv_name, &rf_frequency_id);
	
	memset(pv_name, 0, sizeof(pv_name));
	snprintf(pv_name, sizeof(pv_name), "BO-RF-SGN1:setFrequency");
	ca_search(pv_name, &set_rf_id);

	status = ca_pend_io(IO_TIMEOUT);
	switch(status)
	{
		case ECA_NORMAL:
			return SOFB_OK;
		case ECA_TIMEOUT:
			printf("Some PVs were not found during initialization\n");
			return SOFB_ERROR_CA;
		default:
			printf("Unknown CA error occured\n");
			return SOFB_ERROR_CA;
	}
}

int read_positions()
{
	int i, j, status;

	for(i = 0; i < LIBERA_COUNT; i++)
	{
		for(j = 0; j < 4; j++)
		{
			ca_get(DBR_DOUBLE, x_positions_id[i * 4 + j], &x_positions[i * 4 + j]);
			ca_get(DBR_DOUBLE, y_positions_id[i * 4 + j], &y_positions[i * 4 + j]);
		}
	}

	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return SOFB_ERROR_CA;
	
	for(i = 0; i < LIBERA_COUNT; i++)
	{
		for(j = 0; j < 4; j++)
		{
			x_positions[i * 4 + j] /= 1000.0;
			y_positions[i * 4 + j] /= 1000.0;
		}
	}

	return SOFB_OK;
}

int read_position_x(int libera, int bpm, double* value)
{
	int index;
	int status;
	
	index = (libera - 1) * 4 + (bpm - 1);
	ca_get(DBR_DOUBLE, x_positions_id[index], &x_positions[index]);
	
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return SOFB_ERROR_CA;
	
	x_positions[index] /= 1000.0;
	*value = x_positions[index];
	return SOFB_OK;
}

int read_position_y(int libera, int bpm, double* value)
{
	int index;
	int status;

	index = (libera - 1) * 4 + (bpm - 1);
	ca_get(DBR_DOUBLE, y_positions_id[index], &y_positions[index]);
	
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return SOFB_ERROR_CA;

	y_positions[index] /= 1000.0;
	*value = y_positions[index];
	return SOFB_OK;
}

int read_currents()
{
	int i, j, status;

	for(i = 0; i < CELL_COUNT; i++)
	{
		for(j = 0; j < 2; j++)
		{
			ca_get(DBR_DOUBLE, hc_reference_id[i * 2 + j], &hc_load[i * 2 + j]);
			ca_get(DBR_DOUBLE, vc_reference_id[i * 2 + j], &vc_load[i * 2 + j]);
		}
	}

	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return SOFB_ERROR_CA;

	return SOFB_OK;
}

int read_h_current(int cell, int index, double* value)
{
	int status;
    int ind = (cell-1) * 2 + (index - 1);
	ca_get(DBR_DOUBLE, hc_load_id[ind], &hc_load[ind]);
	
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return SOFB_ERROR_CA;
	
	*value = hc_load[ind];
	return SOFB_OK;
}

int read_v_current(int cell, int index, double* value)
{
	int status;
    int ind = (cell-1) * 2 + (index - 1);
	ca_get(DBR_DOUBLE, vc_load_id[ind], &vc_load[ind]);
	
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return SOFB_ERROR_CA;
	
	*value = vc_load[ind];
	return SOFB_OK;
}

int read_rf(double* value)
{
	int status;

	ca_get(DBR_DOUBLE, rf_frequency_id, value);
	status = ca_pend_io(IO_TIMEOUT);
	return (status == ECA_NORMAL) ? SOFB_OK : SOFB_ERROR_CA;
}

int set_rf(double value)
{
	int status;

	ca_put(DBR_DOUBLE, set_rf_id, &value);
	status = ca_pend_io(IO_TIMEOUT);
	return (status == ECA_NORMAL) ? SOFB_OK : SOFB_ERROR_CA;
}

