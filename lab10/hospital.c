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

int waitingPatientsCount = 0;
int allPatientsCnt;
bool visitInProgress = false;
int patientsAtVisitId[MAX_PATIENTS_CNT];

pthread_mutex_t waitingPatientsCountLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doctorAwakeningCondition = PTHREAD_COND_INITIALIZER;

pthread_mutex_t visitLock;
pthread_cond_t visitCondition = PTHREAD_COND_INITIALIZER;

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

    // go to the hospital
    int randSleepTime = getRandSleepTime();

    getCurrTime(timeCharBuffer);
    printf("[%s] - PATIENT(%d): Going to the hospital, will be in %d [s]\n", timeCharBuffer, patientId, randSleepTime);

    sleep(randSleepTime);

    bool joinedQueue = false;

    while (true) {
        // check number of waiting patients
        pthread_mutex_lock(&waitingPatientsCountLock);

        getCurrTime(timeCharBuffer);

        if (waitingPatientsCount >= MAX_PATIENTS_CNT) {
            // sleep for random amount of time
            randSleepTime = getRandSleepTime();

            printf("[%s] - PATIENT(%d): To much waiting patients, will be back in %d [s]\n", timeCharBuffer, patientId, randSleepTime);
        } else {
            // join the queue
            patientsAtVisitId[waitingPatientsCount] = patientId;
            waitingPatientsCount++;
            joinedQueue = true;

            printf("[%s] - PATIENT(%d): Waiting patients %d\n", timeCharBuffer, patientId, waitingPatientsCount);

            // wake up doctor
            if (MAX_PATIENTS_CNT == waitingPatientsCount) {
                printf("[%s] - PATIENT(%d): I wake up doctor\n", timeCharBuffer, patientId);

                pthread_mutex_lock(&visitLock);

                visitInProgress = true;
                pthread_cond_signal(&doctorAwakeningCondition);

                pthread_mutex_unlock(&visitLock);
            }
        }

        pthread_mutex_unlock(&waitingPatientsCountLock);

        if (joinedQueue) {
            break;
        } else {
            sleep(randSleepTime);
        }
    }

    pthread_mutex_lock(&visitLock);

    while ( visitInProgress ) {
        pthread_cond_wait(&visitCondition, &visitLock);
    }

    // update waiting patients count
    pthread_mutex_lock(&waitingPatientsCountLock);
    waitingPatientsCount--;
    allPatientsCnt--;
    pthread_mutex_unlock(&waitingPatientsCountLock);

    pthread_mutex_unlock(&visitLock);

    printf("[%s] - PATIENT(%d): Visit ended\n", timeCharBuffer, patientId);
}

void* doctor(void* args) {
    char timeCharBuffer[TIME_BUFFER_SIZE];

    pthread_mutex_lock(&visitLock);

    while ( ! visitInProgress ){
        pthread_cond_wait(&doctorAwakeningCondition, &visitLock);
    }

    getCurrTime(timeCharBuffer);
    printf("[%s] - DOCTOR: I'm waking up\n", timeCharBuffer);

    getCurrTime(timeCharBuffer);
    printf("[%s] - DOCTOR: Consulting patients %d, %d, %d\n", timeCharBuffer, patientsAtVisitId[0], patientsAtVisitId[1], patientsAtVisitId[2]);

    sleep(rand() % 2 + 2);

    visitInProgress = false;
    pthread_cond_broadcast(&visitCondition);

    pthread_mutex_unlock(&visitLock);

    getCurrTime(timeCharBuffer);
    printf("[%s] - DOCTOR: I'm falling asleep\n", timeCharBuffer);
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

    // pthread_mutexattr_init(&visitLockAttr);
    // pthread_mutexattr_settype(&visitLockAttr, PTHREAD_MUTEX_RECURSIVE);
    // pthread_mutex_init(&visitLock, &visitLockAttr);

    pthread_t* patientThreads = malloc(sizeof(pthread_t) * allPatientsCnt);
    int* patientArgs = malloc(sizeof(int) * allPatientsCnt);

    for (int patientId = 0; patientId < allPatientsCnt; patientId++) {
        pthread_create(&patientThreads[patientId], NULL, patient, &patientArgs[patientId]);
    }

    pthread_t doctorThread;

    pthread_create(&doctorThread, NULL, doctor, NULL);

    pthread_join(doctorThread, NULL);

    for (int patientId = 0; patientId < allPatientsCnt; patientId++) {
        pthread_join(patientThreads[patientId], NULL);
    }

    pthread_mutex_destroy(&waitingPatientsCountLock);
    pthread_cond_destroy(&doctorAwakeningCondition);
    pthread_mutex_destroy(&visitLock);
    pthread_cond_destroy(&visitCondition);

    free(patientThreads);
    free(patientArgs);

    return 0;
}
