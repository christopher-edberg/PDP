/**********************************************************************
 * A simple "hello world" program for MPI/C
 *
 **********************************************************************/

#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

  MPI_Init(&argc, &argv);               /* Initialize MPI               */
  int rank;  
MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  printf("Hello World, rank %d!\n", rank);             /* Print a message              */

  MPI_Finalize();                       /* Shut down and clean up MPI   */

  return 0;
}
