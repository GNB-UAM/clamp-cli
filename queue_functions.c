#include "queue_functions.h"


int open_queue (void ** msqid) {
	int ret = 0;
	key_t key_q;

	*msqid = (void *)malloc(sizeof(int));

	key_q = ftok("/bin/ls", 28);
    if (key_q == (key_t) -1) {
        perror("Error obtaining message queue key.");
        return ERR;
    }

    ret = msgget(key_q, 0600 | IPC_CREAT);
    *(int*)(*msqid) = ret;
    if (ret == -1) {
        perror("Error obtaining message queue ID.");
        return ERR;
    }

    return OK;
}

int send_to_queue (void * msqid, message * msg) {
	int id = *(int*)msqid;

	if (msgsnd(id, (struct msgbuf *) msg, sizeof((*msg)) - sizeof(long), IPC_NOWAIT) == -1) {
		return ERR;
	}

	return OK;
}



int receive_from_queue (void * msqid, message * msg) {
	int id = *(int*)msqid;

	if (msgrcv(id, (struct msgbuf *) msg, sizeof((*msg)) - sizeof(long), 1, 0) == -1) {
		return ERR;
	}

	return OK;
}

int close_queue (void ** msqid) {
	int id = *(int*)(*msqid);


	if (msgctl(id, IPC_RMID, (struct msqid_ds *)NULL) != 0) {
		return ERR;
	}

	free(*msqid);

	return OK;
}