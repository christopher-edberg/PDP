#include "stencil.h"


int main(int argc, char **argv) {
	if (4 != argc) {
		printf("Usage: stencil input_file output_file number_of_applications\n");
		return 1;
	}
	char *input_name = argv[1];
	char *output_name = argv[2];
	int num_steps = atoi(argv[3]);

	double *input;
	int num_values;
	
	int nproc, rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD,&nproc);

	// Read input file
	
	if(rank == 0)
	if (0 > (num_values = read_input(input_name, &input))) {
		return 2;
	}
	MPI_Bcast(&num_values, 1, MPI_INT, 0, MPI_COMM_WORLD); //distributes initial values to all proccesses. 
	MPI_Bcast(&num_steps, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if(rank != 0) //allocates for proccesses other than 0 which does it through scanf.
		input =malloc(num_values* sizeof(double));

	int workPerProc = num_values/nproc;
	MPI_Bcast(input, num_values, MPI_DOUBLE, 0, MPI_COMM_WORLD);


	// Stencil values
	double h = 2.0*PI/num_values;
	const int STENCIL_WIDTH = 5;
	const int EXTENT = STENCIL_WIDTH/2;
	const double STENCIL[] = {1.0/(12*h), -8.0/(12*h), 0.0, 8.0/(12*h), -1.0/(12*h)};


	MPI_Barrier(MPI_COMM_WORLD);
	double *output;
	// Start timer
	double start = MPI_Wtime();
	// Allocate data for result
		if (NULL == (output = malloc(num_values * sizeof(double)))) {
			perror("Couldn't allocate memory for output");
			return 2;
		}



	// Repeatedly apply stencil
	for (int s=0; s<num_steps; s++) {
		// Apply stencil
		if(rank == 0)
			for (int i=0; i<EXTENT; i++) { //handles left edge case
				double result = 0; 
				for (int j=0; j<STENCIL_WIDTH; j++) {
					int index = (i - EXTENT + j + num_values) % num_values;
					result += STENCIL[j] * input[index];
				}
				output[i] = result;
			} 
		if(rank ==0)	
			for (int i= EXTENT; i<( workPerProc) && i<num_values-EXTENT; i++) { //handles most, rank*workPerProc+Extent gives starting value for the division of work
				double result = 0;
				for (int j=0; j<STENCIL_WIDTH; j++) {
					int index = i - EXTENT + j;
					result += STENCIL[j] * input[index];
				}
				output[i] = result;
			} else
				for (int i= (rank*workPerProc); i<( (rank*workPerProc)+ workPerProc) && i<num_values-EXTENT; i++) { //handles most, rank*workPerProc+Extent gives starting value for the division of work
					double result = 0;
					for (int j=0; j<STENCIL_WIDTH; j++) {
						int index = i - EXTENT + j; //gives offset to the left
						result += STENCIL[j] * input[index];
					}
					output[i] = result;
				}
		if(rank == nproc-1)
			for (int i=num_values-EXTENT; i<num_values; i++) { //handles the right edge case
				double result = 0;
				for (int j=0; j<STENCIL_WIDTH; j++) {
					int index = (i - EXTENT + j) % num_values;
					result += STENCIL[j] * input[index];
				}
				output[i] = result;
			}
		// Swap input and output
		if (s < num_steps-1) { 
			double *tmp = input;
			input = output;
			output = tmp;
			MPI_Request req[workPerProc];
			for(int i = 0; i< workPerProc; i++){ //handles communication with neighbors. for performance sake, we assume only the nearest neighbors will need our values. 
				if(rank !=0 ) { 
					MPI_Isend(&input[i+workPerProc*rank],1,MPI_DOUBLE,rank-1, rank,MPI_COMM_WORLD,&req[i]);
				} else if(rank ==0 && rank != nproc-1 &&nproc!=2) {
					MPI_Isend(&input[i],1,MPI_DOUBLE,nproc-1, rank,MPI_COMM_WORLD,&req[i]);
				}

				if(rank !=nproc-1 ) {
					MPI_Isend(&input[i+workPerProc*rank],1,MPI_DOUBLE,rank+1, rank,MPI_COMM_WORLD,&req[i]);
				} else if(rank ==nproc-1 && rank != 0 &&nproc!=2) {
					MPI_Isend(&input[i+workPerProc*rank],1,MPI_DOUBLE,0, rank,MPI_COMM_WORLD,&req[i]);
				}
			} 
			for(int i = 0; i< workPerProc; i++){
				if(rank !=0) {//exists a node to the left
					MPI_Recv( &input[i+workPerProc*(rank-1)], 1, MPI_DOUBLE, rank-1, rank-1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				} else if(rank ==0 && rank != nproc-1 &&nproc!=2){ //if rank == 0 and not only node
					MPI_Recv( &input[i+workPerProc*(nproc-1)], 1, MPI_DOUBLE, nproc-1, nproc-1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				} 

				if(rank !=nproc-1 ){ //if node to the right exists
					MPI_Recv( &input[i+workPerProc*(rank+1)], 1, MPI_DOUBLE, rank+1, rank+1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				} else if(rank == nproc-1 && rank != 0 &&nproc!=2){ //if last node and not only node
					MPI_Recv( &input[i], 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				}
			} 
		}
		
		
		

	}

	

	
	//Stop timer
	double my_execution_time = MPI_Wtime() - start;

	double collect[workPerProc];
	double *times =malloc(nproc* sizeof(double));//[nproc];
	
	for(int y = 0; y<workPerProc; y++){
		collect[y]= output[rank*workPerProc+y];
	}
		
	times[rank] = my_execution_time;

	
	


	if(rank !=0) { //Function to gather all values of collect from the different nodes and add them to the array in rank 0.
		MPI_Gather(&collect,workPerProc, MPI_DOUBLE, NULL,workPerProc, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		//MPI_Gather(&my_execution_time,1, MPI_DOUBLE, NULL,nproc, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	} else {
		MPI_Gather(&collect,workPerProc, MPI_DOUBLE, output,workPerProc, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		//MPI_Gather(&my_execution_time,1, MPI_DOUBLE, times,nproc, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	}
 	
	if (rank==0) //gather times.
	{
		for(int i = 1; i< nproc; i++)
			MPI_Recv(&times[i],1,MPI_DOUBLE, i, i, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
	} else {
		MPI_Send(&times[rank],1,MPI_DOUBLE, 0, rank, MPI_COMM_WORLD);
	}
	if(rank == 0)
	for(int i = 0; i<nproc; i++) //saves largest time at index 0 in times. 
		if(times[0]< times[i])
			times[0] = times[i];
	 
	
	//Write result
	if(rank ==0)
	printf("%f\n", times[0]);

#ifdef PRODUCE_OUTPUT_FILE
	if(rank == 0){
		if (0 != write_output(output_name, output, num_values)) {
			return 2;
		} 
	}
#endif

	// Clean up
	MPI_Finalize();
	free(output);
	free(input); 
	return 0;
}


int read_input(const char *file_name, double **values) {
	FILE *file;
	if (NULL == (file = fopen(file_name, "r"))) {
		perror("Couldn't open input file");
		return -1;
	}
	int num_values;
	if (EOF == fscanf(file, "%d", &num_values)) {
		perror("Couldn't read element count from input file");
		return -1;
	}
	if (NULL == (*values = malloc(num_values * sizeof(double)))) {
		perror("Couldn't allocate memory for input");
		return -1;
	}
	for (int i=0; i<num_values; i++) {
		if (EOF == fscanf(file, "%lf", &((*values)[i]))) {
			perror("Couldn't read elements from input file");
			return -1;
		}
	}
	if (0 != fclose(file)){
		perror("Warning: couldn't close input file");
	}
	return num_values;
}


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
