#include "real_time_functions.h"



/* TIME MANAGEMENT FUNCTIONS */
void ts_substraction (struct timespec * start, struct timespec * stop, struct timespec * result) {
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}


void ts_assign (struct timespec * ts1,  struct timespec ts2) {
    ts1->tv_sec = ts2.tv_sec;
    ts1->tv_nsec = ts2.tv_nsec;
}


void ts_add_time (struct timespec * ts, int sec, int nsec) {
    ts->tv_nsec += nsec;

    while (ts->tv_nsec >= NSEC_PER_SEC) {
          ts->tv_nsec -= NSEC_PER_SEC;
          ts->tv_sec++;
    }

    ts->tv_sec += sec;
}


/*REAL-TIME FUNCTIONS */
void prepare_real_time (pthread_t id) {
	struct sched_param param;
    unsigned char dummy[MAX_SAFE_STACK];


    /* Set priority */
    param.sched_priority = PRIORITY;
    if(pthread_setschedparam(id, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler failed");
        exit(-1);
    }

    /* Set core affinity */
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(CORE, &mask);
    if (pthread_setaffinity_np(id, sizeof(mask), &mask) != 0) {
        perror("Affinity set failure\n");
        exit(-2);
    }

    /* Lock memory */

    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        perror("mlockall failed");
        exit(-3);
    }

    /* Pre-fault our stack */
    memset(dummy, 0, MAX_SAFE_STACK);

    return;
}



/* CALIBRATION FUNCTIONS (MANU) */

int ini_recibido (double *min, double *min_abs, double *max, Comedi_session session, int chan){
    
    /*Vamos a escanear 10000 puntos durante x segundos para determinar min y max*/
    int i=0;
    double retval=0.0, valor_old=0.0, resta=0.0, pendiente_max=-999999;
    struct timespec ts_target, ts_iter, ts_result, ts_start;
    //double bajada_mayor=-999999;
    //double subida_mayor=-999999;

    int segs_observo=8;    

    double maxi=-999999;
    double mini=999999;
    double miniB=999999;

    int n_channels = 1;
    int in_channels [1];
    double ret_values [1];

    in_channels[0] = chan;


    clock_gettime(CLOCK_MONOTONIC, &ts_target);
    ts_assign (&ts_start,  ts_target);
    ts_add_time(&ts_target, 0, PERIOD);


    for (i=0; i<10000*(segs_observo); i++){
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts_target, NULL);
        if (read_comedi(session, n_channels, in_channels, ret_values) != 0) {
            return -1;
        }

        retval = ret_values[0];
        
        if(retval>maxi){
            maxi=retval;
        }else if(retval<mini){
            mini=retval;
        }
        
        if(i>2){
            resta=retval-valor_old;
            
            if(resta>pendiente_max){
                pendiente_max=resta;
                miniB=valor_old;
            }
        }
        
        if(i%10==0)
            valor_old=retval;

        ts_add_time(&ts_target, 0, PERIOD); 
    }


    /*printf("LECTURA INICIAL\n");
    printf("  max_leido=%f ", maxi);
    printf("// min_leido=%f ", mini);
    printf("// min_leido_rel=%f\n\n", mini*0.55);*/

    *min_abs = mini;
    *min = mini*0.55;
    *max = maxi;
    return 1;
}


void calcula_escala (double min_virtual, double max_virtual, double min_viva, double max_viva, double *scale_virtual_to_real, double *scale_real_to_virtual, double *offset_virtual_to_real, double *offset_real_to_virtual){
    
    double rg_virtual, rg_viva;
    
    rg_virtual = max_virtual-min_virtual;
    rg_viva = max_viva-min_viva;
    
    //printf("rg_virtual=%f, rg_viva=%f\n", rg_virtual, rg_viva);
    
    *scale_virtual_to_real = rg_viva / rg_virtual;
    *scale_real_to_virtual = rg_virtual / rg_viva;
    
    *offset_virtual_to_real = min_viva - (min_virtual*(*scale_virtual_to_real));
    *offset_real_to_virtual = min_virtual - (min_viva*(*scale_real_to_virtual));

    //printf("ESCALAS CALCULADAS\n\n");
    return;
}



/* THREADS FUNCTIONS */

void copy_1d_array (double * src, double * dst, int n_elems) {
    int i;

    for (i = 0; i < n_elems; ++i) {
        dst[i] = src[i];
    }

    return;
}


