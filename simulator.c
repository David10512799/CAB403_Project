#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "carpark.h"
#include "carlist.h"

extern int errno;

shared_carpark_t carpark;

// pthread_mutex_t queueChangeMutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t queueSleepMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wakeUp = PTHREAD_COND_INITIALIZER;

pthread_mutex_t localGateMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t localGate = PTHREAD_COND_INITIALIZER;


typedef struct node node_t;
struct node
{
    char *plate;
    node_t *next;
};

node_t *car_list[ENTRIES];

//Car Simulation

void generate_plates(int arg, char ** argg);

int get_plate_count();

bool init_carpark(shared_carpark_t* carpark);

void init_carpark_values(carpark_t* park);

void *sim_car(void *arg);

void start_car_simulation(char** arg, shared_carpark_t carpark, htab_t *verified_cars, int argg);

node_t *node_add(node_t *head, char *plate);

node_t *node_find_name(node_t *head, char *plate);

node_t *node_find_name_array(node_t **node_array, char *plate, int array_len);

node_t *node_delete(node_t *head, char *plate);

void *monitor_gate(void *arg);

int plate_count;

int main(void){

    // Generate Shared Memory
    
    if (!init_carpark(&carpark))
    {
        fprintf(stderr, "Error initialising shared memory: %s\n", strerror(errno));
    }

    // Generate Hash Table
    htab_t verified_cars;
    int buckets = 40;

    if (!htab_init(&verified_cars, buckets))
    {
        printf("failed to initialise hash table\n");
    }
    
    htab_insert_plates(&verified_cars);
    
    plate_count = get_plate_count();

    // Create array and store plate char[] to it
    char plate_registry_temp[plate_count][PLATE_LENGTH];
    FILE* input_file = fopen("plates.txt", "r");
    for (int i = 0; fscanf(input_file, "%s", plate_registry_temp[i]) != EOF; i++)
    {
    }
    fclose(input_file);

    // Create pointer array to each of the plates
    // char **plate_registry = calloc(plate_count, PLATE_LENGTH);
    char *plate_registry[plate_count];
    for (int i = 0; i < plate_count; i++)
    {
        plate_registry[i] = plate_registry_temp[i];
    }

    // monitor entry boom gates transitions
    for( int i = 0; i < ENTRIES; i++){
        pthread_t gate;
        pthread_create(&gate, NULL, monitor_gate, &carpark.data->entrance[i].gate);
    }

    // monitor exit boom gates transitions
    for( int i = 0; i < ENTRIES; i++){
        pthread_t gate;
        pthread_create(&gate, NULL, monitor_gate, &carpark.data->exit[i].gate);
    }


    start_car_simulation(plate_registry, carpark, &verified_cars, plate_count);

    //Cleanup protocol

    return EXIT_SUCCESS;
}

void *monitor_gate(void *arg)
{
    gate_t *gate = (gate_t *)arg;

    for(;;)
    {
        //lock gate mutex and wait for signal
        pthread_mutex_lock(&gate->mutex);
        while(gate->status != LOWERING && gate->status != RAISING)
        {
            pthread_cond_wait(&gate->condition, &gate->mutex);
        }
        // delay for 10ms * TIMEX
        ms_pause(10);
        
        char status = gate->status;

        if (status == LOWERING)
        {
            gate->status = CLOSED;
        }
        if (status == RAISING)
        {
            gate->status = OPEN;
        }

        // signal condition variable and unlock mutex
        pthread_cond_signal(&gate->condition); // Signal manager it set to lowering or closed
        pthread_cond_broadcast(&localGate); // Signal car to check boomgate status
        pthread_mutex_unlock(&gate->mutex);
    }

    return NULL;
}

