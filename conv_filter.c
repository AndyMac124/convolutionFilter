#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "mpi.h"
#include "matrix.h"

#define MainProcess 0

int get_matrix_size(char* file_name)
{
        int fd;
        if ((fd = open(file_name, O_RDONLY)) == -1) {
                fprintf(stderr, "Failed to open input file\n");
                exit(1);
        }
        // Calculate the file size
        off_t file_size = lseek(fd, 0, SEEK_END);
        if (file_size == -1) {
                perror("lseek failed");
                return -1;
        }

        int num_elements = file_size / sizeof(int);
        int matrix_size = (int)sqrt(num_elements);

        if (close(fd) == -1) {
                perror("Failed to close file");
                exit(EXIT_FAILURE);
        }

        return matrix_size;
}

int rounded_weighted_average(int **local_matrix, int x, int y, int n, int size, int me)
{
        double sub_total = 0;
        double num_neighbours = 0;

        for (int i = x + -n; i <= x + n; i++) {
                for (int j = y + -n; j <= y + n; j++) {
                        double ay = abs(i - x);
                        double ax = abs(j - y);
                        double c_depth = (ax > ay) ? ax : ay;
                        double m_n = n / c_depth;
                        if ((i == x && j == y) || i > size-1 || j > size-1 || i < 0 || j < 0) {
                                continue;
                        }
                        sub_total += m_n * local_matrix[i][j];
                        num_neighbours++;
                }
        }

        if (num_neighbours == 0) {
                fprintf(stderr, "Error: Division by zero in rounded_weighted_average.\n");
                exit(EXIT_FAILURE);
        }
        int total = sub_total / num_neighbours;
        return total;
}

int parse_args(int argc, char *argv[], int *fd_in, int *fd_out, int *max_depth) {

        if (argc != 4) {
                fprintf(stderr, "Usage: %s matrix_file dimension\n", argv[0]);
                return -1;
        }

        *fd_in = open (argv[1], O_RDONLY);
        *fd_out = open (argv[2], O_WRONLY | O_CREAT, 0666);

        if (*fd_in == -1) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                return -1;
        }

        if (*fd_out == -1) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                close(*fd_in);
                return -1;
        }

        // TODO Can depth be 0?
        *max_depth = atoi(argv[3]);
        if (*max_depth <= 0) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                close(*fd_in);
                close(*fd_out);
                return -1;
        }
        return 0;
}

// TODO This is from lectures
void get_matrix(int fd, int matrix_size, int **matrix){
        int row, col, slot;

        for(row = 1; row <= matrix_size; row++)
                for(col = 1; col <= matrix_size; col++){
                        if(get_slot(fd, matrix_size, row, col, &slot) == -1){
                                fprintf(stderr,"get_slot failed at [%d][%d]\n", row, col);
                                exit(-1);
                        }
                        matrix[row - 1][col - 1] = slot;
                }
}

void print2DArray(int **array, int rows, int columns) {
        for (int i = 0; i < rows; i++) {
                for (int j = 0; j < columns; j++) {
                        printf("%d ", array[i][j]);
                }
                printf("\n");
        }
        printf("\n\n");
}

