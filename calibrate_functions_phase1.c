#include "calibrate_functions_phase1.h"

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