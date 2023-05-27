#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PRODUCE_OUTPUT_FILE
//Alter for 1d array instead of matrix.
int read_input(const char *file_name, long **values) {
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
	if (NULL == (*values = malloc((size) * sizeof(long)))) {
		perror("Couldn't allocate memory for input");
		return -1;
	}
	for (int i=0; i<size; i++) {
		if (EOF == fscanf(file, "%ld", &((*values)[i]))) {
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
int write_output(char *file_name, const long *output, int num_values) {
	FILE *file;
	if (NULL == (file = fopen(file_name, "w"))) {
		perror("Couldn't open output file");
		return -1;
	}
	for (int i = 0; i < num_values; i++) {
		if (0 > fprintf(file, "%ld ", output[i])) {
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
double median(long *values, int length){
    //qsort(values,length, sizeof(long),comp); //will always be sorted regardless
    if(length == 0) {
       
        return -1;
    }
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(length % 2 == 0){
        int half = length/2;
        
        double a1= values[half];  
        double a2 = values[half-1];

        double returnval = (a1 + a2)/2.0;
        
        return ( (a1 + a2)/2.0 ) ;
    } else {
        int index = length/2; 
        return values[index];
    }
}

double medianAll(double *values, int length){
    if(length == 0) return -1; //should never occur but is there to failsafe
    int counter = length;
    
    for(int i = 0; i<length; i++){
        if(values[i] == -1) {
            counter--;
        }
    }
    double val[counter];
    int counter2 =0;
    for(int i = 0; i<length; i++){
        if(values[i] != -1){
            val[counter2] = values[i];
            counter2++;
        }
    }
    qsort(val,counter, sizeof(double),comp);
    if(length % 2 == 0){
        int half = counter/2;
        double a1= val[half]; 
        double a2 = val[half-1];
        return ( (a1 + a2)/2.0 ) ;
    } else {
        return val[counter/2];
    }
}

double mean(long *values, int length) {
    int sum = 0;
    int counter = length;
    for(int i = 0; i< length; i++){
        if(values[i] == 0){
            counter --;
        }
        sum += values[i];
    }
    return sum/counter;
}
double meanAll(double *values, int length) {
    double sum = 0;
    double counter = 0;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    for(int i = 0; i< length; i++){
        if(rank == 0)
        printf("\n value: %f \n", values[i]);
        if(values[i] != -1) {
            sum += values[i];
            counter = counter +1;
        }
        //sum += values[i];
    }   
    
    if(rank == 0)
        printf("\nrank %d counter %f, sum %f \n",rank,counter, sum);
    if(counter == 0) return 0;
    return (sum/counter);
}

//Used to find index of pivot to split list.
int FindPivotIndex(long *list, double value, int length){
    /* int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank == 5)
    printf("rank %d, val: %f, length: %d",rank,value,length); */
    for(int i = 0; i<length; i++){
        if(list[i]>=value) {
            return i;
        }
    }
    return -1; //No element larger than pivot found
}


void merge(long **list1, long **list2, int length1,int length2,int start){
    int c1 = 0,c2 = 0;
    if(length1 == 0 && length2 == 0) {
        return;
    }
    long * merged;
    if(length1 == 0){
        free(*list1);
        *list1 = *list2;
        return;
    } else if(length2 == 0) {
        merged = malloc((length1) *sizeof(long));
        for(int i = 0; i<length1; i++)
           merged[i] = (*list1)[i+start];
        free(*list1);
        *list1 = merged;
        return;
    }
    if(start != 0) {
        merged = malloc((length1+length2) *sizeof(long));
        //while (c1 < length1 && c2 < length2){//
        while(c1+c2 < length1+length2){
            if(c1 != length1 && c2 != length2){
                if ((*list1)[c1+start]<(*list2)[c2])
                {
                    merged[c1+c2] = (*list1)[c1+start];
                    c1++;
                }  else {
                    merged[c1+c2] =(*list2)[c2];
                    c2++;
                    
                } 
            } else if (c1 ==length1){
                merged[c1+c2] =(*list2)[c2];
                    c2++;
            } else {
                merged[c1+c2] = (*list1)[c1+start];
                c1++;
            }  
            
        }
    } else {
        merged = malloc((length1+length2) *sizeof(long));
        //while (c1 < length1 && c2 < length2) {
            while(c1+c2 < length1+length2){
            if(c1 != length1 && c2 != length2){
                if ((*list1)[c1]<(*list2)[c2])
                {
                    merged[c1+c2] = (*list1)[c1];
                    c1++;
                }  else {
                    merged[c1+c2] =(*list2)[c2];
                    c2++;
                    
                } 
            } else if (c1 ==length1){
                merged[c1+c2] =(*list2)[c2];
                    c2++;
            } else {
                merged[c1+c2] = (*list1)[c1];
                c1++;
            }  
            
        }
    }
    free(*list1);
    free(*list2);
    *list1 = merged;
}

void quicksort(int pivot_strat, int *grouplist,long **input,int **send, int rank,int cutoff, int nproctotal ){
    double med =234234234;
    int *start = grouplist;
    int *end = grouplist+ nproctotal/2;
    if(nproctotal == 1 ){
            return ;
        }

    double medians[nproctotal];
    //MPI_Barrier(MPI_COMM_WORLD);
    MPI_Request req[2];
    switch(pivot_strat) {
        case 1:
            if(rank == grouplist[0]){
               med = median(*input, (*send)[rank]);
                //printf("rank %d med: %f \n",rank, med );
               for(int i = 1; i<nproctotal; i++){
                    //if(i != rank)
                    MPI_Isend(&med, 1, MPI_DOUBLE,  grouplist[i], 1,MPI_COMM_WORLD, &req[0]);
               }
            } else {
                MPI_Recv(&med, 1, MPI_DOUBLE, grouplist[0], 1,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                //printf("rank %d med: %f \n",rank, med );
            }
        
            break;
        case 2: 
            
            med = median(*input, (*send)[rank]);
            if(rank == grouplist[0]){
                medians[0] = med;
                
                for(int i = 1; i<nproctotal; i++){
                    MPI_Recv(&medians[i], 1,MPI_DOUBLE, grouplist[i], 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //recv
                }
                med = medianAll(medians, nproctotal);
                for(int i = 1; i<nproctotal; i++){
                    MPI_Isend(&med, 1, MPI_DOUBLE,  grouplist[i], 2,MPI_COMM_WORLD, &req[0]);
                }
            } else {
                MPI_Isend(&med, 1, MPI_DOUBLE,  grouplist[0], 1,MPI_COMM_WORLD, &req[0]);
                MPI_Recv(&med, 1, MPI_DOUBLE, grouplist[0], 2,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            
            break;
        case 3: 
            med = median(*input, (*send)[rank]);
            if(rank == grouplist[0]){
                medians[0]= med;
                printf("first rank %d: median: %f",rank,med);
                for(int i = 1; i<nproctotal; i++){
                    MPI_Recv(&medians[i], 1,MPI_DOUBLE, grouplist[i], 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
                }
                med = meanAll(medians, nproctotal);
                printf("rank %d: median: %f",rank,med);
                for(int i = 1; i<nproctotal; i++){
                    MPI_Isend(&med, 1, MPI_DOUBLE,  grouplist[i], 2,MPI_COMM_WORLD, &req[0]);
                }
            } else {
                MPI_Isend(&med, 1, MPI_DOUBLE,  grouplist[0], 1,MPI_COMM_WORLD, &req[0]);
                MPI_Recv(&med, 1, MPI_DOUBLE, grouplist[0], 2,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            //if(rank == 5)
            //printf("rank %d: median: %f",rank,med);
            break;
        default :
           
            if(rank == grouplist[0]){
               med = median(*input, (*send)[rank]);

               for(int i = 1; i<nproctotal; i++){
                    MPI_Isend(&med, 1, MPI_DOUBLE,  grouplist[i], 1,MPI_COMM_WORLD, &req[0]);
               }
            } else {
                MPI_Recv(&med, 1, MPI_DOUBLE, grouplist[0], 1,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
        int piv = FindPivotIndex(*input, med, (*send)[rank]); 
        
        int tosend;
        int toRecv;
        long *recvbuf;
        if(rank <cutoff){ //send right
            if(piv != -1){ 
                tosend = (*send)[rank]-piv;
            } else {
                tosend = 0;
            }
            
            MPI_Isend(&tosend,1,MPI_INT, rank+nproctotal/2,1, MPI_COMM_WORLD,&req[0]);
            if(tosend != 0)
                MPI_Isend(&((*input)[piv]),tosend,MPI_LONG, rank+nproctotal/2,1, MPI_COMM_WORLD,&req[1]);
            MPI_Recv(&toRecv,1,MPI_INT, rank+nproctotal/2,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            if( rank == 4){
                printf("\npiv = %d Tosend: %d, torecv: %d sending to: %d, receving from %d \n",piv,tosend, toRecv, rank+nproctotal/2, rank+nproctotal/2);
                /* for(int i = 0; i<*send[rank]; i++){
                    printf(" %ld ", *input[i]);
                } */
            } 
            recvbuf = malloc(toRecv*sizeof(long));
            if(toRecv != 0)
                MPI_Recv(recvbuf,toRecv,MPI_LONG, rank+nproctotal/2,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                /* for(int i = 0; i<toRecv; i++){
                    printf("rank: %d, element %ld",rank, recvbuf[i]);
                } */
            if(tosend != 0)
            MPI_Wait(&req[1],MPI_STATUS_IGNORE);
            //MPI_Barrier(MPI_COMM_WORLD);
           
            merge(input, &recvbuf, (*send)[rank]-tosend,toRecv,0);
            
            
        } else { //Send left
            if(piv != -1){ 
                tosend = piv;
            } else {
                tosend = 0;
            }
            MPI_Isend(&tosend,1,MPI_INT, rank-nproctotal/2,1, MPI_COMM_WORLD,&req[0]);
            if(tosend != 0)
                MPI_Isend(*input,piv,MPI_LONG, rank-nproctotal/2,1, MPI_COMM_WORLD,&req[1]); //safeguard vs -1 piv
            MPI_Recv(&toRecv,1,MPI_INT, rank-nproctotal/2,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
             if( rank == 4){
                printf("\npiv %d Tosend: %d, torecv: %d sending to: %d, receving from %d \n",piv,tosend, toRecv, rank-nproctotal/2, rank-nproctotal/2);
                /* for(int i = 0; i<*send[rank]; i++){
                    printf(" %ld ", *input[i]);
                } */
            }
            recvbuf = malloc(toRecv*sizeof(long));
            if(toRecv != 0)
                MPI_Recv(recvbuf,toRecv,MPI_LONG, rank-nproctotal/2,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
           
            if(tosend != 0)
            MPI_Wait(&req[1],MPI_STATUS_IGNORE);
            /* for(int i = 0; i<toRecv; i++){
                    printf("rank: %d, element %ld",rank, recvbuf[i]);
            } */
            //MPI_Barrier(MPI_COMM_WORLD); //replace with MPI_Wait for req            
            merge(input, &recvbuf, (*send)[rank]-tosend,toRecv,tosend);
            
        } 
        //MPI_Barrier(MPI_COMM_WORLD);

       
        (*send)[rank] = (*send)[rank]-tosend+toRecv;
        if(rank < cutoff) {
            int cutoff2 = nproctotal/4; 
            quicksort(pivot_strat, start, input, send, rank, cutoff-cutoff2,nproctotal/2); //if below cutoff
        } else {
            int cutoff2 = nproctotal/4;        
            quicksort(pivot_strat,end, input, send, rank, cutoff+cutoff2,nproctotal/2);
        } 


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

    long *input; 
    int length;
    if(rank == 0) {
        if (0 > (length = read_input(input_name, &input))) {
			return 2;
		}
        //printf("\nPAST READ\n");
    } 
    
    MPI_Bcast(&length, 1,MPI_INT, 0, MPI_COMM_WORLD);
    
    double start = MPI_Wtime();
    
    int part = length/nproc;
    int remainder = length % nproc; //For non divisible list lengths. 
    int dsp[nproc]; //used for number of sends and the displacement.
    int index = 0;
    int *send = malloc(nproc*sizeof(int));
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
        input = malloc(send[rank] *sizeof(long));
    }
    
    MPI_Scatterv(input, send, dsp, MPI_LONG,input ,send[rank], MPI_LONG, 0, MPI_COMM_WORLD);

    qsort(input,send[rank], sizeof(long),comp);
    
    //pivot element
    double median;

    int grouplist[nproc];
    for(int i = 0; i<nproc; i++)
        grouplist[i] = i;

    quicksort(pivot_strat,grouplist, &input, &send,rank,nproc/2,nproc);
    
    index = 0;
    
    long *gatherarr;
    if(rank == 0) gatherarr = malloc(length*sizeof(long));
    
    MPI_Gather(&send[rank], 1,MPI_INT, send, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if(rank == 0) {
        for(int i =0; i<nproc; i++){
            dsp[i] = index;
            index = index + send[i];
        }
        MPI_Gatherv(input, send[rank], MPI_LONG,gatherarr, send,dsp,MPI_LONG,0, MPI_COMM_WORLD );
    } else {
        MPI_Gatherv(input, send[rank], MPI_LONG,NULL, NULL,NULL,MPI_LONG,0, MPI_COMM_WORLD );
    }
    
    double my_execution_time = MPI_Wtime() - start; //use mpi reduce
    
    
    if(rank == 0) {
        printf("%f\n", my_execution_time);
    }
    #ifdef PRODUCE_OUTPUT_FILE
        if(rank == 0){
            write_output(output_name,gatherarr, length);
        }
    #endif
    free(input);
    free(send);
    if(rank == 0) free(gatherarr);
    MPI_Finalize();
    return 0;
}






/*  if(rank == 0)
        for(int i = 0; i<length; i++)
            printf(" %d ", input[i]); */





























