#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <chrono>
#include <random>
#include <fstream>

using namespace std;
using namespace std::chrono;

#define timestamp std::chrono::high_resolution_clock::time_point
#define current_time() std::chrono::high_resolution_clock::now()
#define get_seconds(t2, t1) duration_cast<seconds>(t2 - t1).count()

default_random_engine generator;
poisson_distribution<int> distribution(4);

int N, M, w, x, y;
int totalSubmissions = 0;
int printingStations[4] = {0};
int bindingStations[2] = {0};

pthread_mutex_t mutex_printingStations;
pthread_mutex_t mutex_bindingStations;
pthread_mutex_t mutex_entryBook;
pthread_mutex_t mutex_readers;

sem_t sem_printingStations;
sem_t sem_bindingStations;

ofstream outputFile;

/* ************************* */
void* studentPrinting(void* arg);
void* groupLeaderBinding(void* arg);
void* groupLeaderSubmission(void* arg);
/* ************************* */

/***** Helper Functions ******/
int getAvailablePrintingStation(int studentId) {
    int station = (studentId % 4) + 1;
    while (true) {
        pthread_mutex_lock(&mutex_printingStations);
        if (printingStations[station - 1] == 0) {
            printingStations[station - 1] = studentId;
            pthread_mutex_unlock(&mutex_printingStations);
            return station;
        }
        pthread_mutex_unlock(&mutex_printingStations);
        usleep(1000); // Wait for 1ms before trying again
    }
}

int getAvailableBindingStation() {
    while (true) {
        pthread_mutex_lock(&mutex_bindingStations);
        for (int i = 0; i < 2; i++) {
            if (bindingStations[i] == 0) {
                bindingStations[i] = 1;
                pthread_mutex_unlock(&mutex_bindingStations);
                return i + 1;
            }
        }
        pthread_mutex_unlock(&mutex_bindingStations);
        usleep(1000); // Wait for 1ms before trying again
    }
}

void releasePrintingStation(int station) {
    pthread_mutex_lock(&mutex_printingStations);
    printingStations[station - 1] = 0;
    pthread_mutex_unlock(&mutex_printingStations);
}

void releaseBindingStation(int station) {
    pthread_mutex_lock(&mutex_bindingStations);
    bindingStations[station - 1] = 0;
    pthread_mutex_unlock(&mutex_bindingStations);
}

void waitToWriteEntryBook() {
    while (true) {
        pthread_mutex_lock(&mutex_entryBook);
        if (totalSubmissions == 0) {
            return;
        }
        pthread_mutex_unlock(&mutex_entryBook);
        usleep(1000); // Wait for 1ms before trying again
    }
}

void startReadingEntryBook(int staffId) {
    pthread_mutex_lock(&mutex_readers);
    pthread_mutex_lock(&mutex_entryBook);
    if (totalSubmissions == 0) {
        outputFile << "Staff " << staffId << " has started reading the entry book at time " << get_seconds(current_time(), startTime) << ". No. of submission = 0\n";
        pthread_mutex_unlock(&mutex_entryBook);
    } else {
        outputFile << "Staff " << staffId << " has started reading the entry book at time " << get_seconds(current_time(), startTime) << ". No. of submission = " << totalSubmissions << "\n";
        pthread_mutex_unlock(&mutex_readers);
        pthread_mutex_unlock(&mutex_entryBook);
    }
}

void stopReadingEntryBook(int staffId) {
    pthread_mutex_lock(&mutex_entryBook);
    if (totalSubmissions == 0) {
        outputFile << "Staff " << staffId << " has finished reading the entry book at time " << get_seconds(current_time(), startTime) << ". No. of submission = 0\n";
    } else {
        outputFile << "Staff " << staffId << " has finished reading the entry book at time " << get_seconds(current_time(), startTime) << ". No. of submission = " << totalSubmissions << "\n";
    }
    pthread_mutex_unlock(&mutex_entryBook);
}
/* ************************* */
/* ************************* */

