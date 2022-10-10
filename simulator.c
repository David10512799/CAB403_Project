#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>


//Car Simulation

//Declare function that builds the LPR (License plate reader)
void LPR();

//Declare Function to create a simulated car with random license plate
//Simulated car queues up at a random entrance triggering LPR when queue position = 0/1.
//Parameters: plates.txt --> plateList
char createCar();

//A function to check if the license plate of the car at the front of the queue is in plates.txt
void searchPlates();

//Declare function that runs after LPR is triggered to display digital sign.
//Parameters
char digitalSign(char licensePlate);

//The car will move to the chosen level by the manager and displayed on the digital sign.
void moveToLevel(int level, char licensePlate);

//The car will park for a random period of time.
void timeParked(char licensePlate);

//After the car is finished parking, the LPR will be triggered and the car will move to an exit and trigger
//the exit LPR and the boom gate will open. The car will exit the simulation.
//void exit(char licensePlate);

//BOOM GATE
void boomGateWait();

//TEMPERATURE SENSOR
void updateTemp();

//Create a license plate that is either unique or matches with a license plate in the plates.txt file.
char createCar();

typedef struct item item_t;
    struct item
    {
        size_t key;
        char *value;
        item_t *next;
    };


    typedef struct htab htab_t;
    struct htab
    {
        item_t *buckets;
        size_t size;
    };

    size_t buckets = 20; 
    htab_t h;

bool htab_add(size_t key, char *value);

bool htab_init(htab_t *h, size_t n);

size_t htab_index(htab_t *h, size_t key);

item_t *htab_bucket(htab_t *h, int key);

size_t djb_hash(char *c);

bool htab_insert_plates();

void init();

int main(int argc, char **argv){
    init();
    return 0;
}

    //Create Hashmap table
    bool htab_init(htab_t *h, size_t n)
    {
        printf("Creating HashTable\n");
        h->size = n;
        h->buckets = (item_t **)calloc(n, sizeof(item_t*));
        return h->buckets != 0;
    }

    void item_print(item_t *i)
{
    printf("key=%s value=%d", i->key, i->value);
}
    
    //find pointer of h
    // item_t *htab_bucket(htab_t *h, int key)
    // {
    //     size_t index = htab_index(h, key);
    //     return h->buckets[index];
    // }

    //Bernstein hash function
    size_t djb_hash(char *c)
    {
    size_t hash = 5381;
    int s;
    while ((s = *c++) != '\0')
    {
        hash = ((hash << 5) + hash) + s;
    }
    return hash;
    }

    //offset for bucket for key in hash table
    // size_t htab_index(htab_t *h, size_t key)
    // {
    //     return djb_hash(key) % h->size;
    // }


    bool htab_add(size_t key, char *value)
    {
    //item_t *new_plate = (item_t *)malloc(sizeof(item_t));

    //h->buckets[key] = malloc(sizeof(item_t));


    // if(new_plate == NULL)
    // {
    //     return false;
    // }

    item_t new_plate;

    new_plate.key = key;
    new_plate.value = value;
    new_plate.next = NULL;

    new_plate.next = h.buckets[key];
    h.buckets[key] = &new_plate;

    return true;
    }

    bool htab_insert_plates()
    {
    FILE* input_file = fopen("plates.txt", "r");
    int num;
    int number_plates;

    if (input_file == NULL)
    {
        fprintf(stderr, "Error: failed to open file %s", "plates.txt");
        exit(1);
    }

    char plate[6];
    while(fscanf(input_file, "%s", plate) != EOF)
    {
        //printf("%s\n", plate);
        size_t key = djb_hash(plate) % h.size;
        //printf("%d\n", key);
        htab_add(key, plate);
    }

    fclose(input_file);
    return true;
    }

void htab_print()
{
    printf("hash table with %d buckets\n", h.size);
    for (size_t i = 0; i < h.size; ++i)
    {
        printf("bucket %d: ", i);
        if (h.buckets[i] == NULL)
        {
            printf("empty\n");
        }
        else
        {
            // for (item_t *j = h.buckets[i]; j != NULL; j = j->next)
            // {
            //     item_print(j);
            //     if (j->next != NULL)
            //     {
            //         printf(" -> ");
            //     }
            // }
            printf("%d", h.buckets[i].value);
            printf("\n");
        }
    }
}
//Open plates.txt and search the list of plates against the plate of the first car in the queue. 
void init(){
    {
    htab_init(&h, buckets);
    htab_insert_plates();
    htab_print();
    
}
};

