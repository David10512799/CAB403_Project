#ifndef PLATES_HEADERS
#define PLATES_HEADERS
#include <inttypes.h> // for portable integer declarations
#include <stdbool.h>  // for bool type
#include <stdio.h>    // for printf()
#include <stdlib.h>   // for malloc(), free(), NULL
#include <string.h>   // for strcmp()
#include <pthread.h>
#include <sys/time.h>
#define PLATE_LENGTH 6
#endif
// An car inserted into a hash table.
// As hash collisions can occur, multiple cars can exist in one bucket.
// Therefore, each bucket is a linked list of cars that hashes to that bucket.
typedef struct car car_t;
struct car
{
    char plate[PLATE_LENGTH];
    int current_level;
    bool in_carpark;
    struct timeval entry_time;
    car_t *next;
};

// A hash table mapping a string to an integer.
typedef struct htab htab_t;
struct htab
{
    car_t **buckets;
    size_t size;
};

// HASHTABLE FUNCTIONS

//Inserts plates from plates.txt into hashtable
//Preconditions: The hashtable has been initialised
//Postconditions: the hashtable has been filled with plates from the plates.txt file
bool htab_insert_plates(htab_t *h);

//Initialise hashtable
//Preconditions: None
//Postconditions: Initialises the hashtable in memory
bool htab_init(htab_t *h, size_t n);

//Calculate the offset for the bucket for key in hash table.
//Preconditions: The hashtable has been initialised
//Postconditions: The plate in the hashtable has been indexed
size_t htab_index(htab_t *h, char *key);

//Find pointer to head of list for key in hash table.
//Preconditions: The hashtable has been initialised
//Postconditions: returns the pointer to the head of the link list with given plate.
car_t *htab_bucket(htab_t *h, char *key);

//hashes each entry into hashtable
//Preconditions: The hashtable has been initialised
//Postconditions: hashes the plate in the hashtable
size_t djb_hash(char *c);

//Adds a character array into hashtable and hashes it
//Preconditions: The hashtable has been initialised
//Postconditions: The hashtable is updated with the plate
bool htab_add(htab_t *h,char *plate);

//Searches the hashtable for the plate given
//Preconditions: The hashtable has been initialised
//Postconditions: true if plate is in hashtable, false if plate is not in hashtable
bool htab_search_plate(htab_t *h, char *search);

//Finds the corrosponding key to access a plate in the hashtable
//Preconditions: The hashtable has been initialised
//Postconditions: The key to a plate in the hashtable
car_t *htab_find(htab_t *h, char *key);

//Frees the memory taken by the hashtable
//Preconditions: The hashtable has been initialised
//Postconditions: The memory has been freed
void htab_destroy(htab_t *h);


void car_print(car_t *i)
{
    printf("plate=%.6s, in_carpark=%d",i->plate, i->in_carpark);
}

bool htab_init(htab_t *h, size_t n)
{
    h->size = n;
    h->buckets = calloc(n, sizeof(car_t*));
    // array of pointers, therefore no matter how large table grows, same amount of memory
    // calloc guarantess all that memory is initialised to zero
    return h->buckets != NULL;
}

bool htab_insert_plates(htab_t *h)
    {
    FILE* input_file = fopen("plates.txt", "r");

    if (input_file == NULL)
    {
        fprintf(stderr, "Error: failed to open file %s", "plates.txt");
        exit(1);
    }

    char scan[7];
    while( fscanf(input_file, "%s", scan) != EOF )
    {
        char plate[6];
        string2charr(scan,plate);
        htab_add(h, plate);
    }

    fclose(input_file);
    return true;
}

bool htab_add(htab_t *h, char *plate)
{
    // TODO: implement this function

    size_t index = htab_index(h, plate);

    car_t *new_car = malloc(sizeof(car_t));
    if (new_car == NULL){
        printf("Failed to allocate memory to new car\n");
        return false;
    }

    string2charr(plate, new_car->plate);
    new_car->in_carpark = false;
    new_car->next = h->buckets[index];

    h->buckets[index] = new_car;

    return true;
}

size_t djb_hash(char *s)
{
    size_t hash = 5381;
    for(int i = 0; i < PLATE_LENGTH; i++)
    {
        int c = (int)s[i];
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}


size_t htab_index(htab_t *h, char *key)
{
    return djb_hash(key) % h->size;
}

car_t *htab_bucket(htab_t *h, char *key)
{
    return h->buckets[htab_index(h, key)];
}

car_t *htab_find(htab_t *h, char *key)
{
    for (car_t *i = htab_bucket(h, key); i != NULL; i = i->next)
    {
        if (strncmp(i->plate, key, 6) == 0)
        { // found the key
            return i;
        }
    }
    return NULL;
}

bool htab_search_plate(htab_t *h, char *search)
{
    bool retVal = false;
    for (size_t i = 0; i < h->size; ++i)
    {
        for (car_t *bucket = h->buckets[i]; bucket != NULL; bucket = bucket->next) {
            if (strncmp(bucket->plate, search, 6) == 0)
            {
                retVal = true;
            } 
        }
    }
    return retVal;
}

void htab_destroy(htab_t *h)
{
    // free linked lists
    for (size_t i = 0; i < h->size; ++i)
    {
        car_t *bucket = h->buckets[i];
        while (bucket != NULL)
        {
            car_t *next = bucket->next;
            free(bucket);
            bucket = next;
        }
    }

    // free buckets array
    free(h->buckets);
    h->buckets = NULL;
    h->size = 0;
}

