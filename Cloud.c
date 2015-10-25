#include "assignment1.h"
static int cloud_running_flag;
static int cloud_pipe_fd;
static int res;
static char command_buffer[BUFFER_SIZE+1];
static char pid_buffer[BUFFER_SIZE+1];

void childProcess();
void parentProcess();
void writePid(char pid[BUFFER_SIZE]);

int main()
{
    cloud_running_flag = 1;
    //Setup FIFO
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

    memset(command_buffer, '\0', sizeof(command_buffer));
    memset(pid_buffer, '\0', sizeof(pid_buffer));

    int pid = fork();
    if ( pid == 0 )
    {
        childProcess();
    }
    else if (pid == -1)
    {
        exit(EXIT_FAILURE);
    }
    else
    {
        parentProcess();
    }
    
    exit(EXIT_FAILURE);
}

void writePid(char pid[BUFFER_SIZE])
{
    char send_buffer[BUFFER_SIZE];
    //Remove the \n
    strncpy(send_buffer, pid, strlen(pid)-1);
    if (write(cloud_pipe_fd,send_buffer,BUFFER_SIZE) == -1 )
    {
        fprintf(stderr,"write error on pipe\n");
    }
}

void childProcess()
{
    printf("[Cloud Child]Waiting for Controller's FIFO Setup!\n");
    cloud_pipe_fd = open(CLOUD_FIFO_NAME, O_WRONLY);
    printf("[Cloud Child]Started!\n");
    while(cloud_running_flag)
    {
        printf("Enter Command: GET or PUT, enter q to quit Cloud processes:\n");
        fgets(command_buffer, BUFFER_SIZE, stdin);
        if (strcmp(command_buffer, "GET\n") == 0)
        {
            printf("Enter the pid of the required sensor, number only:\n");
            fgets(pid_buffer, BUFFER_SIZE, stdin);
            writePid(pid_buffer);
        }
        else if (strcmp(command_buffer, "PUT\n") == 0)
        {
            printf("Enter the pid of the required actuator, number only:\n");
            fgets(pid_buffer, BUFFER_SIZE, stdin);
            writePid(pid_buffer);
        }
        else if (strcmp(command_buffer, "q\n") == 0)
        {
            cloud_running_flag = 0;
        }
        else
        {
            printf("Invalid command, PUT or GET only\n");
        }
        
    }
    kill(getppid(),SIGKILL);
    printf("Parent process is stopped, cleaning...\n");
    printf("[Cloud Child]Bye!\n");
    unlink(CLOUD_FIFO_NAME);
    unlink(FIFO_NAME);
    exit(EXIT_SUCCESS);
}




void parentProcess()
{
//    printf("%d\n", open(FIFO_NAME, O_RDONLY));
    char buffer[BUFFER_SIZE + 1];
    res = open(FIFO_NAME, O_RDONLY| O_NONBLOCK);
    printf("[Cloud Parent] Started!\n");
    /* open fifo in read-only
    mode */
    while(1) {
        if(read(res, buffer, BUFFER_SIZE) > 0){
        printf("%s\n", buffer);
        }
    }
    exit(EXIT_SUCCESS);
}

