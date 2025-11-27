// Compiling command: gcc -fopenmp  ising.c -o <executable name>
// To run the program type: <executable name> <size> <temperature> <steps>

// The program returns a directory with the evolution of the system over time, a file of the system energy values over every time step
// and a file of the system magnetization over time. Gnuplot can be used for plotting. 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <omp.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(name) _mkdir(name)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(name) mkdir(name, 0777)
#endif

typedef struct {
    int rows;
    int cols;
    int **data;
} Grid;

// Create the grid
Grid* Create_Grid(int rows, int cols) {
    Grid* system2D = malloc(sizeof(Grid));
    system2D->rows = rows;
    system2D->cols = cols;

    system2D->data = malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++) {
        system2D->data[i] = malloc(cols * sizeof(int));
    }

    return system2D;
}

// Initialize with random spins (-1 o +1)
void Fill_Grid(Grid* system2D) {
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < system2D->rows; i++) {
        for (int j = 0; j < system2D->cols; j++) {
            double rate = (double)rand() / RAND_MAX;
            system2D->data[i][j] = (rate > 0.5) ? 1 : -1;
        }
    }
}

// Local energy with periodic boundary condition
int single_energy(Grid* system2D, int i, int j) {
    int up    = system2D->data[(i - 1 + system2D->rows) % system2D->rows][j];
    int down  = system2D->data[(i + 1) % system2D->rows][j];
    int left  = system2D->data[i][(j - 1 + system2D->cols) % system2D->cols];
    int right = system2D->data[i][(j + 1) % system2D->cols];
    int s     = system2D->data[i][j];
    return -s * (up + down + left + right);
}

// Total energy
double total_energy(Grid* system2D) {
    double energy = 0.0;

    #pragma omp parallel for collapse(2) reduction(+:energy)
    for (int i = 0; i < system2D->rows; i++) {
        for (int j = 0; j < system2D->cols; j++) {
            energy += single_energy(system2D, i, j);
        }
    }
    return energy / 2.0;
}

// Magnetization calculation
int total_magnet(Grid* system2D){
    int magnet = 0;

    #pragma omp parallel for collapse(2) reduction(+:magnet)
    for(int i=0; i<system2D->rows; i++){
        for(int j=0; j<system2D->cols; j++){
            magnet += system2D->data[i][j];
        }
    }

    return magnet;
}

int random_int(int min, int max) {
    return min + rand() % (max - min + 1);
}


// Monte Carlo step
void spin_flip(Grid* system2D, double beta) {
    int i = random_int(0, system2D -> rows-1);
    int j = random_int(0, system2D -> cols-1);

    int dE = -2 * single_energy(system2D, i, j);
    double p = exp(-beta * dE);
    double r = (double)rand() / RAND_MAX;
    if (r < p) {
        system2D->data[i][j] *= -1;
    }
}

// State writing on file
void write_state(Grid* system2D, int step) {
    char filename[64];
    snprintf(filename, sizeof(filename), "evolution/state_step_%d.txt", step);
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Error opening file");
        return;
    }

    for (int i = 0; i < system2D->rows; i++) {
        for (int j = 0; j < system2D->cols; j++) {
            fprintf(f, "%2d ", system2D->data[i][j]);
        }
        fprintf(f, "\n");
    }

    fclose(f);
}

// Energy writing on file
void write_energy(int step, float energy){
    char filename[64];
    strcpy(filename, "en_vs_time.txt");


    FILE *f = fopen(filename, "a");
    if(!f){
        perror("Error opening file");
        return;
    }

    fprintf(f, "%d %f\n", step, energy);

    fclose(f);
}

void write_magnet(int step, int magnet){
    char filename[64];
    strcpy(filename, "magnet_vs_time.txt");

    FILE *f = fopen(filename, "a");
    if(!f){
        perror("Error opening file");
        return;
    }

    fprintf(f, "%d %d\n", step, magnet);

    fclose(f);
}

// MAIN
int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <Size> <Temperature> <Steps>\n", argv[0]);
        return 1;
    }

    omp_set_num_threads(8);

    int size = atoi(argv[1]);
    double T = atof(argv[2]);
    int step = atoi(argv[3]);
    float en;
    int magnet;

    // Cross-platform directory maker
    if (MKDIR("evolution") == 0) {
        printf("Directory created\n");
    }
    else {
        perror("Error in directory making");
    }

    srand(time(NULL));

    double start = omp_get_wtime();

    Grid* system2D = Create_Grid(size, size);
    Fill_Grid(system2D);

    double beta = 1.0/T;

    for (int s = 0; s < step; s++) {
        en = total_energy(system2D);
        magnet = total_magnet(system2D);
        write_energy(s, en);
        write_magnet(s, magnet);
        spin_flip(system2D, beta);
        if(s % 100 == 0){
            write_state(system2D, s);
            printf("Step %d completed\n", s);
        }
        
    }

    double stop = omp_get_wtime();

    printf("Elapsed time: %f s", stop-start);

    return 0;
}


