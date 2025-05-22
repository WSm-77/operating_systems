#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <pthread.h>

#define MAX_PATIENTS_CNT 3
#define TIME_BUFFER_SIZE 9

#define DEBUG 1

#define PRINTF_DEBUG(...) \
    if (DEBUG) { \
        printf("|DEBUG PRINT|: "); \
        printf(__VA_ARGS__); \
    }

int waitingPatientsCount = 0;
int allPatientsCnt;
bool visitInProgress = false;
bool waitingForVisit = true;
int patientsAtVisitCnt = 0;
int patientsAtVisitId[MAX_PATIENTS_CNT];

pthread_mutex_t waitingPatientsCountLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doctorAwakeningCondition = PTHREAD_COND_INITIALIZER;

pthread_mutex_t visitLock;
pthread_cond_t visitStartCondition = PTHREAD_COND_INITIALIZER;
pthread_cond_t visitEndCondition = PTHREAD_COND_INITIALIZER;
pthread_cond_t allPatientsAtVisitCondition = PTHREAD_COND_INITIALIZER;

int getRandSleepTime() {
    return rand() % 3 + 2;
}

void getCurrTime(char* buffer) {
    time_t rawTime;
    struct tm *timeInfo;

    time(&rawTime);
    timeInfo = localtime(&rawTime);

    strftime(buffer, TIME_BUFFER_SIZE, "%H:%M:%S", timeInfo);
}

void* patient(void* args) {
    int patientId = *(int *)args;
    char timeCharBuffer[TIME_BUFFER_SIZE];

    srand(patientId);

    int randSleepTime = getRandSleepTime();

    getCurrTime(timeCharBuffer);
    printf("[%s] - PATIENT(%d): Going to the hospital, will be in %d [s]\n", timeCharBuffer, patientId, randSleepTime);

    sleep(randSleepTime);

    bool joinedQueue = false;

    while (true) {
        pthread_mutex_lock(&waitingPatientsCountLock);

        getCurrTime(timeCharBuffer);

        if (waitingPatientsCount >= MAX_PATIENTS_CNT) {
            randSleepTime = getRandSleepTime();
            printf("[%s] - PATIENT(%d): Too many waiting patients, will be back in %d [s]\n", timeCharBuffer, patientId, randSleepTime);
            pthread_mutex_unlock(&waitingPatientsCountLock);
            sleep(randSleepTime);
        } else {
            // join the queue
            patientsAtVisitId[waitingPatientsCount] = patientId;
            waitingPatientsCount++;
            joinedQueue = true;

            printf("[%s] - PATIENT(%d): Waiting patients %d\n", timeCharBuffer, patientId, waitingPatientsCount);

            // wake up doctor if queue is full
            if (MAX_PATIENTS_CNT == waitingPatientsCount) {
                printf("[%s] - PATIENT(%d): I wake up doctor\n", timeCharBuffer, patientId);

                pthread_mutex_lock(&visitLock);
                visitInProgress = true;
                pthread_cond_signal(&doctorAwakeningCondition);
                pthread_mutex_unlock(&visitLock);
            }
            pthread_mutex_unlock(&waitingPatientsCountLock);
            break;
        }
    }

    // Wait for visit to start
    pthread_mutex_lock(&visitLock);
    while (waitingForVisit) {
        getCurrTime(timeCharBuffer);
        PRINTF_DEBUG("[%s] - PATEINT(%d): waiting for visit loop\n", timeCharBuffer, patientId);

        pthread_cond_wait(&visitStartCondition, &visitLock);
    }

    // enter visit room
    patientsAtVisitCnt++;

    getCurrTime(timeCharBuffer);
    PRINTF_DEBUG("[%s] - PATIENT(%d): entering visit\n", timeCharBuffer, patientId);

    // check if you are the last patient to enter the visit room
    if (patientsAtVisitCnt == MAX_PATIENTS_CNT) {
        pthread_cond_signal(&allPatientsAtVisitCondition);
    }

    pthread_mutex_unlock(&visitLock);

    // Wait for visit to end
    pthread_mutex_lock(&visitLock);
    while (visitInProgress) {
        pthread_cond_wait(&visitEndCondition, &visitLock);
    }

    pthread_mutex_unlock(&visitLock);

    pthread_mutex_lock(&waitingPatientsCountLock);
    // tell other patients to wait for the visit
    waitingForVisit = true;
    waitingPatientsCount--;
    pthread_mutex_unlock(&waitingPatientsCountLock);

    getCurrTime(timeCharBuffer);
    printf("[%s] - PATIENT(%d): Visit ended\n", timeCharBuffer, patientId);

    return NULL;
}

