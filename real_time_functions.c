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

int ini_recibido (double *min, double *minABS, double *max, Comedi_session session_v, comedi_range * range_info_in_v, lsampl_t maxdata_in_v){
    
    /*Vamos a escanear 10000 puntos durante x segundos para determinar min y max*/
    int i=0;
    double valor_recibido=0.0, valor_old=0.0, resta=0.0, pendiente_max=-999999;
    struct timespec ts_target, ts_iter, ts_result, ts_start;
    //double bajada_mayor=-999999;
    //double subida_mayor=-999999;

    int segs_observo=8;    

    double maxi=-999999;
    double mini=999999;
    double miniB=999999;


    clock_gettime(CLOCK_MONOTONIC, &ts_target);
    ts_assign (&ts_start,  ts_target);
    ts_add_time(&ts_target, 0, PERIOD);


    for (i=0; i<10000*(segs_observo); i++){

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts_target, NULL);
        valor_recibido = read_single_data_comedi(session_v, range_info_in_v, maxdata_in_v);

        //if (valor_recibido = 999) return -1;

        if(valor_recibido>maxi){
            maxi=valor_recibido;
        }else if(valor_recibido<mini){
            mini=valor_recibido;
        }

        if(i>2){
            resta=valor_recibido-valor_old;
            
            if(resta>pendiente_max){
                pendiente_max=resta;
                miniB=valor_old;
            }
        }
        
        if(i%10==0)
            valor_old=valor_recibido;

        ts_add_time(&ts_target, 0, PERIOD); 
        

    }


    /*printf("LECTURA INICIAL\n");
    printf("  max_leido=%f ", maxi);
    printf("// min_leido=%f ", mini);
    printf("// min_leido_rel=%f\n\n", mini*0.55);*/

    *minABS = mini;
    *min = mini*0.55;
    *max = maxi;
    return 1;
}


void calcula_escala (double min_virtual, double max_virtual, double min_viva, double max_viva, double *escala_virtual_a_viva, double *escala_viva_a_virtual, double *offset_virtual_a_viva, double *offset_viva_a_virtual){
    
    double rg_virtual, rg_viva;
    
    rg_virtual = max_virtual-min_virtual;
    rg_viva = max_viva-min_viva;
    
    //printf("rg_virtual=%f, rg_viva=%f\n", rg_virtual, rg_viva);
    
    *escala_virtual_a_viva = rg_viva / rg_virtual;
    *escala_viva_a_virtual = rg_virtual / rg_viva;
    
    *offset_virtual_a_viva = min_viva - (min_virtual*(*escala_virtual_a_viva));
    *offset_viva_a_virtual = min_virtual - (min_viva*(*escala_viva_a_virtual));

    //printf("ESCALAS CALCULADAS\n\n");
    return;
}


void ini_hr (double * vars, double *min, double *minABS, double *max){
    vars[0]=-0.712841;
    vars[1]=-1.936878;
    vars[2]=3.165682;
    *min=-1.608734;
    *minABS=-1.608734;
    *max=1.797032;
    //rafaga_hr=260166;
    return;
}

void ini_iz (double * vars, double *min, double *minABS, double *max){
    vars[0]=30.240263;
    vars[1]=-5.544592;
    *min=-50.000000;
    *minABS=-74.235106;
    *max=30.240470;
    return;
}




/* THREADS FUNCTIONS */
void * writer_thread(void * arg) {
    message msg;
    pthread_t id;
    FILE * f;
    writer_args * args;
    int i;

    args = arg;
    id = pthread_self();
    f = fopen(args->filename, "w");

    for (i = 0; i < args->points * args->s_points; i++) {
        if (i % args->s_points == 0) {
            msgrcv(args->msqid, (struct msgbuf *)&msg, sizeof(message) - sizeof(long), 1, 0);

            fprintf(f, "%f %f %f %f %f %f %ld\n", msg.t_unix, msg.absol, msg.data, msg.nerea, msg.viva, msg.corriente_viva, msg.corriente_model, msg.lat);
        }
    }
    
    fclose(f);
    pthread_exit(NULL);
}



