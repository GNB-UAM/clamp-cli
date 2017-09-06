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


/*typedef struct {
    long id;
    int s_points;
    double period_disp_real;
} message_s_points;*/


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


typedef struct {
    char * filename;
    char * path;
    void * msqid;
    long points;
    int s_points;
    int type_syn;
    int model;
    int period;
    int freq;
    int time_var;
    int calibration;
    int anti;
    int important;
} writer_args;

void ts_substraction (struct timespec * start, struct timespec * stop, struct timespec * result);

void ts_add_time (struct timespec * ts, int sec, int nsec);

void ts_assign (struct timespec * ts1,  struct timespec ts2);

void prepare_real_time (pthread_t id);

void copy_1d_array (double * src, double * dst, int n_elems);

void * writer_thread (void * arg);

void * rt_thread (void * arg);

int signal_convolution (double * lectura, int size_l, double * result, int size_r);

int signal_average(double * lectura, int size_l, double * result, int size_r);

double signal_period(int freq, double * signal, int size, double th);

void array_to_file(double * array, int size, char * filename_date, char * tittle);

