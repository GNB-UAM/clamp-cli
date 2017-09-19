#ifndef CALIBRATE_FUNCTIONS_PHASE2_A_H__
#define CALIBRATE_FUNCTIONS_PHASE2_A_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sched.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include "calibrate_functions_phase2.h"
#include "queue_functions.h"

int auto_calibration(
					rt_args * args,
					calibration_args * cs,
					double * ret_values,
					double rafaga_viva_pts,
					double * ecm_result,
					message * msg,
					int * cal_on,
					double * g_virtual_to_real,
					double * g_real_to_virtual
					);

#endif 