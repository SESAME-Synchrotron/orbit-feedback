#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <cadef.h>

#include "mkl.h"

#define LIBERA_COUNT	12
#define BPM_COUNT		48
#define CELL_COUNT      16
#define CORRECTOR_COUNT 32
#define IO_TIMEOUT		1

#define SOFB_OK			0
#define SOFB_ERROR_CA	1

#define ADD_PV(name, id) \
	memset(pv_name, 0, sizeof(pv_name)); \
	snprintf(pv_name, sizeof(pv_name), name); \
	ca_search(pv_name, &id);

#define ALPHA	1
#define BETA	0
#define N		1

chid x_positions_id[ BPM_COUNT ];
chid y_positions_id[ BPM_COUNT ];
chid hc_reference_id[ CORRECTOR_COUNT ];
chid vc_reference_id[ CORRECTOR_COUNT ];
chid hc_load_id[ CORRECTOR_COUNT ];
chid vc_load_id[ CORRECTOR_COUNT ];
chid rf_frequency_id;
chid set_rf_id;

chid id_sampling_frequency;
chid id_singular_values;
chid id_max_delta_current;
chid id_max_delta_rf;
chid id_current_alarm;
chid id_include_rf;

double* orbit_x;
double* orbit_y;
double* gold_orbit_x;
double* gold_orbit_y;
double* hc_load;
double* vc_load;
double* hc_delta;
double* vc_delta;
double* Rmat;

double sampling_frequency;
double max_delta_current;
double max_delta_rf;
double current_alarm;
int include_rf;
int singular_values;

int sofb_init_epics();
int sofb_init_params();
int sofb_read_positions();
int sofb_read_position_x(int libera, int bpm, double* value);
int sofb_read_position_y(int libera, int bpm, double* value);
int sofb_read_currents();
int sofb_read_current_h(int cell, int index, double* value);
int sofb_read_current_v(int cell, int index, double* value);
int sofb_read_rf(double* value);
int sofb_set_rf(double value);