void start_car_simulation(char **plate_registry, shared_carpark_t carpark, htab_t *verified_cars, int plate_count){
    // pthread_cond_init(&wakeUp, NULL);
    // Simulate cars while firealarms are off
    while (!carpark.data->level[0].temperature.alarm) 
    {   
        
        int gen_rand = rand() % 2; // Can be 0 or 1
        car_t *car;
        char *key;
        char rand_plate[PLATE_LENGTH];
        int rand_wait = (rand() % 99) + 1;

        // generate either a valid or invalid plate and start simulating a car with it
        switch (gen_rand){
            case 0: // Valid plate            
                // Generate random valid plates until found one that is not being used
                do {                
                    srand(time(NULL));
                    int plate_number;
                    plate_number = (rand() % (plate_count -1));
                    key = plate_registry[plate_number]; // random number between 0 and plate_count
                    car = htab_find(verified_cars, key);

                } while(car->in_carpark || node_find_name_array(car_list, key, ENTRIES) != NULL);
                
                pthread_t valid_sim;
                pthread_create(&valid_sim, NULL, sim_car, key);

                ms_pause(rand_wait); // Random time between 1 and 100             
                break;

            case 1: // Invalid plate
                do
                {
                    srand(time(NULL));
                    for (int i = 0; i < 3; i++)
                    {
                        rand_plate[i] = (rand() % 10) + 48;
                    }
                    for (int i = 3; i < 6; i++)
                    {
                        rand_plate[i] = (rand() % 26) + 65;
                    }
                    car = htab_find(verified_cars, rand_plate);
                } while (node_find_name_array(car_list, key, ENTRIES) != NULL || car != NULL);
                

                pthread_t invalid_sim;
                pthread_create(&invalid_sim, NULL, sim_car, rand_plate);

                ms_pause(rand_wait); //random number between 1 to 100;
                break;    

            default:
                break;
        }

        // // break;
    }    
}
// create thread
void *sim_car(void *arg)
{  
    char *plate = (char*)arg;

    int random_entry = rand() % ENTRIES;
    // int random_exit = rand() % EXITS;
    
    // Add car to queue
    pthread_mutex_lock(&queueMutex);
    // node_t *new_head = node_add(car_list[random_entry], plate);
    car_list[random_entry] = node_add(car_list[random_entry], plate);;   
    printf("%s joined the line for entry %d\n", car_list[random_entry]->plate, random_entry + 1);
    pthread_mutex_unlock(&queueMutex); 

    // Wait in line until boomgate is available    
    pthread_mutex_lock(&queueMutex);
    while(node_find_name(car_list[random_entry], plate)->next != NULL){
        // printf("%s next is %s\n", plate, node_find_name(car_list[random_entry], plate)->next->plate);
        pthread_cond_wait(&wakeUp, &queueMutex);
        // printf("%s signaled\n", plate);
        // if (node_find_name(car_list[random_entry], plate)->next != NULL){
        //     printf("%s next is %s\n", plate, node_find_name(car_list[random_entry], plate)->next->plate);
        // }
    } 
    // printf("%s passed signal loop\n", plate);
    pthread_mutex_unlock(&queueMutex);

    // Wait at boomgate until it is fully closed
    printf("%s arrived at boomgate %d\n", plate, random_entry + 1);
    pthread_mutex_lock(&localGateMutex);
    while(carpark.data->entrance[random_entry].gate.status != CLOSED)
    {
        printf("%s waiting on gate to close. gate is %c\n", plate, carpark.data->entrance[random_entry].gate.status);
        pthread_cond_wait(&localGate, &localGateMutex);
        // printf("%s gate signaled. gate is %c\n", plate, carpark.data->entrance[random_entry].gate.status);
    }
    // printf("%s passed closed gate loop\n", plate);
    pthread_mutex_unlock(&localGateMutex);
    
    // Give licence plate to LPR and signal manager to check it
    // printf("%s entry is %d\n", plate, random_entry + 1);
    pthread_mutex_lock(&carpark.data->entrance[random_entry].LPR.mutex);
    strcpy(carpark.data->entrance[random_entry].LPR.plate, plate);
    pthread_mutex_unlock(&carpark.data->entrance[random_entry].LPR.mutex);
    pthread_cond_signal(&carpark.data->entrance[random_entry].LPR.condition);

    // Wait for the display to update and read its value
    // printf("checking sign\n");
    // printf("the sign says %c\n", carpark.data->entrance[random_entry].sign.display);
    pthread_mutex_lock(&carpark.data->entrance[random_entry].sign.mutex);
    while (carpark.data->entrance[random_entry].sign.display == '-')
    {
        printf("%s is waiting for display to update\n", plate);
        pthread_cond_wait(&carpark.data->entrance[random_entry].sign.condition, &carpark.data->entrance[random_entry].sign.mutex);
    }
    pthread_mutex_unlock(&carpark.data->entrance[random_entry].sign.mutex);
    printf("%s read the sign saying %c\n", plate, carpark.data->entrance[random_entry].sign.display);
    char display = carpark.data->entrance[random_entry].sign.display;

    // If denied entry drive off
    int level = (int)display - 48;
    if(!(0 < level) || !(level < 6))
    {
        printf("%s denied entry so it drives off...\n", plate);

        ms_pause(10);

        pthread_mutex_lock(&queueMutex);
        car_list[random_entry] = node_delete(car_list[random_entry], plate);
        pthread_mutex_unlock(&queueMutex);
        pthread_cond_broadcast(&wakeUp);

        return NULL;
    }
    printf("level value is %d\n", level);
    // level -= 1;
    // Wait for boomgate to open
    // printf("%s waiting for boomgate to open\n", plate);

    pthread_mutex_lock(&localGateMutex);
    // printf("gate status is %c\n", carpark.data->entrance[random_entry].gate.status);
    while (carpark.data->entrance[random_entry].gate.status != OPEN)
    {
        printf("%s is waiting for gate to open. It is %c\n", plate, carpark.data->entrance[random_entry].gate.status);
        pthread_cond_wait(&localGate, &localGateMutex);
    }
    pthread_mutex_unlock(&localGateMutex);
    // printf("gate status is %c\n", carpark.data->entrance[random_entry].gate.status);
    
    // Remove car from line and signal next car
    printf("%s entered car park\n", plate);
    pthread_mutex_lock(&queueMutex);
    car_list[random_entry] = node_delete(car_list[random_entry], plate);
    pthread_mutex_unlock(&queueMutex);
    // printf("sent signal\n");
    pthread_cond_broadcast(&wakeUp);

    // Travel to level
    ms_pause(10); // could be after level lpr

    // Trigger level LPR  

    pthread_mutex_lock(&carpark.data->level[level].LPR.mutex);
    strcpy(carpark.data->level[level].LPR.plate, plate);
    pthread_cond_signal(&carpark.data->level[level].LPR.condition);
    pthread_mutex_unlock(&carpark.data->level[level].LPR.mutex);
 
    return NULL;
}

