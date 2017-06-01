#include <stdio.h>
#include <stdlib.h>


/* MACROS */

/*Izhikevich*/
#define I_IZ 0
#define A_IZ 1
#define B_IZ 2
#define C_IZ 3
#define D_IZ 4

/*Hindmarsh-Rose*/
#define I_HR 0
#define R_HR 1
#define S_HR 2




/* INTEGRATION FUNCTIONS */
void runge_kutta_6 (void (*f) (double *, double *, double *, double), int dim, double dt, double * vars, double * params, double syn);

/* SYNAPSES */
double elec_syn (double v1, double v2, double g);

/* IZHIKEVICH */
void izh_f (double * vars, double * ret, double * params, double syn);

void izhikevich (int dim, double dt, double * vars, double * params, double syn);



/* HINDMARSH-ROSE */
void hr_f (double * vars, double * ret, double * params, double syn);

void hindmarsh_rose (int dim, double dt, double * vars, double * params, double syn);