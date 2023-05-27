#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


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
double median(long *values, int length){
    qsort(values,length, sizeof(long),comp);
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

double mean(long *values, int length) {
    int sum = 0;
    for(int i = 0; i< length; i++){
        sum += values[i];
    }
    return sum/length;
}

//Used to find index of pivot to split list.
int FindPivotIndex(long *list, double value, int length){
/*     if(value <= list[0]){
        return 0;
    } */
    for(int i = 0; i<length; i++){
        if(list[i]>=value) {
            return i;
        }
    }
    return 0; //No element larger than pivot found
}


void* merge(long **list1, long **list2, int length1,int length2,int start){
    int c1,c2 = 0;
    //printf("length1: %d, length 2: %d \n", length1, length2);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   /*  if(rank == 2) {
        printf(" \n");
        for(int i =0; i<length1; i++)
        printf(" %ld ", (*list1)[i]);
        for(int i =0; i<length2; i++)
        printf(" %ld ", (*list2)[i]);
    } */
    long * merged;
    if(start != 0) {
        merged = malloc((length1+length2) *sizeof(long));
        while(c1+c2 < length1+length2){
            if(c1 != length1 && c2 != length2){
                if ((*list1)[c1+start]<(*list2)[c2])
                {
                    merged[c1+c2] = (*list1)[c1+start];
                    //printf("c1: %d\n",*list1[0]);
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
        while(c1+c2 < length1+length2){
            if(c1 != length1 && c2 != length2){
                if ((*list1)[c1]<(*list2)[c2])
                {
                    merged[c1+c2] = (*list1)[c1];
                    //printf("c1: %d\n",*list1[0]);
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
    /* if(rank == 2) {
        printf(" \n");
        for(int i =0; i<length1+length2; i++)
        printf(" %ld ", merged[i]);
    } */
    free(*list1);
    free(*list2);
    *list1 = merged;
    //printf("c1: %d\n",*list1[0]);
}

long int *quicksort(int pivot_strat, int *grouplist,long **input,int **send, int rank,int cutoff, int nproctotal ){
    double med =234234234;
    int *start = grouplist;
    int *end = grouplist+ nproctotal/2;
    if(rank == 7){
                    printf("\n start:  ");
                    for(int i = 0; i<*send[rank]; i++){
                            printf(" %ld ", *input[i]);
                    }
                    printf("\n end  \n"); 
                }
              /*   if(rank ==0){
        printf("rank: %d ",rank );
            for(int i =0; i<send[rank]; i++)
            printf(" %ld ", input[i]);
    } */
    if(nproctotal == 1 ){
        /* printf("after :rank: %d ",rank);
            for(int i =0; i<send[rank]; i++)
            printf(" %ld ", input[i]);
            printf("\n"); */
        

        return 0;
    }
    MPI_Request req[2];
    switch(pivot_strat) {
        case 1:
            if(rank == grouplist[0]){
               med = median(*input, *send[rank]);
               //if(rank == 0){
                 //printf("\n rank %d, send: %d, median: %f \n",rank,send[rank],med);
                /* for(int i = 0; i<send[rank]; i++){
                        printf(" %ld ", input[i]);
                }   */
               //}
               for(int i = 1; i<nproctotal; i++){
                    MPI_Isend(&med, 1, MPI_DOUBLE,  grouplist[i], 1,MPI_COMM_WORLD, &req[0]);
               }
            } else {
                MPI_Recv(&med, 1, MPI_DOUBLE, grouplist[0], 1,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            
            
            break;
        case 2: 
            //medianAll; //median of all processes in group
            break;
        case 3: 
            //mean; //create mean
            break;
        }
        int piv = FindPivotIndex(*input, med, *send[rank]); //use for list split. TODO IMPLEMENT LATER
        //int cutoff = nproc;
        int tosend;// = send[rank]-piv;
        int toRecv;
        long *recvbuf;
        
        
        /*  if(rank ==1){
            printf("rank: %d, %d ",rank,toRecv );
            for(int i =0; i<send[rank]; i++)
            printf(" %ld ", input[i]);
        } */
        if(rank <cutoff){ //send right
            tosend = *send[rank]-piv;
            MPI_Isend(&tosend,1,MPI_INT, rank+nproctotal/2,1, MPI_COMM_WORLD,&req[0]);
            MPI_Isend(&(*input)[piv],tosend,MPI_LONG, rank+nproctotal/2,1, MPI_COMM_WORLD,&req[1]);

            MPI_Recv(&toRecv,1,MPI_INT, rank+nproctotal/2,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            recvbuf = malloc(toRecv*sizeof(long));
            MPI_Recv(recvbuf,toRecv,MPI_LONG, rank+nproctotal/2,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            /* if(rank ==2) {
                printf("\n rank : %d\n", rank);
                for(int i = 0; i<toRecv; i++){
                    printf("recieving: %ld", recvbuf[i]);
                }
            } */
            merge(&(*input), &recvbuf, *send[rank]-tosend,toRecv,0);
        } else { //Send left
            tosend = piv;
            MPI_Isend(&tosend,1,MPI_INT, rank-nproctotal/2,1, MPI_COMM_WORLD,&req[0]);
            MPI_Isend(*input,piv,MPI_LONG, rank-nproctotal/2,1, MPI_COMM_WORLD,&req[1]);
            MPI_Recv(&toRecv,1,MPI_INT, rank-nproctotal/2,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            recvbuf = malloc(toRecv*sizeof(long));
            MPI_Recv(recvbuf,toRecv,MPI_LONG, rank-nproctotal/2,1, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            /* if(rank ==2) {
            printf("\n rank : %d\n", rank);
            for(int i = 0; i<toRecv; i++){
                    printf("recieving: %ld", recvbuf[i]);
                }
            } */
            merge(&(*input), &recvbuf, *send[rank]-tosend,toRecv,piv);
        } 

        /* if(rank ==3){
        printf("\nbefore: rank: %d,send %d,recv %d, sent %d \n",rank,send[rank]-tosend,toRecv,tosend );
            for(int i =0; i<send[rank]; i++)
            printf(" %ld ", input[i]);
            printf("\n");
        } */
       
        *send[rank] = *send[rank]-tosend+toRecv;
        /* if(rank ==3){
         printf("after :rank: %d \n",rank  );
            for(int i =0; i<send[rank]; i++)
            printf(" %ld ", input[i]);
            printf("\n");
        } */
        //send[rank] = send[rank]-tosend+toRecv;
        if(rank < cutoff) {
            int cutoff2 = nproctotal/4; 
            quicksort(pivot_strat, start, &(*input), &(*send), rank, cutoff-cutoff2,nproctotal/2); //if below cutoff
        } else {
            int cutoff2 = nproctotal/4;          
            quicksort(pivot_strat,end, &(*input), &(*send), rank, cutoff+cutoff2,nproctotal/2);
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
    } 

    MPI_Bcast(&length, 1,MPI_INT, 0, MPI_COMM_WORLD);
    int part = length/nproc;
    int remainder = length % nproc; //For non divisible list lengths. 
    int *send, dsp[nproc]; //used for number of sends and the displacement.
    int index = 0;
    send = malloc(nproc*(sizeof(int)));
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
    /* if(rank ==0){
        printf("rank: %d ",rank );
            for(int i =0; i<send[rank]; i++)
            printf(" %d ", input[i]);
    } */
    //pivot elemental
    double median;

    int grouplist[nproc];
    for(int i = 0; i<nproc; i++)
        grouplist[i] = i;

    quicksort(pivot_strat,grouplist, &input, &send,rank,nproc/2,nproc);
    /* for(int i = 1; i<nproc; i++)[
        dsp[i] = send[rank]
    ] */
    if(rank ==7){
        printf("rank: %d ",rank );
            //for(int i =0; i<send[rank]; i++) printf(" %ld input", input[0]);
            //printf(" %ld ", input[i]);
    }
    long *gatherarr = malloc(length*sizeof(long));
    //MPI_Gather(gatherarr, send[rank], MPI_DOUBLE, send[rank], )
    MPI_Finalize();
    //FREE INPUT
    return 0;
}






/*  if(rank == 0)
        for(int i = 0; i<length; i++)
            printf(" %d ", input[i]); */





























