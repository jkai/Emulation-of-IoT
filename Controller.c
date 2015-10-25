#include "assignment1.h"

static int message_queue_id;
static bool controller_stop = 0;
static device_info device_list[10] = {};
static int device_list_size = 0;
static int pipe_fd;
static int cloud_res;
static fetch_struct fetch_buffer;
static pid_t required_pid = 0;
static int measurement_buffer = 0;

void setupMessageQueue();
void sendControlMessage(int pid, Status status);
void registerDevice(device_info info);
void startActuator(char sensor_name[25], int sensor_data, int sensor_threshold);
void childProcess();
void parentProcess(int child_pid);
void cleanUp();

int main(int argc, char *argv[]) {

	if (argc != 2) {			
		//argv must include Controller name
        perror("Please input proper controller name.\n");
        exit(EXIT_FAILURE);
    }
	printf("Controller name is %s\n", argv[1]);
	setupMessageQueue();

	if (access(CLOUD_FIFO_NAME,F_OK) == -1) {
		printf("can't access cloud fifo, making one\n");
		if (mkfifo(CLOUD_FIFO_NAME,0666) != 0) {
			fprintf(stderr,"Could not create fifo: %s\n", CLOUD_FIFO_NAME);
			exit(EXIT_FAILURE);
		} 
	}

	if (access(FIFO_NAME,F_OK) == -1) {
		printf("can't access fifo, making one\n");
		if (mkfifo(FIFO_NAME,0666) != 0) {
			fprintf(stderr,"Could not create fifo: %s\n", FIFO_NAME);
			exit(EXIT_FAILURE);
		} 
	}

	cloud_res = open(CLOUD_FIFO_NAME, O_RDONLY| O_NONBLOCK);

	int pid = fork();
	if ( pid == 0 )
	{
		childProcess();
	}
	else
	{
		parentProcess(pid);
	}
	
	exit(EXIT_FAILURE);
}

void cloud_sig_handler(int sig)
{
	printf("\n[Parent]Set controller running flag to quit!\n");
	controller_stop = 1;
}

void fetch_sig_handler(int sig)
{
	int size_buffer = device_list_size;
	fetch_buffer.message_type = FETCH_MESSAGE_TYPE;
	fetch_buffer.info.device_list_size = size_buffer;
	for (int i = 0; i < size_buffer; i++)
	{
		fetch_buffer.info.device_list[i].pid = device_list[i].pid;
		fetch_buffer.info.device_list[i].device_type = device_list[i].device_type;
		fetch_buffer.info.device_list[i].current_value = measurement_buffer;
		fetch_buffer.info.device_list[i].threshold = device_list[i].threshold;
		strcpy(fetch_buffer.info.device_list[i].name,device_list[i].name);
	}

	if (msgsnd(message_queue_id, (void *)&fetch_buffer, sizeof(fetch_buffer.info), 0) == -1)
	{
       	fprintf(stderr, "msgsnd to parent failed: %d\n", errno);
    }
    kill(getppid(),SIGUSR1);
}