int main(int argc, char *argv[]) {
        int fd_in, fd_out, matrix_size, max_depth, me, nprocs;
        int rows_each, M_rows;

        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &me);
        MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

        int **matrix = NULL;

        if (me == MainProcess) {
                if (parse_args(argc, argv, &fd_in, &fd_out, &max_depth) < 0) {
                        MPI_Finalize();
                        exit(EXIT_FAILURE);
                }

                matrix_size = get_matrix_size(argv[1]);

                // TODO Add a check some for whether matrix_size is less than nprocs
                rows_each = matrix_size / nprocs;
                M_rows = rows_each + (matrix_size % nprocs);

                if (nprocs > matrix_size) {
                        rows_each = 1;
                        M_rows = 1;
                }
        }


        if (me == MainProcess) {
                matrix = (int **) malloc(matrix_size * sizeof(int *));
                for (int i = 0; i < matrix_size; i++) {
                        matrix[i] = (int *) malloc(matrix_size * sizeof(int));
                }
                get_matrix(fd_in, matrix_size, matrix);
                print2DArray(matrix, matrix_size, matrix_size);
        }

        // COULD CREATE SUPERSIZED MATRIX HERE

        //USE MPI STRUCTS?
        MPI_Bcast(&max_depth, 1, MPI_INT, MainProcess, MPI_COMM_WORLD);
        MPI_Bcast(&matrix_size, 1, MPI_INT, MainProcess,
                  MPI_COMM_WORLD);
        MPI_Bcast(&M_rows, 1, MPI_INT, MainProcess, MPI_COMM_WORLD);
        MPI_Bcast(&rows_each, 1, MPI_INT, MainProcess, MPI_COMM_WORLD);

        int **local_matrix;
        int **sub_result;

        // Giving enough room for all
        if (me == MainProcess) {
                local_matrix = (int **)malloc((M_rows + max_depth) * sizeof(int *));
                for (int i = 0; i < (M_rows + max_depth); i++) {
                        local_matrix[i] = (int *)malloc(matrix_size * sizeof(int));
                }
        } else {
                local_matrix = (int **)malloc((rows_each + (2 * max_depth)) * sizeof(int *));
                for (int i = 0; i < (rows_each + (2 * max_depth)); i++) {
                        local_matrix[i] = (int *)malloc(matrix_size * sizeof(int));
                }
                sub_result = (int **)malloc((rows_each) * sizeof(int *));
                for (int i = 0; i < rows_each; i++) {
                        sub_result[i] = (int *)malloc(matrix_size * sizeof(int));
                }
        }

        if (me == MainProcess) {
                for (int i = 0; i < M_rows + max_depth; i++) {
                        if (i >= matrix_size) {
                                break;
                        }
                        for (int j = 0; j < matrix_size; j++) {
                                local_matrix[i][j] = matrix[i][j];
                        }
                }
                for (int i = 1; i < nprocs; i++) {
                        if ((M_rows + ((i - 1) * rows_each)) > matrix_size) {
                                continue;
                        }
                        int first_row = (M_rows + ((i - 1) * rows_each)) - max_depth;
                        int last_row = (M_rows + ((i - 1) * rows_each) + (rows_each - 1) + max_depth);
                        if (last_row > matrix_size - 1) {
                                last_row = matrix_size - 1;
                        }
                        if (first_row < 0) {
                                first_row = 0;
                        }
                        for (int j = first_row; j <= last_row; j++) {
                                MPI_Send(matrix[j], matrix_size, MPI_INT, i, 0, MPI_COMM_WORLD);
                        }
                }
        }

        if (me != MainProcess && ((M_rows + ((me - 1) * rows_each)) < matrix_size)) {
                int first_row = (M_rows + ((me - 1) * rows_each)) - max_depth;
                int last_row = (M_rows + ((me - 1) * rows_each) + (rows_each - 1) + max_depth);
                if (last_row > matrix_size - 1) {
                        last_row = matrix_size - 1;
                }
                if (first_row < 0) {
                        first_row = 0;
                }
                int recv_rows = last_row - first_row + 1;
                for (int k = 0; k < recv_rows; k++) {
                        MPI_Recv(local_matrix[k], matrix_size, MPI_INT, MainProcess, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
        }

        if (me == MainProcess) {
                int row = 0;
                for (int i = 0; i < M_rows; i++) {
                        for (int j = 0; j < matrix_size; j++) {
                                matrix[row][j] = rounded_weighted_average(local_matrix, row, j, max_depth, matrix_size, me);
                        }
                        row++;
                }
        } else if ((M_rows + ((me - 1) * rows_each)) < matrix_size){
                int row = 0;
                int start = (M_rows + ((me - 1) * rows_each));
                if (start > max_depth) {
                        start = max_depth;
                }
                for (int i = start; i < start + rows_each; i++) {
                        for (int j = 0; j < matrix_size; j++) {
                                sub_result[row][j] = rounded_weighted_average(local_matrix, i, j, max_depth, matrix_size, me);
                        }
                        row++;
                }
        }

        if (!(me == MainProcess) && ((M_rows + ((me - 1) * rows_each)) < matrix_size)) {
                for (int i = 0; i < rows_each; i++) {
                        MPI_Send(sub_result[i], matrix_size, MPI_INT, MainProcess, 0, MPI_COMM_WORLD);
                }
        }

        MPI_Barrier(MPI_COMM_WORLD);

        if (me == MainProcess) {
                int row = M_rows;
                for (int i = 1; i < nprocs; i++) {
                        if ((M_rows + ((i - 1) * rows_each)) < matrix_size) {
                                for (int j = 0; j < rows_each; j++) {
                                        MPI_Recv(matrix[row], matrix_size,
                                                 MPI_INT, i, 0, MPI_COMM_WORLD,
                                                 MPI_STATUS_IGNORE);
                                        row++;
                                }
                        }
                }
        }

        //fprintf(stderr, "Process %d FINISHED GATHER\n", me);

        if (me == MainProcess) {
                for (int i = 0; i < matrix_size; i++) {
                        if (set_row(fd_out, matrix_size, i + 1, matrix[i]) < 0) {
                                fprintf(stderr, "Failed to write row %d to file\n", i + 1);
                                close(fd_out);
                                exit(EXIT_FAILURE);
                        }
                }
                print2DArray(matrix, matrix_size, matrix_size);
        }

        if (me == MainProcess) {
                for (int i = 0; i < M_rows + max_depth; i++) {
                        free(local_matrix[i]);
                }
                free(local_matrix);
        } else {
                for (int i = 0; i < rows_each; i++) {
                        free(sub_result[i]);
                }
                free(sub_result);
                for (int i = 0; i < rows_each + (2 * max_depth); i++) {
                        free(local_matrix[i]);
                }
                free(local_matrix);
        }

        close(fd_in);
        close(fd_out);

        MPI_Finalize ();
        exit (EXIT_SUCCESS);
}
