/* basic matrix library for  comp381                            */
/* written by ian a. mason @ une  march 15  '99                 */
/* added to easter '99                                          */

void get_slot(int fd, int matrix_size, int row, int col, int *slot);
void set_slot(int fd, int matrix_size, int row, int col, int value);

void get_row(int fd, int matrix_size, int row, int matrix_row[]);
void set_row(int fd, int matrix_size, int row, int matrix_row[]);

void get_column(int fd, int matrix_size, int col, int matrix_col[]);
