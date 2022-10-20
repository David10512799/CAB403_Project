#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "carpark.h"
#include "carlist.h"

extern int errno;

typedef struct car_sim car_sim_t;
struct car_sim
{
    carpark_t *carpark;
    char *plate;
};
//Car Simulation

void generate_plates(int arg, char ** argg);

int get_plate_count();

bool init_carpark(shared_carpark_t* carpark);

void init_carpark_values(carpark_t* park);

void *sim_car(void *arg);

void start_car_simulation(char** arg, shared_carpark_t carpark, htab_t *verified_cars, int argg);

int plate_count;

int main(void){

    // Generate Shared Memory
    shared_carpark_t carpark;
    
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
    char **plate_registry = calloc(plate_count, PLATE_LENGTH);
    for (int i = 0; i < plate_count; i++)
    {
        plate_registry[i] = plate_registry_temp[i];
    }
    
    // generate_plates(plate_count, plate_registry);
    
    for (int i = 0; i < plate_count; i++)
    {
        printf("%s", plate_registry[i]);

    }
    
    
    // for(int i=0; i < plate_count; i++){
    //     printf("%s\n", plate_registry[i]);
    // }

    //printf("%s", plate_registry[25]);
    //printf("\n");

    //start_car_simulation(plate_registry, carpark, &verified_cars, plate_count);

    return EXIT_SUCCESS;
}

void start_car_simulation(char **plate_registry, shared_carpark_t carpark, htab_t *verified_cars, int plate_count){
    // Simulate cars while firealarms are off
    while (!carpark.data->level[0].temperature.alarm) 
    {   
        car_t *car;
        char *key;
        plate_registry;

        do {
        srand(time(NULL));
        int plate_number;
        plate_number = (rand() % (plate_count -1));
        key = plate_registry[plate_number]; // random number between 0 and plate_count
        //printf("%s", key);

        //car = htab_find(verified_cars, key);

        } while(car->in_carpark);

        // pthread_t valid_sim;
        // car_sim_t sim;
        // sim.carpark = carpark.data;
        // sim.plate = key;
        // pthread_create(&valid_sim, NULL, sim_car, &sim);

        // ms_pause(100);

        // char *invalid_plate = "123ABC";

        // pthread_t invalid_sim;
        // sim.plate = invalid_plate;
        // pthread_create(&invalid_sim, NULL, sim_car, &sim);

        // ms_pause(100); //random number between 1 to 100;
        break;
    }    

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

void generate_plates(int plates_count, char** plate_registry){
    // Read Plates File
    FILE* input_file = fopen("plates.txt", "r");

    // Going back to start of file
    char plate[PLATE_LENGTH];
    int i = 0;

    while( fscanf(input_file, "%s", plate) != EOF)
    {
        char temp[PLATE_LENGTH];
        strcpy(temp, plate);
        plate_registry[i] = temp;
        printf("%s\n", plate_registry[i]);
        i++;
    }    

    fclose(input_file);
}


// create thread
void *sim_car(void *arg)
{  
    car_sim_t *car = (car_sim_t *)arg;
    carpark_t *carpark = car->carpark;
    char *plate = car->plate;

    int random_entry = rand() % ENTRIES;
    int random_exit = rand() % EXITS;
    int random_level = rand() % LEVELS;
    
    //Create Queue
    //When car reaches front of Queue
    //Send plate to entrance (1 through 5), await response.
    car->carpark->entrance[random_entry].LPR.plate;

    //Set Cars random Parameters (Level and Exit)
    car->carpark->level[random_level].LPR.plate;
    car->carpark->exit[random_exit].LPR.plate;

    //pthread_mutex_lock(&carpark->entrance->LPR.mutex);
    //while (string_equal(carpark->entrance->LPR.plate, EMPTY_LPR))
    //pthread_cond_wait(&carpark->entrance->sign.display, &carpark->entrance->sign.mutex);

    if(isdigit(carpark->entrance->sign.display) != 0)
    {
        //Remove Car from Queue
        return NULL;
    }

    //Wait for Boom gate to OPEN
    //pthread_cond_wait(&carpark->entrance->gate.status, &carpark->entrance->gate.status);

    if(carpark->entrance->gate.status == 'O')
    {
        car->carpark->level->LPR.plate;
    }
 
    return NULL;
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
        park->entrance[i].gate.status = 'C';
        pthread_mutex_init(&park->entrance[i].gate.mutex, &mutex_attr);
        pthread_cond_init(&park->entrance[i].gate.condition, &cond_attr);

        
        park->entrance[i].LPR.plate = EMPTY_LPR;
        pthread_mutex_init(&park->entrance[i].LPR.mutex, &mutex_attr);
        pthread_cond_init(&park->entrance[i].LPR.condition, &cond_attr);

        pthread_mutex_init(&park->entrance[i].sign.mutex, &mutex_attr);
        pthread_cond_init(&park->entrance[i].sign.condition, &cond_attr);        

        // EXITS
        park->exit[i].gate.status = 'C';
        pthread_mutex_init(&park->exit[i].gate.mutex, &mutex_attr);
        pthread_cond_init(&park->exit[i].gate.condition, &cond_attr);

        park->exit[i].LPR.plate = EMPTY_LPR;
        pthread_mutex_init(&park->exit[i].LPR.mutex, &mutex_attr);
        pthread_cond_init(&park->exit[i].LPR.condition, &cond_attr);

        // LEVELS
        park->level[i].LPR.plate = EMPTY_LPR;
        pthread_mutex_init(&park->level[i].LPR.mutex, &mutex_attr);
        pthread_cond_init(&park->level[i].LPR.condition, &cond_attr);

        park->level[i].temperature.alarm = 0;
        park->level[i].temperature.sensor = 0;
        
    }
}
