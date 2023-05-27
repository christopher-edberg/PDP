#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

//Fixa så den klarar av en NxN matris istället för 1D array
//Borrowed from assignment 1.
int write_output(char *file_name, const double *output, int num_values) {
	FILE *file;
	if (NULL == (file = fopen(file_name, "w"))) {
		perror("Couldn't open output file");
		return -1;
	}
	for (int i = 0; i < num_values; i++) {
		if (0 > fprintf(file, "%.4f ", output[i])) {
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

//REWRITE FOR CLARITY
//Serial version, a,b,c are matrixes. 
void mul_ikj(int dim, double **a, double **b, double **c)
{
  int i, j, k;
  double* y,*p,x; //placeholders for cache performance. 

  for (i=0; i<dim; i++) {
    p = c[i];
    for (k=0; k<dim; k++) {
      x = a[i][k];
      y = b[k];
      for (j=0; j<dim; j++)
	      p[j] += x * y[j];
    }
  }
}








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

	double **a, **b, **c;
	int dim;
	double *input; //TODO: Free
	int Bdim, blocksize;
	int ndim = sqrt(nproc); //Calculates processor grid dimensions. ! has to be square !
	double *aDis, *bDis; //used to distribute the values of each block in form of 1D array. Array[number of proccesors][number of values]
	//Retrieves dimensions and elements from the input file.
	if(rank == 0) {
		if (0 > (dim = read_input(input_name, &input))) {
			return 2;
		}
		Bdim = dim/ndim; //Calculates block size.
		blocksize = Bdim*Bdim;
		//allocateScalable(&aDis, nproc,blocksize);
		aDis = (double *)malloc(nproc*blocksize * sizeof(double));
		//allocateScalable(&bDis, nproc,blocksize);
		bDis = (double *)malloc(nproc*blocksize * sizeof(double));
		int index,row,proc = 0;
		for(int i = 0; i<ndim; i++){ //Used to determine Block of a,b for each processor and store as an 1D array.
			for(int k = 0; k<ndim; k++) {
				//printf(" \nk = %d \n",i+k);
				row =0;
				index = 0;	
				for(int y = 0; y < Bdim; y++) {
					for(int x = 0; x < Bdim; x++){
						aDis[proc*blocksize+index] = input[k*Bdim + (row+i*Bdim)*dim + x];
						bDis[proc*blocksize+index] = input[k*Bdim + (row+i*Bdim)*dim + x + dim*dim];
						/* aDis[proc][index] = input[k*Bdim + (row+i*Bdim)*dim + x];
						bDis[proc][index] = input[k*Bdim + (row+i*Bdim)*dim + x + dim*dim]; */
						//printf("%f ", aDis[i][index]);
						index ++;	
					}
					row++;
				}
				proc ++;
			}
		}
	} 
	MPI_Bcast(&Bdim, 1,MPI_INT, 0, MPI_COMM_WORLD);
	allocate(&a, Bdim);	
	allocate(&b, Bdim);	
	allocate(&c, Bdim);
	blocksize = Bdim * Bdim;
	if(rank != 0) 	{ //Test direct assignment vs allocating new.
		aDis = (double *)malloc(blocksize * sizeof(double));
		bDis = (double *)malloc(blocksize * sizeof(double));
	}
	MPI_Scatter(aDis, blocksize, MPI_DOUBLE, aDis, blocksize, MPI_DOUBLE, 0, MPI_COMM_WORLD );
	
	
		
	


	MPI_Finalize();
	return 0;
} //TODO: FREE ALL THE MATRIXES



//code to print array
/* for(int k = 0; k<dim; k++) {
		printf("\n");
		for(int l = 0; l<dim; l++)
			printf(" %f",a[k][l]);
	}
	printf("\n");
	
	for(int k = 0; k<dim; k++) {
		printf("\n");
		for(int l = 0; l<dim; l++)
			printf(" %f",b[k][l]);
	} */



//mul_ikj(dim,a,b,c);
	/* for(int k = 0; k<dim; k++) { 
		printf("\n");
		for(int l = 0; l<dim; l++)
			printf(" %f",b[k][l]);
	} */

 

/* allocate(&a, dim);
		allocate(&b, dim);
		//allocate(&c, dim);
		for(int i =0; i<dim; i++) //Assigns the values row-wise for matrix A. 
			for(int j = 0; j<dim; j++)
				a[i][j] =input[i*dim+ j];
		*/

	/* for(int i =0; i<dim; i++) //Assigns the values column wise for matrix B. so A[0]*B[0] = C[0][0]
			for(int j = 0; j<dim; j++)
				b[j][i] =input[(i*dim+ j)+ dim*dim];  */

				//allocate(&b, )
	
	//code that determines work per proc.
	//int part = (dim*dim)/nproc;
	//MPI_Bcast(&part, 1,MPI_INT, 0, MPI_COMM_WORLD);
	/* if(rank != 0){
		input = malloc(sizeof(double)* part);
		//allocateScalable(&b, 2,)
	} 
	
	//mpi scatter to divide A. using input array. 
	MPI_Scatter(input,part, MPI_DOUBLE, input, part, MPI_DOUBLE, 0, MPI_COMM_WORLD); */

	/* for(int i = 0; i<ndim; i++){
			for(int k = 0; k<ndim; k++) {
				for(int y = 0; y < Bdim; y++) {
					for(int x = 0; x < Bdim; x++){
						aDis[i][index] = input[k*Bdim + (row)*dim + x];
						index ++;
						
					}
					row++;
				}
				
			}
			index = 0;
			row =0;
		}printf("bdim is %f \n",aDis[0][0]); */