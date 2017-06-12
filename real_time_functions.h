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
#include "time_management.h"
//#include "comedi_functions.h"
#include "model_library.h"
#include "calibrate_functions_phase1.h"
#include "calibrate_functions_phase2.h"

#define MAX_SAFE_STACK (8*1024)
#define PRIORITY (99)
#define MAX_LAT (900000)
#define CORE (0)

#define ERR -1
#define OK 0
#define TRUE 1
#define FALSE 0

typedef struct {
    long id;
    double t_unix;
    double t_absol;
    int i;

    /* Fichero 1*/ 
    long lat;
    double v_model;
    double v_model_scaled;
    double c_model;
    double c_real;
    int n_in_chan;
    int n_out_chan;
    double * data_in;
    double * data_out;
    /* Fichero 2*/
    double * g_real_to_virtual;
    double * g_virtual_to_real;
    int n_g;
    double ecm; 
    double extra;
} message;

typedef struct {
    long id;
    int s_points;
    double period_disp_real;
} message_s_points;


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
    int msqid;
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
} rt_args;


typedef struct {
    char * filename;
    char * path;
    int msqid;
    long points;
    int s_points;
    int type_syn;
    int model;
    int period;
    int freq;
    int time_var;
    int calibration;
    int anti;
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

