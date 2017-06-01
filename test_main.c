#include "model_library.h"

/*
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

dataset = pd.read_csv("salida_test.txt", delimiter=' ', header=None)
array = dataset.values

v = array[:,0]

plt.plot(v)
plt.show()
*/

#define IZHIKEVICH 0
#define HR 1

int main () {
	int i, model;
	FILE * f = fopen("salida_test.txt", "w");

	double * vars = NULL;
	double * params = NULL;

	int dim;
	double dt;
	double syn = 0;
	void (*func)(int, double, double*, double*, double);

	model = 1;

	switch (model){
		case IZHIKEVICH:
			vars = (double*) malloc (sizeof(double) * 2);
			params = (double*) malloc (sizeof(double) * 4);

			vars[0] = 10.0;
			vars[1] = 0.0;

			params[I_IZ] = 10.0;
			params[A_IZ] = 0.02;
			params[B_IZ] = 0.2;
			params[C_IZ] = -50.0;
			params[D_IZ] = 2.0;

			dim = 2;
			dt = 0.03;

			func = &izhikevich;

			break;
		case HR:
			vars = (double*) malloc (sizeof(double) * 3);
			params = (double*) malloc (sizeof(double) * 3);

			vars[0] = -2.0;
			vars[1] = 0.0;
			vars[2] = 0.0;

			params[I_HR] = 3.0;
			params[R_HR] = 0.0021;
			params[S_HR] = 4.0;

			dim = 3;
			dt = 0.03;

			func = &hindmarsh_rose;

			break;
		default:
			break;
	}



	for (i = 0; i < 1000000; i++) {
		(*func)(dim, dt, vars, params, syn);
		fprintf(f, "%f\n", vars[0]);
	}

	free(vars);
	free(params);
	fclose(f);
	return 1;
}