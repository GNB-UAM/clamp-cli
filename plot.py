import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import argparse
import math

import plot_aux as aux
import plot_funcs as funcs

ap = argparse.ArgumentParser()
data1 = aux.DataStruct1(ap)
data2 = aux.DataStruct2(ap)

funcs.voltage(data1)
funcs.lat_dist(data1)

print (data2.ecm[0])