#include "assignment1.h"

static int message_queue_id;

void setupMessageQueue()
{
	message_queue_id = msgget((key_t)MESSAGE_KEY, 0666 | IPC_CREAT);
	if (message_queue_id == -1) {
        fprintf(stderr, "msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    printf("message queue is setup: %d\n", message_queue_id);
}

/**/
int main(int argc, char *argv[])
{
	message_struct actuator_data, control_data;
	control_data.info.status = INIT;
	control_data.message_type = getpid();
	if (argc != 2) {			
		//argv must include Actuator name
        perror("Please input proper actuator name.\n");
        exit(EXIT_FAILURE);
    }
	
	printf("Fetching Actuator information...\n");
	printf("Actuator name is %s\n", argv[1]);
	strcpy(actuator_data.info.name, argv[1]);
    actuator_data.info.device_type = ACTUATOR;
	actuator_data.info.status = INIT;
    actuator_data.info.threshold = 1000;
	actuator_data.info.current_value = 0;
	actuator_data.info.pid = getpid();
    if (actuator_data.info.pid == -1) {
        perror("Get PID failed!\n");
        exit(EXIT_FAILURE);
    }
    printf("Device's PID = %d\n", actuator_data.info.pid);
	setupMessageQueue();
	
	/*Send Controller the actuator info*/
	actuator_data.message_type = DEVICE_MESSAGE_TYPE;
    if (msgsnd(message_queue_id, (void *)&actuator_data, sizeof(actuator_data.info), 0) == -1) {
        fprintf(stderr, "msgsnd failed: %d\n", errno);
        exit(EXIT_FAILURE);
    }
	printf("Sent info to Controller.\n");
	actuator_data.info.status = RUN;

	/*Run as a actuator*/
	while (control_data.info.status != QUIT)
	{
		/*Check Server ACK*/
		if (msgrcv(message_queue_id, (void *)&control_data, sizeof(control_data.info), actuator_data.info.pid, IPC_NOWAIT) == -1)
		{	
			if (errno == ENOMSG) {continue;}
			else
			{
				printf("[%s]Alarm: shutting down...\n", actuator_data.info.name);
				exit(EXIT_SUCCESS);
			}  
		}
		
		/*Run if said so*/
		if (control_data.info.status == RUN)
		{
			//Send info to Controller
			if (msgsnd(message_queue_id, (void *)&actuator_data, sizeof(actuator_data.info), 0) == -1)
			{
				fprintf(stderr, "msgsnd failed: %d\n", errno);
				exit(EXIT_FAILURE);
			}
			printf("[Action][%s] is started!\n", actuator_data.info.name);
		}

		if (control_data.info.status == QUIT)
		{
			printf("From Cloud: [%s]Alarm: quitting!\n", actuator_data.info.name);
			exit(EXIT_SUCCESS);	
		}	
	}
	
	/*Quit*/
	printf("From Controller: Actuator shutting down...\n");
	exit(EXIT_SUCCESS);	
}

