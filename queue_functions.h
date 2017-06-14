#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>


int send_to_queue (void * msqid, message * msg);

int receive_from_queue (void * msqid, message * msg);