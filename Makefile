CC = gcc
LIBS_COMEDI = -g -lcomedi -lm
LIBS_RT = -lpthread -D_GNU_SOURCE

main: main.o model_library.o real_time_functions.o comedi_functions.o calibrate_functions_phase1.o calibrate_functions_phase2.o time_functions.o queue_functions.o
	$(CC) -o main main.o model_library.o real_time_functions.o comedi_functions.o calibrate_functions_phase1.o calibrate_functions_phase2.o time_functions.o queue_functions.o $(LIBS_COMEDI) $(LIBS_RT)

main.o: main.c model_library.c model_library.h real_time_functions.c real_time_functions.h comedi_functions.c comedi_functions.h queue_functions.h
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

time_functions.o: time_functions.c time_functions.h
	$(CC) -c time_functions.c

queue_functions.o: queue_functions.c queue_functions.h types.h
	$(CC) -c queue_functions.c

clean:
	rm -f main *.o