void * writer_thread(void * arg) {
    message msg;
    pthread_t id;
    FILE * f1, * f2;
    writer_args * args;
    int i, j;

    args = arg;
    id = pthread_self();

    char filename_1 [strlen(args->filename) + 6];
    char filename_2 [strlen(args->filename) + 6];

    if (sprintf(filename_1, "%s_1.txt", args->filename) < 0) {
        printf("Error creating file 1 name\n;");
        pthread_exit(NULL);
    }

    if (sprintf(filename_2, "%s_2.txt", args->filename) < 0) {
        printf("Error creating file 2 name\n;");
        pthread_exit(NULL);
    }


    umask(1);
    f1 = fopen(filename_1, "w");
    f2 = fopen(filename_2, "w");
    

    for (i = 0; i < args->points * args->s_points; i++) {
        if (i % args->s_points == 0) {
            msgrcv(args->msqid, (struct msgbuf *)&msg, sizeof(message) - sizeof(long), 1, 0);

            if (i == 0) fprintf(f1, "%d %d\n", msg.in_chan, msg.out_chan);

            fprintf(f1, "%f %f %d %ld %f %f %f %f", msg.t_unix, msg.t_absol, msg.i, msg.lat, msg.v_model, msg.v_model_scaled, msg.c_model, msg.c_real);
            fprintf(f2, "%f %d %f %f\n", msg.t_absol, msg.i, msg.g_real_to_virtual, msg.g_virtual_to_real);

            for (j = 0; j < msg.in_chan; ++j) {
                fprintf(f1, " %f", msg.data_in[j]);
            }

            for (j = 0; j < msg.out_chan; ++j) {
                fprintf(f1, " %f", msg.data_out[j]);
            }

            fprintf(f1, "\n");

            for (j = 0; j < msg.n_g; ++j) {
                fprintf(f2, " %f", msg.g_real_to_virtual[j]);
                fprintf(f2, " %f", msg.g_virtual_to_real[j]);
            }


            fprintf(f2, "\n");
            free(msg.data_in);
            free(msg.data_out);
        }
    }
    
    fclose(f1);
    fclose(f2);
    pthread_exit(NULL);
}



