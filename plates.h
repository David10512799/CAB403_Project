#ifndef PLATES_HEADERS
#define PLATES_HEADERS
#include <inttypes.h> // for portable integer declarations
#include <stdbool.h>  // for bool type
#include <stdio.h>    // for printf()
#include <stdlib.h>   // for malloc(), free(), NULL
#include <string.h>   // for strcmp()
#include <pthread.h>
#define PLATE_LENGTH 7
#endif
// An item inserted into a hash table.
// As hash collisions can occur, multiple items can exist in one bucket.
// Therefore, each bucket is a linked list of items that hashes to that bucket.
typedef struct item item_t;
struct item
{
    char value[PLATE_LENGTH];
    item_t *next;
};

// A hash table mapping a string to an integer.
typedef struct htab htab_t;
struct htab
{
    item_t **buckets;
    size_t size;
};

// HASHTABLE FUNCTIONS
bool htab_insert_plates(htab_t *h);
bool htab_init(htab_t *h, size_t n);
size_t htab_index(htab_t *h, char *key);
item_t *htab_bucket(htab_t *h, char *key);
size_t djb_hash(char *c);
bool htab_add(htab_t *h,char *value);
void htab_print(htab_t *h);
bool htab_search_value(htab_t *h, char *search);


void item_print(item_t *i)
{
    printf("value=%s",i->value);
}

bool htab_init(htab_t *h, size_t n)
{
    // TODO: implement this function
    h->size = n;
    h->buckets = calloc(n, sizeof(item_t*)); // array of pointers, therefore no matter how large table grows, same amount of memory
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

bool htab_add(htab_t *h, char *value)
{
    // TODO: implement this function

    size_t index = htab_index(h, value);

    item_t *new_item = malloc(sizeof(item_t));
    if (new_item == NULL){
        printf("Failed to allocate memory to new item\n");
        return false;
    }

    sprintf(new_item->value, value);
    new_item->next = h->buckets[index];

    h->buckets[index] = new_item;

}

void htab_print(htab_t *h)
{
    printf("hash table with %d buckets\n", h->size);
    for (size_t i = 0; i < h->size; ++i)
    {
        printf("bucket %d: ", i + 1);
        if (h->buckets[i] == NULL)
        {
            printf("empty\n");
        }
        else
        {
            for (item_t *j = h->buckets[i]; j != NULL; j = j->next)
            {
                item_print(j);
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
item_t *htab_bucket(htab_t *h, char *key)
{
    // TODO: implement this function (uses htab_index())
    size_t index = htab_index(h, key);
    return h->buckets[index];
}

bool htab_search_value(htab_t *h, char *search)
{
    for (size_t i = 0; i < h->size; ++i)
    {
        for (item_t *bucket = h->buckets[i]; bucket != NULL; bucket = bucket->next) {
            if (bucket->value == search) return true;
        }
    }
    return false;
}