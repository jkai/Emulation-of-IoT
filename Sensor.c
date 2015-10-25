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
	message_struct sensor_data, control_data;
	control_data.info.status = INIT;
	control_data.message_type = getpid();
	if (argc != 3) {			
		//argv must include Device name and Device Threshold
        perror("Please input proper sensor info: Name and Threshold.\n");
        exit(EXIT_FAILURE);
    }
	
	printf("Fetching sensor information...\n");
	printf("Sensor name is %s\n", argv[1]);
	strcpy(sensor_data.info.name, argv[1]);
    sensor_data.info.device_type = SENSOR;
    printf("Sensor Threshold is %s\n", argv[2]);
    sensor_data.info.threshold = atoi(argv[2]);
    sensor_data.info.status = INIT;
	sensor_data.info.pid = getpid();
    if (sensor_data.info.pid == -1) {
        perror("Get PID failed!\n");
        exit(EXIT_FAILURE);
    }
    printf("Device's PID = %d\n", sensor_data.info.pid);
	setupMessageQueue();
	
	/*Send Controller the sensor info*/
	sensor_data.message_type = DEVICE_MESSAGE_TYPE;
    if (msgsnd(message_queue_id, (void *)&sensor_data, sizeof(sensor_data.info), 0) == -1) {
        fprintf(stderr, "msgsnd failed: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    sensor_data.info.status = RUN;
	printf("Sent info to Controller.\n");
	/*Run as a sensor*/
	while (1)
	{
		/*Check Server ACK*/
		if (msgrcv(message_queue_id, (void *)&control_data, sizeof(control_data.info), sensor_data.info.pid, IPC_NOWAIT) == -1)
		{
			if (errno == ENOMSG) {continue;}
			else
			{
				printf("[%s]Alarm: shutting down...\n", sensor_data.info.name);
				exit(EXIT_SUCCESS);
			}  
		}
		
		/*Run if said so*/
		if (control_data.info.status == RUN)
		{			
			//Sensor measurement generator, from 0 to 60
			sensor_data.info.current_value = rand() % 61;
			//Send measurement to Controller
			if (msgsnd(message_queue_id, (void *)&sensor_data, sizeof(sensor_data.info), 0) == -1)
			{
				fprintf(stderr, "msgsnd failed: %d\n", errno);
				exit(EXIT_FAILURE);
			}
			if (sensor_data.info.current_value >= sensor_data.info.threshold)
			{
				printf("[%s]Alarm: threshold crossing!\n", sensor_data.info.name);
			}
			printf("[%s]Current value=%d\n", sensor_data.info.name, sensor_data.info.current_value);
			sleep(2); //Sensor fetch data every 2 seconds.
		}
		if (control_data.info.status == QUIT)
		{
			printf("from cloud [%s]Alarm: quitting!\n", sensor_data.info.name);
			exit(EXIT_SUCCESS);	
		}

	}
	/*Quit*/
	printf("From controller: Sensor shutting down...\n");
	exit(EXIT_SUCCESS);	
}
