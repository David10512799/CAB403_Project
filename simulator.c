#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "carpark.h"

shared_carpark_t carpark;


int main(int argc, char **argv){

    init_carpark(&carpark);

    return 0;
}



