CC = gcc
CCFLAGS = -Wall --pedantic -g -w
LIBS_COMEDI = -lcomedi -lm
LIBS_RT = -lpthread -D_GNU_SOURCE -lrt

clamp-cli_preempt: obj/clamp-cli_preempt.o obj/model_library.o obj/rt_thread_functions.o obj/writer_thread_functions.o obj/comedi_functions.o obj/calibrate_functions_phase1.o obj/calibrate_functions_phase2.o obj/calibrate_functions_phase2_a.o  obj/time_functions.o obj/queue_functions.o
	$(CC) $(CCFLAGS) -o clamp-cli_preempt obj/clamp-cli_preempt.o obj/model_library.o obj/rt_thread_functions.o obj/writer_thread_functions.o obj/comedi_functions.o obj/calibrate_functions_phase1.o obj/calibrate_functions_phase2.o obj/calibrate_functions_phase2_a.o obj/time_functions.o obj/queue_functions.o $(LIBS_COMEDI) $(LIBS_RT)
	@echo "\n\t\033[32mCompiled and created clamp-cli_preempt\033[00m\n"

obj/clamp-cli_preempt.o: src/clamp-cli_preempt.c src/model_library.c includes/model_library.h src/rt_thread_functions.c includes/rt_thread_functions.h src/writer_thread_functions.c includes/writer_thread_functions.h src/comedi_functions.c includes/comedi_functions.h includes/queue_functions.h
	@echo "\n\t\033[31mCompiling and creating clamp-cli_preempt\033[00m\n"
	$(CC) $(CCFLAGS) -c $(LIBS_COMEDI) src/clamp-cli_preempt.c -o obj/clamp-cli_preempt.o

obj/model_library.o: src/model_library.c includes/model_library.h
	$(CC) $(CCFLAGS) -c src/model_library.c -o obj/model_library.o

obj/rt_thread_functions.o: src/rt_thread_functions.c includes/rt_thread_functions.h includes/queue_functions.h
	$(CC) $(CCFLAGS) -c $(LIBS_COMEDI) $(LIBS_RT) src/rt_thread_functions.c -o obj/rt_thread_functions.o

obj/writer_thread_functions.o: src/writer_thread_functions.c includes/writer_thread_functions.h includes/queue_functions.h
	$(CC) $(CCFLAGS) -c src/writer_thread_functions.c -o obj/writer_thread_functions.o

obj/comedi_functions.o: src/comedi_functions.c includes/comedi_functions.h includes/types.h
	$(CC) $(CCFLAGS) -c $(LIBS_COMEDI) src/comedi_functions.c -o obj/comedi_functions.o

obj/calibrate_functions_phase2_a.o: src/calibrate_functions_phase2_a.c includes/calibrate_functions_phase2_a.h includes/types.h
	$(CC) $(CCFLAGS) -c src/calibrate_functions_phase2_a.c -o obj/calibrate_functions_phase2_a.o

obj/calibrate_functions_phase1.o: src/calibrate_functions_phase1.c includes/calibrate_functions_phase1.h
	$(CC) $(CCFLAGS) -c src/calibrate_functions_phase1.c -o obj/calibrate_functions_phase1.o

obj/calibrate_functions_phase2.o: src/calibrate_functions_phase2.c includes/calibrate_functions_phase2.h
	$(CC) $(CCFLAGS) -c src/calibrate_functions_phase2.c -o obj/calibrate_functions_phase2.o

obj/time_functions.o: src/time_functions.c includes/time_functions.h
	$(CC) $(CCFLAGS) -c src/time_functions.c -o obj/time_functions.o

obj/queue_functions.o: src/queue_functions.c includes/queue_functions.h includes/types.h
	$(CC) $(CCFLAGS) -c src/queue_functions.c -o obj/queue_functions.o

clean:
	rm -f clamp-cli_preempt obj/*.o

mclan:
	rm -f clamp-cli_preempt obj/*.o
	make