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
#include "comedi_functions.h"


#define MAX_SAFE_STACK (8*1024)
#define PRIORITY (99)
#define PERIOD (100000) //In ns
#define NSEC_PER_SEC (1000000000) /* The number of nsecs per sec. */
#define MAX_LAT (900000)
#define CORE (0)


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
    int in_chan;
    int out_chan;
    double * data_in;
    double * data_out;

    /* Fichero 2*/
    double g_real_to_virtual;
    double g_virtual_to_real;
    double ecm; 
} message;


typedef struct {
    void (*func)(int, double, double*, double*, double);
    double * vars;
    double * params;
    int dim;
    double dt;
    long points;
    int s_points;
    int msqid;
} rt_args;


typedef struct {
    char * filename;
    int msqid;
    long points;
    int s_points;
} writer_args;

void ts_substraction (struct timespec * start, struct timespec * stop, struct timespec * result);

void ts_add_time (struct timespec * ts, int sec, int nsec);

void ts_assign (struct timespec * ts1,  struct timespec ts2);

void prepare_real_time (pthread_t id);

void copy_1d_array (double * src, double * dst, int n_elems);

void * writer_thread (void * arg);

void * rt_thread (void * arg);