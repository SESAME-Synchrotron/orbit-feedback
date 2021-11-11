#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <cadef.h>

#define LIBERA_COUNT	12
#define BPM_COUNT		48
#define IO_TIMEOUT		1

chid x_positions_id[BPM_COUNT];
chid y_positions_id[BPM_COUNT];
chid rf_frequency_id;
chid set_rf_id;
double x_positions[BPM_COUNT];
double y_positions[BPM_COUNT];

static bool initialize_epics();
static bool read_positions();
static bool read_position_x(int libera, int bpm, double* value);
static bool read_position_y(int libera, int bpm, double* value);
static bool read_rf(double* value);
static bool set_rf(double* value);

int main()
{
	double value;

	initialize_epics();

	read_position_x(1, 1, &value);
	printf("Value: %.3f\n", value);
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

			x_positions[i * 4 + j] /= 1000.0;
			y_positions[i * 4 + j] /= 1000.0;
		}
	}

	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return false;

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

bool read_rf(double* value)
{
	int status;

	ca_get(DBR_DOUBLE, rf_frequency_id, value);
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return false;
	
	return true;
}

bool set_rf(double* value)
{
	int status;

	ca_put(DBR_DOUBLE, set_rf_id, value);
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return false;

	return true;
}

