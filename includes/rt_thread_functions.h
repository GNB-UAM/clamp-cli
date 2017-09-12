#include "time_functions.h"
//#include "comedi_functions.h"
#include "model_library.h"
#include "queue_functions.h"
#include "calibrate_functions_phase1.h"
#include "calibrate_functions_phase2.h"

#define MAX_SAFE_STACK (8*1024)
#define PRIORITY (99)
#define MAX_LAT (900000)
#define CORE (0)

typedef struct {
    void (*func)(int, double, double*, double*, double);
    void (*ini)(double*, double*, double*, double*);
    void (*syn)(double, double, double*, double*, double*);
    double * vars;
    double * params;
    int dim;
    double dt;
    int type_syn;
    long points;
    int s_points;
    void * msqid;
    int period;
    int n_in_chan;
    int n_out_chan;
    int * in_channels;
    int * out_channels;
    int freq;
    int rafaga_modelo_pts;
    char * filename;
    int calibration;
    int anti;
    int model;
} rt_args;


void prepare_real_time (pthread_t id);

void copy_1d_array (double * src, double * dst, int n_elems);

void * rt_thread (void * arg);