#include "model_library.h"
#include "real_time_functions.h"

/*
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

dataset = pd.read_csv("salida_test.txt", delimiter=' ', header=None)
array = dataset.values

v = array[:,0]

plt.plot(v)
plt.show()
*/

#define IZHIKEVICH 0
#define HR 1

int main (int argc, char * argv[]) {
	key_t key_q;
	pthread_attr_t attr;
	pthread_t writer, rt;
	int err;
	int msqid;

	time_t t;
	struct tm tm;
	char path [18];
	char hour [12];
	char filename [22];


	writer_args w_args;
	rt_args r_args;

	double * params;
	double * vars;

	double rafaga_modelo_pts_hr;
	double rafaga_modelo_pts_iz;
    double pts_por_s;
    double t_rafaga_viva_s;
    double rafaga_viva_pts;


	int model = atoi(argv[2]);

	switch (model){
		case IZHIKEVICH:
			vars = (double*) malloc (sizeof(double) * 2);
			params = (double*) malloc (sizeof(double) * 4);

			rafaga_modelo_pts_iz = 59324.0;
		    pts_por_s = 10000.0;
		    t_rafaga_viva_s = 0.3;
		    rafaga_viva_pts = pts_por_s * t_rafaga_viva_s;


			vars[0] = 10.0;
			vars[1] = 0.0;

			params[I_IZ] = 10.0;
			params[A_IZ] = 0.02;
			params[B_IZ] = 0.2;
			params[C_IZ] = -50.0;
			params[D_IZ] = 2.0;

			r_args.params = params;
			r_args.vars = vars;

			r_args.dim = 2;
			r_args.s_points = (int) (rafaga_modelo_pts_iz / rafaga_viva_pts);
			r_args.dt = 0.001;

			r_args.func = &izhikevich;

			break;
		case HR:
			vars = (double*) malloc (sizeof(double) * 3);
			params = (double*) malloc (sizeof(double) * 3);

			rafaga_modelo_pts_hr = 260166.0;
		    pts_por_s = 10000.0;
		    t_rafaga_viva_s = 0.4;
		    rafaga_viva_pts = pts_por_s * t_rafaga_viva_s;

			vars[0] = -2.0;
			vars[1] = 0.0;
			vars[2] = 0.0;

			params[I_HR] = 3.0;
			params[R_HR] = 0.0021;
			params[S_HR] = 4.0;

			r_args.params = params;
			r_args.vars = vars;

			r_args.dim = 3;
			r_args.s_points = (int) (rafaga_modelo_pts_hr / rafaga_viva_pts);
			r_args.dt = 0.003;

			r_args.func = &hindmarsh_rose;

			break;
		default:
			return -1;
	}

	/*Messages queue*/
    key_q = ftok ("/bin/ls", 28);
    if (key_q == (key_t) -1) {
        perror("Error obtaining message queue key.");
        exit(EXIT_FAILURE);
    }

    msqid = msgget (key_q, 0600 | IPC_CREAT);
    if (msqid == -1) {
        perror("Error obtaining message queue ID.");
        return(0);
    }


    t = time(NULL);
	tm = *localtime(&t);
	sprintf(path, "data/%dy_%dm_%dd", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

	filename[0] = '\0';
	strcat(filename, path);


	struct stat st = {0};

	if (stat(path, &st) == -1) {
		mkdir(path, 0777);
	}

	sprintf(hour, "/%dh_%dm_%ds", tm.tm_hour, tm.tm_min, tm.tm_sec);
	strcat(filename, hour);

    r_args.msqid = msqid;
    r_args.points = atoi(argv[1]) * 10000;

    w_args.filename = filename;
    w_args.points = r_args.points;
    w_args.s_points = r_args.s_points;
    w_args.msqid = msqid;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);



    err = pthread_create(&(writer), &attr, &writer_thread, (void *) &w_args);
    if (err != 0)
        printf("Can't create thread :[%s]", strerror(err));

    err = pthread_create(&(rt), &attr, &rt_thread, (void *) &r_args);
    if (err != 0)
        printf("Can't create thread :[%s]", strerror(err));

    


    pthread_join(rt, NULL);
    pthread_join(writer, NULL);

    msgctl (msqid, IPC_RMID, (struct msqid_ds *)NULL);
    free(vars);
    free(params);
	return 1;
}