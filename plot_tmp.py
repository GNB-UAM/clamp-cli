import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import argparse
import math

ap = argparse.ArgumentParser()
ap.add_argument("-f", "--file", required=True, help="file")
ap.add_argument("-f2", "--file2", required=False, help="file")
args = vars(ap.parse_args())

dataset = pd.read_csv(args["file"], delimiter=' ', header=1)
array = dataset.values

v = array[:, 0]

t = np.linspace(0,len(v), len(v))
t = t / 10000

f, axarr = plt.subplots(1, 3, sharex=True, figsize=(12,6))

axarr[0].plot(t, v, label="", linewidth=0.4, color='C1')
axarr[0].set_title("Señal sin filtrar")
axarr[0].legend()

if 1==1:

	dataset = pd.read_csv(args["file2"], delimiter=' ', header=1)
	array = dataset.values

	v = array[:, 0]

	axarr[1].plot(t, v, label="", linewidth=0.4, color='C1')
	axarr[1].set_title("Señal filtrada - 1 convolución")
	axarr[1].legend()

if 1==1:

	dataset = pd.read_csv(args["file2"], delimiter=' ', header=1)
	array = dataset.values

	v = array[:, 0]

	for veces in range(9):

		for i in range(len(v)):
			if i>3:
				v[i] = 0.2*v[i] + 0.2*v[i-1] + 0.2*v[i-2] + 0.2*v[i-3] + 0.2*v[i-4]

	axarr[2].plot(t, v, label="", linewidth=0.4, color='C1')
	axarr[2].set_title("Señal filtrada - 10 convoluciones")
	axarr[2].legend()

axarr[0].set_ylabel('Potencial de membrama')
axarr[1].set_ylabel('Potencial de membrama')
axarr[2].set_ylabel('Potencial de membrama')
axarr[0].set_xlabel('Tiempo (s)')
axarr[1].set_xlabel('Tiempo (s)')
axarr[2].set_xlabel('Tiempo (s)')
plt.tight_layout()
plt.show()