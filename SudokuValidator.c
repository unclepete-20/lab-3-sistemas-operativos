/* 
  Nombre: Pedro Pablo Arriola Jimenez (20188)
  Fecha: 29 de marzo de 2023
  Versión: 1.0
  Descripción: Programa que soluciona Sudoku utilizando pthreading
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdbool.h>
#include <omp.h>

#define NUM_THREADS 9
#define NUM_ROWS 9
#define NUM_COLS 9

int sudoku[NUM_ROWS][NUM_COLS];
int valid;

int check_row(int fila) {
    // Validamos cada fila
    int numeros[10] = {0}; // Inicializamos un arreglo para contar cuántas veces aparece cada número
    for (int j = 0; j < 9; j++) {
        int numero = sudoku[fila][j];
        if (numero == 0) {
            valid = 0; // Si alguna celda está vacía, la fila no es válida
        }
        numeros[numero]++;
        if (numeros[numero] > 1) {
            valid = 0; // Si algún número aparece más de una vez en la fila, la fila no es válida
        }
    }
    valid = 1;
    // printf("se termino de revisar la fila\n");
}

// función para revisar si los números de una columna están completos
void *check_cols(void *params) {
    int tid = *(int*) params;  // Obtener el TID del hilo
    int numeros[NUM_THREADS] = {0};
    int i, j;
    //printf("Hilo %d ejecutando revisión de columna %d\n", syscall(SYS_gettid), tid);
    //#pragma omp parallel for
    for (i = 0; i < NUM_THREADS; i++) {
        int numero = sudoku[i][tid] - '0';
        if (numeros[numero - 1] == 1) {
            printf("Error en la columna %d\n", tid + 1);
            valid = 0;
        }
        numeros[numero - 1] = 1;
    }
    // printf("valid %d\n",valid);
    //printf("Thread %d ha terminado de revisar las columnas.\n", tid + 1);
    printf("Thread numero %d tiene el PID %d\n", tid+1,syscall(SYS_gettid));
    pthread_exit(0);
}

int main(int argc, char *argv[]) {

    omp_set_num_threads(1);
    omp_set_nested(true);
    
    if (argc != 2) {
        printf("Usage: ./SudokuValidator <filename>\n");
        return 1;
    }

    // Abrir archivo y mappear a memoria
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    off_t length = lseek(fd, 0, SEEK_END);
    char *addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Copiar datos del archivo al arreglo bidimensional
    int index = 0;
    int row, col;
    for (row = 0; row < NUM_ROWS ; row++) {
        for (col = 0; col < NUM_ROWS ; col++) {
            char c = addr[row * 9 + col];
            sudoku[row][col] = c - '0';
        }
    }

    // Imprimir tabla de sudoku
    printf("Sudoku table:\n");
    for (int row = 0; row < NUM_ROWS ; row++) {
        for (int col = 0; col < NUM_ROWS ; col++) {
            printf("%d ", sudoku[row][col]);
        }
        printf("\n");
    }
    pid_t pid;
    int parent_pid;

    // Inicializar el flag de solución válida
    valid = 1;
    
    printf("El trhread que se ejecuta el metodo para ejecutar el metodo de revision de columna es: %d\n",getpid());

    parent_pid = getpid();  // obtiene el número de proceso del padre
    // Crear un proceso hijo
    pid = fork();

    if (pid == 0) {
        // Proceso hijo
        char pid_str[10];
        sprintf(pid_str, "%d", parent_pid);
        execlp("ps", "ps", "-p", pid_str, "-lLf", (char *)NULL);
        exit(0);
    } else {
        // Proceso padre
        int i, j;
        
        // Validación de columnas en un hilo separado
        int ids_columnas[9];
        pthread_t thread_columnas[9];
        #pragma omp parallel for private(i) schedule(dynamic)
        for (i = 0; i < 9; i++) {
            ids_columnas[i] = i;
            pthread_create(&thread_columnas[i], NULL, check_cols, &ids_columnas[i]);
        }
        
        // Esperar a que se completen las validaciones de columnas
        #pragma omp parallel for private(i) schedule(dynamic)
        for (i = 0; i < 9; i++) {
            pthread_join(thread_columnas[i], NULL);
            
        }
        printf("El thread que ejecuta el main es: %d\n", getpid());
        // Espere a que concluya el hijo que está ejecutando ps
        wait(NULL);
        
        
        // revisar filas
        for (int i = 0; i < 9; i++) {
            if (!check_row(i)) {
                valid = 0; // Si alguna fila no es válida, el sudoku no es válido
            }
        }
  
        // Revisar si el sudoku es válido
        if (valid) {
            printf("El sudoku es válido.\n");
        } else {
            printf("El sudoku no es válido.\n");
        }
        printf("Antes de terminar el estado de este proceso y sus threads es: \n");

        // Crear un proceso hijo
        pid = fork();
        //parent_pid = getpid();

        if (pid == 0) {
            // Proceso hijo
            char pid_str[10];
            sprintf(pid_str, "%d", parent_pid);
            execlp("ps", "ps", "-p", pid_str, "-lLf", NULL);
            exit(0);
        } else {
            // Proceso padre
            // Esperar a que termine el hijo
            wait(NULL);
            
            // Liberamos la memoria mapeada y cerramos el archivo
            munmap(addr, length);
            close(fd);
           return 0;
        }
    }
}
