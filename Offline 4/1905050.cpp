#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <chrono>
#include <random>

using namespace std;
using namespace std::chrono;

#define timestamp std::chrono::high_resolution_clock::time_point
#define time_now() std::chrono::high_resolution_clock::now()
#define time_diff(start, end) std::chrono::duration_cast<std::chrono::seconds>(end - start).count()

// define types - student, group leader, staff
#define STUDENT 0
#define GROUP_LEADER 1
#define STAFF 2

#define NUM_PRINTING_STATIONS 4
#define NUM_BINDING_STATIONS 2
#define NUM_STAFFS 2

// define states
#define FREE 0
#define PRINTING 1
#define FINISHED_PRINTING 2
#define WAITING 3



default_random_engine random_generator;
poisson_distribution<int> distribution(4);

int num_students, size_group, time_printing, time_binding, time_entry_book;
int total_submissions = 0;
int reader_count = 0;
bool PSisAvailable[NUM_PRINTING_STATIONS];
bool BSisAvailable[NUM_BINDING_STATIONS];
vector<int> state;
vector<int> counter_binder;

pthread_mutex_t mutex_printingStations[NUM_PRINTING_STATIONS];
pthread_mutex_t mutex_bindingStations;
pthread_mutex_t mutex_entryBook;
pthread_mutex_t mutex_resource;
pthread_mutex_t mutex_timer;
pthread_mutex_t mutex_counter;
pthread_mutex_t mutex_print;

// vector of semaphores for each student
vector<sem_t> sem_students;

// semaphores for groups
vector<sem_t> sem_groups_binder;

sem_t sem_printingStations;
sem_t sem_bindingStations;


timestamp start_time;



/* ************************* */
void *arrivalPS(void *arg);
void *entrybook(void *arg);

/* ************************* */



/***** Helper Functions ******/
int getPrinterID(int studentID)
{
    return (studentID % NUM_PRINTING_STATIONS) + 1;
}

int getGroupID(int studentID)
{
    return (studentID - 1) / size_group + 1;
}

// get ids of group members
vector<int> getGroupMateIDs(int studentID)
{
    vector<int> groupMateIDs;
    int groupNumber = (studentID - 1) / size_group + 1;
    int startID = (groupNumber - 1) * size_group + 1;

    for (int i = 0; i < size_group; i++)
    {
        int mateID = startID + i;
        if (mateID != studentID && mateID <= num_students)
        {
            groupMateIDs.push_back(mateID);
        }
    }

    return groupMateIDs;
}

void print(int id, int type, string description, int time)
{ 
    pthread_mutex_lock(&mutex_print);
    string msg;

    if (type == STUDENT)
    {
        msg = "Student " + to_string(id) + " " + description + " at time " + to_string(time);
    }
    else if (type == GROUP_LEADER)
    {
        msg = "Group " + to_string(id) + " " + description + " at time " + to_string(time);
    }
    else if (type == STAFF)
    {
        msg = "Staff " + to_string(id) + " " + description + " at time " + to_string(time) + ". No. of submission = " + to_string(total_submissions);
    }

    cout << msg << endl;
    pthread_mutex_unlock(&mutex_print);
}



/* *******************   entry book   ************************* */

void submitBook(int id, int time, timestamp last_time)
{
    // writer
    pthread_mutex_lock(&mutex_resource);

    time += time_diff(time_now(), last_time);
    int groupID = getGroupID(id);
    print(groupID, GROUP_LEADER, "has submitted the report ", time);

    total_submissions++;

    pthread_mutex_unlock(&mutex_resource);

    sleep(time_entry_book);
}

void *entrybook(void *arg)
{
    int id = *((int *)arg);
    delete (int *)arg;
    arg = nullptr;

    while (1)
    {

        // random delay using poisson distribution
        int delay = distribution(random_generator);
        usleep(delay * 1000000);

        pthread_mutex_lock(&mutex_entryBook);
        reader_count++;
        if (reader_count == 1)
        {
            pthread_mutex_lock(&mutex_resource);
        }
        pthread_mutex_unlock(&mutex_entryBook);

        // group leader starts writing
        timestamp arrival_time = time_now();
        int time = time_diff(start_time, arrival_time);
        print(id, STAFF, "has started reading the entry book", time);
        sleep(time_entry_book);

        // group leader finishes writing

        pthread_mutex_lock(&mutex_entryBook);

        reader_count--;
        if (reader_count == 0)
        {
            pthread_mutex_unlock(&mutex_resource);
        }

        pthread_mutex_unlock(&mutex_entryBook);
    }

    return NULL;
}





/* *******************   binding station   ************************* */

void studentBinding(int id, int time, timestamp last_time)
{

    sem_wait(&sem_bindingStations);

    pthread_mutex_lock(&mutex_bindingStations);
    int groupID = getGroupID(id);
    time += time_diff(last_time, time_now());
    print(groupID, GROUP_LEADER, "has finished printing ", time);
    sleep(time_binding);

    pthread_mutex_unlock(&mutex_bindingStations);

    sem_post(&sem_bindingStations);

    submitBook(id, time, time_now());
}

/* ******************  printing station  *********************** */

void test(int i)
{

    int printerID = getPrinterID(i);

    if (state[i - 1] == WAITING && PSisAvailable[printerID - 1] == true)
    {
        state[i - 1] = PRINTING;
        PSisAvailable[printerID - 1] = false;
        sem_post(&sem_students[i - 1]);
    }
}

void take_printer(int id)
{
    int printer = getPrinterID(id) - 1;

    pthread_mutex_lock(&mutex_printingStations[printer]);
    state[id - 1] = WAITING;
    test(id);
    pthread_mutex_unlock(&mutex_printingStations[printer]);

    sem_wait(&sem_students[id - 1]);
}

