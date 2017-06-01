#include <stdio.h>
#include <stdlib.h>
#include <comedilib.h>
#include <ctype.h>
#include <math.h>

typedef struct {
	comedi_t * device;
	int in_subdev;		/*input subdevice */
	int out_subdev;		/*output subdevice */
	int in_chan;		/*input channel*/
	int out_chan;		/*output channel*/
	int range;			/* more on this later */
	int aref;		/* more on this later */
} Comedi_session;


comedi_t * open_device_comedi (char * dev_name);

int close_device_comedi (comedi_t * device);

Comedi_session create_session_comedi (comedi_t * device, int in_chan, int out_chan, int aref, int unit);

int get_range_comedi (comedi_t * device, int subdev, int chan, double min, double max, int unit);

comedi_range * get_range_info_comedi (Comedi_session session, int direction);

lsampl_t get_maxdata_comedi (Comedi_session session,int direction);

double read_single_data_comedi (Comedi_session session, comedi_range * range_info, lsampl_t maxdata);

int write_single_data_comedi (Comedi_session session, comedi_range * range_info, lsampl_t maxdata, double data);