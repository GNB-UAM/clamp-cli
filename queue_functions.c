#include "queue_functions.h"




int send_to_queue (void * msqid, message * msg) {
	int id = (int)(*msqid);

	return msgsnd(id, (struct msgbuf *) msg, sizeof((*msg)) - sizeof(long), IPC_NOWAIT);
}



int receive_from_queue (void * msqid, message * msg) {
	int id = (int)(*msqid);

	return msgrcv(id, (struct msgbuf *) msg, sizeof((*msg)) - sizeof(long), 1, 0);
}