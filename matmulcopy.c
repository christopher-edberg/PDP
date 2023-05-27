#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define PRODUCE_OUTPUT_FILE 1
//Altered version from Assigment 1.
int read_input(const char *file_name, double **values) {
	FILE *file;
	if (NULL == (file = fopen(file_name, "r"))) {
		perror("Couldn't open input file");
		return -1;
	}
	int dim;
	if (EOF == fscanf(file, "%d", &dim)) {
		perror("Couldn't read dimension from input file");
		return -1;
	}
	if (NULL == (*values = malloc(2*(dim*dim) * sizeof(double)))) {
		perror("Couldn't allocate memory for input");
		return -1;
	}
	for (int i=0; i<2*(dim*dim); i++) {
		if (EOF == fscanf(file, "%lf", &((*values)[i]))) {
			perror("Couldn't read elements from input file");
			return -1;
		}
	}
	if (0 != fclose(file)){
		perror("Warning: couldn't close input file");
	}
	return dim;
}

//Borrowed from assignment 1.
int write_output(char *file_name, const double *output, int num_values) {
	FILE *file;
	if (NULL == (file = fopen(file_name, "w"))) {
		perror("Couldn't open output file");
		return -1;
	}
	for (int i = 0; i < num_values; i++) {
		if (0 > fprintf(file, "%.6f ", output[i])) {
			perror("Couldn't write to output file");
		}
	}
	if (0 > fprintf(file, "\n")) {
		perror("Couldn't write to output file");
	}
	if (0 != fclose(file)) {
		perror("Warning: couldn't close output file");
	}
	return 0;
}

//Serial version, a,b,c are matrixes. Written by me in HPP course.
void mul_ikj(int dim, double *a, double *b, double *c)
{
  int i, j, k;
  for (i=0; i<dim; i++) {
    for (k=0; k<dim; k++) {
      for (j=0; j<dim; j++)
	      c[i*dim+j] += a[i*dim+k] * b[k*dim+j];
    }
  }
}






//Functions below are not used in final version. 

//Allocates row-wise, used for Matrix A and the result array C
void allocate(double ***matrix, int dim){
	*matrix = (double **)malloc(dim*sizeof(double*));
	for(int i = 0; i<dim; i++) (*matrix)[i] = (double *)malloc(dim * sizeof(double));
}
void allocateScalable(double ***matrix, int dim1,int dim2){
	*matrix = (double **)malloc(dim1*sizeof(double*));
	for(int i = 0; i<dim1; i++) (*matrix)[i] = (double *)malloc(dim2 * sizeof(double));
}

void free_mem(int** matrix, int n)
{
	for(int i=0; i<n; i++) free(matrix[i]);
	free(matrix);
}



