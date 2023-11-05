#include "sofb.h"

int sofb_init_epics()
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

int sofb_read_positions()
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

int sofb_read_position_x(int libera, int bpm, double* value)
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

int sofb_read_position_y(int libera, int bpm, double* value)
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

int sofb_read_currents()
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

int sofb_read_current_h(int cell, int index, double* value)
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

int sofb_read_current_v(int cell, int index, double* value)
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

int sofb_read_rf(double* value)
{
	int status;

	ca_get(DBR_DOUBLE, rf_frequency_id, value);
	status = ca_pend_io(IO_TIMEOUT);
	return (status == ECA_NORMAL) ? SOFB_OK : SOFB_ERROR_CA;
}

int sofb_set_rf(double value)
{
	int status;

	ca_put(DBR_DOUBLE, set_rf_id, &value);
	status = ca_pend_io(IO_TIMEOUT);
	return (status == ECA_NORMAL) ? SOFB_OK : SOFB_ERROR_CA;
}

