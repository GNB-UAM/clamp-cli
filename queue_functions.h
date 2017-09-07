/*#include <sys/ipc.h>
#include <sys/msg.h>*/
#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <sys/types.h>
#include <unistd.h>
#include "types.h"

int open_queue (void ** msqid);

int send_to_queue (void * msqid, message * msg);

int receive_from_queue (void * msqid, message * msg);

int close_queue (void ** msqid);