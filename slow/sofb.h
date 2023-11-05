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

double  x_positions[ BPM_COUNT ];
double  y_positions[ BPM_COUNT ];
double  hc_load[ CORRECTOR_COUNT ];
double  vc_load[ CORRECTOR_COUNT ];
double* orm;

int sofb_init_epics();
int sofb_read_positions();
int sofb_read_position_x(int libera, int bpm, double* value);
int sofb_read_position_y(int libera, int bpm, double* value);
int sofb_read_currents();
int sofb_read_current_h(int cell, int index, double* value);
int sofb_read_current_v(int cell, int index, double* value);
int sofb_read_rf(double* value);
int sofb_set_rf(double value);

