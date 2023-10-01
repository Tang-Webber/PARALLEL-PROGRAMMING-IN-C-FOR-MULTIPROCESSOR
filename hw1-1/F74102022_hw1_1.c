#include "mpi.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

int main( int argc, char *argv[])
{
    int n, m, myid, numprocs, num, part, cost;
    int count = 0;
    int sum;
    unsigned int pow2n = 1;             //pow2n : process represent in binary
    unsigned int pow2m = 1;             //pow2m : num of premutations
    unsigned int set = 0;
    unsigned int t;
    unsigned int tests[32] = {0};
    double startwtime = 0.0, endwtime;
    int  namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    char input[50];
    char output[50];

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);
    //MPI_Get_processor_name(processor_name,&namelen);
    if (myid == 0) {
        //startwtime = MPI_Wtime(); 
        scanf("%s", input);
        FILE *input_file = fopen(input, "r");
//printf("open%s!\n", input);
        if(input_file == NULL){
            printf("could not open file %s\n", input);
            fclose(input_file);
            return 1;
        }
        fscanf(input_file, "%d %d", &n, &m);
        for (int j = 0; j < n; j++) {
            pow2n *= 2;
        }
        pow2n--;
        for (int j = 0; j < m; j++) {
            pow2m *= 2;
        }
        for (int j = 0; j < m; j++) {
            fscanf(input_file, "%d %d", &num, &cost);
            tests[j] = 0;
            for (int k = 0; k < num; k++) {
                fscanf(input_file, "%d", &part);
                tests[j] |= (1 << (part - 1));
            }
        }
        fclose(input_file);
    }
    MPI_Datatype testarr;
    MPI_Type_contiguous(32, MPI_UNSIGNED, &testarr);
    MPI_Type_commit(&testarr);
    MPI_Bcast(tests, 1, testarr, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);   
    MPI_Bcast(&pow2n, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    MPI_Bcast(&pow2m, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD); 
//printf("_________ID : %d\n__________", myid, numprocs, n,m,pow2n,pow2m);   
    for (unsigned int i = myid; i < pow2m; i += numprocs){
        t = i;
        set = 0;
        for(int k = 0; k < m; k++){
            if ((t & (1u << k)) != 0) 
                set = set | tests[k];
        }
        if(set == pow2n)
            count++;
//printf("TEST : id:%d i=%d c=%d   ", myid, i, count);
//printf("set=%u, ans=%u \n", set, pow2n);              
    } 
    MPI_Barrier(MPI_COMM_WORLD); 
    MPI_Reduce(&count, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);      
    if (myid == 0){
        printf("%d\n", sum);
        //fprintf(output_file, "%d\n", sum); 
        //fclose(output_file);
    } 
    MPI_Finalize();
    return 0;
}