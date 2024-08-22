#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "mpi.h"
#include "matrix.h"
#include <stdbool.h>

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

int parse_args(int argc, char *argv[], int *fd_in, int *fd_out, int *max_depth)
{

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
void get_matrix(int fd, int matrix_size, int **matrix)
{
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

void print2DArray(int **array, int rows, int columns)
{
        for (int i = 0; i < rows; i++) {
                for (int j = 0; j < columns; j++) {
                        printf("%d ", array[i][j]);
                }
                printf("\n");
        }
        printf("\n\n");
}

void divide_rows(int* rows_each, int* M_rows, int matrix_size, int nprocs)
{
        *rows_each = matrix_size / nprocs;
        *M_rows = *rows_each + (matrix_size % nprocs);
        if (nprocs > matrix_size) {
                *rows_each = 1;
                *M_rows = 1;
        }
}

void build_matrix(int ***matrix, int rows, int col)
{
        *matrix = (int **) malloc(rows * sizeof(int *));
        for (int i = 0; i < rows; i++) {
                (*matrix)[i] = (int *) malloc(col * sizeof(int));
        }
}

void write_to_file(int rows, int **matrix, int fd_out)
{
        for (int i = 0; i < rows; i++) {
                if (set_row(fd_out, rows, i + 1, matrix[i]) < 0) {
                        fprintf(stderr, "Failed to write row %d to file\n", i + 1);
                        close(fd_out);
                        exit(EXIT_FAILURE);
                }
        }
}


void receive_results(int row, int nprocs, int rows_each, int matrix_size, int **matrix)
{
        for (int i = 1; i < nprocs; i++) {
                if ((row + ((i - 1) * rows_each)) < matrix_size) {
                        for (int j = 0; j < rows_each; j++) {
                                MPI_Recv(matrix[row], matrix_size,
                                         MPI_INT, i, 0, MPI_COMM_WORLD,
                                         MPI_STATUS_IGNORE);
                                row++;
                        }
                }
        }
}

void free_int_array_memory(int **array, int rows)
{
        for (int i = 0; i < rows; i++) {
                free(array[i]);
        }
        free(array);
}


void calculate_values(int start, int end, int size, int **arr_out, int **arr_in, int depth, int me)
{
        int row = 0;
        for (int i = start; i <= end; i++) {
                for (int j = 0; j < size; j++) {
                        arr_out[row][j] = rounded_weighted_average(arr_in, i, j, depth, size, me);
                }
                row++;
        }
}

void receive_rows(int size, int **arr_in, int first_calc_row, int last_calc_row, int depth)
{
        int first_row = first_calc_row - depth;
        int last_row = last_calc_row + depth;
        if (last_row > size - 1) {
                last_row = size - 1;
        }
        if (first_row < 0) {
                first_row = 0;
        }
        int recv_rows = last_row - first_row + 1;
        for (int k = 0; k < recv_rows; k++) {
                MPI_Recv(arr_in[k], size, MPI_INT, MainProcess, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
}

int calc_receiving_rows(int m_rows, int me, int rows_each, int depth, int size)
{
        int first_row = m_rows + ((me - 1) * rows_each) - depth;

        int last_row = (m_rows + ((me - 1) * rows_each) + (rows_each - 1) + depth);
        if (last_row > size - 1) {
                last_row = size - 1;
        }
        if (first_row < 0) {
                first_row = 0;
        }
        return last_row - first_row + 1;
}

void rec_rows(int rows, int **arr, int size)
{
        for (int k = 0; k < rows; k++) {
                MPI_Recv(arr[k], size, MPI_INT, MainProcess, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
}

int calc_last_row(int first_row, int row_each, int final_row)
{
        int last_row = first_row + row_each - 1;
        if (last_row > final_row) {
                last_row = final_row;
        }
        return last_row;
}


int calc_first_row(int M_rows, int me, int rows_each, int depth)
{
        int first_row = M_rows + ((me - 1) * rows_each);
        if (first_row > depth) {
                first_row = depth;
        }
        if (first_row < 0) {
                first_row = 0;
        }
        return first_row;
}


void copy_to_matrix(int row, int final_row, int size, int **arr_in, int **arr_out)
{
        if (row > final_row) {
                return;
        }
        for (int col = 0; col < size; col++) {
                arr_in[row][col] = arr_out[row][col];
        }
}


void send_rows(int m_rows, int rows_each, int depth, int final_row, int size, int **arr, int i)
{
        int first_row = (m_rows + ((i - 1) * rows_each)) - depth;
        int last_row = (m_rows + ((i - 1) * rows_each) + (rows_each - 1) + depth);
        if (last_row > final_row) {
                last_row = final_row;
        }
        if (first_row < 0) {
                first_row = 0;
        }
        for (int j = first_row; j <= last_row; j++) {
                MPI_Send(arr[j], size, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
}


typedef struct {
    int max_depth;
    int matrix_size;
    int M_rows;
    int rows_each;
    int final_row;
} REQ_DATA;

typedef struct {
    int first_calc_row;
    int last_calc_row;
    int me;
} PROCESS_DATA;


int main(int argc, char *argv[]) {
        int fd_in, fd_out, matrix_size, max_depth, me, nprocs, rows_each, M_rows;
        REQ_DATA rd;

        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &me);
        MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

        int **matrix = NULL;
        bool is_main = (me == MainProcess);
        PROCESS_DATA p_data;

        if (is_main) {
                if (parse_args(argc, argv, &fd_in, &fd_out, &max_depth) < 0) {
                        MPI_Finalize();
                        exit(EXIT_FAILURE);
                }
                matrix_size = get_matrix_size(argv[1]);
                divide_rows(&rows_each, &M_rows, matrix_size, nprocs);
                build_matrix(&matrix, matrix_size, matrix_size);
                get_matrix(fd_in, matrix_size, matrix);

                // TODO This will be removed
                print2DArray(matrix, matrix_size, matrix_size);

                rd.max_depth = max_depth;
                rd.matrix_size = matrix_size;
                rd.M_rows = M_rows;
                rd.rows_each = rows_each;
                rd.final_row = rd.matrix_size - 1;
        }

        MPI_Bcast(&rd, sizeof(REQ_DATA), MPI_BYTE, MainProcess, MPI_COMM_WORLD);

        bool has_jobs = ((rd.M_rows + ((me - 1) * rd.rows_each)) < rd.matrix_size);

        // TODO Form a struct to pass around?
        int final_row = rd.final_row;
        int depth = rd.max_depth;
        int size = rd.matrix_size;

        int first_calc_row, last_calc_row;
        int **local_matrix;
        int **sub_result;

        // SETUP
        if (is_main) {
                build_matrix(&local_matrix, (rd.M_rows + rd.max_depth), matrix_size);
        } else if (!is_main && has_jobs) {
                build_matrix(&local_matrix, rd.rows_each + (2 * rd.max_depth), rd.matrix_size);
                build_matrix(&sub_result, rd.rows_each, rd.matrix_size);
                first_calc_row = calc_first_row(rd.M_rows, me, rd.rows_each, depth);
                last_calc_row = calc_last_row(first_calc_row, rd.rows_each, final_row);
        }

        // DO WORK
        if (is_main) {
                for (int r = 0; r < rd.M_rows + rd.max_depth; r++) {
                        copy_to_matrix(r, final_row, size, local_matrix, matrix);
                }
                for (int i = 1; i < nprocs; i++) {
                        if (!has_jobs) {
                                continue;
                        }
                        send_rows(rd.M_rows, rd.rows_each, depth, final_row, size, matrix, i);
                }
                calculate_values(0, rd.M_rows - 1, size, matrix, local_matrix, depth, me);
                receive_results(rd.M_rows, nprocs, rows_each, matrix_size, matrix);
                // SETTING OUTPUT
                write_to_file(matrix_size, matrix, fd_out);
                // TODO Remove this last
                print2DArray(matrix, rd.matrix_size, rd.matrix_size);
                free_int_array_memory(local_matrix, rd.M_rows + rd.max_depth);
        } else if (has_jobs) {
                int recv_rows = calc_receiving_rows(rd.M_rows, me, rd.rows_each, depth, size);
                rec_rows(recv_rows, local_matrix, size);
                calculate_values(first_calc_row, last_calc_row, size, sub_result, local_matrix, depth, me);
                for (int i = 0; i < rd.rows_each; i++) {
                        MPI_Send(sub_result[i], rd.matrix_size, MPI_INT, MainProcess, 0, MPI_COMM_WORLD);
                }
                free_int_array_memory(sub_result, rows_each);
                free_int_array_memory(local_matrix, rd.rows_each + (2 * rd.max_depth));
        }

        close(fd_in);
        close(fd_out);

        MPI_Finalize ();
        exit (EXIT_SUCCESS);
}