void* doctor(void* args) {
    char timeCharBuffer[TIME_BUFFER_SIZE];
    PRINTF_DEBUG("Doctor thread started\n");

    while (1) {
        pthread_mutex_lock(&visitLock);
        while (!visitInProgress) {
            pthread_cond_wait(&doctorAwakeningCondition, &visitLock);
        }

        getCurrTime(timeCharBuffer);
        printf("[%s] - DOCTOR: I'm waking up\n", timeCharBuffer);

        // inform patients about visit
        waitingForVisit = false;

        pthread_cond_broadcast(&visitStartCondition);

        pthread_mutex_unlock(&visitLock);

        // wait for all patients to enter visit room
        pthread_mutex_lock(&visitLock);
        while (patientsAtVisitCnt < MAX_PATIENTS_CNT) {
            pthread_cond_wait(&allPatientsAtVisitCondition, &visitLock);
        }
        pthread_mutex_unlock(&visitLock);

        // start visit
        pthread_mutex_lock(&visitLock);

        getCurrTime(timeCharBuffer);
        printf("[%s] - DOCTOR: Consulting patients %d, %d, %d\n", timeCharBuffer, patientsAtVisitId[0], patientsAtVisitId[1], patientsAtVisitId[2]);

        sleep(rand() % 2 + 2);

        visitInProgress = false;
        patientsAtVisitCnt = 0;

        pthread_cond_broadcast(&visitEndCondition);

        pthread_mutex_unlock(&visitLock);

        getCurrTime(timeCharBuffer);
        printf("[%s] - DOCTOR: I'm falling asleep\n", timeCharBuffer);

        pthread_mutex_lock(&waitingPatientsCountLock);

        allPatientsCnt -= MAX_PATIENTS_CNT;
        PRINTF_DEBUG("allPatientsCnt: %d\n", allPatientsCnt);

        if (allPatientsCnt == 0) {
            pthread_mutex_unlock(&waitingPatientsCountLock);
            break;
        }
        pthread_mutex_unlock(&waitingPatientsCountLock);
    }

    getCurrTime(timeCharBuffer);
    printf("[%s] - DOCTOR: All patients were served\n", timeCharBuffer);

    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Wrong number of arguments!!!\n");
        printf("Provide number of patients and pharmacists!!!\n");
        return 1;
    }

    allPatientsCnt = atoi(argv[1]);

    if (allPatientsCnt % MAX_PATIENTS_CNT != 0) {
        printf("Provide patinent number divisable by %d\n", MAX_PATIENTS_CNT);

        return 1;
    }

    pthread_mutexattr_t visitLockAttr;

    pthread_t* patientThreads = malloc(sizeof(pthread_t) * allPatientsCnt);
    int* patientArgs = malloc(sizeof(int) * allPatientsCnt);

    pthread_t doctorThread;
    pthread_create(&doctorThread, NULL, doctor, NULL);

    for (int patientId = 0; patientId < allPatientsCnt; patientId++) {
        patientArgs[patientId] = patientId;
        pthread_create(&patientThreads[patientId], NULL, patient, &patientArgs[patientId]);
    }

    pthread_join(doctorThread, NULL);

    for (int patientId = 0; patientId < allPatientsCnt; patientId++) {
        pthread_join(patientThreads[patientId], NULL);
    }

    PRINTF_DEBUG("Finished job -> destroing locks and conditions...\n");

    pthread_mutex_destroy(&waitingPatientsCountLock);
    pthread_cond_destroy(&doctorAwakeningCondition);
    pthread_mutex_destroy(&visitLock);
    pthread_cond_destroy(&visitStartCondition);
    pthread_cond_destroy(&visitEndCondition);
    pthread_cond_destroy(&allPatientsAtVisitCondition);

    free(patientThreads);
    free(patientArgs);

    return 0;
}
