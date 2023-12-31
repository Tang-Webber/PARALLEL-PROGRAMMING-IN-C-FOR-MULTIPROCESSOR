#include "mpi.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

struct Point {
    int id, x, y;
} P[12000];
int cross(struct Point o, struct Point a, struct Point b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}
int compare(const void* a, const void* b)
{
    return ((struct Point*)a)->x - ((struct Point*)b)->x;
}

int main( int argc, char *argv[])
{
    int n, myid, numprocs;
    char input[50];
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);
    //scan the input
    if (myid == 0) {
        scanf("%s", input);
        FILE *input_file = fopen(input, "r");
        if(input_file == NULL){
            printf("could not open file %s\n", input);
            fclose(input_file);
            return 1;
        }
        fscanf(input_file, "%d", &n);
        for (int i = 0; i < n; i++) {
            fscanf(input_file, "%d %d", &P[i].x, &P[i].y);
            P[i].id = i + 1;
        }
        fclose(input_file);
        qsort(P, n, sizeof(struct Point), compare);         //sort
    }
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int local_count = n / numprocs;
    int rest = n % numprocs;
    struct Point* local_P = (struct Point*)malloc(local_count * sizeof(struct Point));
    struct Point* local_upper_ch = (struct Point*)malloc(n * sizeof(struct Point));
    struct Point* local_lower_ch = (struct Point*)malloc(n * sizeof(struct Point));    
    if(rest == 0){
        MPI_Scatter(P, local_count * sizeof(struct Point), MPI_BYTE, local_P, local_count* sizeof(struct Point), MPI_BYTE, 0, MPI_COMM_WORLD);              
    }
    else{       //rest
        int *recv_counts = (int*)malloc(numprocs * sizeof(int));
        int *displacements = (int*)malloc(numprocs * sizeof(int));
        int base_count = n / numprocs;        
        for (int i = 0; i < numprocs; i++) {
            recv_counts[i] = base_count;
            if (i == numprocs - 1) {
                recv_counts[i] += rest;
            }
            displacements[i] = i * base_count;
        }  
        local_count = recv_counts[myid]; 
        free(local_P);
        free(local_upper_ch);
        free(local_lower_ch);
        local_P = (struct Point*)malloc(local_count * sizeof(struct Point));    
        local_upper_ch = (struct Point*)malloc(n * sizeof(struct Point));
        local_lower_ch = (struct Point*)malloc(n * sizeof(struct Point)); 

        MPI_Datatype PointType;
        MPI_Aint offsets[3];
        MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
        offsets[0] = offsetof(struct Point, id);
        offsets[1] = offsetof(struct Point, x);
        offsets[2] = offsetof(struct Point, y);
        int block_lengths[3] = {1, 1, 1};    
        MPI_Type_create_struct(3, block_lengths, offsets, types, &PointType);
        MPI_Type_commit(&PointType);        
        MPI_Scatterv(P, recv_counts, displacements, PointType, local_P, recv_counts[myid], PointType, 0, MPI_COMM_WORLD);
        MPI_Type_free(&PointType);
    }
    int up = 0;
    int down = 0;
    struct Point **gathered_up = (struct Point**)malloc(numprocs * sizeof(struct Point*));
    struct Point **gathered_down = (struct Point**)malloc(numprocs * sizeof(struct Point*));
    for(int i = 0; i < numprocs ;i++){
        gathered_up[i] = (struct Point*)malloc(n * sizeof(struct Point));
        gathered_down[i] = (struct Point*)malloc(n * sizeof(struct Point));
    }
    //Andrew's Monotone Chain
    for (int i = 0; i < local_count; i++){
        while (down >= 2 && cross(local_lower_ch[down-2], local_lower_ch[down-1], local_P[i]) <= 0) down--;
        local_lower_ch[down++] = local_P[i];
        while (up >= 2 && cross(local_upper_ch[up-2], local_upper_ch[up-1], local_P[i]) >= 0) up--;
        local_upper_ch[up++] = local_P[i];
    }     
    //Combine
    struct Point* next_upper_ch = (struct Point*)malloc(n * sizeof(struct Point));
    struct Point* next_lower_ch = (struct Point*)malloc(n * sizeof(struct Point)); 
    int left, right;     
    int next_up = 0, next_down = 0;
    int x = 1, y = 2;
    while(x != numprocs){
        if(myid % y == x){          //get data
            MPI_Send(&up, 1, MPI_INT, myid - x, 0, MPI_COMM_WORLD);
            MPI_Send(&down, 1, MPI_INT, myid - x, 0, MPI_COMM_WORLD);            
            MPI_Send(local_upper_ch, up * sizeof(struct Point), MPI_BYTE, myid - x, 0, MPI_COMM_WORLD);
            MPI_Send(local_lower_ch, down * sizeof(struct Point), MPI_BYTE, myid - x, 0, MPI_COMM_WORLD);
        }
        if(myid % y == 0){          //recieve data
            MPI_Recv(&next_up, 1, MPI_INT, myid + x, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&next_down, 1, MPI_INT, myid + x, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(next_upper_ch, next_up * sizeof(struct Point), MPI_BYTE, myid + x, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(next_lower_ch, next_down * sizeof(struct Point), MPI_BYTE, myid + x, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            //Lower
            left = down - 1;
            right = 0;
            while(1){
                if(left == 0 || right == next_down - 1)
                    break;           
                if(cross(local_lower_ch[left - 1], next_lower_ch[right], local_lower_ch[left]) <= 0 && cross(local_lower_ch[left], next_lower_ch[right + 1], next_lower_ch[right]) <= 0)
                    break;         
                if(cross(next_lower_ch[right], local_lower_ch[left - 1], local_lower_ch[left]) < 0)
                    left--;
                if(cross(local_lower_ch[left], next_lower_ch[right + 1], next_lower_ch[right]) > 0)
                    right++;
            }
            //Combine the result
            for(int j = 0; j < next_down - right; j++)
                local_lower_ch[left + j + 1] = next_lower_ch[j + right];
            down = left + 1 + next_down - right;
            right = 0;
            //Upper
            left = up - 1;
            right = 0;
            while(1){
                if(left == 0 || right == next_down - 1)
                    break;
                if((cross(local_upper_ch[left - 1], next_upper_ch[right], local_upper_ch[left]) >= 0 && cross(local_upper_ch[left], next_upper_ch[right + 1], next_upper_ch[right]) >= 0))
                    break; 
                if(cross(next_upper_ch[right], local_upper_ch[left - 1], local_upper_ch[left]) > 0)
                    left--;
                if(cross(local_upper_ch[left], next_upper_ch[right + 1], next_upper_ch[right]) < 0)
                    right++;
            }
            //Combine the results to final_ch
            for(int j = 0; j < next_up - right; j++){
                local_upper_ch[left + j + 1] = next_upper_ch[j + right];
            }
            up = left + 1 + next_up - right;
            right = 0;
        }
        x *= 2;
        y *= 2;
    }
    //output
    if(myid == 0){
        for(int i = 0;i < up; i++)
            printf("%d ", local_upper_ch[i].id);
        for(int i = down - 2; i > 0; i--)
            printf("%d ", local_lower_ch[i].id);
    }
    //Free memory
    for (int i = 0; i < numprocs; i++) {
        free(gathered_up[i]);
        free(gathered_down[i]);
    }       
    free(gathered_up);
    free(gathered_down);     
    free(local_P);
    free(local_upper_ch);
    free(local_lower_ch);
    MPI_Finalize();
    return 0;
}