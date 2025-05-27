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
#define FIRST_AID_KIT_CAPACITY 6
#define LOW_MEDICINE_CNT 3
#define TIME_BUFFER_SIZE 9

#define DEBUG 0

#define INSTANT_PRINTF(...) \
    printf(__VA_ARGS__); \
    fflush(stdout);

#define PRINTF_DEBUG(...) \
    if (DEBUG) { \
        INSTANT_PRINTF("|DEBUG PRINT|: "); \
        INSTANT_PRINTF(__VA_ARGS__); \
    }

int waitingPatientsCount = 0;
int patientsToServeCnt;
bool visitInProgress = false;
bool waitingForVisit = true;
bool awakeDoctor = false;
int patientsAtVisitCnt = 0;
int patientsAtVisitId[MAX_PATIENTS_CNT];
int medicinesCnt = FIRST_AID_KIT_CAPACITY;

int exitedThreadsCnt = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t doctorAwakeningCondition = PTHREAD_COND_INITIALIZER;

pthread_cond_t visitStartCondition = PTHREAD_COND_INITIALIZER;
pthread_cond_t visitEndCondition = PTHREAD_COND_INITIALIZER;
pthread_cond_t allPatientsAtVisitCondition = PTHREAD_COND_INITIALIZER;

volatile bool deliveringMedicine = false;

pthread_cond_t lowMedicinesCntCondition = PTHREAD_COND_INITIALIZER;
pthread_cond_t medicineDeliveryEndCondition = PTHREAD_COND_INITIALIZER;

bool doctorExited = false;

int getRandSleepTime() {
    return rand() % 4 + 2;
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
    INSTANT_PRINTF("[%s] - PATIENT(%d): Going to the hospital, will be in %d [s]\n", timeCharBuffer, patientId, randSleepTime);

    sleep(randSleepTime);

    bool joinedQueue = false;

    while (true) {
        pthread_mutex_lock(&mutex);

        getCurrTime(timeCharBuffer);

        if (waitingPatientsCount >= MAX_PATIENTS_CNT) {
            randSleepTime = getRandSleepTime();
            INSTANT_PRINTF("[%s] - PATIENT(%d): Too many waiting patients, will be back in %d [s]\n", timeCharBuffer, patientId, randSleepTime);
            pthread_mutex_unlock(&mutex);

            sleep(randSleepTime);
        } else {
            // join the queue
            patientsAtVisitId[waitingPatientsCount] = patientId;
            waitingPatientsCount++;
            joinedQueue = true;

            INSTANT_PRINTF("[%s] - PATIENT(%d): Waiting patients %d\n", timeCharBuffer, patientId, waitingPatientsCount);

            // wake up doctor if queue is full
            if (MAX_PATIENTS_CNT == waitingPatientsCount) {
                INSTANT_PRINTF("[%s] - PATIENT(%d): I wake up doctor\n", timeCharBuffer, patientId);

                awakeDoctor = true;
                pthread_cond_signal(&doctorAwakeningCondition);
            }
            pthread_mutex_unlock(&mutex);
            break;
        }
    }

    pthread_mutex_lock(&mutex);

    // Wait for visit to start
    while (waitingForVisit) {
        getCurrTime(timeCharBuffer);
        PRINTF_DEBUG("[%s] - PATIENT(%d): waiting for visit to start loop\n", timeCharBuffer, patientId);

        pthread_cond_wait(&visitStartCondition, &mutex);
    }

    // enter visit room
    patientsAtVisitCnt++;

    getCurrTime(timeCharBuffer);
    PRINTF_DEBUG("[%s] - PATIENT(%d): entering visit\n", timeCharBuffer, patientId);

    // check if you are the last patient to enter the visit room
    if (patientsAtVisitCnt == MAX_PATIENTS_CNT) {
        pthread_cond_signal(&allPatientsAtVisitCondition);
    }
    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&mutex);
    // Wait for visit to end
    while (visitInProgress) {
        getCurrTime(timeCharBuffer);
        PRINTF_DEBUG("[%s] - PATIENT(%d): waiting for visit to end loop\n", timeCharBuffer, patientId);

        pthread_cond_wait(&visitEndCondition, &mutex);
    }

    getCurrTime(timeCharBuffer);
    INSTANT_PRINTF("[%s] - PATIENT(%d): Visit ended\n", timeCharBuffer, patientId);

    exitedThreadsCnt++;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void pharmacist_deliver_medicine(int pharmacistId) {
    char timeCharBuffer[TIME_BUFFER_SIZE];

    getCurrTime(timeCharBuffer);
    INSTANT_PRINTF("[%s] - PHARMACIST(%d): I wake up doctor\n", timeCharBuffer, pharmacistId);

    awakeDoctor = true;
    pthread_cond_signal(&doctorAwakeningCondition);

    deliveringMedicine = true;

    getCurrTime(timeCharBuffer);
    INSTANT_PRINTF("[%s] - PHARMACIST(%d): I deliver medicine\n", timeCharBuffer, pharmacistId);

    // wait until delivery ends
    while (deliveringMedicine) {
        getCurrTime(timeCharBuffer);
        PRINTF_DEBUG("[%s] - PHARMACIST(%d): waiting for delivery to end loop\n", timeCharBuffer, pharmacistId);

        pthread_cond_wait(&medicineDeliveryEndCondition, &mutex);
    }

    getCurrTime(timeCharBuffer);
    INSTANT_PRINTF("[%s] - PHARMACIST(%d): I've completed delivery\n", timeCharBuffer, pharmacistId);
}

