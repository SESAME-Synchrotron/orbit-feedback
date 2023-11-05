#include "sofb.h"

int main()
{
	double value;

	sofb_init_epics();
	sofb_init_params();

    // BPM
	sofb_read_position_x(1, 1, &value);
	printf("Value: %.3f\n", value);
	
	sofb_read_positions();
	value = x_positions[0];
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

	return 0;
}