void notice_sig_handler(int sig)
{
	char buffer[BUFFER_SIZE+1];
	notice_struct notice;
	fetch_struct fetch;
	if (msgrcv(message_queue_id, (void *)&notice, sizeof(notice.info), NOTICE_MESSAGE_TYPE, IPC_NOWAIT) == -1)
		{
			if (errno == EINTR) {printf("Caught signal while blocked on msgrcv(). Retrying... msgrcv()\n");}
		}

	if (msgrcv(message_queue_id, (void *)&fetch, sizeof(fetch.info), FETCH_MESSAGE_TYPE, IPC_NOWAIT) == -1)
		{
			if (errno == EINTR) {printf("Caught signal while blocked on msgrcv(). Retrying... msgrcv()\n");}
		}

	if (fetch.info.device_list_size != 0 && required_pid != 0)
	{
		for (int i = 0; i < fetch.info.device_list_size; i++)
		{
			if (fetch.info.device_list[i].pid == required_pid)
			{
				if (fetch.info.device_list[i].device_type == SENSOR)
				{
					sprintf(buffer, "[Controller Parent to Cloud]The sensor %s's current value = %d threshold = %d.", fetch.info.device_list[i].name, fetch.info.device_list[i].current_value, fetch.info.device_list[i].threshold);
					required_pid = 0;
				}
				else
				{
					sprintf(buffer, "[Controller Parent to Cloud]The actuator %s will be shut down!\n", fetch.info.device_list[i].name);
					sendControlMessage(fetch.info.device_list[i].pid, QUIT);
					required_pid = 0;
				}
			}
			if (i == fetch.info.device_list_size) {
				sprintf(buffer, "[Controller Parent to Cloud]No such a device, Please double check!\n");
			}
		}
	}
	else
	{
		if (strcmp(notice.info.actuator, "idle")==0)
		{
			sprintf(buffer, "[Controller Parent to Cloud]A threshold crossing occurs in Sensor %s, its current value = %d, its threshold = %d. But no action is performed, because all available actuators are running!", notice.info.name, notice.info.sensor_data, notice.info.threshold);
		}
		else
		{
			sprintf(buffer, "[Controller Parent to Cloud]%s is started due to a threshold crossing in Sensor %s, its current value = %d, its threshold = %d.", notice.info.actuator, notice.info.name, notice.info.sensor_data, notice.info.threshold);
		}	
	}

	if (write(pipe_fd,buffer,BUFFER_SIZE) == -1 )
	{	
		fprintf(stderr,"write error on pipe\n");
	}

}

void parentProcess(int child_pid)
{
	printf("[Parent] Waiting Cloud's FIFO Setup...\n");
	pipe_fd = open(FIFO_NAME, O_WRONLY);
	printf("[Parent] is started!\n");
	char pid_buffer[BUFFER_SIZE + 1];
	char info_buffer[BUFFER_SIZE + 1];
	
	struct sigaction parent_action;
	struct sigaction notification_action;

	parent_action.sa_handler = cloud_sig_handler;
	notification_action.sa_handler = notice_sig_handler;
	sigemptyset(&parent_action.sa_mask);
	sigemptyset(&notification_action.sa_mask);
	parent_action.sa_flags = 0;
	notification_action.sa_flags = 0;

	sigaction(SIGINT, &parent_action, 0);
	sigaction(SIGUSR1, &notification_action, 0);

	while (!controller_stop)
	{
		if(read(cloud_res, pid_buffer, BUFFER_SIZE) > 0){
	        printf("[Parent][From Cloud}pid %s is received from cloud, making action...\n", pid_buffer);
	        //Find device based on pid
	        required_pid = atoi(pid_buffer);
	        printf("[Parent]Waiting child process fetching data...\n");
	        kill(child_pid,SIGUSR2);
	        printf("[Parent]Action completed!\n");
        }
	}

	printf("[Parent] is stopped!\n");
	cleanUp();
}