void* pharmacist(void* args) {
    int pharmacistId = *(int *)args;
    char timeCharBuffer[TIME_BUFFER_SIZE];

    srand(pharmacistId);

    int randSleepTime = rand() % 11 + 5;

    getCurrTime(timeCharBuffer);
    INSTANT_PRINTF("[%s] - PHARMACIST(%d): Going to the hospital, will be in %d [s]\n", timeCharBuffer, pharmacistId, randSleepTime);

    sleep(randSleepTime);

    pthread_mutex_lock(&mutex);

    while ((medicinesCnt >= LOW_MEDICINE_CNT || deliveringMedicine) && !doctorExited) {
        if (deliveringMedicine) {
            getCurrTime(timeCharBuffer);
            PRINTF_DEBUG("[%s] - PHARMACIST(%d): For other pharmacist to end delivery\n", timeCharBuffer, pharmacistId);
        }

        getCurrTime(timeCharBuffer);
        INSTANT_PRINTF("[%s] - PHARMACIST(%d): Waiting for first aid kit to be emptied\n", timeCharBuffer, pharmacistId);

        pthread_cond_wait(&lowMedicinesCntCondition, &mutex);
    }

    if (!doctorExited) {
        pharmacist_deliver_medicine(pharmacistId);
    }

    pthread_mutex_unlock(&mutex);
}