int main(int argc, char **argv) {
    if (3 != argc) {
		printf("Usage: matmul input_file output_file \n");
		return 1;
	}
	//File names for input and output, used in read and write functions.
    char *input_name = argv[1];
	char *output_name = argv[2];
    
	//Nproc: number of proccessors
	int nproc, rank;
	MPI_Init(&argc, &argv); 
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD,&nproc);

	double *a, *b, *c;
	int dim;
	double *input; //TODO: Free
	int Bdim, blocksize; //Block dimension and number of values in a block.
	int ndim = sqrt(nproc); //Calculates processor grid dimensions. ! has to be square !
	
	//Retrieves dimensions and elements from the input file.
	if(rank == 0) {
		if (0 > (dim = read_input(input_name, &input))) {
			return 2;
		}
		Bdim = dim/ndim; //Calculates block size.
		blocksize = Bdim*Bdim;
		a = (double *)malloc(nproc*blocksize * sizeof(double));
		b = (double *)malloc(nproc*blocksize * sizeof(double));
		int index,row,proc = 0;
		for(int i = 0; i<ndim; i++){ //Used to determine Block of a,b for each processor and store as an 1D array.
			for(int k = 0; k<ndim; k++) {
				row =0;
				index = 0;	
				for(int y = 0; y < Bdim; y++) {
					for(int x = 0; x < Bdim; x++){
						a[proc*blocksize+index] = input[k*Bdim + (row+i*Bdim)*dim + x];
						b[proc*blocksize+index] = input[k*Bdim + (row+i*Bdim)*dim + x + dim*dim];
						index ++;	
					}
					row++;
				}
				proc ++;
			}
		}
		free(input);
	} 
	MPI_Bcast(&Bdim, 1,MPI_INT, 0, MPI_COMM_WORLD);

	blocksize = Bdim * Bdim;
	if(rank != 0) 	{ //Test direct assignment vs allocating new.
		a = (double *)malloc(blocksize * sizeof(double));
		b = (double *)malloc(blocksize * sizeof(double));
	}
	c = (double *)malloc(blocksize * sizeof(double));
	for(int i = 0; i<blocksize; i++) //Ensures there is no garbage left in the memory as I use += in the mul_ikj function. 
		c[i]=0;
	MPI_Scatter(a, blocksize, MPI_DOUBLE, a, blocksize, MPI_DOUBLE, 0, MPI_COMM_WORLD );
	MPI_Scatter(b, blocksize, MPI_DOUBLE, b, blocksize, MPI_DOUBLE, 0, MPI_COMM_WORLD );
	int D, U, L, R; // Integers for cartshift to track neighbors for the shift. 
	int loc[2];
	int dimensions[2] = {ndim,ndim};
	int per[2] = {1,1};
	
	MPI_Comm cannon;
	MPI_Cart_create(MPI_COMM_WORLD, 2, dimensions, per, 1, &cannon);

	MPI_Cart_coords(cannon, rank, 2, loc); //Gives the coordinates of the current process.
	MPI_Cart_shift(cannon, 1, loc[0], &L, &R); //Calculates the source and destination of the shift needed using the nodes coordinates.
	MPI_Cart_shift(cannon, 0, loc[1], &U, &D);
	
	//Aligns data
	MPI_Sendrecv_replace(a, blocksize, MPI_DOUBLE, L, 1, R, 1, cannon, MPI_STATUS_IGNORE);
	MPI_Sendrecv_replace(b, blocksize, MPI_DOUBLE, U, 1, D, 1, cannon, MPI_STATUS_IGNORE);

	double start = MPI_Wtime();

	
	for(int i = 0; i<ndim; i++) { //ndim = sqrt(p)
		mul_ikj(Bdim, a,b,c); //Performs multiplications of the local block.

		MPI_Cart_shift(cannon, 1, 1, &L, &R);//Calculates shifts for the data and executes them in the sendrecv. 
		MPI_Cart_shift(cannon, 0, 1, &U, &D);
		MPI_Sendrecv_replace(a, blocksize, MPI_DOUBLE, L, 1, R, 1, cannon, MPI_STATUS_IGNORE);
		MPI_Sendrecv_replace(b, blocksize, MPI_DOUBLE, U, 1, D, 1, cannon, MPI_STATUS_IGNORE);
	}

	double my_execution_time = MPI_Wtime() - start; //use mpi reduce
	double time;
	MPI_Reduce(&my_execution_time, &time, 1, MPI_DOUBLE, MPI_MAX, 0, cannon);
	if(rank == 0)
		printf("%f\n",time);


	
	MPI_Gather(c, blocksize, MPI_DOUBLE, a, blocksize, MPI_DOUBLE,0,cannon);

	if(rank == 0) {
		int index2,row2,proc2 = 0;
		for(int i = 0; i<ndim; i++){ //Restores proper row-major order. 
			for(int k = 0; k<ndim; k++) {
				row2 =0;
				index2 = 0;	
				for(int y = 0; y < Bdim; y++) {
					for(int x = 0; x < Bdim; x++){
						b[k*Bdim + (row2+i*Bdim)*dim + x] = a[proc2*blocksize+index2];
						index2 ++;	
					}
					row2++;
				}
				proc2 ++;
			}
		}
	}

	#ifdef PRODUCE_OUTPUT_FILE
		if(rank == 0){
			if (0 != write_output(output_name, b, dim*dim)) {
				return 2;
			} 
		}
	#endif
	MPI_Finalize();
	free(a);
	free(b);
	free(c);
	return 0;
} //TODO: FREE ALL THE MATRIXES