void* studentPrinting(void* arg) {
    int studentId = *((int*)arg);
    delete (int*)arg;
    arg = nullptr;

    usleep(rand() % 3000000); // Inter-arrival rate

    int station = getAvailablePrintingStation(studentId);
    int time = get_seconds(current_time(), startTime);
    outputFile << "Student " << studentId << " has arrived at the print station at time " << time << "\n";

    usleep(w * 1000); // Printing time

    releasePrintingStation(station);

    time = get_seconds(current_time(), startTime);
    outputFile << "Student " << studentId << " has finished printing at time " << time << "\n";

    pthread_exit(NULL);
}

void* groupLeaderBinding(void* arg) {
    int groupId = *((int*)arg);
    delete (int*)arg;
    arg = nullptr;

    usleep(x * 1000); // Time taken by other students to finish printing

    int station = getAvailableBindingStation();
    timestamp time = get_seconds(current_time(), startTime);
    outputFile << "Group " << groupId << " has started binding at time " << time << "\n";

    usleep(x * 1000); // Binding time

    releaseBindingStation(station);

    time = get_seconds(current_time(), startTime);
    outputFile << "Group " << groupId << " has finished binding at time " << time << "\n";

    pthread_exit(NULL);
}

void* groupLeaderSubmission(void* arg) {
    int groupId = *((int*)arg);
    delete (int*)arg;
    arg = nullptr;

    waitToWriteEntryBook();

    timestamp time = get_seconds(current_time(), startTime);
    outputFile << "Group " << groupId << " has started submitting the report at time " << time << "\n";

    usleep(y * 1000); // Submission time

    pthread_mutex_lock(&mutex_entryBook);
    totalSubmissions++;
    pthread_mutex_unlock(&mutex_entryBook);

    time = get_seconds(current_time(), startTime);
    outputFile << "Group " << groupId << " has submitted the report at time " << time << "\n";

    pthread_exit(NULL);
}

int main() {
    freopen("input.txt", "r", stdin);
    freopen("output.txt", "w", stdout);

    cin >> N >> M;
    cin >> w >> x >> y;

    pthread_mutex_init(&mutex_printingStations, NULL);
    pthread_mutex_init(&mutex_bindingStations, NULL);
    pthread_mutex_init(&mutex_entryBook, NULL);
    pthread_mutex_init(&mutex_readers, NULL);
    sem_init(&sem_printingStations, 0, 4);
    sem_init(&sem_bindingStations, 0, 2);

    pthread_t students[N];
    pthread_t groupLeaders[N / M];

    timestamp startTime = current_time();

    outputFile << std::fixed;
    outputFile.precision(2);

    for (int i = 0; i < N; i++) {
        int* id = new int(i + 1);
        pthread_create(&students[i], NULL, studentPrinting, (void*)id);
    }

    for (int i = 0; i < N / M; i++) {
        int* id = new int(i + 1);
        pthread_create(&groupLeaders[i], NULL, groupLeaderBinding, (void*)id);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(students[i], NULL);
    }

    for (int i = 0; i < N / M; i++) {
        pthread_join(groupLeaders[i], NULL);
    }

    for (int i = 0; i < N / M; i++) {
        int* id = new int(i + 1);
        pthread_create(&groupLeaders[i], NULL, groupLeaderSubmission, (void*)id);
    }

    for (int i = 0; i < N / M; i++) {
        pthread_join(groupLeaders[i], NULL);
    }

    /*      Mutex and Sem Destroy      */
    pthread_mutex_destroy(&mutex_printingStations);
    pthread_mutex_destroy(&mutex_bindingStations);
    pthread_mutex_destroy(&mutex_entryBook);
    pthread_mutex_destroy(&mutex_readers);

    sem_destroy(&sem_printingStations);
    sem_destroy(&sem_bindingStations);
    /**********************************/

    return 0;
}
