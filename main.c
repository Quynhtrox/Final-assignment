#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<pthread.h>
#include<fcntl.h>
#include<string.h>
#include<time.h>

#define MAX_BUFFER_SIZE     1024
#define FIFO_FILE           "./logFIFO"
#define LOG_FILE            "./gateway.log"


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t data_lock = PTHREAD_MUTEX_INITIALIZER;


typedef struct
{
    int SensorNodeID;
    float temperature;
} Sensor_data;

typedef struct
{
    Sensor_data buffer[MAX_BUFFER_SIZE];
    int head;
    int tail;
} Shared_data;

//Hàm ghi vào FIFO
void *log_events(void *args) {
    char *message = (char *)args;
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

//Hàm ghi vào gateway.log
void wr_log(void *args) {
    static int a = 0;
    a++;
    //a là sequence_number

    char *message = (char *)args;

    int *log = fopen(LOG_FILE, "a");

    time_t current_time = time(NULL);
    struct tm *time_info = localtime(&current_time);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", time_info);

    fprintf(log, "%d. %s %s\n", a, timestamp, message);
    fflush(log);
    fclose(log);
}

//Add data into buffer
void add_data(Shared_data *shared, Sensor_data data) {
    pthead_mutex_lock(&data_lock);

    shared->buffer[shared->tail] = data;
    shared->tail = (shared->tail + 1) % MAX_BUFFER_SIZE;

    pthread_mutex_unlock(&data_lock);
}

//Get data from buffer
void get_data(Shared_data *shared, Sensor_data data) {
    pthread_mutex_lock(&data_lock);
        data = shared->buffer[shared->head];
        shared->head = (shared->head + 1) % MAX_BUFFER_SIZE;
    pthread_mutex_unlock(&data_lock);
}

//Connection manager thread
static void *thr_connection(void *args) {

    pthread_mutex_lock(&lock);

    pthread_mutex_unlock(&lock);
}

//Data manager thread
static void *thr_data(void *args) {

    pthread_mutex_lock(&lock);

    pthread_mutex_unlock(&lock);
}

//Storage manager thread
static void *thr_storage(void *args) {

    pthread_mutex_lock(&lock);

    pthread_mutex_unlock(&lock);
}

//Child process
void log_process() {
    printf("Im log process. My PID: %d\n", getpid());

    mkfifo(FIFO_FILE, 0666);
    int fd;
    char buff[MAX_BUFFER_SIZE];

    fd  = open(FIFO_FILE, O_RDONLY);

    while(1) {
        ssize_t bytes = read(fd, buff, sizeof(buff) - 1);
        if (bytes > 0) {
            wr_log(&buff);
        }
    }

    close(fd);
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