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

int ini_recibido (double *min, double *min_abs, double *max, double *period_signal, Comedi_session session, int chan, int period, int freq, char* filename){
    
    /*TIEMPO OBSERVACION*/
    int segs_observo = 4; 

    /*VARIABLES CALCULO DE RANGOS*/
    int i=0;
    double retval=0.0, valor_old=0.0, resta=0.0, pendiente_max=-999999;
    struct timespec ts_target, ts_iter, ts_result, ts_start;
    double maxi=-999999;
    double mini=999999;
    double miniB=999999;

    /*DAQ*/
    int n_channels = 1;
    int in_channels [1];
    double ret_values [1];
    in_channels[0] = chan;

    /*RT*/
    clock_gettime(CLOCK_MONOTONIC, &ts_target);
    ts_assign (&ts_start,  ts_target);
    ts_add_time(&ts_target, 0, period);

    /*DECLARACIONES DE ARRAYS Y SUS TAMAÑOS*/
    int size_lectura = freq*segs_observo;
    double lectura[size_lectura];
    double convolution[size_lectura];
    int size_media = size_lectura / 10;
    double media[size_lectura];


    for (i=0; i<freq*(segs_observo); i++){

    	/*LECTURA DE DATOS*/
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts_target, NULL);
        if (read_comedi(session, n_channels, in_channels, ret_values) != 0) {
            return -1;
        }
        retval = ret_values[0];
        lectura[i] = retval;
        
        /*COMPROBAR DATOS*/
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

        ts_add_time(&ts_target, 0, period); 
    }

    /*RETURN*/
    *min_abs = mini;
    *min = mini*0.7; //0.55
    *max = maxi;

    /*GUARDAR DATOS LEIDOS*/

    /*PERIODO DE LA SEÑAL*/
    signal_convolution (lectura, size_lectura, convolution, size_lectura);
    signal_average (lectura, size_lectura, media, size_media);
    *period_signal = signal_period (segs_observo, convolution, size_lectura, *min);
    //printf("Perido signal = %f\n", *period_signal);
    array_to_file(lectura, size_lectura, filename, "lectura_ini");
    array_to_file(convolution, size_lectura, filename, "lectura_ini_filtro");

    /*PRINTF DEBUG*/
    /*printf("LECTURA INICIAL\n");
    printf("  max_leido=%f ", maxi);
    printf("// min_leido=%f ", mini);
    printf("// min_leido_rel=%f\n\n", mini*0.55);*/

    return OK;
}

int signal_convolution (double * lectura, int size_l, double * result, int size_r){
	if(size_l!=size_r)
		return ERR;
	int i;
	for (i=0; i<size_l; i++){
	  if(i>3){
        result[i]= 0.2*lectura[i]  + 0.2*lectura[i-1] + 0.2*lectura[i-2] + 0.2*lectura[i-3] + 0.2*lectura[i-4];
      }else{
        result[i]=lectura[i];
      }
	}
	return OK;
}

int signal_average(double * lectura, int size_l, double * result, int size_r){
	if (size_r>=size_l)
		return ERR;
	int saltar = size_l / size_r;
	int i, j;
	for (i=0, j=0; i<size_l; i++, j++){
		int counter = i+saltar;
		double sum = 0.0;
		for(; i<counter; i++){
			sum += lectura[i];
		}
		sum = sum / saltar;
		result[j] = sum;

	}
	return OK;
}

double signal_period(int seg_observacion, double * signal, int size, double th){
	int up=FALSE;
	if (signal[0]>th)
		up=TRUE;

	int changes=0, i=0;
	for (i=0; i<size; i++){
		if(up==TRUE && signal[i]<th){
			//Cambio de tendencia
			changes++;
			up=FALSE;
		}else if(up==FALSE && signal[i]>th){
			up=TRUE;
		}
	}
	double period = 1.0 / (changes/seg_observacion);
	return period;
}

void array_to_file(double * array, int size, char * filename_date, char * tittle){
    FILE * f;
    char filename[100];
    sprintf(filename, "%s_%s.txt", filename_date, tittle);
    f = fopen(filename, "w");
    int i;

    for(i=0; i<size; i++){
        fprintf(f, "%f\n", array[i]);
    }

    fflush(f);
    fclose(f);
    sleep(1);
    return;
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
    char * filename_3 = "data/summary.txt";

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

    fprintf(f3, "\n=================================\n\n");

    //fprintf(f3, "%s\nModel: %d\nSynapse: %d\nFreq: %d ns\n\n\n", args->filename, args->model, args->type_syn, args->freq);
    fclose(f3);

    msgrcv(args->msqid, (struct msgbuf *)&msg2, sizeof(message_s_points) - sizeof(long), 1, 0);
    s_points = msg2.s_points;
    

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
    } else {
        /*MODO SIN ENTRADA*/
    	scale_real_to_virtual = 1;
	    scale_virtual_to_real = 1;
	    offset_virtual_to_real = 0;
	    offset_real_to_virtual = 0;
    }

    /*CALIBRADO TEMPORAL*/
    double rafaga_viva_pts = args->freq * period_disp_real;
    args->s_points = args->rafaga_modelo_pts / rafaga_viva_pts;
    msg2.s_points = args->s_points;
    msg2.id = 1;

    printf("\n - Phase 1 OK\n - Phase 2 START\n\n");
    fflush(stdout);

    msgsnd(args->msqid, (struct msgbuf *) &msg2, sizeof(message_s_points) - sizeof(long), IPC_NOWAIT);

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
        msg.c_real = c_real;

        args->func(args->dim, args->dt, args->vars, args->params, -c_real);
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
