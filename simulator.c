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

pthread_mutex_t queueChangeMutex;
pthread_mutex_t queueSleepMutex;
pthread_cond_t wakeUp;

typedef struct node node_t;
struct node
{
    char *plate;
    node_t *next;
};

//Car Simulation

void generate_plates(int arg, char ** argg);

int get_plate_count();

bool init_carpark(shared_carpark_t* carpark);

void init_carpark_values(carpark_t* park);

void *sim_car(void *arg);

void start_car_simulation(char** arg, shared_carpark_t carpark, htab_t *verified_cars, int argg);

node_t *node_add(node_t *head, char *plate);

node_t *node_find_name(node_t *head, char *plate);

node_t *node_delete(node_t *head, char *plate);

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
    printf("%d\n", plate_count);

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

    start_car_simulation(plate_registry, carpark, &verified_cars, plate_count);

    return EXIT_SUCCESS;
}

// node_t *car_list = { .car = NULL, .next = NULL };
node_t *car_list;
void start_car_simulation(char **plate_registry, shared_carpark_t carpark, htab_t *verified_cars, int plate_count){
    pthread_cond_init(&wakeUp, NULL);
    // Simulate cars while firealarms are off
    while (!carpark.data->level[0].temperature.alarm) 
    {   
        car_t *car;
        char *key;

        // Generate random valid plates until found one that is not being used
        do {
            srand(time(NULL));
            int plate_number;
            plate_number = (rand() % (plate_count -1));
            key = plate_registry[plate_number]; // random number between 0 and plate_count
            car = htab_find(verified_cars, key);
        } while(car->in_carpark);

        pthread_t valid_sim;
        pthread_create(&valid_sim, NULL, sim_car, key);

        ms_pause(100);

        char *invalid_plate = "123ABC"; //random invalid plate
        pthread_t invalid_sim;
        pthread_create(&invalid_sim, NULL, sim_car, invalid_plate);

        ms_pause(100); //random number between 1 to 100;
        // break;
    }    
}
// create thread
void *sim_car(void *arg)
{  
    char *plate = (char*)arg;

    int random_entry = rand() % ENTRIES;
    // int random_exit = rand() % EXITS;
    // int random_level = rand() % LEVELS;
    
    // Add car to queue
    pthread_mutex_lock(&queueChangeMutex);
    node_t *new_head = node_add(car_list, plate);
    car_list = new_head;   
    printf("%s joined the line\n", car_list->plate);
    pthread_mutex_unlock(&queueChangeMutex); 

    // Wait in line until boomgate is available    
    pthread_mutex_lock(&queueSleepMutex);
    while(node_find_name(car_list, plate)->next != NULL){
        pthread_cond_wait(&wakeUp, &queueSleepMutex);
    } 
    pthread_mutex_unlock(&queueSleepMutex);

    // Wait for the manager to set the boomgate to lowering
    pthread_mutex_lock(&carpark.data->entrance[random_entry].gate.mutex);
    printf("gate is %c\n", carpark.data->entrance[random_entry].gate.status);
    while (carpark.data->entrance[random_entry].gate.status != LOWERING && carpark.data->entrance[random_entry].gate.status != CLOSED)
    {
        printf("waitng on gate\n");
        pthread_cond_wait(&carpark.data->entrance[random_entry].gate.condition, &carpark.data->entrance[random_entry].gate.mutex);
        printf("gate is %c", carpark.data->entrance[random_entry].gate.status);
    }
    pthread_mutex_unlock(&carpark.data->entrance[random_entry].gate.mutex);

    // Close the boomgate
    if (carpark.data->entrance[random_entry].gate.status == LOWERING)
    {
        printf("Waiting for boomgate to close\n");
        ms_pause(10);
        carpark.data->entrance[random_entry].gate.status = CLOSED;
    }
    
    // Give licence plate to LPR and signal manager to check it
    printf("%s entry is %d\n", plate, random_entry + 1);
    pthread_mutex_lock(&carpark.data->entrance[random_entry].LPR.mutex);
    strcpy(carpark.data->entrance[random_entry].LPR.plate, plate);
    pthread_mutex_unlock(&carpark.data->entrance[random_entry].LPR.mutex);
    pthread_cond_signal(&carpark.data->entrance[random_entry].LPR.condition);

    // Wait for the display to update and read its value
    printf("checking sign\n");
    printf("the sign says %c\n", carpark.data->entrance[random_entry].sign.display);
    pthread_mutex_lock(&carpark.data->entrance[random_entry].sign.mutex);
    while (carpark.data->entrance[random_entry].sign.display == '-')
    {
        printf("waiting\n");
        pthread_cond_wait(&carpark.data->entrance[random_entry].sign.condition, &carpark.data->entrance[random_entry].sign.mutex);
    }
    pthread_mutex_unlock(&carpark.data->entrance[random_entry].sign.mutex);
    printf("the sign says %c\n", carpark.data->entrance[random_entry].sign.display);
    char display = carpark.data->entrance[random_entry].sign.display;

    // If denied entry drive off
    int level = (int)display - 48;
    if(!(0 < level) || !(level < 6))
    {
        printf("denied entry\n");

        ms_pause(10);

        pthread_mutex_lock(&queueChangeMutex);
        node_delete(car_list, plate);
        pthread_mutex_unlock(&queueChangeMutex);
        pthread_cond_signal(&wakeUp);

        return NULL;
    }

    // Wait for raising
    pthread_mutex_lock(&carpark.data->entrance[random_entry].gate.mutex);
    printf("gate is %c\n", carpark.data->entrance[random_entry].gate.status);
    while (carpark.data->entrance[random_entry].gate.status != RAISING)
    {
        pthread_cond_wait(&carpark.data->entrance[random_entry].gate.condition, &carpark.data->entrance[random_entry].gate.mutex);
        printf("gate is %c", carpark.data->entrance[random_entry].gate.status);
    }
    pthread_mutex_unlock(&carpark.data->entrance[random_entry].gate.mutex);
    
    // Wait for boomgate to raise
    printf("%s waiting at boomgate...\n", plate);
    ms_pause(10);
    carpark.data->entrance[random_entry].gate.status = OPEN;
    pthread_cond_signal(&carpark.data->entrance[random_entry].gate.condition);

    // Travel to level
    ms_pause(10); // could be after level lpr

    // Trigger level LPR

    // ms_pause(200);

    // Remove car from line and signal next car
    printf("%s entered care park\n", plate);
    pthread_mutex_lock(&queueChangeMutex);
    node_delete(car_list, plate);
    pthread_mutex_unlock(&queueChangeMutex);
    pthread_cond_signal(&wakeUp);

    // if(carpark->entrance->gate.status == 'O')
    // {
    //     car->carpark->level->LPR.plate;
    // }
 
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
        if (strcmp(head->plate, plate) == 0)
        {
            return head;
        }        
    }
    return NULL;    
}

// delete node and free malloc by plate
node_t *node_delete(node_t *head, char *plate)
{
    if (strcmp(head->plate, plate) == 0)
    {
        node_t* newHead = head->next;
        free(head);
        return newHead;
    }
    node_t *del = node_find_name(head, plate);
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
