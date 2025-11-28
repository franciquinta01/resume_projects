/*The following program recreates the Sierpinski's triangle with the method of the chaos game.
Compile command with gcc: gcc triangle.c -o <executable name>
To show the triangle use Gnuplot: plot "triangle.txt" using 1:2 with points pointtype 7
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int dim;
    int vert;
    float **coord;
} triangle;

triangle* init_triangle(int dim, int vert) {
    triangle *Trig = malloc(sizeof(triangle));
    Trig->vert = vert;
    Trig->dim = dim;

    Trig->coord = malloc(vert * sizeof(float*));
    for (int i = 0; i < vert; i++) {
        Trig->coord[i] = malloc(dim * sizeof(float));
    }

    return Trig;
}

void set_triangle(triangle* Trig, float M[][2]) {
    for (int i = 0; i < Trig->vert; i++) {
        for (int j = 0; j < Trig->dim; j++) {
            Trig->coord[i][j] = M[i][j];
        }
    }
}

int random_int(int min, int max) {
    return min + rand() % (max - min + 1);
}

float random_float(float min, float max) {
    return min + ((float) rand() / RAND_MAX) * (max - min);
}

float* find_point(triangle* Trig) {
    float* arr = malloc(Trig->dim * sizeof(float));

    float xr = random_float(Trig->coord[0][0], Trig->coord[1][0]);

    float max;
    if (xr <= 0) {
        max = ((Trig->coord[0][1] - Trig->coord[2][1]) /
               (Trig->coord[0][0] - Trig->coord[2][0])) * xr +
              (Trig->coord[0][1] -
               ((Trig->coord[0][1] - Trig->coord[2][1]) /
               (Trig->coord[0][0] - Trig->coord[2][0])) * Trig->coord[0][0]);
    } else {
        max = ((Trig->coord[1][1] - Trig->coord[2][1]) /
               (Trig->coord[1][0] - Trig->coord[2][0])) * xr +
              (Trig->coord[1][1] -
               ((Trig->coord[1][1] - Trig->coord[2][1]) /
               (Trig->coord[1][0] - Trig->coord[2][0])) * Trig->coord[1][0]);
    }

    float yr = random_float(Trig->coord[0][1], max);

    arr[0] = xr;
    arr[1] = yr;

    return arr;
}

float* find_middle(triangle* Trig, float* current) {
    float* mid = malloc(Trig->dim * sizeof(float));
    //float* point = find_point(Trig);

    int vertex = random_int(0, 2);

    mid[0] = (Trig->coord[vertex][0] + current[0]) / 2;
    mid[1] = (Trig->coord[vertex][1] + current[1]) / 2;

    //free(point);
    return mid;
}

void write_point(float* arr) {
    FILE *f = fopen("triangle.txt", "a");
    if (!f) {
        perror("Error opening file");
        return;
    }

    fprintf(f, "%f %f\n", arr[0], arr[1]);
    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <# steps>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));

    int steps = atoi(argv[1]);
    int dim = 2;
    int vert = 3;

    float M[3][2] = {
        {-1, 0},
        {1, 0},
        {0, 1}
    };

    triangle* Trig = init_triangle(dim, vert);
    set_triangle(Trig, M);

    float* current = find_point(Trig);
    for (int i = 0; i < steps; i++) {
        float* mid = find_middle(Trig, current);
        write_point(mid);
        current = mid;
    }

    return 0;
}

