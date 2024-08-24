#ifndef A3_FILE_HANDLING_H
#define A3_FILE_HANDLING_H

void get_matrix_from_file(int fd, int matrix_size, int **matrix);
int get_matrix_size(int fd_in);
void write_to_file(int rows, int **matrix, int fd_out);

#endif //A3_FILE_HANDLING_H