void doctor_admit_patients() {
    char timeCharBuffer[TIME_BUFFER_SIZE];

    // inform patients about visit
    waitingForVisit = false;
    visitInProgress = true;

    pthread_cond_broadcast(&visitStartCondition);

    pthread_mutex_unlock(&mutex);

    // wait for all patients to enter visit room
    pthread_mutex_lock(&mutex);
    while (patientsAtVisitCnt < MAX_PATIENTS_CNT) {
        pthread_cond_wait(&allPatientsAtVisitCondition, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    // start visit
    pthread_mutex_lock(&mutex);

    getCurrTime(timeCharBuffer);
    INSTANT_PRINTF("[%s] - DOCTOR: Consulting patients %d, %d, %d\n", timeCharBuffer, patientsAtVisitId[0], patientsAtVisitId[1], patientsAtVisitId[2]);

    sleep(rand() % 2 + 2);

    visitInProgress = false;
    patientsAtVisitCnt = 0;

    getCurrTime(timeCharBuffer);
    PRINTF_DEBUG("[%s] - DOCTOR: Consultations ended\n", timeCharBuffer);
    pthread_cond_broadcast(&visitEndCondition);

    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&mutex);

    getCurrTime(timeCharBuffer);
    PRINTF_DEBUG("[%s] - DOCTOR: Updating patients/medicine count\n", timeCharBuffer);
    patientsToServeCnt -= MAX_PATIENTS_CNT;
    medicinesCnt -= MAX_PATIENTS_CNT;
    waitingPatientsCount = 0;
    waitingForVisit = true;
    pthread_mutex_unlock(&mutex);
}

void doctor_admit_pharmacist() {
    char timeCharBuffer[TIME_BUFFER_SIZE];

    getCurrTime(timeCharBuffer);
    INSTANT_PRINTF("[%s] - DOCTOR: Accepting medicine delivery\n", timeCharBuffer);

    medicinesCnt++;
    sleep(rand() % 3 + 1);

    deliveringMedicine = false;

    pthread_cond_broadcast(&medicineDeliveryEndCondition);
}

void* doctor(void* args) {
    char timeCharBuffer[TIME_BUFFER_SIZE];
    PRINTF_DEBUG("Doctor thread started\n");

    while (1) {
        pthread_mutex_lock(&mutex);

        // wait only if there is not enough medicine and waiting patients otherwise admit waiting patients
        if (MAX_PATIENTS_CNT != waitingPatientsCount || medicinesCnt < LOW_MEDICINE_CNT) {
            while (!awakeDoctor) {
                pthread_cond_wait(&doctorAwakeningCondition, &mutex);
            }
        }

        getCurrTime(timeCharBuffer);
        INSTANT_PRINTF("[%s] - DOCTOR: I'm waking up\n", timeCharBuffer);

        PRINTF_DEBUG("[%s] - DOCTOR: Medicine count: %d, waiting patients: %d\n", timeCharBuffer, medicinesCnt, waitingPatientsCount);

        if (MAX_PATIENTS_CNT == waitingPatientsCount && medicinesCnt >= LOW_MEDICINE_CNT) {
            doctor_admit_patients();
            PRINTF_DEBUG("patientsToServeCnt: %d\n", patientsToServeCnt);
        } else if (medicinesCnt < LOW_MEDICINE_CNT) {
            doctor_admit_pharmacist();
        } else {
            PRINTF_DEBUG("Should never get here!!!");
            exit(1);
        }

        pthread_mutex_unlock(&mutex);

        pthread_mutex_lock(&mutex);
        getCurrTime(timeCharBuffer);
        INSTANT_PRINTF("[%s] - DOCTOR: I'm falling asleep\n", timeCharBuffer);
        awakeDoctor = false;

        if (patientsToServeCnt == 0) {
            // notify all threads that doctor exited
            doctorExited = true;
            deliveringMedicine = false;
            pthread_cond_broadcast(&lowMedicinesCntCondition);
            pthread_mutex_unlock(&mutex);

            pthread_mutex_lock(&mutex);
            pthread_cond_broadcast(&medicineDeliveryEndCondition);
            pthread_mutex_unlock(&mutex);
            break;
        } else if (medicinesCnt < LOW_MEDICINE_CNT) {
            pthread_cond_signal(&lowMedicinesCntCondition);
        }

        pthread_mutex_unlock(&mutex);
    }

    getCurrTime(timeCharBuffer);
    INSTANT_PRINTF("[%s] - DOCTOR: All patients were served\n", timeCharBuffer);

    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        INSTANT_PRINTF("Wrong number of arguments!!!\n");
        INSTANT_PRINTF("Provide number of patients and pharmacists!!!\n");
        return 1;
    }

    const int allPatientsCnt = atoi(argv[1]);
    patientsToServeCnt = allPatientsCnt;

    if (patientsToServeCnt % MAX_PATIENTS_CNT != 0) {
        INSTANT_PRINTF("Provide patinent number divisable by %d\n", MAX_PATIENTS_CNT);

        return 1;
    }

    const int allPharmacistsCnt = atoi(argv[2]);

    pthread_t* pharmacistThreads = malloc(sizeof(pthread_t) * allPharmacistsCnt);
    int* pharmacistArgs = malloc(sizeof(int) * allPharmacistsCnt);

    pthread_t* patientThreads = malloc(sizeof(pthread_t) * patientsToServeCnt);
    int* patientArgs = malloc(sizeof(int) * patientsToServeCnt);

    pthread_t doctorThread;
    pthread_create(&doctorThread, NULL, doctor, NULL);

    // crate patients threads
    for (int patientId = 0; patientId < patientsToServeCnt; patientId++) {
        patientArgs[patientId] = patientId;
        pthread_create(&patientThreads[patientId], NULL, patient, &patientArgs[patientId]);
    }

    // create pharmacists threads
    for (int pharmacistId = 0; pharmacistId < allPharmacistsCnt; pharmacistId++) {
        pharmacistArgs[pharmacistId] = pharmacistId;
        pthread_create(&pharmacistThreads[pharmacistId], NULL, pharmacist, &pharmacistArgs[pharmacistId]);
    }

    // join doctor thread
    pthread_join(doctorThread, NULL);

    // join patients threads
    for (int patientId = 0; patientId < patientsToServeCnt; patientId++) {
        pthread_join(patientThreads[patientId], NULL);
    }

    // join pharmacists threads
    for (int pharmacistId = 0; pharmacistId < allPharmacistsCnt; pharmacistId++) {
        pthread_join(pharmacistThreads[pharmacistId], NULL);
    }

    PRINTF_DEBUG("Finished job -> destroing locks and conditions...\n");

    while (true) {
        pthread_mutex_lock(&mutex);
        PRINTF_DEBUG("Threads that exited count: %d\n", exitedThreadsCnt);
        PRINTF_DEBUG("Is this correct value? %s\n", exitedThreadsCnt == allPatientsCnt ? "true" : "false");
        if (exitedThreadsCnt == allPatientsCnt) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);

        sleep(1);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&doctorAwakeningCondition);
    pthread_cond_destroy(&visitStartCondition);
    pthread_cond_destroy(&visitEndCondition);
    pthread_cond_destroy(&allPatientsAtVisitCondition);

    pthread_cond_destroy(&lowMedicinesCntCondition);
    pthread_cond_destroy(&medicineDeliveryEndCondition);

    free(pharmacistThreads);
    free(pharmacistArgs);

    free(patientThreads);
    free(patientArgs);

    return 0;
}
