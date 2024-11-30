#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include <sys/stat.h>
#include<pthread.h>
#include<fcntl.h>
#include<string.h>

#define MAX_BUFFER_SIZE     1024
#define FIFO_FILE           "./logFIFO"
#define LOG_FILE            "./gateway.log"


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
    int SensorNodeID;
    time_t timestamp;
    float temperature;
} Sensor_data;

typedef struct
{
    Sensor_data buffer[MAX_BUFFER_SIZE];
    int head;
    int tail;
    int size;
} Shared_data;

void *log_events(char *message) {
    pthread_mutex_lock(&log_lock);
    int fd;
    fd = open(FIFO_FILE, O_WRONLY | O_CREAT, 0666);

    if (fd == -1) {
        printf("call open() failed\n");
    } else {
        write(fd, message, strlen(message));
    }
    pthread_mutex_unlock(&log_lock);
    close(fd);
}

static void *thr_connection(void *args) {

    pthread_mutex_lock(&lock);

    pthread_mutex_unlock(&lock);
}

static void *thr_data(void *args) {

    pthread_mutex_lock(&lock);

    pthread_mutex_unlock(&lock);
}

static void *thr_storage(void *args) {

    pthread_mutex_lock(&lock);

    pthread_mutex_unlock(&lock);
}

//Child process
void log_process() {
    printf("Im log process. My PID: %d\n", getpid());

    mkfifo(FIFO_FILE, 0666);
    int fd, fd1;
    char buff[MAX_BUFFER_SIZE];


    fd  = open(FIFO_FILE, O_RDONLY);
    fd1 = open(LOG_FILE, O_APPEND | O_CREAT, 0666);

    while(1) {
        ssize_t bytes = read(fd, buff, sizeof(buff) - 1);
        if (bytes > 0) {
            buff[bytes] = "\0";
            write(fd, buff, strlen(buff));
        }
    }

    close(fd);
    close(fd1);

    exit(0);
}

int main(int *args, char *argv[])
{
    pid_t pid;

    pid = fork();

    pthread_t threadconn, threaddata, threadstorage;

    if (pid == 0) {
        log_process();

    } else if (pid > 0) { //Parent process
        printf("Im main process. My PID: %d\n", getpid());

        pthread_create(&threadconn, NULL, &thr_connection, NULL);
        pthread_create(&threaddata, NULL, &thr_data, NULL);
        pthread_create(&threadstorage, NULL, &thr_storage, NULL);

        while(1);

        wait(NULL);

    } else {
        printf("Call fork() failed\n");
        exit(EXIT_FAILURE);
    }

    pthread_join(threadconn, NULL);
    pthread_join(threaddata, NULL);
    pthread_join(threadstorage, NULL);

    return 0;
}