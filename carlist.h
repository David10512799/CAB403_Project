#ifndef PLATES_HEADERS
#define PLATES_HEADERS
#include <inttypes.h> // for portable integer declarations
#include <stdbool.h>  // for bool type
#include <stdio.h>    // for printf()
#include <stdlib.h>   // for malloc(), free(), NULL
#include <string.h>   // for strcmp()
#include <pthread.h>
#include <sys/time.h>
#define PLATE_LENGTH 7
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
bool htab_insert_plates(htab_t *h);
bool htab_init(htab_t *h, size_t n);
size_t htab_index(htab_t *h, char *key);
car_t *htab_bucket(htab_t *h, char *key);
size_t djb_hash(char *c);
bool htab_add(htab_t *h,char *plate);
void htab_print(htab_t *h);
bool htab_search_plate(htab_t *h, char *search);
car_t *htab_find(htab_t *h, char *key);


void car_print(car_t *i)
{
    printf("plate=%s, in_carpark=%d",i->plate, i->in_carpark);
}

bool htab_init(htab_t *h, size_t n)
{
    // TODO: implement this function
    h->size = n;
    h->buckets = calloc(n, sizeof(car_t*)); // array of pointers, therefore no matter how large table grows, same amount of memory
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

    char plate[PLATE_LENGTH];
    while( fscanf(input_file, "%s", plate) != EOF )
    {
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

    sprintf(new_car->plate, plate);
    new_car->in_carpark = false;
    new_car->next = h->buckets[index];

    h->buckets[index] = new_car;

    return true;
}

void htab_print(htab_t *h)
{
    printf("hash table with %ld buckets\n", h->size);
    for (size_t i = 0; i < h->size; ++i)
    {
        printf("bucket %ld: ", i + 1);
        if (h->buckets[i] == NULL)
        {
            printf("empty\n");
        }
        else
        {
            for (car_t *j = h->buckets[i]; j != NULL; j = j->next)
            {
                car_print(j);
                if (j->next != NULL)
                {
                    printf(" -> ");
                }
            }
            printf("\n");
        }
    }
}

size_t djb_hash(char *s)
{
    size_t hash = 5381;
    int c;
    while ((c = *s++) != '\0')
    {
        // hash = hash * 33 + c
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// Calculate the offset for the bucket for key in hash table.
size_t htab_index(htab_t *h, char *key)
{
    return djb_hash(key) % h->size;
}

// Find pointer to head of list for key in hash table.
car_t *htab_bucket(htab_t *h, char *key)
{
    return h->buckets[htab_index(h, key)];
}

// Find an item for key in hash table.
// pre: true
// post: (return == NULL AND item not found)
//       OR (strcmp(return->key, key) == 0)
car_t *htab_find(htab_t *h, char *key)
{
    for (car_t *i = htab_bucket(h, key); i != NULL; i = i->next)
    {
        if (strcmp(i->plate, key) == 0)
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
            if (strcmp(bucket->plate, search) == 0)
            {
                retVal = true;
            } 
        }
    }
    return retVal;
}

