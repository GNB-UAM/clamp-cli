#include "real_time_functions.h"

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
    message_s_points msg2;
    pthread_t id;
    FILE * f1, * f2, *f3;
    writer_args * args;
    int i, j;
    int s_points;

    args = arg;
    id = pthread_self();

    char filename_1 [strlen(args->filename) + 6];
    char filename_2 [strlen(args->filename) + 6];
    char filename_3 [strlen(args->path) + 12];

    if (sprintf(filename_1, "%s_1.txt", args->filename) < 0) {
        printf("Error creating file 1 name\n;");
        pthread_exit(NULL);
    }

    if (sprintf(filename_2, "%s_2.txt", args->filename) < 0) {
        printf("Error creating file 2 name\n;");
        pthread_exit(NULL);
    }

    if (sprintf(filename_2, "%s/summary.txt", args->path) < 0) {
        printf("Error creating file 2 name\n;");
        pthread_exit(NULL);
    }

    umask(1);
    f1 = fopen(filename_1, "w");
    f2 = fopen(filename_2, "w");
    f3 = fopen(filename_3, "a");
    
    fprintf(f3, "%s\nModel: ", args->filename);
    if(args->model==0){
        fprintf(f3, "Hindmarsh Rose\n");
    }else if(args->model==1){
        fprintf(f3, "Izhikevich\n");
    }else if(args->model==2){
        fprintf(f3, "Rulkov Map\n");
    }

    fprintf(f3, "Synapse: ");
    /*Esto tiene que dar mas detalles de hacia que lado en las quimicas hay de cual*/
    if(args->type_syn==0){
        fprintf(f3, "Electric\n");
    }else if(args->type_syn==1){
        fprintf(f3, "Chemical\n");
    }

    fprintf(f3, "Freq = %d Hz\n", args->freq);

    fprintf(f3, "Duration = %d s\n", args->time_var);

    if(args->anti==1){
        fprintf(f3, "Antiphase = True\n");
    }else{
        fprintf(f3, "Antiphase = False\n");
    }

    fprintf(f3, "Calibration mode = %d\n", args->calibration);

    msgrcv(args->msqid, (struct msgbuf *)&msg2, sizeof(message_s_points) - sizeof(long), 1, 0);
    s_points = msg2.s_points;

    fprintf(f3, "Puntos saltar = %d\n", s_points);

    fprintf(f3, "Periodo de disparo %d s\n", msg2.period_disp_real);

    fprintf(f3, "\n=================================\n\n");

    //fprintf(f3, "%s\nModel: %d\nSynapse: %d\nFreq: %d ns\n\n\n", args->filename, args->model, args->type_syn, args->freq);
    fclose(f3);

    
    /*****************/

    for (i = 0; i < (5 * args->freq + args->points) * s_points; i++) {
        if (i % s_points == 0) {
            msgrcv(args->msqid, (struct msgbuf *)&msg, sizeof(message) - sizeof(long), 1, 0);

            if (i == 0) fprintf(f1, "%d %d\n", msg.n_in_chan, msg.n_out_chan);

            fprintf(f1, "%f %f %d %ld %f %f %f %f", msg.t_unix, msg.t_absol, msg.i, msg.lat, msg.v_model, msg.v_model_scaled, msg.c_model, msg.c_real);
            fprintf(f2, "%f %d", msg.t_absol, msg.i);

            for (j = 0; j < msg.n_in_chan; ++j) {
                fprintf(f1, " %f", msg.data_in[j]);
            }

            for (j = 0; j < msg.n_out_chan; ++j) {
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
    message_s_points msg2;
    pthread_t id;

    double max_model, min_model, min_abs_model;
    double max_real, min_real, min_abs_real;
    double scale_real_to_virtual;
    double scale_virtual_to_real;
    double offset_virtual_to_real;
    double offset_real_to_virtual;
    double period_disp_real;
    double rafaga_viva_pts;

    double * g_virtual_to_real;
    double * g_real_to_virtual;
    double retval = 0;
    double c_real = 0, c_model = 0;
    double * syn_aux_params;

    id = pthread_self();
    args = arg;
    msg.c_real = c_real;

    comedi_t * d;
    Comedi_session session;
    /*int in_channels [] = {0};
    int out_channels [] = {0, 1};
    int n_in_chan = 1;
    int n_out_chan = 2;*/
    double ret_values [1];
    double out_values [2];
    int calib_chan = 0;

    msg.n_in_chan = args->n_in_chan;
    msg.n_out_chan = args->n_out_chan;

    d = open_device_comedi("/dev/comedi0");

    /*Envia el voltage para ver que hace el modelo*/
    session = create_session_comedi(d, AREF_GROUND);

    prepare_real_time(id);

    args->ini(args->vars, &min_model, &min_abs_model, &max_model);


    /*CALIBRADO ESPACIAL-TEMPORAL*/
    if (args->n_in_chan > 0) {
	    if ( ini_recibido (&min_real, &min_abs_real, &max_real, &period_disp_real, session, calib_chan, args->period, args->freq, args->filename) == -1 ) {
			close_device_comedi(d);
	        pthread_exit(NULL);
		}
	    calcula_escala (min_abs_model, max_model, min_abs_real, max_real, &scale_virtual_to_real, &scale_real_to_virtual, &offset_virtual_to_real, &offset_real_to_virtual);
        rafaga_viva_pts = args->freq * period_disp_real;
        args->s_points = args->rafaga_modelo_pts / rafaga_viva_pts;
    } else {
        /*MODO SIN ENTRADA*/
    	scale_real_to_virtual = 1;
	    scale_virtual_to_real = 1;
	    offset_virtual_to_real = 0;
	    offset_real_to_virtual = 0;
        args->s_points = 1;
    }

    /*CALIBRADO TEMPORAL*/
    msg2.s_points = args->s_points;
    msg2.period_disp_real = period_disp_real;
    msg2.id = 1;
    msgsnd(args->msqid, (struct msgbuf *) &msg2, sizeof(message_s_points) - sizeof(long), IPC_NOWAIT);

    printf("\n - Phase 1 OK\n - Phase 2 START\n\n");
    fflush(stdout);
    sleep(1);


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

    		g_virtual_to_real[G_FAST] = 0.1;
    		g_virtual_to_real[G_SLOW] = 0.2;
    		g_real_to_virtual[G_FAST] = 0.1;
    		g_real_to_virtual[G_SLOW] = 0.2;
    		msg.n_g = 2;


			break;
		default:
			close_device_comedi(d);
        	pthread_exit(NULL);
	}

    clock_gettime(CLOCK_MONOTONIC, &ts_target);
    ts_assign (&ts_start,  ts_target);
    ts_add_time(&ts_target, 0, args->period);

    for (i = 0; i < 5 * args->freq * args->s_points; i++) {
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

            msg.data_in = (double *) malloc (sizeof(double) * args->n_in_chan);
            msg.data_out = (double *) malloc (sizeof(double) * args->n_out_chan);

            copy_1d_array(ret_values, msg.data_in, args->n_in_chan);
            copy_1d_array(out_values, msg.data_out, args->n_out_chan);

            write_comedi(session, args->n_out_chan, args->out_channels, out_values);

            msg.g_real_to_virtual = g_real_to_virtual;
            msg.g_virtual_to_real = g_virtual_to_real;

            msgsnd(args->msqid, (struct msgbuf *) &msg, sizeof(message) - sizeof(long), IPC_NOWAIT);

            ts_add_time(&ts_target, 0, args->period);

            if (read_comedi(session, args->n_in_chan, args->in_channels, ret_values) != 0) {
                close_device_comedi(d);
                free(syn_aux_params);
                free(g_virtual_to_real);
    			free(g_real_to_virtual);
    			free(args->in_channels);
    			free(args->out_channels);
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

            msg.data_in = (double *) malloc (sizeof(double) * args->n_in_chan);
            msg.data_out = (double *) malloc (sizeof(double) * args->n_out_chan);

            copy_1d_array(ret_values, msg.data_in, args->n_in_chan);
            copy_1d_array(out_values, msg.data_out, args->n_out_chan);

            write_comedi(session, args->n_out_chan, args->out_channels, out_values);

            msgsnd(args->msqid, (struct msgbuf *) &msg, sizeof(message) - sizeof(long), IPC_NOWAIT);

            ts_add_time(&ts_target, 0, args->period);

            if (read_comedi(session, args->n_in_chan, args->in_channels, ret_values) != 0) {
                for (i = 0; i < args->n_out_chan; i++) {
                    out_values[i] = 0;
                }
                write_comedi(session, args->n_out_chan, args->out_channels, out_values);
                close_device_comedi(d);
                free(syn_aux_params);
                free(g_virtual_to_real);
    			free(g_real_to_virtual);
    			free(args->in_channels);
    			free(args->out_channels);
                pthread_exit(NULL);
            }
        }

         
        args->syn(ret_values[0] * scale_real_to_virtual + offset_real_to_virtual, args->vars[0], g_real_to_virtual, &c_real, syn_aux_params);
        msg.c_real = c_real * scale_virtual_to_real + offset_virtual_to_real;

        args->func(args->dim, args->dt, args->vars, args->params, args->anti*c_real);
    }

    for (i = 0; i < args->n_out_chan; i++) {
    	out_values[i] = 0;
    }
    write_comedi(session, args->n_out_chan, args->out_channels, out_values);
    close_device_comedi(d);
    free(syn_aux_params);
    free(g_virtual_to_real);
    free(g_real_to_virtual);
    free(args->in_channels);
    free(args->out_channels);
    pthread_exit(NULL);
}
