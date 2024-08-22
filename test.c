#include <stdio.h>
#include "mpi.h"
#include "trap.h"
void Get_data4(int me, float* a_ptr, float* b_ptr, int* n_ptr){
        int root = 0, position = 0;
        char buffer[100];
        if(me == 0){
                printf("Enter a, b, and n\n");
                scanf("%f %f %d", a_ptr, b_ptr, n_ptr);
                MPI_Pack(a_ptr, 1, MPI_FLOAT, buffer, 100, &position, MPI_COMM_WORLD);
                MPI_Pack(b_ptr, 1, MPI_FLOAT, buffer, 100, &position, MPI_COMM_WORLD);
                MPI_Pack(n_ptr, 1, MPI_INT, buffer, 100, &position, MPI_COMM_WORLD);
                MPI_Bcast(buffer, 100, MPI_PACKED, root, MPI_COMM_WORLD);
        } else {
                MPI_Bcast(buffer, 100, MPI_PACKED, root, MPI_COMM_WORLD);
                MPI_Unpack(buffer, 100, &position, a_ptr, 1, MPI_FLOAT, MPI_COMM_WORLD);
                MPI_Unpack(buffer, 100, &position, b_ptr, 1, MPI_FLOAT, MPI_COMM_WORLD);
                MPI_Unpack(buffer, 100, &position, n_ptr, 1, MPI_INT, MPI_COMM_WORLD);
        }
}

int main(int argc, char** argv) {
        int me, nproc, n, local_n;
        float h, a, b, local_a, local_b, integral, total;
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &me);
        MPI_Comm_size(MPI_COMM_WORLD, &nproc);
        Get_data4(me, &a, &b, &n);
        h = (b-a)/n;
        local_n = n/nproc;
        local_a = a + me*local_n*h;
        local_b = local_a + local_n*h;
        integral = Trap(local_a, local_b, local_n, h);
        MPI_Reduce(&integral, &total, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

        MPI_Finalize();
        return 0;
}




#include <stdio.h>
#include "mpi.h"
#include "trap.h"
#define MainProcess 0

typedef struct {
    float a;
    float b;
    int n;
} INDATA_TYPE;

int main(int argc, char** argv) {
        int me, nproc, local_n;
        float h, local_a, local_b, integral, total;
        INDATA_TYPE indata;

        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &me);
        MPI_Comm_size(MPI_COMM_WORLD, &nproc);

        if (me == MainProcess){
                printf("Enter a, b, and n\n");
                scanf("%f %f %d", &(indata.a), &(indata.b), &(indata.n));
        }
        MPI_Bcast(&indata, sizeof(INDATA_TYPE), MPI_BYTE, MainProcess, MPI_COMM_WORLD);

        h = (indata.b-indata.a)/indata.n;
        local_n = indata.n/nproc;
        local_a = indata.a + me*local_n*h;
        local_b = local_a + local_n*h;
        integral = Trap(local_a, local_b, local_n, h);

        MPI_Reduce(&integral, &total, 1, MPI_FLOAT, MPI_SUM, MainProcess, MPI_COMM_WORLD);
        if(me == MainProcess) {
                printf("With n = %d trapezoids, our estimate\n", indata.n);
                printf("of the integral from %f to %f = %f\n",
                       indata.a, indata.b, total);
        }

        MPI_Finalize();
        return 0;
}




#include <stdio.h>
#include "mpi.h"
#include "trap.h"
typedef struct {
    float a;
    float b;
    int n;
}  INDATA_TYPE;
void Build_derived_type(INDATA_TYPE* indata,
                        MPI_Datatype* message_type_ptr){
        int block_lengths[3];
        MPI_Aint displacements[3];
        MPI_Aint addresses[4];
        MPI_Datatype typelist[3];
        typelist[0] = MPI_FLOAT;
        typelist[1] = MPI_FLOAT;
        typelist[2] = MPI_INT;
        block_lengths[0] = block_lengths[1] =  block_lengths[2] = 1;
        MPI_Get_address(indata, &addresses[0]);
        MPI_Get_address(&(indata->a), &addresses[1]);
        MPI_Get_address(&(indata->b), &addresses[2]);
        MPI_Get_address(&(indata->n), &addresses[3]);
        displacements[0] = addresses[1] - addresses[0];
        displacements[1] = addresses[2] - addresses[0];
        displacements[2] = addresses[3] - addresses[0];
        MPI_Type_create_struct(3, block_lengths, displacements, typelist,message_type_ptr);
        MPI_Type_commit(message_type_ptr);
}

void Get_data3(int me, INDATA_TYPE* indata){
        MPI_Datatype message_type;
        int root = 0, count = 1;
        if (me == 0){
                printf("Enter a, b, and n\n");
                scanf("%f %f %d", &(indata->a), &(indata->b), &(indata->n));
        }
        Build_derived_type(indata, &message_type);
        MPI_Bcast(indata, count, message_type, root, MPI_COMM_WORLD);
}

int main(int argc, char** argv) {
        int me, nproc, local_n;
        float h, local_a, local_b, integral, total;
        INDATA_TYPE indata;
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &me);
        MPI_Comm_size(MPI_COMM_WORLD, &nproc);
        Get_data3(me, &indata);
        h = (indata.b-indata.a)/indata.n;
        local_n = indata.n/nproc;
        local_a = indata.a + me*local_n*h;
        local_b = local_a + local_n*h;
        integral = Trap(local_a, local_b, local_n, h);
        MPI_Reduce(&integral, &total, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
        if(me == 0) {
                printf("With n = %d trapezoids, our estimate\n", indata.n);
                printf("of the integral from %f to %f = %f\n",
                       indata.a, indata.b, total);
        }
        MPI_Finalize();
        return 0;
}