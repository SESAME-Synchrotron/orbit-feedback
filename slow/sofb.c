#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <cadef.h>

#define LIBERA_COUNT	12
#define BPM_COUNT		48
#define CELL_COUNT      16
#define CORRECTOR_COUNT 32
#define IO_TIMEOUT		1

chid x_positions_id[BPM_COUNT];
chid y_positions_id[BPM_COUNT];
chid h_correctors_id[CORRECTOR_COUNT];
chid v_correctors_id[CORRECTOR_COUNT];
chid rf_frequency_id;
chid set_rf_id;
double x_positions[BPM_COUNT];
double y_positions[BPM_COUNT];
double h_correctors_currents[CORRECTOR_COUNT];
double v_correctors_currents[CORRECTOR_COUNT];

static bool initialize_epics();
static bool read_positions();
static bool read_position_x(int libera, int bpm, double* value);
static bool read_position_y(int libera, int bpm, double* value);
static bool read_currents();
static bool read_h_current(int cell, int index, double* value);
static bool read_v_current(int cell, int index, double* value);
static bool read_rf(double* value);
static bool set_rf(double value);

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
    printf("H1C1 Current Value %.3f\n", h_correctors_currents[0]);
    printf("V1C1 Current Value %.3f\n", v_correctors_currents[0]);

    // RF
	read_rf(&value);
	printf("RF: %f\n", value);
	
	return 0;
}

bool initialize_epics()
{
	int i, j, status;
	char pv_name[30];

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
            ca_search(pv_name, &h_correctors_id[i * 2 + j]);

            memset(pv_name, 0, sizeof(pv_name));
            snprintf(pv_name, sizeof(pv_name), "SRC%02d-PS-VC%d:setReference", i + 1, j + 1);
            ca_search(pv_name, &v_correctors_id[i * 2 + j]);
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
			return true;
		case ECA_TIMEOUT:
			printf("Some PVs were not found during initialization\n");
			return false;
		default:
			printf("Unknown CA error occured\n");
			return false;
	}
}

bool read_positions()
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
		return false;
	
	for(i = 0; i < LIBERA_COUNT; i++)
	{
		for(j = 0; j < 4; j++)
		{
			x_positions[i * 4 + j] /= 1000.0;
			y_positions[i * 4 + j] /= 1000.0;
		}
	}

	return true;
}

bool read_position_x(int libera, int bpm, double* value)
{
	int index;
	int status;
	
	index = (libera - 1) * 4 + (bpm - 1);
	ca_get(DBR_DOUBLE, x_positions_id[index], &x_positions[index]);
	
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return false;
	
	x_positions[index] /= 1000.0;
	*value = x_positions[index];
	return true;
}

bool read_position_y(int libera, int bpm, double* value)
{
	int index;
	int status;

	index = (libera - 1) * 4 + (bpm - 1);
	ca_get(DBR_DOUBLE, y_positions_id[index], &y_positions[index]);
	
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return false;

	y_positions[index] /= 1000.0;
	*value = y_positions[index];
	return true;
}

bool read_currents()
{
	int i, j, status;

	for(i = 0; i < CELL_COUNT; i++)
	{
		for(j = 0; j < 2; j++)
		{
			ca_get(DBR_DOUBLE, h_correctors_id[i * 2 + j], &h_correctors_currents[i * 2 + j]);
			ca_get(DBR_DOUBLE, v_correctors_id[i * 2 + j], &v_correctors_currents[i * 2 + j]);
		}
	}

	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return false;

	return true;
}

bool read_h_current(int cell, int index, double* value)
{

	int status;
    int ind = (cell-1) * 2 + (index - 1);
	ca_get(DBR_DOUBLE, h_correctors_id[ind], &h_correctors_currents[ind]);
	
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return false;
	
	*value = h_correctors_currents[ind];
	return true;
}

bool read_v_current(int cell, int index, double* value)
{

	int status;
    int ind = (cell-1) * 2 + (index - 1);
	ca_get(DBR_DOUBLE, v_correctors_id[ind], &v_correctors_currents[ind]);
	
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return false;
	
	*value = v_correctors_currents[ind];
	return true;
}

bool read_rf(double* value)
{
	int status;

	ca_get(DBR_DOUBLE, rf_frequency_id, value);
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return false;
	
	return true;
}

bool set_rf(double value)
{
	int status;

	ca_put(DBR_DOUBLE, set_rf_id, &value);
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return false;

	return true;
}

