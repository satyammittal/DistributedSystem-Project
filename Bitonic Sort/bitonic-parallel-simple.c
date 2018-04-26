/*
 * compile: mpic++ bitonic-parallel-simple.c
 * run: mpirun -n p ./a.out num
 * bitonic-parallel-simple.c: Parallel implementation of Bitonic Sort where each process applies bitonic sort
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#define SWAP(x,y) t = x; x = y; y = t;
void merge(int *, int *, int, int, int);
void mergeSort(int *, int *, int, int);
int up = 1;
int down = 0;
int* compare(int *a, int *b, int i, int j, int dir)
{
    int t;
    if (dir == (a[i] > a[j]))
    {
        SWAP(a[i], a[j]);
    }
    return a;
}
/*
 * Sorts a bitonic sequence in ascending order if dir=1
 * otherwise in descending order
 */

void bitonicmerge(int *a, int *b, int low, int c, int dir)
{
    int k, i;
    if (c > 1)
    {
         k = c / 2;
        for (i = low;i < low+k ;i++)
            a=compare(a,b,i, i+k, dir);    
        bitonicmerge(a,b,low, k, dir);
        bitonicmerge(a,b,low+k, k, dir);    
    }
}
/*
 * Generates bitonic sequence by sorting recursively
 * two halves of the array in opposite sorting orders
 * bitonicmerge will merge the resultant data
 */
void recbitonic(int *a, int *b,int low, int c, int dir)
{
    int k;
    if (c > 1)
    {
        k = c / 2;
        recbitonic(a,b,low, k, up);
        recbitonic(a,b,low + k, k, down);
        bitonicmerge(a,b,low, c, dir);
    }
}
int main(int argc, char** argv) {
	
	int world_rank;
	int world_size;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	

	int n = atoi(argv[1]);
	int *original_array = malloc(n * sizeof(int));
	int c;
	srand(time(NULL));
	if (world_rank == 0)
	{
	printf("This is the unsorted array: ");
	for(c = 0; c < n; c++) {
		original_array[c] = rand() % n;
		printf("%d ", original_array[c]);
		}

	printf("\n");
	printf("\n");
	}
		
	int size = n/world_size;
	
	int *sub_array = malloc(size * sizeof(int));
	MPI_Scatter(original_array, size, MPI_INT, sub_array, size, MPI_INT, 0, MPI_COMM_WORLD);
	
	int *tmp_array = malloc(size * sizeof(int));
	recbitonic(sub_array, tmp_array, 0, size, up);
	
	int *sorted = NULL;
	if(world_rank == 0) {
		
		sorted = malloc(n * sizeof(int));
		
		}
	
	MPI_Gather(sub_array, size, MPI_INT, sorted, size, MPI_INT, 0, MPI_COMM_WORLD);
	
	if(world_rank == 0) {
		
		int *other_array = malloc(n * sizeof(int));
		recbitonic(sorted, other_array, 0, n, up );
		
		printf("This is the sorted array: ");
		for(c = 0; c < n; c++) {
			
			printf("%d ", sorted[c]);
			
			}
			
		printf("\n");
		printf("\n");
			
		free(sorted);
		free(other_array);
			
		}
	
	free(original_array);
	free(sub_array);
	free(tmp_array);
	
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	
}