void put_printer(int id)
{
    int printer = getPrinterID(id) - 1;

    pthread_mutex_lock(&mutex_printingStations[printer]);

    state[id - 1] = FINISHED_PRINTING;
    PSisAvailable[printer] = true;

    // first test group members and then others
    vector<int> groupMateIDs = getGroupMateIDs(id);
    for (int i = 0; i < groupMateIDs.size(); i++)
    {
        //cout << " Group ID : " << groupMateIDs[i] << endl;
        test(groupMateIDs[i]);
    }

    // test others
    for (int i = 1; i <= num_students; i++)
    {
        test(i);
    }

    int groupID = getGroupID(id);

    pthread_mutex_lock(&mutex_counter);
    counter_binder[groupID - 1]++;
    pthread_mutex_unlock(&mutex_counter);

    if (counter_binder[groupID - 1] == size_group)
    {
        sem_post(&sem_groups_binder[groupID - 1]);
    }

    // if id is group leader, execute sem_wait
    if (id % size_group == 0)
    {
        sem_wait(&sem_groups_binder[groupID - 1]);
    }

    pthread_mutex_unlock(&mutex_printingStations[printer]);
}

void studentPrinting(int id, int time, timestamp last_time)
{
    // printing begins
    take_printer(id);

    sleep(time_printing);

    // printing ends
    put_printer(id);

    time += time_diff(last_time, time_now());

    // group leader goes to binding station and others' threads exit

    if (id % size_group != 0)
    {
        pthread_exit(NULL);
    }
    
    //cout << "id : " << id << endl;
    studentBinding(id, time, time_now());
}

void *arrivalPS(void *arg)
{
    int id = *((int *)arg);
    delete (int *)arg;
    arg = nullptr;

    // random delay using poisson distribution
    int delay = distribution(random_generator);
    usleep(delay * 1000000);

    // pthread_mutex_lock(&mutex_timer);

    // student arrives
    timestamp arrival_time = time_now();
    int time = time_diff(start_time, arrival_time);

    // pthread_mutex_unlock(&mutex_timer);

    print(id, STUDENT, "has arrived at the print station", time);

    studentPrinting(id, time, time_now());
    return NULL;
}

/* ****** Main ***** */
int main()
{

    freopen("input.txt", "r", stdin);
    freopen("output.txt", "w", stdout);

    // Initialize isAvailable array
    for (int i = 0; i < NUM_PRINTING_STATIONS; i++)
    {
        PSisAvailable[i] = true;
    }

    for (int i = 0; i < NUM_BINDING_STATIONS; i++)
    {
        BSisAvailable[i] = true;
    }

    cin >> num_students >> size_group >> time_printing >> time_binding >> time_entry_book;

    // Initialize mutexes
    for (int i = 0; i < NUM_PRINTING_STATIONS; i++)
    {
       pthread_mutex_init(&mutex_printingStations[i], NULL);
    }

    pthread_mutex_init(&mutex_bindingStations, NULL);
    pthread_mutex_init(&mutex_entryBook, NULL);
    pthread_mutex_init(&mutex_resource, NULL);
    pthread_mutex_init(&mutex_timer, NULL);
    pthread_mutex_init(&mutex_counter, NULL);
    pthread_mutex_init(&mutex_print, NULL);

    // Initialize semaphores
    sem_init(&sem_bindingStations, 0, NUM_BINDING_STATIONS);

    // Initialize semaphores for each student
    for (int i = 0; i < num_students; i++)
    {
        sem_t sem;
        sem_init(&sem, 0, 0);
        sem_students.push_back(sem);
    }

    // Initialize semaphores for each group
    for (int i = 0; i < size_group; i++)
    {
        counter_binder.push_back(0);
        sem_t sem;
        sem_init(&sem, 0, 0);
        sem_groups_binder.push_back(sem);
    }

    // array of threads for students
    pthread_t students[num_students];

    start_time = time_now();

    // threads initialization
    for (int i = 0; i < num_students; i++)
    {

        // set is student free
        state.push_back(FREE);

        int *id = new int(i + 1);
        pthread_create(&students[i], NULL, arrivalPS, (void *)id);
    }

    // threads initialization for 2 staffs
    pthread_t staffs[NUM_STAFFS];
    for (int i = 0; i < NUM_STAFFS; i++)
    {
        int *id = new int(i + 1);
        pthread_create(&staffs[i], NULL, entrybook, (void *)id);
    }

    // wait for all students to finish printing
    for (int i = 0; i < num_students; i++)
    {
        pthread_join(students[i], NULL);
    }

    // wait for all staffs to finish entry book
    for (int i = 0; i < NUM_STAFFS; i++)
    {
        pthread_join(staffs[i], NULL);
    }

    // Mutexes and semaphores destroy
    
    for (int i = 0; i < NUM_PRINTING_STATIONS; i++)
    {
       pthread_mutex_destroy(&mutex_printingStations[i]);
    }

    pthread_mutex_destroy(&mutex_bindingStations);
    pthread_mutex_destroy(&mutex_entryBook);
    pthread_mutex_destroy(&mutex_resource);
    pthread_mutex_destroy(&mutex_timer);
    pthread_mutex_destroy(&mutex_counter);
    pthread_mutex_destroy(&mutex_print);

    sem_destroy(&sem_bindingStations);

    for (int i = 0; i < num_students; i++)
    {
        sem_destroy(&sem_students[i]);
    }

    for (int i = 0; i < size_group; i++)
    {
        sem_destroy(&sem_groups_binder[i]);
    }

    return 0;
}
