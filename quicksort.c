#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


//Alter for 1d array instead of matrix.
int read_input(const char *file_name, int **values) {
	FILE *file;
	if (NULL == (file = fopen(file_name, "r"))) {
		perror("Couldn't open input file");
		return -1;
	}
	int size;
	if (EOF == fscanf(file, "%d", &size)) {
		perror("Couldn't read length from input file");
		return -1;
	}
	if (NULL == (*values = malloc((size) * sizeof(int)))) {
		perror("Couldn't allocate memory for input");
		return -1;
	}
	for (int i=0; i<size; i++) {
		if (EOF == fscanf(file, "%d", &((*values)[i]))) {
			perror("Couldn't read elements from input file");
			return -1;
		}
	}
	if (0 != fclose(file)){
		perror("Warning: couldn't close input file");
	}
	return size;
}

//Borrowed from assignment 1. TODO ALTER TO INTEGER
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


int comp(const void * a, const void * b) {
    int a1 = *(int*) a;
    int a2 = *(int*) b;
    return (a1 > a2 ? 1: (a1 < a2 ? -1: 0));
}


//Perhaps should be double? 
double median(int *values, int length){
    qsort(values,length, sizeof(int),comp);
    if(length % 2 == 0){
        int half = length/2;
        double a1= values[half]; //Done to avoid overflow. 
        double a2 = values[half-1];
        return ( (a1 + a2)/2.0 ) ;
    } else {
        return values[length/2];
    }
}

double medianAll(){
    return 0;
}

double mean(int *values, int length) {
    int sum = 0;
    for(int i = 0; i< length; i++){
        sum += values[i];
    }
    return sum/length;
}

//Used to find index of pivot to split list.
int FindPivotIndex(int *list, double value, int length){
/*     if(value <= list[0]){
        return 0;
    } */
    for(int i = 0; i<length; i++){
        if(list[i]>=value) {
            return i;
        }
    }
    return length; //No element larger than pivot found
}


void* merge(int **list1, int **list2, int length1,int length2){
    int c1,c2 = 0;
    //printf("length1: %d, length 2: %d \n", length1, length2);
    int * merged = malloc((length1+length2) *sizeof(int));
    while(c1+c2 <= length1+length2){
        if(c1 != length1 && c2 != length2){
             if (*list1[c1]<*list2[c2])
            {
                merged[c1+c2] = *list1[c1];
                //printf("c1: %d\n",*list1[0]);
                c1++;
            }  else {
                merged[c1+c2] =*list2[c2];
                c2++;
                
            } 
        } else if (c1 ==length1){
            merged[c1+c2] =*list2[c2];
                c2++;
        } else {
            merged[c1+c2] = *list1[c1];
            c1++;
        } 
        
    }
    free(*list1);
    free(*list2);
    *list1 = merged;
    //printf("c1: %d\n",*list1[0]);
}

