import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import argparse
import math

ap = argparse.ArgumentParser()
ap.add_argument("-f", "--file", required=True, help="date")
#ap.add_argument("-n", "--name", required=True, help="output name")
ap.add_argument("-c", "--calibration", required=True, help="calibration")
args = vars(ap.parse_args())

##### FICHERO 1
dataset = pd.read_csv(args["file"]+"_1.txt", delimiter=' ', header=1)
array = dataset.values
data = array[1:,:]

t = t_absol = data[:,1] / 1000
v_model_scaled = data[:, 5]
v_live = data[:, 8]

##### FICHERO 2

dataset = pd.read_csv(args["file"]+"_2.txt", delimiter=' ', header=None)
array = dataset.values

time = array[:, 0] / 1000
index = array[:,1]
ecm = array[:,2]
extra = array[:,3]
g0 = array[:,4]

def c6():

	f, axarr = plt.subplots(3, sharex=True, figsize=(8.5,5.1))

	axarr[0].plot(t, v_model_scaled, label="Modelo", linewidth=0.4)
	axarr[0].plot(t, v_live, label="Viva", linewidth=0.4)
	axarr[0].set_title("Voltaje")
	axarr[0].legend()

	axarr[1].plot(time, ecm)
	axarr[1].set_title("ECM")
	axarr[1].legend()

	axarr[2].plot(time, extra)
	axarr[2].set_title("Parametro que cambia")
	axarr[2].legend()

	plt.xlabel("Tiempo (s)")
	plt.tight_layout()
	plt.show()


def c1():
	
	aux=[]
	for i in range(len(ecm)):
		if i>0:
			if ecm[i]!=ecm[i-1] and t[i]<5:
				aux.append(ecm[i])

	ini = sum(aux) / len (aux)
	

	f, axarr = plt.subplots(3, sharex=True, figsize=(8.5,5.1))

	axarr[0].plot(t, v_model_scaled, label="Modelo", linewidth=0.4)
	axarr[0].plot(t, v_live, label="Viva", linewidth=0.4)
	axarr[0].set_title("Voltaje")
	axarr[0].legend(loc=1)

	axarr[1].plot(time, ecm)
	axarr[1].set_title("ECM")
	axarr[1].axhline(y=ini, color='r', linestyle='--', linewidth=0.4, label="ECM medio inicial")
	axarr[1].axhline(y=ini*0.4, color='g', linestyle='--', linewidth=0.4, label="ECM objetivo")
	#axarr[1].axvline(x=5, color='b', linestyle='--', linewidth=0.4, label="Inicio")
	axarr[1].legend(loc=1)

	axarr[2].plot(time, g0)
	axarr[2].set_title("Conductancia")
	axarr[2].legend(loc=1)

	plt.xlim([0, 20])
	plt.xlabel("Tiempo (s)")
	plt.tight_layout()
	plt.show()


if args["calibration"]=="1":
	c1()


if args["calibration"]=="6":
	c6()