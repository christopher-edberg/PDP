/**********************************************************************
 * Point-to-point communication using MPI
 *
 **********************************************************************/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int rank, size;
  double a, b;
  MPI_Status status;

  MPI_Init(&argc, &argv);               /* Initialize MPI               */
  MPI_Comm_size(MPI_COMM_WORLD, &size); /* Get the number of processors */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); /* Get my number                */
  
  a = 100.0 + (double) rank;  /* Different a on different processors */
  
  /* Exchange variable a, notice the send-recv order */
  /*
  if (rank == 0) {
    MPI_Send(&a, 1, MPI_DOUBLE, 1, 111, MPI_COMM_WORLD);
    MPI_Recv(&b, 1, MPI_DOUBLE, 2, 333, MPI_COMM_WORLD, &status);
    printf("Processor 0 got %f from processor 2\n", b);
  } else if (rank==1) {
    MPI_Recv(&b, 1, MPI_DOUBLE, 0, 111, MPI_COMM_WORLD, &status);
    MPI_Send(&a, 1, MPI_DOUBLE, 2, 222, MPI_COMM_WORLD);
    printf("Processor 1 got %f from processor 0\n", b);
  } else if(rank ==2){
    MPI_Send(&a, 1, MPI_DOUBLE, 0, 333, MPI_COMM_WORLD);
    MPI_Recv(&b, 1, MPI_DOUBLE, 1, 222, MPI_COMM_WORLD, &status);
    printf("Processor 2 got %f from processor 1\n", b);
  }
  */
  /*
    for(int i = 0; i<=size; i++){
      if( i == rank){
        if(rank == 0) {
          MPI_Send(&a, 1, MPI_DOUBLE, i+1, i, MPI_COMM_WORLD);
          MPI_Recv(&b, 1, MPI_DOUBLE, size-1, size-1, MPI_COMM_WORLD, &status);
          printf("Processor %d got %f from processor %d\n",i,b,size-1);
        } else if (rank==size-1){
          MPI_Send(&a, 1, MPI_DOUBLE, 0, i, MPI_COMM_WORLD);
          MPI_Recv(&b, 1, MPI_DOUBLE, i-1, i-1, MPI_COMM_WORLD, &status);
          printf("Processor %d got %f from processor %d\n",i,b,i-1);
        } else {
          MPI_Send(&a, 1, MPI_DOUBLE, i+1, i, MPI_COMM_WORLD);
          MPI_Recv(&b, 1, MPI_DOUBLE, i-1, i-1, MPI_COMM_WORLD, &status);
          printf("Processor %d got %f from processor %d\n",i,b,i-1);
        }
      }
    }
    */
    MPI_Request request;
    for(int i = 0; i<=size; i++){
      if( i == rank){
        if(rank == 0) {
          MPI_Request request0;
          MPI_Isend(&a, 1, MPI_DOUBLE, i+1, i, MPI_COMM_WORLD, &request0);
          MPI_Irecv(&b, 1, MPI_DOUBLE, size-1, size-1, MPI_COMM_WORLD, &request0);
          MPI_Wait(&request0,MPI_STATUS_IGNORE);
          printf("Processor %d got %f from processor %d\n",i,b,size-1);
        } else if (rank==size-1){
          MPI_Request request1;
          MPI_Isend(&a, 1, MPI_DOUBLE, 0, i, MPI_COMM_WORLD, &request1);
          MPI_Irecv(&b, 1, MPI_DOUBLE, i-1, i-1, MPI_COMM_WORLD, &request1);
          MPI_Wait(&request1,MPI_STATUS_IGNORE);
          printf("Processor %d got %f from processor %d\n",i,b,i-1);
        } else {
          MPI_Request request2;
          MPI_Isend(&a, 1, MPI_DOUBLE, i+1, i, MPI_COMM_WORLD, &request2);
          MPI_Irecv(&b, 1, MPI_DOUBLE, i-1, i-1, MPI_COMM_WORLD, &request2);
          MPI_Wait(&request2,MPI_STATUS_IGNORE);
          printf("Processor %d got %f from processor %d\n",i,b,i-1);
        }
      }
    }
    

  
  MPI_Finalize(); 

  return 0;
}
