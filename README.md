# clamp-cli
Command line program to make neuron hybrid circuits

### Developed by
Rodrigo Amaducci

Manuel Reyes-Sanchez (manuel.reyes@uam.es - [scholar](https://scholar.google.es/citations?user=JlKzj1cAAAAJ))

Grupo de Neurocomputación Biológica [(GNB)](http://arantxa.ii.uam.es/~gnb/) - Escuela Politécnica Superior [(EPS)](http://www.uam.es/ss/Satellite/EscuelaPolitecnica/es/home.htm)

Universidad Autónoma de Madrid [(UAM)](http://www.uam.es)

![alt tag](https://raw.githubusercontent.com/manurs/clamp-cli/master/img.png)

## Dependencies
- Linux RT kernel (RT PREEMPT)
- Comedi and DAQ

## Install
- Debian 9 [guide](https://github.com/manurs/clamp-cli/wiki/Install-on-Debian-9)

## Usage

```
-f, --frequency: sample frequency (in KHz)
-t, --time: simulation time (in ns)
-m, --model: neural model (0 = Izhikevich, 1 = Hindmarsh-Rose, 2 = Rulkov Map)
-s, --synapse: synapse type (0 = electrical, 1 = gradual)
-a, --antiphase: turn on antiphase
-c, --calibration: automatic calibration process
 - Don't use with antiphase
 - Synapse will be ignored
 - Number codes in github wiki
-I, --Important: mark experiment in summary.txt
-i, --input_channels: input channels, separated by commas (ej: 0,2,3,7)
-o, --output_channels: output channels, separated by commas (ej: 0,2,3,7)
-h, --help: print this help
```


## Data file details
see data/about.txt

## Plots
Some Python Matplotlib programs are included. The programs read the files properly and include the data in Python variables. You can adapt the programs to plot what you need. 

## Neuron models
- Hindmarsh–Rose
- Izhikevich
- Rulkov map

## Synapsis models
- Electrical synapse
- Gradual chemical synapse