void * rt_thread(void * arg) {
    int i;
    rt_args * args;
    struct timespec ts_target, ts_iter, ts_result, ts_start;
    message msg;
    pthread_t id;

    double max_model, min_model, min_abs_model;
    double max_real, min_real, min_abs_real;
    double scale_real_to_virtual;
    double scale_virtual_to_real;
    double offset_virtual_to_real;
    double offset_real_to_virtual;

    double * g_virtual_to_real;
    double * g_real_to_virtual;
    double retval = 0;
    double c_real = 0, c_model = 0;
    double * syn_aux_params;


    double rafaga_modelo_pts_hr = 260166.0;
    double pts_por_s = 10000.0;
    double t_rafaga_viva_s = 0.4;
    double rafaga_viva_pts = pts_por_s * t_rafaga_viva_s;
    


    id = pthread_self();
    args = arg;
    msg.c_real = c_real;


    comedi_t * d;
    Comedi_session session;
    int in_channels [] = {0};
    int out_channels [] = {0, 1};
    int n_in_chan = 1;
    int n_out_chan = 2;
    double ret_values [1];
    double out_values [2];
    int calib_chan = 0;


    msg.in_chan = n_in_chan;
    msg.out_chan = n_out_chan;



    d = open_device_comedi("/dev/comedi0");

    /*Envia el voltage para ver que hace el modelo*/
    session = create_session_comedi(d, AREF_GROUND);


    prepare_real_time(id);



    args->ini(args->vars, &min_model, &min_abs_model, &max_model);
    if ( ini_recibido (&min_real, &min_abs_real, &max_real, session, calib_chan) == -1 ) {
		close_device_comedi(d);
        pthread_exit(NULL);
	}
    calcula_escala (min_abs_model, max_model, min_abs_real, max_real, &scale_virtual_to_real, &scale_real_to_virtual, &offset_virtual_to_real, &offset_real_to_virtual);

    switch (args->type_syn) {
		case ELECTRIC:
			syn_aux_params = NULL;

			g_virtual_to_real = (double *) malloc (sizeof(double) * 1);
    		g_real_to_virtual = (double *) malloc (sizeof(double) * 1);
			g_virtual_to_real[0] = 0.3;
    		g_real_to_virtual[0] = 0.3;
    		msg.n_g = 1;

			break;
		case CHEMICAL:
			syn_aux_params = (double *) malloc (sizeof(double) * 2);
			syn_aux_params[0] = min_abs_model * scale_virtual_to_real;
			syn_aux_params[1] = args->dt;
			syn_aux_params[2] = 0;

			g_virtual_to_real = (double *) malloc (sizeof(double) * 2);
    		g_real_to_virtual = (double *) malloc (sizeof(double) * 2);

    		g_virtual_to_real[G_FAST] = 0.01;
    		g_virtual_to_real[G_SLOW] = 0.02;
    		g_real_to_virtual[G_FAST] = 0.01;
    		g_real_to_virtual[G_SLOW] = 0.02;
    		msg.n_g = 2;


			break;
		default:
			close_device_comedi(d);
        	pthread_exit(NULL);
	}


    clock_gettime(CLOCK_MONOTONIC, &ts_target);
    ts_assign (&ts_start,  ts_target);
    ts_add_time(&ts_target, 0, PERIOD);



    for (i = 0; i < 5 * 10000 * args->s_points; i++) {
        if (i % args->s_points == 0) {
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts_target, NULL);
            clock_gettime(CLOCK_MONOTONIC, &ts_iter);

            ts_substraction(&ts_target, &ts_iter, &ts_result);
            msg.id = 1;
            msg.i = i;
            msg.v_model_scaled = args->vars[0] * scale_virtual_to_real + offset_virtual_to_real;
            msg.v_model = args->vars[0];
            msg.c_model = 0;
            msg.lat = ts_result.tv_sec * NSEC_PER_SEC + ts_result.tv_nsec;

            ts_substraction(&ts_start, &ts_iter, &ts_result);
            msg.t_absol = (ts_result.tv_sec * NSEC_PER_SEC + ts_result.tv_nsec) * 0.000001;
            msg.t_unix = (ts_iter.tv_sec * NSEC_PER_SEC + ts_iter.tv_nsec) * 0.000001;

            out_values[0] = msg.c_model;
            out_values[1] = msg.v_model_scaled;

            msg.data_in = (double *) malloc (sizeof(double) * n_in_chan);
            msg.data_out = (double *) malloc (sizeof(double) * n_out_chan);

            copy_1d_array(ret_values, msg.data_in, n_in_chan);
            copy_1d_array(out_values, msg.data_out, n_out_chan);

            write_comedi(session, n_out_chan, out_channels, out_values);

            msg.g_real_to_virtual = g_real_to_virtual;
            msg.g_virtual_to_real = g_virtual_to_real;

            msgsnd(args->msqid, (struct msgbuf *) &msg, sizeof(message) - sizeof(long), IPC_NOWAIT);

            ts_add_time(&ts_target, 0, PERIOD);

            if (read_comedi(session, n_in_chan, in_channels, ret_values) != 0) {
                close_device_comedi(d);
                free(syn_aux_params);
                free(g_virtual_to_real);
    			free(g_real_to_virtual);
                pthread_exit(NULL);
            }
        }
        msg.c_real = 0;
        args->func(args->dim, args->dt, args->vars, args->params, c_real);
    }


    for (i = 0; i < args->points * args->s_points; i++) {
        if (i % args->s_points == 0) {
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts_target, NULL);
            clock_gettime(CLOCK_MONOTONIC, &ts_iter);

            ts_substraction(&ts_target, &ts_iter, &ts_result);
            msg.id = 1;
            msg.i = i;
            msg.v_model_scaled = args->vars[0] * scale_virtual_to_real + offset_virtual_to_real;
            msg.v_model = args->vars[0];
            msg.lat = ts_result.tv_sec * NSEC_PER_SEC + ts_result.tv_nsec;

            args->syn(args->vars[0] * scale_virtual_to_real + offset_virtual_to_real, ret_values[0], g_virtual_to_real, &c_model, syn_aux_params);
            msg.c_model = c_model;


            ts_substraction(&ts_start, &ts_iter, &ts_result);
            msg.t_absol = (ts_result.tv_sec * NSEC_PER_SEC + ts_result.tv_nsec) * 0.000001;
            msg.t_unix = (ts_iter.tv_sec * NSEC_PER_SEC + ts_iter.tv_nsec) * 0.000001;

            out_values[0] = c_model;
            out_values[1] = msg.v_model_scaled;

            msg.g_real_to_virtual = g_real_to_virtual;
            msg.g_virtual_to_real = g_virtual_to_real;

            msg.data_in = (double *) malloc (sizeof(double) * n_in_chan);
            msg.data_out = (double *) malloc (sizeof(double) * n_out_chan);

            copy_1d_array(ret_values, msg.data_in, n_in_chan);
            copy_1d_array(out_values, msg.data_out, n_out_chan);

            write_comedi(session, n_out_chan, out_channels, out_values);

            msgsnd(args->msqid, (struct msgbuf *) &msg, sizeof(message) - sizeof(long), IPC_NOWAIT);

            ts_add_time(&ts_target, 0, PERIOD);

            if (read_comedi(session, n_in_chan, in_channels, ret_values) != 0) {
                close_device_comedi(d);
                free(syn_aux_params);
                free(g_virtual_to_real);
    			free(g_real_to_virtual);
                pthread_exit(NULL);
            }
        }

         
        args->syn(ret_values[0] * scale_real_to_virtual + offset_real_to_virtual, args->vars[0], g_real_to_virtual, &c_real, syn_aux_params);
        msg.c_real = c_real;

        args->func(args->dim, args->dt, args->vars, args->params, -c_real);
    }

    close_device_comedi(d);
    free(syn_aux_params);
    free(g_virtual_to_real);
    free(g_real_to_virtual);
    pthread_exit(NULL);
}
