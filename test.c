int rounded_weighted_average(int **local_matrix, int i, int j, int n, int size, int me)
{
        fprintf(stderr, "Process %d entered rounded weight\n", me);
        int sub_total = 0;
        int num_neighbours = 0;

        int cur_depth = 1;
        int m_n = 0;

        while(cur_depth <= n) {
                m_n = n / cur_depth;
                for (int i = -cur_depth; i <= cur_depth; i++) {
                        for (int j = -cur_depth; j <= cur_depth; j++) {
                                if ((i == 0 && j == 0) || i > size-1 || j > size-1 || i < 0 || j < 0) {
                                        continue;
                                }
                                sub_total += m_n * local_matrix[i][j];
                                num_neighbours++;
                        }
                }
                cur_depth++;
        }
        if (num_neighbours == 0) {
                fprintf(stderr, "Error: Division by zero in rounded_weighted_average.\n");
                exit(EXIT_FAILURE);
        }
        fprintf(stderr, "Process %d exiting rounded weight\n", me);
        return sub_total / num_neighbours;
}

int main(int argc, char *argv[]) {

}