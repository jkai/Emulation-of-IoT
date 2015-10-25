#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>
#include <limits.h>

#define FIFO_NAME "/tmp/junjie_fifo"
#define CLOUD_FIFO_NAME "/tmp/junjie_cloud_fifo"
#define BUFFER_SIZE 512
#define MESSAGE_KEY 1191
#define DEVICE_MESSAGE_TYPE 1
#define NOTICE_MESSAGE_TYPE 2
#define FETCH_MESSAGE_TYPE 3

typedef enum {
	SENSOR,
	ACTUATOR
}Device_Type;

typedef enum {
	INIT,
	RUN,
	QUIT
}Status;

typedef struct {
	pid_t pid;              	//Process id
    char name [25];    //Device name, e.g. smoke sensor
    Device_Type device_type; 	//Device type
    int threshold;        	 	//Threshold value of Device, 
    int current_value;     	 	//current_value of sensor
    Status status;		//Device status 
	bool if_threhold_crossing;
}device_info;

typedef struct {
	int device_list_size;
    device_info device_list[10];
}fetch_info;

typedef struct {
	long int message_type;
    device_info info;
}message_struct;

typedef struct {
	char name[25];
	int sensor_data;
	int threshold;
	char actuator[25];
	bool ifFetch;
	Device_Type device_type; 	//Device type
}notice_info;

typedef struct {
	long int message_type;
    notice_info info;
}notice_struct;

typedef struct {
	long int message_type;
    fetch_info info;
}fetch_struct;





