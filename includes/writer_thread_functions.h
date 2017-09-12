#include "queue_functions.h"

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

void * writer_thread (void * arg);