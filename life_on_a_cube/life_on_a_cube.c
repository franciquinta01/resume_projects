/*Il codice qui presentato si basa sull'implementazione proposta da Moreno Marzolla per il 
game of life in 2D adattato al problema in esame.*/

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <errno.h>
#include <string.h>
#include <direct.h>

#define SIZE 6   // Numero di facce del cubo
#define DIM 40   // Dimensione di ogni faccia
#define CUBES_PER_ROW 2 // Numero di cubi per riga
#define CUBES_PER_COL 2 // Numero di cubi per colonna

typedef struct {
    int values[DIM+2][DIM+2];
} Face;

typedef struct {
    Face faces[SIZE]; // Array di 6 facce
} Cube;

typedef struct {
    Cube cubes[CUBES_PER_ROW][CUBES_PER_COL]; // Griglia 2x2 di cubi
} SuperCube;

#define MAX_PATH_LENGTH 1024

// Funzione per creare directory ricorsivamente
int mkdir_recursive(const char *path) {
    char temp[MAX_PATH_LENGTH];
    char *pos = temp;

    // Copia il percorso nella stringa temporanea
    strncpy(temp, path, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    // Creare le directory intermedie
    while ((pos = strchr(pos, '/')) != NULL) {
        *pos = '\0';
        if (_mkdir(temp) && errno != EEXIST) {
            perror("Errore nella creazione della cartella");
            return -1;
        }
        *pos = '/';
        pos++;
    }

    // Creare la directory finale
    if (_mkdir(temp) && errno != EEXIST) {
        perror("Errore nella creazione della cartella");
        return -1;
    }

    return 0;
}

const int TOP    = 1;
const int BOTTOM = DIM;
const int LEFT   = 1;
const int RIGHT  = DIM;

/*
    LEFT          RIGHT                 
     |              |
     v              V
  +-+----------------+-+
  |\|\\\\\\\\\\\\\\\\|\|
  +-+----------------+-+              
  |\|                |\| <- TOP
  |\|                |\|
  |\|                |\|
  |\|                |\|
  |\|                |\|
  |\|                |\| <- BOTTOM
  +-+----------------+-+
  |\|\\\\\\\\\\\\\\\\|\|
  +-+----------------+-+

           +-------+
           |       |
           |   1   |
           |       |
   +-------+-------+-------+-------+
   |       |       |       |       |
   |   2   |   0   |   4   |   5   |     indicizzazione facce
   |       |       |       |       |
   +-------+-------+-------+-------+
           |       |
           |   3   |
           |       |
           +-------+

        +-------+-------+
        |       |       |
        |  00   |  01   |
        |       |       |
        +-------+-------+   indicizzazione cubi (si suppone per frontale la faccia 0)
        |       |       |
        |  10   |  11   |
        |       |       |
        +-------+-------+
 */

/*funzione che riempie gli spigoli della halo con le righe 
comunicanti della faccia adiacente*/
void copy_sides(Cube *cube){
    int i, j;
    const int HALO_TOP    = TOP-1;
    const int HALO_BOTTOM = BOTTOM+1;
    const int HALO_LEFT   = LEFT-1;
    const int HALO_RIGHT  = RIGHT+1;

    #pragma omp parallel for
    for(j=LEFT; j<RIGHT+1; j++){
        //faccia 0
        cube->faces[0].values[HALO_BOTTOM][j] = cube->faces[3].values[TOP][j];
        cube->faces[0].values[HALO_TOP][j] = cube->faces[1].values[BOTTOM][j];
        //faccia 1
        cube->faces[1].values[HALO_BOTTOM][j] = cube->faces[0].values[TOP][j];
        cube->faces[1].values[HALO_TOP][j] = cube->faces[5].values[TOP][j];
        //faccia 2
        cube->faces[2].values[HALO_BOTTOM][j] = cube->faces[3].values[j][LEFT];
        cube->faces[2].values[HALO_TOP][j] = cube->faces[1].values[j][LEFT];
        //faccia 3
        cube->faces[3].values[HALO_BOTTOM][j] = cube->faces[5].values[j][BOTTOM];
        cube->faces[3].values[HALO_TOP][j] = cube->faces[0].values[j][BOTTOM];
        //faccia 4
        cube->faces[4].values[HALO_BOTTOM][j] = cube->faces[3].values[j][RIGHT];
        cube->faces[4].values[HALO_TOP][j] = cube->faces[1].values[j][RIGHT];
        //faccia 5
        cube->faces[5].values[HALO_BOTTOM][j] = cube->faces[3].values[BOTTOM][j];
        cube->faces[5].values[HALO_TOP][j] = cube->faces[1].values[TOP][j];
    }

    #pragma omp parallel for
    for(i=TOP; i<BOTTOM+1; i++){
        //faccia 0
        cube->faces[0].values[i][HALO_RIGHT] = cube->faces[4].values[i][LEFT];
        cube->faces[0].values[i][HALO_LEFT] = cube->faces[2].values[i][RIGHT];
        //faccia 1
        cube->faces[1].values[i][HALO_RIGHT] = cube->faces[4].values[TOP][i];
        cube->faces[1].values[i][HALO_LEFT] = cube->faces[2].values[TOP][i];
        //faccia 2
        cube->faces[2].values[i][HALO_RIGHT] = cube->faces[0].values[i][LEFT];
        cube->faces[2].values[i][HALO_LEFT] = cube->faces[5].values[i][RIGHT];
        //faccia 3
        cube->faces[3].values[i][HALO_RIGHT] = cube->faces[4].values[BOTTOM][i];
        cube->faces[3].values[i][HALO_LEFT] = cube->faces[2].values[BOTTOM][i];
        //faccia 4
        cube->faces[4].values[i][HALO_RIGHT] = cube->faces[5].values[LEFT][i];
        cube->faces[4].values[i][HALO_LEFT] = cube->faces[0].values[i][RIGHT];
        //faccia 5
        cube->faces[5].values[i][HALO_RIGHT] = cube->faces[5].values[LEFT][i];
        cube->faces[5].values[i][HALO_LEFT] = cube->faces[0].values[i][RIGHT];
    }
}

//fa l'operazione di copia delle halo per una griglia di cubi 2x2
void copy_sides_super(SuperCube *supercube){
    int i,j,f;
    const int HALO_TOP    = TOP-1;
    const int HALO_BOTTOM = BOTTOM+1;
    const int HALO_LEFT   = LEFT-1;
    const int HALO_RIGHT  = RIGHT+1;

    //spigoli della halo per ogni facia di ogni cubo
    for(i=0; i<CUBES_PER_ROW; i++){
        for(j=0; j<CUBES_PER_COL; j++){
            copy_sides(&supercube->cubes[i][j]);
        }
    }
    
    //vertici delle facce
    #pragma omp parallel for
    for(f=0; f<SIZE; f++){
        //diagonale
        supercube->cubes[0][0].faces[f].values[HALO_TOP][HALO_LEFT] = supercube->cubes[1][1].faces[f].values[BOTTOM][RIGHT];
        supercube->cubes[1][1].faces[f].values[HALO_BOTTOM][HALO_RIGHT] = supercube->cubes[0][0].faces[f].values[TOP][LEFT];
        supercube->cubes[0][0].faces[f].values[HALO_BOTTOM][HALO_RIGHT] = supercube->cubes[1][1].faces[f].values[TOP][LEFT];
        supercube->cubes[1][1].faces[f].values[HALO_TOP][HALO_LEFT] = supercube->cubes[0][0].faces[f].values[BOTTOM][RIGHT];
        //antidiagonale
        supercube->cubes[1][0].faces[f].values[HALO_BOTTOM][HALO_LEFT] = supercube->cubes[0][1].faces[f].values[TOP][RIGHT];
        supercube->cubes[0][1].faces[f].values[HALO_TOP][HALO_RIGHT] = supercube->cubes[1][0].faces[f].values[BOTTOM][LEFT];
        supercube->cubes[1][0].faces[f].values[HALO_TOP][HALO_RIGHT] = supercube->cubes[0][1].faces[f].values[BOTTOM][LEFT];
        supercube->cubes[0][1].faces[f].values[HALO_BOTTOM][HALO_LEFT] = supercube->cubes[1][0].faces[f].values[TOP][RIGHT];
        //resto
        supercube->cubes[0][0].faces[f].values[HALO_BOTTOM][HALO_LEFT] = supercube->cubes[1][0].faces[f].values[TOP][LEFT];
        supercube->cubes[1][0].faces[f].values[HALO_TOP][HALO_LEFT] = supercube->cubes[0][0].faces[f].values[BOTTOM][LEFT];
        supercube->cubes[0][0].faces[f].values[HALO_TOP][HALO_RIGHT] = supercube->cubes[0][1].faces[f].values[TOP][LEFT];
        supercube->cubes[0][1].faces[f].values[HALO_TOP][HALO_LEFT] = supercube->cubes[0][0].faces[f].values[TOP][RIGHT];
        supercube->cubes[0][1].faces[f].values[HALO_BOTTOM][HALO_RIGHT] = supercube->cubes[1][1].faces[f].values[TOP][RIGHT];
        supercube->cubes[1][1].faces[f].values[HALO_TOP][HALO_RIGHT] = supercube->cubes[0][1].faces[f].values[BOTTOM][RIGHT];
        supercube->cubes[1][1].faces[f].values[HALO_BOTTOM][HALO_LEFT] = supercube->cubes[1][0].faces[f].values[BOTTOM][RIGHT];
        supercube->cubes[1][0].faces[f].values[HALO_BOTTOM][HALO_RIGHT] = supercube->cubes[1][0].faces[f].values[BOTTOM][LEFT];
        }
}

//funzione per scrivere su file lo stato corrente di una faccia di un cubo
void write_pbm(Face *grid, const char* fname )
{
    int x, y;
    FILE *f = fopen(fname, "w");
    if (!f) {
        printf("Cannot open %s for writing\n", fname);
        abort();
    }
    fprintf(f, "P1\n");
    fprintf(f, "# produced by life_on_a_cube_omp.c\n");
    fprintf(f, "%d %d\n", DIM, DIM);
    for (x=TOP; x<BOTTOM+1; x++) {
        for (y=LEFT; y<RIGHT+1; y++) {
            fprintf(f, "%d ", grid->values[x][y]);
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

//funzione che computa lo stato successivo del sistema
void step(SuperCube *cur, SuperCube *next, int i, int j){
    int f, x, y;
    int newI;
    int newJ;
                        
    Face *tmp = malloc(sizeof(Face));

    for(f=0; f<SIZE; f++){
        #pragma omp parallel for collapse(2) private(x, y, newI, newJ)
        for(x=TOP; x<BOTTOM+1; x++){
            for(y=LEFT; y<RIGHT+1; y++){
                //conta vicini vivi
                int nbors =\
                cur->cubes[i][j].faces[f].values[x-1][y-1] + cur->cubes[i][j].faces[f].values[x-1][y] + cur->cubes[i][j].faces[f].values[x-1][y+1] +\
                cur->cubes[i][j].faces[f].values[x][y-1] +                                              cur->cubes[i][j].faces[f].values[x][y+1] +\
                cur->cubes[i][j].faces[f].values[x+1][y-1] + cur->cubes[i][j].faces[f].values[x+1][y] + cur->cubes[i][j].faces[f].values[x+1][y+1];

                //condizione di sovrappopolamento e salto su altro cubo (in diagonale)
                if(cur->cubes[i][j].faces[f].values[x][y] && nbors == 4){
                    newI = (i+1) % 2;
                    newJ = (j+1) % 2;
                    if(!cur->cubes[newI][newJ].faces[f].values[x][y]){
                        #pragma omp critical  //critical necessario per non creare race conditions
                        {
                        tmp->values[x][y] = cur->cubes[i][j].faces[f].values[x][y];
                        cur->cubes[i][j].faces[f].values[x][y] = cur->cubes[newI][newJ].faces[f].values[x][y];
                        cur->cubes[newI][newJ].faces[f].values[x][y] = tmp->values[x][y];
                        }
                    } else{
                        cur->cubes[i][j].faces[f].values[x][y] = 1;
                    }
                }
                //applico le regole del gol
                if(cur->cubes[i][j].faces[f].values[x][y] && (nbors<2 || nbors>3)){
                    next->cubes[i][j].faces[f].values[x][y] = 0;
                } else{
                    if(!cur->cubes[i][j].faces[f].values[x][y] && (nbors == 3)) {
                        next->cubes[i][j].faces[f].values[x][y] = 1;
                    /*} else if(cur->cubes[i][j].faces[f].values[x][y] && nbors == 4){  //condizione di sovrappopolamento e salto su altro cubo (in diagonale)
                        newI = (i+1) % 2;
                        newJ = (j+1) % 2;
                        if(!cur->cubes[newI][newJ].faces[f].values[x][y]){
                            #pragma omp critical  //critical necessario per non creare race conditions
                            {
                            tmp->values[x][y] = cur->cubes[i][j].faces[f].values[x][y];
                            cur->cubes[i][j].faces[f].values[x][y] = cur->cubes[newI][newJ].faces[f].values[x][y];
                            cur->cubes[newI][newJ].faces[f].values[x][y] = tmp->values[x][y];
                            }
                        } else{
                            cur->cubes[i][j].faces[f].values[x][y] = 0;
                        }*/
                    } else{
                        next->cubes[i][j].faces[f].values[x][y] = cur->cubes[i][j].faces[f].values[x][y];
                    }
                } 
            }
        }
    }
}

//funzione che inizializza il supercubo
void initSuperCube(SuperCube *worlds, double p){
    int i,j,f,x,y;
    #pragma omp parallel for collapse(5)
    for(i=0; i<CUBES_PER_ROW; i++){
        for(j=0; j<CUBES_PER_COL; j++){
            for(f=0; f<SIZE; f++){
                for(x=TOP; x<BOTTOM+1; x++){
                    for(y=LEFT; y<RIGHT+1; y++){
                        worlds->cubes[i][j].faces[f].values[x][y] = (((float)rand()/RAND_MAX)<p);
                    }
                }
            }
        }
    }
}

int main( int argc, char* argv[] ){
    int s, nsteps;
    SuperCube *cur = (SuperCube*)malloc(sizeof(SuperCube));
    SuperCube *next = (SuperCube*)malloc(sizeof(SuperCube));
    
    #define STRBUFSZ 128
    #define FOLDER_PATH_SIZE 512
    char fullFilePath[MAX_PATH_LENGTH];
    char fname[STRBUFSZ];
    char folderPath[FOLDER_PATH_SIZE];

    int nthreads = 4; // Specifica il numero di thread
    omp_set_num_threads(nthreads);

    srand(omp_get_wtime()); /* init RNG */

    if ( argc > 2 ) {
        printf("Usage: %s [nsteps]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if ( argc == 2 ) {
        nsteps = atoi(argv[1]);
    } else {
        nsteps = 256;
    }

    double t_start = omp_get_wtime();
    initSuperCube(cur, 0.3);
    //Iterazione per ogni stato temporale
    for (s=0; s<nsteps; s++) {
        SuperCube *tmp; //Supercubo temoraneo per lo scambio puntatori
        copy_sides_super(cur);
        for(int i=0; i<CUBES_PER_ROW; i++){
            for(int j=0; j<CUBES_PER_COL;j++){
                snprintf(folderPath, FOLDER_PATH_SIZE, "step%d/cube[%d][%d]", s, i, j); //Definizione percorso file
                if (mkdir_recursive(folderPath) == -1) { //Creazione cartelle con percorso definito
                    return EXIT_FAILURE;
                }
                #pragma omp parallel for
                for(int f=0; f<SIZE; f++){
                    snprintf(fname, STRBUFSZ, "face_%d.pbm", f);
                    snprintf(fullFilePath, MAX_PATH_LENGTH, "%s/%s", folderPath, fname);
                    write_pbm(&cur->cubes[i][j].faces[f],fullFilePath); //Scrittura file nel percorso specificato
                }
                step(cur, next, i, j); //Va allo step successivo
            }  
        }
        //scambio cur e next
        tmp = cur;
        cur = next;
        next = tmp;
    }
    double t_end = omp_get_wtime();
    printf("Elapsed time: %fs",(t_end-t_start));

    free(cur);
    free(next);

    return EXIT_SUCCESS;
}