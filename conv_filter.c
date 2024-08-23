#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "mpi.h"
#include "matrix.h"
#include <stdbool.h>

#define MainProc 0

typedef struct {
    int max_depth;
    int matrix_size;
    int m_rows;
    int rows_each;
    int final_row;
} SHARED_DATA;

typedef struct {
    int first_calc_row;
    int last_calc_row;
    int me;
} PROCESS_DATA;

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

int rounded_weighted_average(int **local_matrix, int x, int y, int n, int size)
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
void get_matrix_from_file(int fd, int matrix_size, int **matrix)
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

void build_empty_matrix(int ***matrix, int rows, int col)
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

bool process_has_jobs(SHARED_DATA *shared, int me)
{
        return ((shared->m_rows + ((me - 1) * shared->rows_each)) < shared->matrix_size);
}


void receive_results(SHARED_DATA *data, int **matrix, int nprocs)
{
        int row = data->m_rows;
        for (int i = 1; i < nprocs; i++) {
                if (process_has_jobs(data, i)) {
                        for (int j = 0; j < data->rows_each; j++) {
                                MPI_Recv(matrix[row], data->matrix_size,
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


void calculate_values(PROCESS_DATA *p_data, SHARED_DATA *shared, int **arr_out, int **arr_in)
{
        int row = 0;
        for (int i = p_data->first_calc_row; i <= p_data->last_calc_row; i++) {
                for (int j = 0; j < shared->matrix_size; j++) {
                        arr_out[row][j] = rounded_weighted_average(arr_in, i, j, shared->max_depth, shared->matrix_size);
                }
                row++;
        }
}

void receive_rows(PROCESS_DATA *p_data, SHARED_DATA *shared, int **arr_in)
{
        int first_row = p_data->first_calc_row - shared->max_depth;
        int last_row = p_data->last_calc_row + shared->max_depth;
        if (last_row > shared->matrix_size - 1) {
                last_row = shared->matrix_size - 1;
        }
        if (first_row < 0) {
                first_row = 0;
        }
        int recv_rows = last_row - first_row + 1;
        for (int k = 0; k < recv_rows; k++) {
                MPI_Recv(arr_in[k], shared->matrix_size, MPI_INT, MainProc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
}

int calc_receiving_rows(SHARED_DATA *shared, int me)
{
        int first_row = shared->m_rows + ((me - 1) * shared->rows_each) - shared->max_depth;

        int last_row = (shared->m_rows + ((me - 1) * shared->rows_each) + (shared->rows_each - 1) + shared->max_depth);
        if (last_row > shared->matrix_size - 1) {
                last_row = shared->matrix_size - 1;
        }
        if (first_row < 0) {
                first_row = 0;
        }
        return last_row - first_row + 1;
}

void rec_rows(int rows, int **arr, int size)
{
        for (int k = 0; k < rows; k++) {
                MPI_Recv(arr[k], size, MPI_INT, MainProc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
}

int calc_last_row(SHARED_DATA *shared, int first_row)
{
        int last_row = first_row + shared->rows_each - 1;
        if (last_row > shared->final_row) {
                last_row = shared->final_row;
        }
        return last_row;
}


int calc_first_row(SHARED_DATA *shared, int me)
{
        int first_row = shared->m_rows + ((me - 1) * shared->rows_each);
        if (first_row > shared->max_depth) {
                first_row = shared->max_depth;
        }
        if (first_row < 0) {
                first_row = 0;
        }
        return first_row;
}


void copy_matrix(SHARED_DATA *shared, int **arr_in, int **arr_out)
{
        for (int row = 0; row < shared->m_rows + shared->max_depth; row++) {
                if (row > shared->final_row) {
                        return;
                }
                for (int col = 0; col < shared->matrix_size; col++) {
                        arr_in[row][col] = arr_out[row][col];
                }
        }
}


void send_rows(SHARED_DATA *shared, int **arr, int nprocs)
{
        for (int i = 1; i < nprocs; i++) {
                if (process_has_jobs(shared, i)) {
                        int first_row = (shared->m_rows + ((i - 1) * shared->rows_each)) - shared->max_depth;
                        int last_row = (shared->m_rows + ((i - 1) * shared->rows_each) + (shared->rows_each - 1) + shared->max_depth);
                        if (last_row > shared->final_row) {
                                last_row = shared->final_row;
                        }
                        if (first_row < 0) {
                                first_row = 0;
                        }
                        for (int j = first_row; j <= last_row; j++) {
                                MPI_Send(arr[j], shared->matrix_size, MPI_INT, i, 0, MPI_COMM_WORLD);
                        }
                }
        }
}


void set_process_data(PROCESS_DATA *p_data, SHARED_DATA shared, bool is_main, int me)
{
        if (is_main) {
                p_data->first_calc_row = 0;
                p_data->last_calc_row = shared.m_rows - 1;
                p_data->me = me;
        } else {
                p_data->first_calc_row = calc_first_row(&shared, me);
                p_data->last_calc_row = calc_last_row(&shared, p_data->first_calc_row);
                p_data->me = me;
        }
}

void set_required_data(SHARED_DATA *shared, int max_depth, int matrix_size, int m_rows, int rows_each)
{
        shared->max_depth = max_depth;
        shared->matrix_size = matrix_size;
        shared->m_rows = m_rows;
        shared->rows_each = rows_each;
        shared->final_row = shared->matrix_size - 1;
}

void send_results_to_main(SHARED_DATA *shared, int **arr)
{
        for (int i = 0; i < shared->rows_each; i++) {
                MPI_Send(arr[i], shared->matrix_size, MPI_INT, MainProc, 0,
                         MPI_COMM_WORLD);
        }
}

int main(int argc, char *argv[]) {
        int fd_in, fd_out, matrix_size, max_depth, me, nprocs, p_rows, m_rows;
        SHARED_DATA shared;

        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &me);
        MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

        int **matrix = NULL;
        int **local_matrix;
        int **sub_result;
        PROCESS_DATA p_data;

        bool is_main = (me == MainProc);

        if (is_main) {
                if (parse_args(argc, argv, &fd_in, &fd_out, &max_depth) < 0) {
                        MPI_Finalize();
                        exit(EXIT_FAILURE);
                }
                matrix_size = get_matrix_size(argv[1]);
                divide_rows(&p_rows, &m_rows, matrix_size, nprocs);
                build_empty_matrix(&matrix, matrix_size, matrix_size);
                get_matrix_from_file(fd_in, matrix_size, matrix);
                print2DArray(matrix, matrix_size, matrix_size); // TODO This will be removed
                set_required_data(&shared, max_depth, matrix_size, m_rows, p_rows);
        }

        MPI_Bcast(&shared, sizeof(SHARED_DATA), MPI_BYTE, MainProc, MPI_COMM_WORLD);

        bool has_jobs = process_has_jobs(&shared, me);

        if (has_jobs) {
                set_process_data(&p_data, shared, is_main, me);
                build_empty_matrix(&local_matrix, shared.rows_each + (2 * shared.max_depth), shared.matrix_size);
                build_empty_matrix(&sub_result, shared.rows_each, shared.matrix_size);
        }

        // DO WORK
        if (is_main) {
                build_empty_matrix(&local_matrix, (shared.m_rows + shared.max_depth), shared.matrix_size);
                copy_matrix(&shared, local_matrix, matrix);
                calculate_values(&p_data, &shared, matrix, local_matrix);
                send_rows(&shared, matrix, nprocs);
                receive_results(&shared, matrix, nprocs);
                write_to_file(matrix_size, matrix, fd_out);
                print2DArray(matrix, matrix_size, matrix_size); // TODO Remove this last
                free_int_array_memory(local_matrix, shared.m_rows + shared.max_depth);
                close(fd_in);
                close(fd_out);
        } else if (has_jobs) {
                rec_rows(calc_receiving_rows(&shared, p_data.me), local_matrix, shared.matrix_size);
                calculate_values(&p_data, &shared, sub_result, local_matrix);
                send_results_to_main(&shared, sub_result);
                free_int_array_memory(sub_result, shared.rows_each);
                free_int_array_memory(local_matrix, shared.rows_each + (2 * shared.max_depth));
        }

        MPI_Finalize ();
        exit (EXIT_SUCCESS);
}