void *quicksort(int pivot_strat, int *grouplist, int *input,int *send, int rank,int nproc ){
    double median;
    int *start = grouplist;
    int *end = grouplist+ nproc/2;
    /* if(rank == 1){
    printf("First list \n");
    for(int i =0; i<nproc/2; i++){
        printf(" %d ", start[i]);
    }
    printf("\n Second list \n");
    for(int i =0; i<nproc/2; i++){
        printf(" %d ", end[i]);
    }
    } */
    switch(pivot_strat) {
        case 1:
            //median; //median of one processor.
            break;
        case 2: 
            //medianAll; //median of all processes in group
            break;
        case 3: 
            //mean; //create mean
            break;
        }
        int piv = FindPivotIndex(input, median, send[rank]); //use for list split. TODO IMPLEMENT LATER
        int cutoff = nproc/(2*1);
        int tosend = send[rank]-piv;
        int toRecv;
        int *recvbuf;
        MPI_Request req[2];
        if(cutoff ==0 ){return 0;}
        if(rank <nproc/2)
        if(rank < cutoff){ //send right
            MPI_Isend(&tosend,1,MPI_INT, cutoff+rank,1, MPI_COMM_WORLD,&req[0]);
            MPI_Isend(&input[piv],send[rank]-piv,MPI_INT, cutoff+rank,1, MPI_COMM_WORLD,&req[1]);
            MPI_Recv(&toRecv,1,MPI_INT, cutoff+rank,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            recvbuf = malloc(toRecv*sizeof(int));
            MPI_Recv(recvbuf,toRecv,MPI_INT, cutoff+rank,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        } else { //Send left
            MPI_Isend(&tosend,1,MPI_INT, rank-cutoff,1, MPI_COMM_WORLD,&req[0]);
            MPI_Isend(&input[piv],send[rank]-piv,MPI_INT, rank-cutoff,1, MPI_COMM_WORLD,&req[1]);
            MPI_Recv(&toRecv,1,MPI_INT, rank-cutoff,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            recvbuf = malloc(toRecv*sizeof(int));
            MPI_Recv(recvbuf,toRecv,MPI_INT, rank-cutoff,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }
        merge(&input,&recvbuf, send[rank],toRecv);

        send[rank] = send[rank]-tosend+toRecv;

}

int main(int argc, char **argv){
    if (4 != argc) {
		printf("Usage: quicksort input_file output_file pivot_strategy \n");
		return 1;
	}
    
    char *input_name = argv[1];
	char *output_name = argv[2];
    int pivot_strat = atoi(argv[3]);
    //Nproc: number of proccessors
    int nproc, rank;
	MPI_Init(&argc, &argv); 
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD,&nproc);

    int *input, length;
    if(rank == 0) {
        if (0 > (length = read_input(input_name, &input))) {
			return 2;
		}
    } 

    MPI_Bcast(&length, 1,MPI_INT, 0, MPI_COMM_WORLD);
    int part = length/nproc;
    int remainder = length % nproc; //For non divisible list lengths. 
    int send[nproc], dsp[nproc]; //used for number of sends and the displacement.
    int index = 0;
    for(int i =0; i<nproc; i++){
        dsp[i] = index;
        if(i<remainder){ //Divides remainder over the processes by increasing send count used in scatterv. 
            send[i] = part +1;
        } else {
            send[i] = part;
        }
        index = index + send[i]; //To get correct start of the buffers for each scatterv. 

    }
    if(rank != 0) {
        input = malloc(send[rank] *sizeof(int));
    }
    MPI_Scatterv(input, send, dsp, MPI_INT,input ,send[rank], MPI_INT, 0, MPI_COMM_WORLD);
    qsort(input,send[rank], sizeof(int),comp);
    
    //pivot elemental
    double median;
    int steps = 1;// log2(nproc);
    MPI_Request req[2];
    int grouplist[nproc];
    for(int i = 0; i<nproc; i++)
        grouplist[i] = i;
    quicksort(0,grouplist, NULL, input,rank,nproc);
    for(int i = 1; i<=steps; i++){    
        switch(pivot_strat) {
        case 1:
            //median; //median of one processor.
            break;
        case 2: 
            //medianAll; //median of all processes in group
            break;
        case 3: 
            //mean; //create mean
            break;
        }
        int piv = FindPivotIndex(input, median, send[rank]); //use for list split. TODO IMPLEMENT LATER
        int cutoff = nproc/(2*i);
        int tosend = send[rank]-piv;
        int toRecv;
        int *recvbuf;
        if(cutoff ==1 ){break;}
        if(rank <nproc/2)
        if(rank < cutoff){ //send right
            MPI_Isend(&tosend,1,MPI_INT, cutoff+rank,1, MPI_COMM_WORLD,&req[0]);
            MPI_Isend(&input[piv],send[rank]-piv,MPI_INT, cutoff+rank,1, MPI_COMM_WORLD,&req[1]);
            MPI_Recv(&toRecv,1,MPI_INT, cutoff+rank,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            recvbuf = malloc(toRecv*sizeof(int));
            MPI_Recv(recvbuf,toRecv,MPI_INT, cutoff+rank,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        } else { //Send left
            MPI_Isend(&tosend,1,MPI_INT, rank-cutoff,1, MPI_COMM_WORLD,&req[0]);
            MPI_Isend(&input[piv],send[rank]-piv,MPI_INT, rank-cutoff,1, MPI_COMM_WORLD,&req[1]);
            MPI_Recv(&toRecv,1,MPI_INT, rank-cutoff,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            recvbuf = malloc(toRecv*sizeof(int));
            MPI_Recv(recvbuf,toRecv,MPI_INT, rank-cutoff,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }
        merge(&input,&recvbuf, send[rank],toRecv);

        send[rank] = send[rank]-tosend+toRecv;
    }

    MPI_Finalize();
    //FREE INPUT
    return 0;
}






/*  if(rank == 0)
        for(int i = 0; i<length; i++)
            printf(" %d ", input[i]); */





























