CC = gcc
LIBS_COMEDI = -g -lcomedi -lm
LIBS_RT = -lpthread -D_GNU_SOURCE

main: main.o model_library.o real_time_functions.o comedi_functions.o calibrate_functions_phase1.o calibrate_functions_phase2.o time_management.o
	$(CC) -o main main.o model_library.o real_time_functions.o comedi_functions.o calibrate_functions_phase1.o calibrate_functions_phase2.o time_management.o $(LIBS_COMEDI) $(LIBS_RT)

main.o: main.c model_library.c model_library.h real_time_functions.c real_time_functions.h comedi_functions.c comedi_functions.h
	$(CC) -c $(LIBS_COMEDI) main.c

model_library.o: model_library.c model_library.h
	$(CC) -c -g model_library.c

real_time_functions.o: real_time_functions.c real_time_functions.h
	$(CC) -c $(LIBS_COMEDI) $(LIBS_RT) real_time_functions.c

comedi_functions.o: comedi_functions.c comedi_functions.h
	$(CC) -c $(LIBS_COMEDI) comedi_functions.c

calibrate_functions_phase1.o: calibrate_functions_phase1.c calibrate_functions_phase1.h
	$(CC) -c calibrate_functions_phase1.c

calibrate_functions_phase2.o: calibrate_functions_phase2.c calibrate_functions_phase2.h
	$(CC) -c calibrate_functions_phase2.c

time_management.o: time_management.c time_management.h
	$(CC) -c time_management.c

clean:
	rm -f main *.o