#include <stdio.h>

#include <cadef.h>

#define LIBERA_COUNT	12
#define BPM_COUNT		48

chid x_positions_id[BPM_COUNT];
chid y_positions_id[BPM_COUNT];

double x_positions[BPM_COUNT];
double y_positions[BPM_COUNT];

bool initialize_epics()
{
	int i, j, status;
	char pv_name[20];

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

	status = ca_pend_io(2.0);
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

void read_positions()
{
	int i, j, status;

	for(i = 0; i < LIBERA_COUNT; i++)
	{
		for(j = 0; j < 4; j++)
		{
			ca_get(DBR_DOUBLE, &x_positions_id[i * 4 + j], &x_positions[i * 4 + j]);
			ca_get(DBR_DOUBLE, &y_positions_id[i * 4 + j], &y_positions[i * 4 + j]);
		}
	}

	status = ca_pend_io(2.0);
	if(status != ECA_NORMAL)
		return false;

	return true;
}

double read_position_x(int libera, int bpm)
{
	ca_get(DBR_DOUBLE, &x_positions_id[libera * 4 + bpm], &x_positions[libera * 4 + j]);
	int status = ca_pend_io(2.0);
	if(status != ECA_NORMAL)
		return -1;
	return x_positions[libera * 4 + j];
}
