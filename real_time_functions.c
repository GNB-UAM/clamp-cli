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
    message msg2;
    pthread_t id;
    FILE * f1, * f2, *f3;
    writer_args * args;
    int i, j;
    int s_points;

    args = arg;
    id = pthread_self();

    char * filename_1 = (char *) malloc (sizeof(char)*(strlen(args->filename)+6));
    char * filename_2 = (char *) malloc (sizeof(char)*(strlen(args->filename)+6));
    char * filename_3 = (char *) malloc (sizeof(char)*(strlen(args->path)+12));

    if (sprintf(filename_1, "%s_1.txt", args->filename) < 0) {
        printf("Error creating file 1 name\n;");
        pthread_exit(NULL);
    }

    if (sprintf(filename_2, "%s_2.txt", args->filename) < 0) {
        printf("Error creating file 2 name\n;");
        pthread_exit(NULL);
    }

    if (sprintf(filename_3, "%s/summary.txt", args->path) < 0) {
        printf("Error creating file 2 name\n;");
        pthread_exit(NULL);
    }

    umask(1);
    f1 = fopen(filename_1, "w");
    f2 = fopen(filename_2, "w");
    f3 = fopen(filename_3, "a");
    
    if (args->important==1){
        fprintf(f3, "*********IMPORTANT RECORD********\n");
    }

    fprintf(f3, "%s\nModel: ", args->filename);
    if(args->model==1){
        fprintf(f3, "Hindmarsh Rose\n");
    }else if(args->model==0){
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

    receive_from_queue(args->msqid, &msg2);
    
    s_points = msg2.i;

    fprintf(f3, "Model jump points = %d\n", s_points);

    fprintf(f3, "Burst duration = %f s\n", msg2.t_unix);

    printf("Periodo disparo = %f\n", msg2.t_unix);

    fprintf(f3, "\n=================================\n\n");

    //fprintf(f3, "%s\nModel: %d\nSynapse: %d\nFreq: %d ns\n\n\n", args->filename, args->model, args->type_syn, args->freq);
    fclose(f3);

    /*****************/

    for (i = 0; i < (5 * args->freq + args->points) * s_points; i++) {
        if (i % s_points == 0) {
            receive_from_queue(args->msqid, &msg);

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

            fprintf(f2, " %f", msg.ecm);
            fprintf(f2, " %f", msg.extra);
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
    free(filename_1);
    free(filename_2);
    free(filename_3);

    printf("End writer\n");
    pthread_exit(NULL);
}



void * rt_thread(void * arg) {
    int i, cont_send=0;
    rt_args * args;
    struct timespec ts_target, ts_iter, ts_result, ts_start;
    message msg;
    message msg2;
    pthread_t id;

    double max_model, min_model, min_abs_model;
    double max_real, min_real, min_abs_real, max_real_relativo;
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

    double ecm_result = 0;

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
	    if ( ini_recibido (&min_real, &min_abs_real, &max_real, &max_real_relativo, &period_disp_real, session, calib_chan, args->period, args->freq, args->filename) == -1 ) {
			close_device_comedi(d);
	        pthread_exit(NULL);
		}
		//printf("Periodo disparo = %f\n", period_disp_real);
        /*fflush(stdout);
        sleep(1);*/
        //period_disp_real = 0.27;
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
        period_disp_real = 0;
    }

    /*CALIBRADO TEMPORAL*/
    msg2.i = args->s_points;
    msg2.t_unix = period_disp_real;
    msg2.id = 1;
    send_to_queue(args->msqid, &msg2);

    //printf("\n - Phase 1 OK\n - Phase 2 START\n\n");
    /*fflush(stdout);
    sleep(1);*/

    double ini_k1=0.4;
    double ini_k2=0.01;

    switch (args->type_syn) {
		case ELECTRIC:
			syn_aux_params = NULL;

			g_virtual_to_real = (double *) malloc (sizeof(double) * 1);
    		g_real_to_virtual = (double *) malloc (sizeof(double) * 1);
			g_virtual_to_real[0] = 0.02;
    		g_real_to_virtual[0] = 0.2;
            if(args->calibration != 0 && args->calibration != 6){
                g_virtual_to_real[0] = 0.0;
                g_real_to_virtual[0] = 0.0;
            }
    		msg.n_g = 1;

			break;
		case CHEMICAL:
			syn_aux_params = (double *) malloc (sizeof(double) * 6);
			syn_aux_params[SC_DT] = args->dt;
			syn_aux_params[SC_OLD] = 0;
            syn_aux_params[SC_BT] = period_disp_real;
            syn_aux_params[SC_MS_K1] = 1;//1;
            syn_aux_params[SC_MS_K2] = 0.03;//0.03;

			g_virtual_to_real = (double *) malloc (sizeof(double) * 2);
    		g_real_to_virtual = (double *) malloc (sizeof(double) * 2);
			if (args->model==0){
                g_virtual_to_real[G_FAST] = 0.0;
                g_virtual_to_real[G_SLOW] = 0.02;
                g_real_to_virtual[G_FAST] = 0.2;
                g_real_to_virtual[G_SLOW] = 0.0;

            }else {
                g_virtual_to_real[G_FAST] = 0.0;
                g_virtual_to_real[G_SLOW] = 0.1;
                g_real_to_virtual[G_FAST] = 0.3;
                g_real_to_virtual[G_SLOW] = 0.0;
            }

            if(args->calibration == 7){
                g_virtual_to_real[G_FAST] = 0.0;
                g_virtual_to_real[G_SLOW] = 0.0;
                g_real_to_virtual[G_FAST] = 0.0;
                g_real_to_virtual[G_SLOW] = 0.0;
            }

        
            if(args->calibration == 8){
                syn_aux_params[SC_MS_K1] = ini_k1;//1;
                syn_aux_params[SC_MS_K2] = ini_k2;//0.03;
            }
    		
    		msg.n_g = 2;

			break;
		default:
			close_device_comedi(d);
        	pthread_exit(NULL);
	}


    double sum_ecm;
    int t_obs=5;
    int sum_ecm_cont=0;

    int cal_on = TRUE;
    double res_phase = 0;
    int cont_lectura=0;
    int size_lectura=2*args->freq;
    double * lectura_a = (double *) malloc (sizeof(double) * 2*args->freq);
    double * lectura_b = (double *) malloc (sizeof(double) * 2*args->freq);
    double * lectura_t = (double *) malloc (sizeof(double) * 2*args->freq);


    clock_gettime(CLOCK_MONOTONIC, &ts_target);
    ts_assign (&ts_start,  ts_target);
    ts_add_time(&ts_target, 0, args->period);

    for (i = 0; i < t_obs * args->freq * args->s_points; i++) {
        if (i % args->s_points == 0) {
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts_target, NULL);
            clock_gettime(CLOCK_MONOTONIC, &ts_iter);

            ts_substraction(&ts_target, &ts_iter, &ts_result);
            msg.id = 1;
            msg.extra = 0;
            msg.i = cont_send;
            cont_send++;
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

            /*CALIBRACION*/
            if(args->calibration == 1){
                //Electrica en fase
                double ecm_old=ecm_result;
                int ret_ecm = calc_ecm(args->vars[0] * scale_virtual_to_real + offset_virtual_to_real, ret_values[0], rafaga_viva_pts, &ecm_result);
                msg.ecm = ecm_result;
                if(ecm_result!=0 && ecm_old!=ecm_result){
                    sum_ecm+=ecm_result;
                    sum_ecm_cont++;
                }
            }else if(args->calibration  == 4){
                 if(cont_lectura<size_lectura){
                    /*Guardamos info*/
                    lectura_b[cont_lectura]=args->vars[0] * scale_virtual_to_real + offset_virtual_to_real;
                    lectura_a[cont_lectura]=ret_values[0];
                    lectura_t[cont_lectura]=msg.t_absol;
                    cont_lectura++;
                }else{
                    int is_syn = calc_phase (lectura_b, lectura_a, lectura_t, size_lectura, max_real_relativo, min_real, &res_phase, args->anti);
                    msg.ecm = res_phase;
                    cont_lectura=0;
                }
            }

            /*msg.g_real_to_virtual = g_real_to_virtual;
            msg.g_virtual_to_real = g_virtual_to_real;*/
            msg.g_real_to_virtual = (double *) malloc (sizeof(double) * msg.n_g);
            msg.g_virtual_to_real = (double *) malloc (sizeof(double) * msg.n_g);

            copy_1d_array(g_real_to_virtual, msg.g_real_to_virtual, msg.n_g);
            copy_1d_array(g_virtual_to_real, msg.g_virtual_to_real, msg.n_g);

            send_to_queue(args->msqid, &msg);

            ts_add_time(&ts_target, 0, args->period);

            if (read_comedi(session, args->n_in_chan, args->in_channels, ret_values) != 0) {
                close_device_comedi(d);
                free(syn_aux_params);
                free(g_virtual_to_real);
    			free(g_real_to_virtual);
    			free(args->in_channels);
    			free(args->out_channels);
                free(lectura_a);
                free(lectura_b);
                free(lectura_t);
                pthread_exit(NULL);
            }
        }
        msg.c_real = 0;
        args->func(args->dim, args->dt, args->vars, args->params, c_real);
    }

    if(args->calibration == 1){
        sum_ecm = sum_ecm / sum_ecm_cont;
        set_is_syn_by_percentage(sum_ecm);
    }
    cont_lectura=0;
    int cont_6=0;
    int counter_mapa=0;
    int cal_7=TRUE;


    /*PULSOS DE SINCRONIZACION*/
    out_values[0] = 0;
    out_values[1] = -10;
    write_comedi(session, args->n_out_chan, args->out_channels, out_values);
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts_target, NULL);
    ts_add_time(&ts_target, 0, args->period);
    out_values[1] = 10;
    write_comedi(session, args->n_out_chan, args->out_channels, out_values);

    for (i = 0; i < args->points * args->s_points; i++) {
        /*TOCA INTERACCION*/
        if (i % args->s_points == 0) {
            
            /*ESPERA HASTA EL MOMENTO DETERMINADO*/
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts_target, NULL);
            clock_gettime(CLOCK_MONOTONIC, &ts_iter);
            ts_substraction(&ts_target, &ts_iter, &ts_result);

            /*GUARDAR INFO*/
            msg.id = 1;
            msg.extra = 0;
            msg.i = cont_send;
            cont_send++;
            msg.v_model_scaled = args->vars[0] * scale_virtual_to_real + offset_virtual_to_real;
            msg.v_model = args->vars[0];
            msg.lat = ts_result.tv_sec * NSEC_PER_SEC + ts_result.tv_nsec;

            /*SINAPSIS Y CORRIENTE EN VIRTUAL TO REAL*/
            if (args->type_syn==CHEMICAL)
                syn_aux_params[SC_MIN] = min_abs_model * scale_virtual_to_real + offset_virtual_to_real;
            args->syn(args->vars[0] * scale_virtual_to_real + offset_virtual_to_real, ret_values[0], g_virtual_to_real, &c_model, syn_aux_params);
            msg.c_model=c_model;

            /*GUARDAR INFO*/
            ts_substraction(&ts_start, &ts_iter, &ts_result);
            msg.t_absol = (ts_result.tv_sec * NSEC_PER_SEC + ts_result.tv_nsec) * 0.000001;
            msg.t_unix = (ts_iter.tv_sec * NSEC_PER_SEC + ts_iter.tv_nsec) * 0.000001;

            /*ENVIO POR LA TARJETA*/
            out_values[0] = c_model;
            out_values[1] = msg.v_model_scaled;

            /*GUARDAR INFO*/
            msg.g_real_to_virtual = g_real_to_virtual;
            msg.g_virtual_to_real = g_virtual_to_real;
            msg.data_in = (double *) malloc (sizeof(double) * args->n_in_chan);
            msg.data_out = (double *) malloc (sizeof(double) * args->n_out_chan);
            copy_1d_array(ret_values, msg.data_in, args->n_in_chan);
            copy_1d_array(out_values, msg.data_out, args->n_out_chan);

            /*ENVIO POR LA TARJETA*/
            write_comedi(session, args->n_out_chan, args->out_channels, out_values);

            /*CALIBRACION*/
            if(args->calibration==1 || args->calibration==2 || args->calibration==3){
                //Electrica en fase - ecm
                int ret_ecm = calc_ecm(args->vars[0] * scale_virtual_to_real + offset_virtual_to_real, ret_values[0], rafaga_viva_pts, &ecm_result);
                msg.ecm = ecm_result;
                if(cal_on && ret_ecm==1){

                    int is_syn;
                    if (args->calibration == 1){
                        //Porcentaje
                        is_syn = is_syn_by_percentage(ecm_result);
                    }else if (args->calibration == 2){
                        //Pendiente
                        is_syn = is_syn_by_slope(ecm_result);
                    }else if (args->calibration == 3){
                        //Var
                        is_syn = is_syn_by_variance(ecm_result);
                    }

                    if (is_syn==TRUE){
                        //printf("CALIBRATION END: g=%f\n", g_virtual_to_real[0]);
                        cal_on=FALSE;
                    }else if(is_syn==FALSE && cal_on==TRUE){
                        change_g(&g_virtual_to_real[0]);
                        change_g(&g_real_to_virtual[0]);
                    }

                }
            }else if (args->calibration==4){
                //Electrica y var
                if(cont_lectura<size_lectura){
                    /*Guardamos info*/
                    lectura_b[cont_lectura]=args->vars[0] * scale_virtual_to_real + offset_virtual_to_real;
                    lectura_a[cont_lectura]=ret_values[0];
                    lectura_t[cont_lectura]=msg.t_absol;
                    msg.ecm = res_phase;
                    cont_lectura++;
                }else{
                    /*Ejecuta metrica*/
                    int is_syn = calc_phase (lectura_b, lectura_a, lectura_t, size_lectura, max_real_relativo, min_real, &res_phase, args->anti);
                    msg.ecm = res_phase;

                    //printf("var = %f\n", msg.ecm);
                    if(cal_on){
                        if (is_syn==TRUE){
                            //printf("CALIBRATION END: g=%f\n", g_virtual_to_real[0]);
                            cal_on=FALSE;
                        }else if (is_syn==FALSE){
                            change_g(&g_virtual_to_real[0]);
                            change_g(&g_real_to_virtual[0]);
                        } 
                    }
                    cont_lectura=0;
                }
            }else if(args->calibration==6){
                cont_6++;
                if(cont_6==10000*3){
                    args->params[R_HR]+=0.0006;
                    //printf("%f\n", args->params[R_HR]);
                    cont_6=0;
                }
                int ret_ecm = calc_ecm(args->vars[0] * scale_virtual_to_real + offset_virtual_to_real, ret_values[0], rafaga_viva_pts, &ecm_result);
                msg.ecm = ecm_result;
                msg.extra = args->params[R_HR];
                
            }else if(args->calibration==7){
                if (cal_7==TRUE){
                    double paso_fast = 0.2;//0.2; //0.3
                    double max_fast = 1;//1.8; //2.7
                    double paso_slow = 0.01;
                    double max_slow = 0.11;

                    //Mapa de conductancia 
                    counter_mapa++;
                    if (counter_mapa>=10000*10){ //Cada 10s hay cambio
                        counter_mapa=0;
                        g_virtual_to_real[G_SLOW] += paso_slow;
                        if (g_virtual_to_real[G_SLOW]>max_slow){
                            g_virtual_to_real[G_SLOW] = 0;
                            g_real_to_virtual[G_FAST] += paso_fast;
                            if(g_real_to_virtual[G_FAST]>=max_fast){
                                printf("FIN\n");
                                printf("Apuntar: %d\n", cont_send);
                                g_virtual_to_real[G_SLOW] = 0;
                                g_real_to_virtual[G_FAST] = 0;
                                cal_7=FALSE;
                            }
                        }
                    }
                }
            }else if(args->calibration==8){
                if (cal_7==TRUE){
                    double paso_k1 = 0.3;
                    double paso_k2 = 0.02;
                    double max_k1 = 1.6;
                    double max_k2 = 0.1;

                    //Mapa de k 
                    counter_mapa++;
                    if (counter_mapa>=10000*10){ //Cada 10s hay cambio
                        counter_mapa=0;
                        syn_aux_params[SC_MS_K1]+=paso_k1;
                        if(syn_aux_params[SC_MS_K1]>=max_k1){
                            syn_aux_params[SC_MS_K1]=ini_k1;
                            syn_aux_params[SC_MS_K2]+=paso_k2;
                            if( syn_aux_params[SC_MS_K2]>=max_k2){
                                printf("FIN\n");
                                printf("Apuntar: %d\n", cont_send);
                                syn_aux_params[SC_MS_K1]=0;
                                syn_aux_params[SC_MS_K2]=0;
                                cal_7=FALSE;
                            }
                        }
                    }
                }
                msg.ecm=syn_aux_params[SC_MS_K1];
                msg.extra=syn_aux_params[SC_MS_K2];

            }
            

            /*GUARDAR INFO*/
            send_to_queue(args->msqid, &msg);

            /*TIEMPO*/
            ts_add_time(&ts_target, 0, args->period);

            /*LECTURA DE LA TARJETA*/
            if (read_comedi(session, args->n_in_chan, args->in_channels, ret_values) != 0) {
                /*ALGO FALLO*/
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
                free(lectura_a);
                free(lectura_b);
                free(lectura_t);
                pthread_exit(NULL);
            }
        }

        /*CALCULO CORRIENTE E INTEGRACIÓN DEL MODELO*/
        if (args->type_syn==CHEMICAL)
            syn_aux_params[SC_MIN] = min_abs_real * scale_real_to_virtual + offset_real_to_virtual;
        args->syn(ret_values[0] * scale_real_to_virtual + offset_real_to_virtual, args->vars[0], g_real_to_virtual, &c_real, syn_aux_params);
        if (args->type_syn==CHEMICAL)
            syn_aux_params[SC_MIN] = min_abs_real;
        args->syn(ret_values[0], args->vars[0]*scale_virtual_to_real + offset_virtual_to_real, g_real_to_virtual, &(msg.c_real), syn_aux_params);
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
    free(lectura_a);
    free(lectura_b);
    free(lectura_t);

    printf("End rt\n");
    pthread_exit(NULL);
}
