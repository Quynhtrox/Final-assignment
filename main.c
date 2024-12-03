#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<fcntl.h>
#include<string.h>
#include<time.h>

#define MAX_BUFFER_SIZE     1024
#define FIFO_FILE           "./logFIFO"
#define LOG_FILE            "./gateway.log"

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

typedef struct
{
    int port;
    int len;
    Shared_data *shared_data;
}Thread_args;

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

    FILE *log = fopen(LOG_FILE, "a");

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
    pthread_mutex_lock(&data_lock);

    shared->buffer[shared->tail] = data;
    shared->tail = (shared->tail + 1) % MAX_BUFFER_SIZE;

    pthread_mutex_unlock(&data_lock);
}

//Get data from buffer
Sensor_data get_data(Shared_data *shared, Sensor_data data) {
    pthread_mutex_lock(&data_lock);
        data = shared->buffer[shared->head];
        shared->head = (shared->head + 1) % MAX_BUFFER_SIZE;
    pthread_mutex_unlock(&data_lock);
}

//Connection manager thread
static void *thr_connection(void *args) {
    Thread_args *ThreadArgs = (Thread_args *)args;
    Shared_data *shared = ThreadArgs->shared_data;

    int server_fd, new_socket_fd;
    struct sockaddr_in servaddr, sensoraddr;

    
    //Tạo socket TCP
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    

    //Khởi tạo địa chỉ cho cổng
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(ThreadArgs->port);

    //Gắn socket vào cổng
    bind(server_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    
    listen(server_fd, 3);

    //Lấy thông tin sensor
    ThreadArgs->len = sizeof(sensoraddr);

    if (new_socket_fd = accept(server_fd, (struct sockaddr *)&sensoraddr, (socklen_t *)&ThreadArgs->len) >= 0) {

    char buffer[MAX_BUFFER_SIZE];
    read(new_socket_fd, buffer, MAX_BUFFER_SIZE);

    //Giải mã dữ liệu từ sensor
    Sensor_data data;
    sscanf(buffer, "%d %f", &data.SensorNodeID, &data.temperature);

    //Thêm dữ liệu vào shared buffer
    add_data(shared, data);

    //Ghi log new connection
    char log_message[MAX_BUFFER_SIZE];
    snprintf(log_message, sizeof(log_message), "A sensor node with %d has opened a new connection\n", data.SensorNodeID);
    log_events(log_message);

    } else {
    Sensor_data data;
    get_data(shared, data);
    //Ghi log closed
    char log_message[MAX_BUFFER_SIZE];
    snprintf(log_message, sizeof(log_message), "The sensor node with %d has closed the connection\n", data.SensorNodeID);
    log_events(log_message);
    }

    close(new_socket_fd);
    close(server_fd);
}

//Data manager thread
static void *thr_data(void *args) {

}

//Storage manager thread
static void *thr_storage(void *args) {

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

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("No port provided\n");
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    pid_t pid;
    Shared_data shared_data = {.head = 0, .tail = 0};
    Thread_args thread_ares = {.port = port, .shared_data = &shared_data};
    pthread_t threadconn, threaddata, threadstorage;

    pid = fork();
    if (pid == 0) {
        log_process();

    } else if (pid > 0) { //Parent process
        printf("Im main process. My PID: %d\n", getpid());

        pthread_create(&threadconn, NULL, &thr_connection, NULL);
        pthread_create(&threaddata, NULL, &thr_data, NULL);
        pthread_create(&threadstorage, NULL, &thr_storage, NULL);

        while(1);
        wait(NULL);
    }

    pthread_join(threadconn, NULL);
    pthread_join(threaddata, NULL);
    pthread_join(threadstorage, NULL);

    return 0;
}