// add plate to linked list
node_t *node_add(node_t *head, char *plate)
{
    node_t *newNode = malloc(sizeof(node_t));
    if (newNode == NULL)
    {
        return NULL;
    }    
    newNode->plate = plate;
    newNode->next = head;
    if (newNode == newNode->next)
    {
        newNode->next = NULL;
    }    
    return newNode;
}

// find car by plate
node_t *node_find_name(node_t *head, char *plate)
{
    for ( ; head != NULL; head = head->next)
    {
        // printf("freeze here maybe?\n");
        if (head->plate != NULL)
        {
            if (strcmp(head->plate, plate) == 0)
            {
                return head;
            }  
        }              
    }
    return NULL;    
}

node_t *node_find_name_array(node_t **node_array, char *plate, int array_len){
    // printf("searching all lists once\n");
    for (int i = 0; i < array_len; i++)
    {
        // printf("no way does it get stuck here right?\n");
        if (node_find_name(node_array[i], plate) != NULL)
        {
            // printf("found dupe\n");
            return node_array[i];
        }
        // printf("passed node find if\n");
    }
    return NULL;    
}

// delete node and free malloc by plate
node_t *node_delete(node_t *head, char *plate)
{
    if (strcmp(head->plate, plate) == 0)
    {
        node_t* newHead = head->next;
        // printf("node is end and start so NULL right? -> %p\n", newHead);
        // if (head != NULL)
        // {
        //     printf("nothing should be waiting on %s\n", head->plate);
        // }        
        free(head);
        return newHead;
    }
    node_t *del = node_find_name(head, plate);
    // printf("this should always be NULL -> %p\n", del->next);
    node_t *temp = head;
    for (; head != NULL; head = head->next)
    {
        if (head->next == del)
        {
            head->next = del->next;
            free(del);
        }        
    }
    return temp;
}

