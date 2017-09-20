#include "../includes/calibrate_functions_phase2_a.h"

int auto_calibration(
					rt_args * args,
					calibration_args * cs,
					double * ret_values,
					double rafaga_viva_pts,
					double * ecm_result,
					message * msg,
					int * cal_on,
					double * g_virtual_to_real,
					double * g_real_to_virtual
					){

	if(args->calibration==1 || args->calibration==2 || args->calibration==3){
		
        //Electrica en fase - ecm
		int ret_ecm = calc_ecm(args->vars[0] * cs->scale_virtual_to_real + cs->offset_virtual_to_real, ret_values[0], rafaga_viva_pts, ecm_result);
        msg->ecm = *ecm_result;

        if(*cal_on && ret_ecm==1){
        	int is_syn;
            if (args->calibration == 1){
                //Porcentaje
                is_syn = is_syn_by_percentage(*ecm_result);
            }else if (args->calibration == 2){
                //Pendiente
                is_syn = is_syn_by_slope(*ecm_result);
            }else if (args->calibration == 3){
                //Var
                is_syn = is_syn_by_variance(*ecm_result);
            }

            if (is_syn==TRUE){
                printf("CALIBRATION END: g=%f\n", g_virtual_to_real[0]);
                *cal_on=FALSE;
            }else if(is_syn==FALSE && *cal_on==TRUE){
                //printf("%f\n", g_virtual_to_real[0]);
                change_g(&g_virtual_to_real[0]);
                change_g(&g_real_to_virtual[0]);
            }
        }

	}	
}