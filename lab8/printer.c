#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>


#define DATA_SIZE 10
#define BUFFER_SIZE 100
#define MAX_SLEEP_TIME 5

#define SHM_NAME "/shared_mem"

#define DEBUG 1

#define PRINTF_DEBUG(...) \
    if (DEBUG) { \
        printf(__VA_ARGS__); \
        fflush(stdout); \
    }

typedef struct {
    pid_t* processesArr;
    int len;
} processes_t;

typedef struct {
    char data[DATA_SIZE];
} print_job_t;

typedef struct {
    print_job_t buffer[BUFFER_SIZE];
    int userIdx;            // index for users
    int printerIdx;         // index for printers
    int usersCnt;
    int printersCnt;
    sem_t mutex;
    sem_t full;
    sem_t empty;
    int shmFd;
} shared_memory_t;

processes_t* userProcesses;
processes_t* printerProcesses;
shared_memory_t* sharedMemory;

char getRandomChar() {
    return (rand() % 2 == 0 ? 'a' : 'A') + rand() % 26;
}

void initSharedMemory(int usersCnt, int printersCnt) {
    int shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);

    ftruncate(shmFd, sizeof(shared_memory_t));

    sharedMemory = mmap(NULL, sizeof(shared_memory_t), PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);

    if (sharedMemory == MAP_FAILED || sharedMemory == NULL) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sharedMemory->usersCnt = usersCnt;
    sharedMemory->printersCnt = printersCnt;
    sharedMemory->userIdx = 0;
    sharedMemory->printerIdx = 0;

    sem_init(&sharedMemory->mutex, 1, 1);
    sem_init(&sharedMemory->full, 1, 0);
    sem_init(&sharedMemory->empty, 1, printersCnt);
}

void cleanupSharedMemory() {
    sem_destroy(&sharedMemory->mutex);
    sem_destroy(&sharedMemory->full);
    sem_destroy(&sharedMemory->empty);

    munmap(sharedMemory, sizeof(shared_memory_t));
    shm_unlink(SHM_NAME);
}

void userProcess(int userId) {
    PRINTF_DEBUG("Initialize user process: %d\n", userId);

    while (true) {
        // wait until there is an empty space to store data in buffor
        sem_wait(&sharedMemory->empty);

        // synchronize shared memory
        sem_wait(&sharedMemory->mutex);

        print_job_t* dataBuffer = &sharedMemory->buffer[sharedMemory->userIdx];
        sharedMemory->userIdx = (sharedMemory->userIdx + 1) % BUFFER_SIZE;

        // fill buffer with random characters
        for (int i = 0; i < DATA_SIZE; i++) {
            dataBuffer->data[i] = getRandomChar();
        }

        PRINTF_DEBUG("User%d writting: %.10s\n", userId, dataBuffer->data);

        // free up resources
        sem_post(&sharedMemory->mutex);
        sem_post(&sharedMemory->full);

        sleep(rand() % MAX_SLEEP_TIME + 1);
    }
}

void printerProcess(int printerId) {
    char messageToPrint[DATA_SIZE];

    PRINTF_DEBUG("Initialize printer process: %d\n", printerId);

    while (true) {
        // wait until there is full buffer
        sem_wait(&sharedMemory->full);

        // synchronize shared memory
        sem_wait(&sharedMemory->mutex);

        print_job_t* dataBuffer = &sharedMemory->buffer[sharedMemory->printerIdx];
        sharedMemory->printerIdx = (sharedMemory->printerIdx + 1) % BUFFER_SIZE;

        memcpy(messageToPrint, dataBuffer->data, DATA_SIZE);

        PRINTF_DEBUG("\n#### Printer%d printing: %.10s ####\n\n", printerId, dataBuffer->data);

        // allow other processes to use shared memory
        sem_post(&sharedMemory->mutex);
        sem_post(&sharedMemory->empty);

        for (int i = 0; i < DATA_SIZE; i++) {
            fprintf(stdout, "printer%d: %c\n", printerId, messageToPrint[i]);
            fflush(stdout);

            sleep(1);
        }

        PRINTF_DEBUG("\n#### Printer%d STOP ####\n\n", printerId);
    }
}

void handleSigint() {
    for (int i = 0; i< userProcesses->len; i++) {
        kill(userProcesses->processesArr[i], SIGINT);
    }

    for (int i = 0; i< printerProcesses->len; i++) {
        kill(printerProcesses->processesArr[i], SIGINT);
    }

    cleanupSharedMemory();

    free(printerProcesses->processesArr);
    free(printerProcesses);
    free(userProcesses->processesArr);
    free(userProcesses);

    exit(EXIT_SUCCESS);
}

void* safe_malloc(size_t size) {
    void* data;
    data = malloc(size);

    if (data == NULL) {
        fprintf(stderr, "couldn't allocate memory with malloc\n");
        cleanupSharedMemory();

        exit(EXIT_FAILURE);
    }

    return data;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <number_of_users> <number_of_printers>\n", argv[0]);
        return 1;
    }

    int usersCnt = atoi(argv[1]);
    int printersCnt = atoi(argv[2]);

    initSharedMemory(usersCnt, printersCnt);

    // set handler fo SIGINT signal
    struct sigaction sigactionStruct;
    sigactionStruct.sa_handler = handleSigint;
    sigemptyset(&sigactionStruct.sa_mask);
    sigaction(SIGINT, &sigactionStruct, NULL);

    printerProcesses = safe_malloc(sizeof(processes_t) * printersCnt);
    printerProcesses->len = printersCnt;
    printerProcesses->processesArr = safe_malloc(sizeof(pid_t) * printerProcesses->len);

    PRINTF_DEBUG("Initializing printers...\n");

    for (int i = 0; i < printerProcesses->len; i++) {
        pid_t child = fork();

        if (child == -1) {
            fprintf(stderr, "fork for child: %d\n", i);
            exit(EXIT_FAILURE);
        } else if (child == 0) {
            printerProcess(i);

            // kill child process
            exit(EXIT_SUCCESS);
        } else {
            printerProcesses->processesArr[i] = child;
        }
    }

    userProcesses = safe_malloc(sizeof(processes_t) * usersCnt);
    userProcesses->len = usersCnt;
    userProcesses->processesArr = safe_malloc(sizeof(pid_t) * userProcesses->len);

    PRINTF_DEBUG("Initializing users...\n");

    for (int i = 0; i < userProcesses->len; i++) {
        pid_t child = fork();

        if (child == -1) {
            fprintf(stderr, "fork for child: %d\n", i);
            exit(EXIT_FAILURE);
        } else if (child == 0) {
            // initialize random number generator for this child
            srand(getpid());

            userProcess(i);

            // kill child process
            exit(EXIT_SUCCESS);
        } else {
            userProcesses->processesArr[i] = child;
        }
    }

    PRINTF_DEBUG("Waiting for any signal...\n\n");

    while (true) {
        pause();
    }

    return 0;
}

