#include "mpi.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

short Adj[50000][50000];            //adjacency matrix 

void custom_min(void *in, void *inout, int *len, MPI_Datatype *datatype) {
    int *in_array = (int *)in;
    int *inout_array = (int *)inout;
    if (in_array[1] == inout_array[1] && inout_array[0] > in_array[0]){
        inout_array[0] = in_array[0];
    }
    else if (in_array[1] < inout_array[1]) {
        inout_array[1] = in_array[1];
        inout_array[0] = in_array[0];
    }
}

int main( int argc, char *argv[]){
    int n, myid, numprocs;
    char input[50];
    int size;
    bool selected[50000];
    int dist[50000];   
    int min[2];         //(index, dist)
    int global_min[2];

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);

    if (myid == 0) {
        scanf("%s", input);
        FILE *input_file = fopen(input, "r");
        if(input_file == NULL){
            printf("could not open file %s\n", input);
            fclose(input_file);
            return 1;
        }
        fscanf(input_file, "%d", &n);
        for(int i=0; i<n;i++){
            for(int j=0;j<n;j++){
                Adj[i][j] = -1;
            } 
        }

        int x, y;
        short temp;
        while (!feof(input_file)) {
            fscanf(input_file, "%d %d %hd", &x, &y, &temp);
            Adj[x][y] = temp;
        }
        fclose(input_file);
    }
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    size = n / numprocs;

    if(size < 70){                      //6, 1000
        if(myid == 0){
            //initialize
            selected[0] = true;
            dist[0] = 0;            
            for(int i = 1; i < n; i++){
                dist[i] = 99999;
                selected[i] = false;
            }
            for(int i = 0; i < n; i++){           
                if(Adj[0][i] != -1){
                    dist[i] = Adj[0][i];
                }
            }
            for(int i = 1; i < n; i++){
                min[1] = 99999;
                for(int j = 0; j < n; j++){
                    if(!selected[j] && dist[j] < min[1]){
                        min[0] = j;
                        min[1] = dist[j];
                    }
                }             
                selected[min[0]] = true;
                for(int j = 0; j < n; j++){
                    if(!selected[j] && Adj[min[0]][j] != -1 && dist[j] > dist[min[0]] + Adj[min[0]][j]){
                        dist[j] = dist[min[0]] + Adj[min[0]][j];
                    }
                }
            }
            for(int i = 0; i < n; i++){
                printf("%d ", dist[i]);
            }
        }
    }
    MPI_Op custom_op;
    MPI_Op_create((MPI_User_function *)custom_min, 1, &custom_op);    
    if(size > 70){                               //1000 50000
        //each process calculate size = n / numprocs  
        //short *temp = (short *)malloc(size * sizeof(short));
        for(int i=0;i<n;i++){
            if(myid == 0){
                for(int j=0;j<numprocs;j++){
                    MPI_Send(Adj[i], n, MPI_SHORT, j, 0, MPI_COMM_WORLD);
                }                
            }
            else{
                MPI_Recv(Adj[i], n, MPI_SHORT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
            }

        }

        int start = myid * size;
        //initialize
        for(int i = 0; i < size; i++){
            dist[i] = 99999;
            selected[i] = false;
        }
        if(myid == 0){
            selected[0] = true;
            dist[0] = 0;            
        }        
        //MPI_Scatter(Adj[0], size, MPI_SHORT, temp, size, MPI_SHORT, 0, MPI_COMM_WORLD);              
        for(int j = 0; j < size; j++){ 
            if(Adj[0][j] != -1){
                dist[j] = Adj[0][j];
            }
        }
        //loop 49999 times
        for(int i = 1; i < n; i++){
            min[1] = global_min[1] = 99999;
            for(int j = 0; j < size; j++){
                if(!selected[j] && dist[j] < min[1]){
                    min[0] = start + j;
                    min[1] = dist[j];
                }
            }        

            MPI_Allreduce(min, global_min, 2, MPI_INT, custom_op, MPI_COMM_WORLD);
            //MPI_Scatter(Adj[global_min[0]], size, MPI_SHORT, temp, size, MPI_SHORT, 0, MPI_COMM_WORLD);              
            if(global_min[0] == min[0])
                selected[min[0] - start] = true;
            
            for(int j = 0; j < size; j++){
                if(!selected[j] && Adj[global_min[0]][start+j] != -1 && dist[j] > global_min[1] + Adj[global_min[0]][start+j]){
                    dist[j] = global_min[1] + Adj[global_min[0]][start+j];
                }
            }
        }
        if(myid!=0){
            MPI_Send(dist, size, MPI_INT, 0, 0, MPI_COMM_WORLD);     
        }
        else{
            for(int i=0;i<size;i++){
                printf("%d ", dist[i]);
            }   
            for(int k=1;k<numprocs;k++){
                MPI_Recv(dist, size, MPI_INT, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for(int i=0;i<size;i++){
                    printf("%d ", dist[i]);
                }   
            }
        }   
    }

    MPI_Op_free(&custom_op);
    MPI_Finalize();
    return 0;
}