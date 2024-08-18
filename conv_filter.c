#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "matrix.h"

// TODO Rewrite this for myself
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

        fprintf(stderr,"Matrix Size: %d\n", matrix_size);
        return matrix_size;
}

// TODO This is from lectures
int get_matrix(int fd, int matrix_size){
        int row, col, slot;

        for(row = 1; row <= matrix_size; row++)
                for(col = 1; col <= matrix_size; col++){
                        if(get_slot(fd, matrix_size, row, col, &slot) == -1){
                                fprintf(stderr,"get_slot failed at [%d][%d]\n", row, col);
                                exit(-1);
                        }
                }
        return 0;
}

int rounded_weighted_average(void)
{
        int sub_total = 0;
        int num_neighbours = 0;

        // TODO for each neighbour  sub_total =+ max_depth/actual_depth * value;

        return sub_total / num_neighbours;
}

int main(int argc, char *argv[]) {
        int fd_in, fd_out, matrix_size, max_depth;
        if (argc != 4) {
                fprintf (stderr, "Usage: %s matrix_file dimension\n", argv[0]);
                exit(1);
        }

        // TODO argv[1] is input file
        if ((fd_in = open(argv[1], O_RDONLY)) == -1) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                exit(1);
        }

        matrix_size = get_matrix_size(argv[1]);

        // TODO Will this create if doesn't exist?
        if ((fd_out = open(argv[2], O_CREAT)) == -1) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                exit(1);
        }

        // TODO Can depth be 0?
        if ((max_depth = atoi(argv[3])) <= 0) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                exit(1);
        }

        /* DO WE WANT TO STORE THE MATRIX OR HAVE EACH PROCESS READ FROM THE FILE
        int **matrix = (int **)malloc(matrix_size * sizeof(int *));
        for (int i = 0; i < rows; i++) {
                matrix[i] = (int *)malloc(matrix_size * sizeof(int));
        }*/

        // TODO get_matrix to be modified for output file (input file doesn't get changed)

        // TODO Divide up between processes

        // TODO Only send each process the slots it needs

        // TODO each goes through its items, rounded_weighted_average and sets_slot

        // TODO Partial results from slaves, communicated to master, collated and written to output file

}
