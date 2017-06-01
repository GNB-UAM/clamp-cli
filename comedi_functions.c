#include "comedi_functions.h"

comedi_t * open_device_comedi (char * dev_name) {
	comedi_t *device;

	device = comedi_open(dev_name);
	if(device == NULL)
	{
		comedi_perror(dev_name);
		return NULL;
	}

	return device;
}

int close_device_comedi (comedi_t * device) {
	if (comedi_close(device) == -1) {
		comedi_perror("Error with comedi_close");
		return -1;
	}

	return 0;
}


Comedi_session create_session_comedi (comedi_t * device, int in_chan, int out_chan, int aref, int unit) {
	Comedi_session d;

	d.device = device;
	d.in_chan = in_chan;
	d.out_chan = out_chan;
	d.range = 0; //get_range_comedi(device, subdev, chan, unit, min, max);
	d.aref = aref;


	d.in_subdev = comedi_find_subdevice_by_type(device, COMEDI_SUBD_AI, in_chan);
	if (d.in_subdev == -1) {
		if (comedi_close(device) == -1) {
			comedi_perror("Error with comedi_close");
		}
		comedi_perror("Error finding input subdevice");
	}

	d.out_subdev = comedi_find_subdevice_by_type(device, COMEDI_SUBD_AO, out_chan);
	if (d.out_subdev == -1) {
		if (comedi_close(device) == -1) {
			comedi_perror("Error with comedi_close");
		}
		comedi_perror("Error finding output subdevice");
	}

	return d;
}

int get_range_comedi (comedi_t * device, int subdev, int chan, double min, double max, int unit) {
	int range = comedi_find_range (device, subdev, chan, unit, min, max);

	if (range == -1) {
		comedi_perror("Error with comedi_find_range");
		return -1;
	}

	return range;
}

comedi_range * get_range_info_comedi (Comedi_session session, int direction) {
	comedi_range * range_info;
	int subdev;
	int chan;

	if (direction == COMEDI_INPUT) {
		subdev = session.in_subdev;
		chan = session.in_chan;
	} else if (direction == COMEDI_OUTPUT) {
		subdev = session.out_subdev;
		chan = session.out_chan;
	} else {
		return NULL;
	}

	range_info = comedi_get_range(session.device, subdev, chan, session.range);

	return range_info;
}

lsampl_t get_maxdata_comedi (Comedi_session session, int direction) {
	lsampl_t maxdata;
	int subdev;
	int chan;

	if (direction == COMEDI_INPUT) {
		subdev = session.in_subdev;
		chan = session.in_chan;
	} else if (direction == COMEDI_OUTPUT) {
		subdev = session.out_subdev;
		chan = session.out_chan;
	} else {
		return 0;
	}

	maxdata = comedi_get_maxdata(session.device, subdev, chan);
	return maxdata;
}


double read_single_data_comedi (Comedi_session session, comedi_range * range_info, lsampl_t maxdata) {
	lsampl_t data;
	double physical_value;
	int retval;

	retval = comedi_data_read(session.device, session.in_subdev,session.in_chan, session.range, session.aref, &data);
	if(retval < 0)
	{
		comedi_perror("read");
		return 0;
	}

	comedi_set_global_oor_behavior(COMEDI_OOR_NAN);
	physical_value = comedi_to_phys(data, range_info, maxdata);
	
	if(isnan(physical_value)) {
		return 0;
	} else {
		return physical_value;
	}

	return 0;
}


int write_single_data_comedi (Comedi_session session, comedi_range * range_info, lsampl_t maxdata, double data) {
	lsampl_t comedi_value;

	comedi_value = comedi_from_phys(data, range_info, maxdata);
	if (comedi_value < 0 || comedi_value > maxdata) {
		return -1;
	}

	return comedi_data_write(session.device, session.out_subdev, session.out_chan, session.range, session.aref, comedi_value);
}