int get_plate_count(){
    FILE* input_file = fopen("plates.txt", "r");

    if (input_file == NULL)
    {
        fprintf(stderr, "Error: failed to open file %s", "plates.txt");
        exit(1);
    }
    plate_count = 0;
    char plate[PLATE_LENGTH];
    while( fscanf(input_file, "%s", plate) != EOF )
    {
        plate_count++;
    }
    fclose(input_file);

    return plate_count;
}

bool init_carpark(shared_carpark_t* carpark) 
{

    shm_unlink(SHARE_NAME);

    carpark->name = SHARE_NAME;

    if ((carpark->fd = shm_open(SHARE_NAME, O_CREAT | O_RDWR , 0666)) < 0 ){
        perror("shm_open");
        printf("Failed to create shared memory object for Carpark\n");
        exit(1);
    }

    if (ftruncate(carpark->fd, sizeof(carpark_t))){
        perror("ftruncate");
        printf("Failed to initialise size of shared memory object for Carpark\n");
        exit(1);
    }

    if  ((carpark->data = mmap(0, sizeof(carpark_t), PROT_READ | PROT_WRITE, MAP_SHARED, carpark->fd, 0)) == (void *)-1 ){
        perror("mmap");
        printf("Failed to map the shared memory for Carpark");
        exit(1);
    }

    init_carpark_values(carpark->data);

    return true;
}



void init_carpark_values(carpark_t* park)
{

    pthread_mutexattr_t mutex_attr;
    if (pthread_mutexattr_init(&mutex_attr) != 0)
    {
        perror("pthread_mutexattr_init");
        exit(1);
    }
    if ( pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED) != 0)
    {
        perror("pthread_mutexattr_setpshared");
        exit(1);
    }

    pthread_condattr_t cond_attr;
    if (pthread_condattr_init(&cond_attr) != 0)
    {
        perror("pthread_mutexattr_init");
        exit(1);
    }
    if ( pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED) != 0)
    {
        perror("pthread_mutexattr_setpshared");
        exit(1);
    }    

    for (int i = 0; i < 5; i++)
    {
        // ENTRANCES
        park->entrance[i].gate.status = CLOSED;
        pthread_mutex_init(&park->entrance[i].gate.mutex, &mutex_attr);
        pthread_cond_init(&park->entrance[i].gate.condition, &cond_attr);


        strcpy(park->entrance[i].LPR.plate, EMPTY_LPR);
        pthread_mutex_init(&park->entrance[i].LPR.mutex, &mutex_attr);
        pthread_cond_init(&park->entrance[i].LPR.condition, &cond_attr);

        park->entrance[i].sign.display = EMPTY_SIGN;
        pthread_mutex_init(&park->entrance[i].sign.mutex, &mutex_attr);
        pthread_cond_init(&park->entrance[i].sign.condition, &cond_attr);        

        // EXITS
        park->exit[i].gate.status = 'C';
        pthread_mutex_init(&park->exit[i].gate.mutex, &mutex_attr);
        pthread_cond_init(&park->exit[i].gate.condition, &cond_attr);

        strcpy(park->exit[i].LPR.plate, EMPTY_LPR);
        pthread_mutex_init(&park->exit[i].LPR.mutex, &mutex_attr);
        pthread_cond_init(&park->exit[i].LPR.condition, &cond_attr);

        // LEVELS
        strcpy(park->level[i].LPR.plate, EMPTY_LPR);
        pthread_mutex_init(&park->level[i].LPR.mutex, &mutex_attr);
        pthread_cond_init(&park->level[i].LPR.condition, &cond_attr);

        park->level[i].temperature.alarm = 0;
        park->level[i].temperature.sensor = 0;
        
    }
}