void setupMessageQueue()
{
	message_queue_id = msgget((key_t)MESSAGE_KEY, 0666 | IPC_CREAT);
	if (message_queue_id == -1) {
        fprintf(stderr, "msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    printf("message queue is setup: %d\n", message_queue_id);
}

void sendControlMessage(int pid, Status status)
{
	message_struct control_data;
	control_data.message_type = pid;
	control_data.info.status = status;
	
	if (msgsnd(message_queue_id, (void *)&control_data, sizeof(control_data.info), 0) == -1) {
        fprintf(stderr, "msgsnd failed: %d\n", errno);
        printf("%s may be terminated already!\n", control_data.info.name);
    }
}

void registerDevice(device_info info)
{
	device_list[device_list_size] = info;
	device_list_size++;
}

void startActuator(char sensor_name[25], int sensor_data, int sensor_threshold)
{
	int size_buffer = device_list_size;
	printf("startAC %d\n", size_buffer);
	printf("[Child]Finding proper action...\n");
	for (int i = 0; i < size_buffer; i++)
	{
		if (device_list[i].device_type == ACTUATOR && device_list[i].status != RUN)
		{
			printf("[Child]Starting %s...\n", device_list[i].name);
			sendControlMessage(device_list[i].pid, RUN);
			printf("[Child]%s is started!\n", device_list[i].name);
			device_list[i].status = RUN;
			notice_struct notice;
			notice.message_type = NOTICE_MESSAGE_TYPE;
			notice.info.ifFetch = 0;
			strcpy(notice.info.actuator, device_list[i].name);
			strcpy(notice.info.name, sensor_name);
			notice.info.sensor_data = sensor_data;
			notice.info.threshold = sensor_threshold;
			if (msgsnd(message_queue_id, (void *)&notice, sizeof(notice.info), 0) == -1)
			{
       			fprintf(stderr, "msgsnd to parent failed: %d\n", errno);
    		}
			kill(getppid(),SIGUSR1);
			break;
		}
		else
		{
			if (device_list[i].device_type == ACTUATOR)
			{
				printf("[Child]%s is already started!\n", device_list[i].name);
			}
			if (i == device_list_size - 1)
			{
				printf("No idle actuator is available, no action is taken.\n");
				notice_struct notice;
				notice.message_type = NOTICE_MESSAGE_TYPE;
				notice.info.ifFetch = 0;
				strcpy(notice.info.actuator, "idle");
				strcpy(notice.info.name, sensor_name);
				notice.info.sensor_data = sensor_data;
				notice.info.threshold = sensor_threshold;
				if (msgsnd(message_queue_id, (void *)&notice, sizeof(notice.info), 0) == -1)
				{
       				fprintf(stderr, "msgsnd to parent failed: %d\n", errno);
    			}
				kill(getppid(),SIGUSR1);
			}
		}
	}
}

void childProcess()
{
	message_struct device_data;

	struct sigaction fetch_action;
	fetch_action.sa_handler = fetch_sig_handler;
	sigemptyset(&fetch_action.sa_mask);
	fetch_action.sa_flags = 0;
	sigaction(SIGUSR2, &fetch_action, 0);
	printf("[Child] is started!\n");
	
	/* Start running */
	while (!controller_stop)
	{
		if (msgrcv(message_queue_id, (void *)&device_data, sizeof(device_data.info), DEVICE_MESSAGE_TYPE, IPC_NOWAIT) == -1)
		{
			if (errno == EINTR) {printf("Caught signal while blocked on msgrcv(). Retrying... msgrcv()\n");}
			else if (errno == ENOMSG) {continue;}
			else {exit(EXIT_FAILURE);}  
		}
		
		/* Print information */
		if (device_data.info.status == INIT)
		{
			if (device_data.info.device_type == SENSOR)
			{
				printf("[Child]A new Sensor founded: %s [pid=%d]\n", device_data.info.name, device_data.info.pid);
				//Start sensor
				printf("Starting this sensor...\n");
				sendControlMessage(device_data.info.pid, RUN);
				printf("Done.\n");
			}
			else
			{
				printf("[Child]A new actuator founded: %s [pid=%d]\n", device_data.info.name, device_data.info.pid);
			}
			registerDevice(device_data.info);
		} 
		else
		{
			if (device_data.info.device_type == SENSOR)
			{
				sendControlMessage(device_data.info.pid, RUN);
				measurement_buffer = device_data.info.current_value;
				if (device_data.info.current_value>=device_data.info.threshold)
				{
					printf("[Child]Alarm: threshold crossing occurs in [%s]!\n",device_data.info.name);
					startActuator(device_data.info.name, device_data.info.current_value, device_data.info.threshold);
				}
			}
		}
		
	}

}

void cleanUp()
{
	printf("[Child] is stopped!\n");
	printf("Cleaning up...\n");
	//Stop all registered devices
	for (int i = 0; i < device_list_size; i++)
	{
		sendControlMessage(device_list[i].pid, QUIT);
	}

	//Clean message queue
	if (msgctl(message_queue_id, IPC_RMID, 0) == -1) {
        fprintf(stderr, "msgctl(IPC_RMID) failed\n");
    }
    unlink(CLOUD_FIFO_NAME);
    unlink(FIFO_NAME);
    printf("Bye!\n");
    exit(EXIT_SUCCESS);
}




