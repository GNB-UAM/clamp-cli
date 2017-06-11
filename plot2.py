import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import argparse
import math

ap = argparse.ArgumentParser()
ap.add_argument("-f", "--file", required=True,
	help="date")
#ap.add_argument("-n", "--name", required=True,
	#help="output name")
ap.add_argument("-t", "--title", required=True,
	help="title")
args = vars(ap.parse_args())

dataset = pd.read_csv(args["file"]+"_2.txt", delimiter=' ', header=None)
array = dataset.values

time = array[:, 0] / 1000
index = array[:,1]
ecm = array[:,2]
g0 = array[:,3]

plt.plot(time, ecm, label="ECM")
plt.title(args["title"])
plt.legend()
plt.show()