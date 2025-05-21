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

int waiting_patients_count = 0;

pthread_mutex_t waiting_patients_count_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

int get_rand_sleep_time() {
    return rand() % 3 + 2;
}

void get_curr_time(char* buffer) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, TIME_BUFFER_SIZE, "%H:%M:%S", timeinfo);
}

void* patient(void* args) {
    int patient_id = *(int *)args;
    char timeCharBuffer[TIME_BUFFER_SIZE];

    srand(time(NULL));

    int time_to_get_to_hospital = get_rand_sleep_time();

    get_curr_time(timeCharBuffer);
    printf("[%s] - PATIENT(%d): Going to the hospital, will be in %d [s]\n", timeCharBuffer, patient_id, time_to_get_to_hospital);

    ptherad_mutex_lock(&waiting_patients_count_lock);

    while (waiting_patients_count == MAX_PATIENTS_CNT) {
        sleep(get_rand_sleep_time());
    }

    ptherad_mutex_unlock(&waiting_patients_count_lock);
}

void* doctor(void* args) {

}

int main(int argc, char** argv) {


    pthread_mutex_destroy(&waiting_patients_count_lock);
    pthread_cond_destroy(&condition);

    return 0;
}