void * rt_thread(void * arg) {
    int i;
    rt_args * args;
    struct timespec ts_target, ts_iter, ts_result, ts_start;
    message msg;
    pthread_t id;
    double syn;

    double maxHR, minHR, minHRabs;
    double maxV, minV, minVabs;
    double escala_viva_a_virtual;
    double escala_virtual_a_viva;
    double offset_virtual_a_viva;
    double offset_viva_a_virtual;

    double g_virtual_a_viva=0.4;
    double g_viva_a_virtual=0.4;
    double corriente, valor_recibido = 0, valor_recibido2 = 0;

    double rafaga_modelo_pts_hr = 260166.0;
    double pts_por_s = 10000.0;
    double t_rafaga_viva_s = 0.4;
    double rafaga_viva_pts = pts_por_s * t_rafaga_viva_s;
    /***************/
    //args->s_points = (int) (rafaga_modelo_pts_hr / rafaga_viva_pts);


    id = pthread_self();
    args = arg;
    syn = 0;


    comedi_t * d1;
    Comedi_session session_v1;
    comedi_range * range_info_in_v1;
    lsampl_t maxdata_in_v1;
    comedi_range * range_info_out_v1;
    lsampl_t maxdata_out_v1;

    Comedi_session session_a1;
    comedi_range * range_info_in_a1;
    lsampl_t maxdata_in_a1;
    comedi_range * range_info_out_a1;
    lsampl_t maxdata_out_a1;

    comedi_t * d2;
    Comedi_session session_v2;
    comedi_range * range_info_in_v2;
    lsampl_t maxdata_in_v2;
    comedi_range * range_info_out_v2;
    lsampl_t maxdata_out_v2;

    Comedi_session session_a2;
    comedi_range * range_info_in_a2;
    lsampl_t maxdata_in_a2;
    comedi_range * range_info_out_a2;
    lsampl_t maxdata_out_a2;



    d1 = open_device_comedi("/dev/comedi0");
    d2 = open_device_comedi("/dev/comedi0");

    /*Envia el voltage para ver que hace el modelo*/

    printf("subdev1 = %d\n", comedi_find_subdevice_by_type(d1, COMEDI_SUBD_AI, 0));

    session_a1 = create_session_comedi(d1, 0, 0, 2, 1, AREF_GROUND, UNIT_mA);
    range_info_in_a1 = get_range_info_comedi(session_a1, COMEDI_INPUT);
    maxdata_in_a1 = get_maxdata_comedi(session_a1, COMEDI_INPUT);
    range_info_out_a1 = get_range_info_comedi(session_a1, COMEDI_OUTPUT);
    maxdata_out_a1 = get_maxdata_comedi(session_a1, COMEDI_OUTPUT);


    //d2 = open_device_comedi("/dev/comedi0");

    /*Envia el voltage para ver que hace el modelo*/

    //printf("subdev2 = %d\n", comedi_find_subdevice_by_type(d1, COMEDI_SUBD_AI, 0));


/*
    session_a2 = create_session_comedi(d2, 1, 1, AREF_GROUND, UNIT_mA);
    range_info_in_a2 = get_range_info_comedi(session_a2, COMEDI_INPUT);
    maxdata_in_a2 = get_maxdata_comedi(session_a2, COMEDI_INPUT);
    range_info_out_a2 = get_range_info_comedi(session_a2, COMEDI_OUTPUT);
    maxdata_out_a2 = get_maxdata_comedi(session_a2, COMEDI_OUTPUT);
*/


    prepare_real_time(id);



    ini_iz(args->vars, &minHR, &minHRabs, &maxHR);
        printf("subdev1 = %d\n", comedi_find_subdevice_by_type(d1, COMEDI_SUBD_AI, 0));

    if ( ini_recibido (&minV, &minVabs, &maxV, session_a1, range_info_in_a1, maxdata_in_a1) == -1 ) return NULL;
        printf("subdev1 = %d\n", comedi_find_subdevice_by_type(d1, COMEDI_SUBD_AI, 0));

    calcula_escala (minHRabs, maxHR, minVabs, maxV, &escala_virtual_a_viva, &escala_viva_a_virtual, &offset_virtual_a_viva, &offset_viva_a_virtual);
    printf("subdev1 = %d\n", comedi_find_subdevice_by_type(d1, COMEDI_SUBD_AI, 0));


    clock_gettime(CLOCK_MONOTONIC, &ts_target);
    ts_assign (&ts_start,  ts_target);
    ts_add_time(&ts_target, 0, PERIOD);


    for (i = 0; i < args->points * args->s_points; i++) {
        if (i % args->s_points == 0) {
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts_target, NULL);
            clock_gettime(CLOCK_MONOTONIC, &ts_iter);

            ts_substraction(&ts_target, &ts_iter, &ts_result);
            msg.id = 1;
            msg.data = args->vars[0] *escala_virtual_a_viva + offset_virtual_a_viva;
            msg.lat = ts_result.tv_sec * NSEC_PER_SEC + ts_result.tv_nsec;
            msg.nerea = valor_recibido2;
            msg.viva = valor_recibido;
            msg.t_unix = (ts_iter.tv_sec * NSEC_PER_SEC + ts_iter.tv_nsec) * 0.000001;
            msg.corriente_viva = syn;

            ts_substraction(&ts_start, &ts_iter, &ts_result);
            msg.absol = (ts_result.tv_sec * NSEC_PER_SEC + ts_result.tv_nsec) * 0.000001;

            corriente=g_virtual_a_viva * ( args->vars[0] *escala_virtual_a_viva + offset_virtual_a_viva - valor_recibido);
            msg.corriente_model = corriente;

            write_single_data_comedi(session_a1, range_info_out_a1, maxdata_out_a1, (valor_recibido2 *escala_virtual_a_viva + offset_virtual_a_viva - valor_recibido));

            
            write_single_data_comedi2(session_a1, range_info_out_a1, maxdata_out_a1, (valor_recibido *escala_viva_a_virtual + offset_viva_a_virtual - valor_recibido2));

            msgsnd(args->msqid, (struct msgbuf *) &msg, sizeof(message) - sizeof(long), IPC_NOWAIT);

            ts_add_time(&ts_target, 0, PERIOD);

            valor_recibido = read_single_data_comedi(session_a1, range_info_in_a1, maxdata_out_a1);
            valor_recibido2 = read_single_data_comedi2(session_a1, range_info_in_a1, maxdata_out_a1);
        }

        
        syn = -( g_viva_a_virtual * ( valor_recibido*escala_viva_a_virtual + offset_viva_a_virtual - args->vars[0] ) );

        //args->func(args->dim, args->dt, args->vars, args->params, syn);
        //args->vars[0] = read_single_data_comedi(session_a2, range_info_in_a2, maxdata_out_a2);
        //write_single_data_comedi(session_a2, range_info_out_a2, maxdata_out_a2, syn);
    }


    write_single_data_comedi(session_a1, range_info_out_a1, maxdata_out_a1, 0);
    write_single_data_comedi2(session_a1, range_info_out_a1, maxdata_out_a1, 0);
    close_device_comedi(d1);
    //close_device_comedi(d2);
    pthread_exit(NULL);
}
