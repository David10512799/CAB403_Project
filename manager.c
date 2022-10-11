#include <stdio.h>
#include <stdlib.h>

#include "carpark.h"

#define NUM_ENTRIES 5
#define NUM_EXITS 5
#define NUM_LEVELS 5
#define CARS_PER_LEVEL 20
#define PLATE_LENGTH 7


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


//Function declarations
void generate_GUI(carpark_t *data); 
char *gate_status(char code); void open_gate(); void close_gate(); 
void generate_bill(char *plate, int useconds);
void screen_controller();

// HASHTABLE FUNCTIONS
bool htab_insert_plates(htab_t *h);
bool htab_init(htab_t *h, size_t n);
size_t htab_index(htab_t *h, char *key);
item_t *htab_bucket(htab_t *h, char *key);
size_t djb_hash(char *c);
bool htab_add(htab_t *h,char *value);
void htab_print(htab_t *h);
void htab_search_value(htab_t *h, char *search);


void item_print(item_t *i)
{
    printf("value=%s",i->value);
}


int main(int argc, char **argv){

    //get_carpark(&carpark);

    size_t buckets = 40;
    htab_t h;
    
    if (!htab_init(&h, buckets))
    {
        printf("failed to initialise hash table\n");
        return EXIT_FAILURE;
    }

    htab_insert_plates(&h);
    
    shared_carpark_t carpark;

    get_carpark(&carpark);



    pthread_t gui;
    pthread_create(&gui, NULL, generate_GUI, &carpark.data);

    

    return 0;

}

void generate_bill(char *plate, int useconds)
{
    float cost = useconds * 0.05;

    FILE* bill = fopen("billing.txt", "a+");

    fprintf(bill, "%s $.2f", plate, cost);

    fclose(bill);
}

void generate_GUI( carpark_t *data )
{
    while (true)
    {
        usleep(50);
        printf("\033[2J"); // Clear screen

        printf(
        "╔═══════════════════════════════════════════════╗\n"
        "║                CARPARK SIMULATOR              ║    by DAVID AND DANIEL \n"
        "╚═══════════════════════════════════════════════╝\n"
        );
        
        printf("\n\e[1mTotal Revenue:\e[m $0.00\n\n");


        printf("\n\e[1mLevel\tCapacity\tLPR\t\tTemp (C)\e[m\n");
        printf("══════════════════════════════════════════════════════════════════\n\n");
        for (int i = 1; i <= NUM_LEVELS; i++)
        {
            printf("%d\t", i);
            printf("%d/%d\t\t", i, CARS_PER_LEVEL);
            printf("%s\t\t", "123ABC");
            printf("%d\t\t", 27);
            printf("\n");
        }

        printf("\n");

        printf("\n\e[1mEntry\tGate\t\tLPR\t\tDisplay\e[m\n");
        printf("══════════════════════════════════════════════════════════════════\n\n");
        for (int i = 1; i <= NUM_ENTRIES; i++)
        {
            printf("%d\t", i);
            printf("%s\t\t", gate_status('C'));
            printf("%s\t\t", "123ABC");
            printf("%c\t", '3');
            printf("\n");
        }

        printf("\n");
        
        printf("\n\e[1mExit\tGate\t\tLPR\e[m\n");
        printf("══════════════════════════════════════════════════════════════════\n\n");
        for (int i = 1; i <= NUM_ENTRIES; i++)
        {
            printf("%d\t", i);
            printf("%s\t", gate_status('L'));
            printf("%s\t", "123ABC");
            printf("\n");
        }

        printf("\n");
    }
}

char *gate_status(char code)
{
    switch((int)code)
    {
        case 67:
            return "Closed";
            break;
        case 79:
            return "Open";
            break;
        case 76:
            return "Lowering";
            break;
        case 82:
            return "Raising";
            break;
        default:
            return NULL;
            break;
    }

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

void htab_search_value(htab_t *h, char *search)
{
    for (size_t i = 0; i < h->size; ++i)
    {
        for (item_t *bucket = h->buckets[i]; bucket != NULL; bucket = bucket->next) {
            if (bucket->value == search) printf("Licence Plate %s has permission to enter", bucket->value);
        }
    }
    printf("\n");
}