/*
 * compile: mpic++ mergeBitonic.cpp
 * run: mpirun -n p ./a.out num
 * bitonicsort.cpp: main parallel implementation of bitonic sort
 * note: p and num should be power of 2.
 * doesn't work for 1 processor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mpi.h"

int numTasks, rank;

int min(int a, int b) {
  return ((a < b) ? a : b);
}

int max(int a, int b) {
  return ((a > b) ? a : b);
}

// return 1 if n is a power of 2, 0 otherwise.
int isPow2(int n) {
  while (n > 0) {
    if (n % 2 == 1 && n / 2 != 0) return 0;
    n /= 2;
  }
  return 1;
}

// return next highest power of 2 if it is not.
int nextPow2(int n) {
  if (isPow2(n)) return n;
  if (n == 0) return 1;

  int log = 0;
  while (n > 0) {
    log++;
    n /= 2;
  }
  n = 1;
  for(int i=0; i<log; i++)
    n *= 2;
  return n;
}

// returns array with random numbers
int *createNumbers(int howMany) {
  int * numbers = (int *) malloc(sizeof(int) * howMany);
  if (numbers == NULL) {
    printf("Error: malloc failed.\n");
    return NULL;
  }
  srand(time(NULL) & rank);
  for(int i=0; i < howMany; i++)
    numbers[i] = rand();
  return numbers;
}

// print array
void printNumbers(int * numbers, int howMany) {
  printf("\n");
  for(int i=0; i < howMany; i++)
    printf("%d\n", numbers[i]);
  printf("\n");
}

// check if array sorted in increasing order
int isSorted(int *numbers, int howMany) {
  for(int i=1; i<howMany; i++) {
    if (numbers[i] < numbers[i-1]) return 0;
  }
  return 1;
}

int compareDescending(const void *item1, const void *item2) {
  int x = * ( (int *) item1), y = * ( (int *) item2);
  return y-x;
}

int compareAscending(const void *item1, const void *item2) {
  int x = * ( (int *) item1), y = * ( (int *) item2);
  return x-y;
}

int *tempArray;

void compareExchange(int *numbers, int howMany, int node1, int node2, int biggerFirst, int sequenceNo) {
  if (node1 != rank && node2 != rank) return;

  memcpy(tempArray, numbers, howMany*sizeof(int));

  MPI_Status status;
  // get numbers from the other node.
  // have the process that is node1 send first and receive first
  int nodeFrom = node1==rank ? node2 : node1;
  if (node1 == rank) {
    MPI_Send(numbers, howMany, MPI_INT, nodeFrom, sequenceNo, MPI_COMM_WORLD);
    MPI_Recv(&tempArray[howMany], howMany, MPI_INT, nodeFrom, sequenceNo,
	     MPI_COMM_WORLD, &status);
  }
  else {
    MPI_Recv(&tempArray[howMany], howMany, MPI_INT, nodeFrom, sequenceNo,
	     MPI_COMM_WORLD, &status);
    MPI_Send(numbers, howMany, MPI_INT, nodeFrom, sequenceNo, MPI_COMM_WORLD);
  }

  // sort them.
  if (biggerFirst) {
    qsort(tempArray, howMany*2, sizeof(int), compareDescending);
  }
  else {
    qsort(tempArray, howMany*2, sizeof(int), compareAscending);
  }

  // keep only half of them.
  if (node1 == rank)
    memcpy(numbers, tempArray, howMany*sizeof(int));
  else
    memcpy(numbers, &tempArray[howMany], howMany*sizeof(int));
}

// performs bitonic merge sort.
void mergeBitonic(int *numbers, int howMany) {
  tempArray = (int *) malloc(sizeof(int) * howMany * 2);
  int log = numTasks;
  int pow2i = 2;
  int sequenceNumber = 0;

  for(int i=1; log > 1 ; i++) {
    int pow2j = pow2i;
    for(int j=i; j >= 1; j--) {
      sequenceNumber++;
      for(int node=0; node < numTasks; node += pow2j) {
	for(int k=0; k < pow2j/2; k++) {
	  //printf("i=%d, j=%d, node=%d, k=%d, pow2i=%d, pow2j=%d\n",
	  // i, j, node, k, pow2i, pow2j);
	  compareExchange(numbers, howMany, node+k, node+k+pow2j/2,
			  ((node+k) % (pow2i*2) >= pow2i),
			  sequenceNumber);
	}
      }
      pow2j /= 2;
      //      printf(" after substage %d", j);
      //      printNumbers(numbers, howMany);
    }
    pow2i *= 2;
    log /= 2;
    //    printf("after stage %d\n", i);
    //    printNumbers(numbers, howMany);
  }
  free(tempArray);
}

int main(int argc, char *argv[]) {
  int howMany;
  long int returnVal;
  int len;
  char hostname[MPI_MAX_PROCESSOR_NAME];

  // initialize
  returnVal = MPI_Init(&argc, &argv);
  if (returnVal != MPI_SUCCESS) {
    printf("Error starting MPI program. Terminating\n");
    MPI_Abort(MPI_COMM_WORLD, returnVal);
    return 0;
  }

  MPI_Comm_size(MPI_COMM_WORLD, &numTasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Get_processor_name(hostname, &len);

  if (argc < 2) {
    if (rank == 0)
      printf("Usage: ./a.out <number of elements to sort>\n");
    MPI_Finalize();
    return 0;
  }

  howMany = atoi(argv[1]);
  howMany = nextPow2(howMany);

  if (!isPow2(numTasks)) {
    if (rank == 0)
      printf("Number of processes must be power of 2.\n");
    MPI_Finalize();
    return 0;
  }

  // each process creates a list of random numbers.
  int * numbers = createNumbers(howMany);
  // printNumbers(numbers, howMany);
  mergeBitonic(numbers, howMany);
  // printNumbers(numbers, howMany);

  // they are all sorted, now just gather them up.
  int * allNumbers = NULL;
  if (rank == 0) {
    allNumbers = (int *) malloc(howMany * numTasks * sizeof(int));
  }
  MPI_Gather(numbers, howMany, MPI_INT, allNumbers, howMany, MPI_INT, 0, MPI_COMM_WORLD);
  if (rank == 0) {
    if (isSorted(allNumbers, howMany * numTasks))
      printf("Successfully sorted!\n");
    else
      printf("Error: numbers not sorted.\n");

    free(allNumbers);
  }

  free(numbers);
  MPI_Finalize();
  return 0;
}
