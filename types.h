#define ERR -1
#define OK 0
#define TRUE 1
#define FALSE 0

typedef struct {
    long id;
    int i; // Also s_points
    double t_unix; // Also period_disp_real
    double t_absol;

    /* File 1*/ 
    long lat;
    double v_model;
    double v_model_scaled;
    double c_model;
    double c_real;
    int n_in_chan;
    int n_out_chan;
    double * data_in;
    double * data_out;
    /* File 2*/
    double * g_real_to_virtual;
    double * g_virtual_to_real;
    int n_g;
    double ecm; 
    double extra;
    char mensaje [100];
} message;