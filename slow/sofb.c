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

	ADD_PV("SOFB:SamplingFrequency", id_sampling_frequency);
	ADD_PV("SOFB:SingularValues", id_singular_values);
	ADD_PV("SOFB:MaxDeltaCurrent", id_max_delta_current);
	ADD_PV("SOFB:MaxDeltaRF", id_max_delta_rf);
	ADD_PV("SOFB:CurrentAlarmLimit", id_current_alarm);
	ADD_PV("SOFB:IncludeRF", id_include_rf);

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

int sofb_init_params()
{
	int status;

	ca_get(DBR_DOUBLE, id_sampling_frequency, &sampling_frequency);
	ca_get(DBR_LONG,   id_singular_values,    &singular_values);
	ca_get(DBR_DOUBLE, id_max_delta_current,  &max_delta_current);
	ca_get(DBR_DOUBLE, id_max_delta_rf,       &max_delta_rf);
	ca_get(DBR_DOUBLE, id_current_alarm,      &current_alarm);
	ca_get(DBR_ENUM,   id_include_rf,         &include_rf);

	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return SOFB_ERROR_CA;
	return SOFB_OK;
}

int sofb_read_positions()
{
	int i, j, status;

	for(i = 0; i < LIBERA_COUNT; i++)
	{
		for(j = 0; j < 4; j++)
		{
			ca_get(DBR_DOUBLE, x_positions_id[i * 4 + j], &orbit_x[i * 4 + j]);
			ca_get(DBR_DOUBLE, y_positions_id[i * 4 + j], &orbit_y[i * 4 + j]);
		}
	}

	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return SOFB_ERROR_CA;
	
	for(i = 0; i < LIBERA_COUNT; i++)
	{
		for(j = 0; j < 4; j++)
		{
			orbit_x[i * 4 + j] /= 1000.0;
			orbit_y[i * 4 + j] /= 1000.0;
		}
	}

	return SOFB_OK;
}

int sofb_read_position_x(int libera, int bpm, double* value)
{
	int index;
	int status;
	
	index = (libera - 1) * 4 + (bpm - 1);
	ca_get(DBR_DOUBLE, x_positions_id[index], &orbit_x[index]);
	
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return SOFB_ERROR_CA;
	
	orbit_x[index] /= 1000.0;
	*value = orbit_x[index];
	return SOFB_OK;
}

int sofb_read_position_y(int libera, int bpm, double* value)
{
	int index;
	int status;

	index = (libera - 1) * 4 + (bpm - 1);
	ca_get(DBR_DOUBLE, y_positions_id[index], &orbit_y[index]);
	
	status = ca_pend_io(IO_TIMEOUT);
	if(status != ECA_NORMAL)
		return SOFB_ERROR_CA;

	orbit_y[index] /= 1000.0;
	*value = orbit_y[index];
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

