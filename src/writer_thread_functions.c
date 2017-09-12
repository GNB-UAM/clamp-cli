#include "../includes/writer_thread_functions.h"


void * writer_thread(void * arg) {
    message msg;
    message msg2;
    pthread_t id;
    FILE * f1, * f2, *f3;
    writer_args * args;
    int i = 0, j;
    int s_points;

    args = arg;
    id = pthread_self();

    char * filename_1 = (char *) malloc (sizeof(char)*(strlen(args->filename)+7));
    char * filename_2 = (char *) malloc (sizeof(char)*(strlen(args->filename)+7));
    char * filename_3 = (char *) malloc (sizeof(char)*(strlen(args->path)+13));

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


    while(1) {
        receive_from_queue(args->msqid, &msg);

        if (msg.id == 2) break;

        if (i == 0) {
            fprintf(f1, "%d %d\n", msg.n_in_chan, msg.n_out_chan);
            i++;
        }

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
        free(msg.g_virtual_to_real);
        free(msg.g_real_to_virtual);
    }
    
    fclose(f1);
    fclose(f2);
    free(filename_1);
    free(filename_2);
    free(filename_3);

    printf("End writer_thread.\n");
    pthread_exit(NULL);
}