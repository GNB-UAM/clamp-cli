#include "calibrate_functions_phase2.h"

/**********************************************
*
*		SCORE OF THE CALIBRATION
*
**********************************************/

//Futuro: no variables globales 
//        conseguir "orientacion a objetos" por medio de una struct que se pasa siempre

/*
* Args:
*         v_a and v_b must be in the same scale
*
* Return: 
*         -1 ERR
*          0 OK, no ecm result yet
*          1 OK, ecm result ready
*/

int count_a_calc_ecm = 0; 
double ecm_calc_ecm =0;

int calc_ecm (double v_a, double v_b, int life_burst_points, double *ecm_result){
	int num_burst = 3;
	double diff = 0;

	if(count_a_calc_ecm<life_burst_points*num_burst){
		count_a_calc_ecm++;
		diff = (v_a-v_b);
		ecm_calc_ecm += diff * diff;
		return 0;
	}else{
		count_a_calc_ecm=0;
		ecm_calc_ecm = ecm_calc_ecm / life_burst_points * num_burst;
		*ecm_result = ecm_calc_ecm;
		return 1;
	}
}

/*
*/
int calc_phase (double v_a, double v_b, double *phase_result){

	return 1;

}

/**********************************************
*
*			SYNCHRONIZATION
*
**********************************************/

double first_val_is_syn_by_percentage = -1;
int count_is_syn_by_percentage = 0; 

int is_syn_by_percentage(double val_sin){
	double percentage = 0.7;
	int times_correct = 2;
	
	/*FIRST TIME*/
	if (first_val_is_syn_by_percentage == -1){
		first_val_is_syn_by_percentage = val_sin;
		count_is_syn_by_percentage = 0;
		return FALSE;
	}
    /*PERCENTAGE REACHED*/
	else if (first_val_is_syn_by_percentage*percentage>val_sin){
		/*ENOUGH CONSECUTIVE TIMES*/
		if(count_is_syn_by_percentage<times_correct){
			count_is_syn_by_percentage++;
			return FALSE;
		}else{
			return TRUE;
		}
	/*NO YET*/
	}else{
		count_is_syn_by_percentage = 0;
		return FALSE;
	}
}

/*
*/

double last_val_sin_is_syn_by_slope_1 = -1;
double last_val_sin_is_syn_by_slope_2 = -1;

int is_syn_by_slope(double val_sin){
	double tolerance = 0.4;

	/*FIRST TIME*/
	if (last_val_sin_is_syn_by_slope_1 == -1){
		last_val_sin_is_syn_by_slope_1 = val_sin;
		return FALSE;
	} else if(last_val_sin_is_syn_by_slope_2 == -1){
		last_val_sin_is_syn_by_slope_2 = val_sin;
		return FALSE;
	}
	/*SLOPE*/
	double slope_1 = last_val_sin_is_syn_by_slope_2 - last_val_sin_is_syn_by_slope_1;
	double slope_2 = val_sin - last_val_sin_is_syn_by_slope_2;
	last_val_sin_is_syn_by_slope_2 = last_val_sin_is_syn_by_slope_1;
	last_val_sin_is_syn_by_slope_1 = val_sin;

	/*SLOPE IS THE SAME??*/
	double diff = fabs(slope_2-slope_1);
	if (diff<=tolerance){
		return TRUE;
	}else{
		return FALSE;
	}
}

/*
*/

double vals_is_syn_by_variance[4] = {-1};
int num_variances_is_syn_by_variance = 4;
int count_is_syn_by_variance = 0;

int is_syn_by_variance(double val_sin){
	double tolerance = 0.1;

	vals_is_syn_by_variance[count_is_syn_by_variance % num_variances_is_syn_by_variance] = val_sin;
	count_is_syn_by_variance++;

	int i;
	double media=0;
	for (i=0; i<num_variances_is_syn_by_variance; i++){
		if (vals_is_syn_by_variance[i] == -1){
			return FALSE;
		}
		media += vals_is_syn_by_variance[i];
	}
	media = media / num_variances_is_syn_by_variance;

    double res = 0;
    double tmp;
	for (i=0; i<num_variances_is_syn_by_variance; i++){
		tmp = vals_is_syn_by_variance[i] - media;
		res += tmp * tmp;
	}
	res = res / num_variances_is_syn_by_variance;

	if (res<=tolerance){
		return TRUE;
	}else{
		return FALSE;
	}
}

/**********************************************
*
*			CHANGE PARAMETERS
*
**********************************************/



