#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "random_distribution.h"

int geometric_dist(int mean){
    double u = ((double) rand() + 1.0) / ((double) RAND_MAX + 1.0);

    return (int)ceil(log(u) / log(1.0 - 1 /(double)(mean)));
}



int uniform_dist(int min, int max) {
    return rand() % (max - min + 1) + min;
}
