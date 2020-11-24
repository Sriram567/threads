#define _POSIX_C_SOURCE 199309L //required for clock
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>

struct arg{
	int l;
	int r;
	int* arr;
};
struct mergearg{
	int l;
	int m;
	int r;

	int* arr;
};
int * shareMem(size_t size){
	key_t mem_key = IPC_PRIVATE;
	int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
	return (int*)shmat(shm_id, NULL, 0);
}


void swap(int *xp, int *yp)
{
	int temp = *xp;
	*xp = *yp;
	*yp = temp;
}

void merge(int arr[], int l, int m, int r) 
{ 
	int n1 = m - l + 1; 
	int n2 = r - m; 

	// Create temp arrays  
	int L[n1], R[n2]; 

	// Copy data to temp arrays L[] and R[]  
	for(int i = 0; i < n1; i++) 
		L[i] = arr[l + i]; 
	for(int j = 0; j < n2; j++) 
		R[j] = arr[m + 1 + j]; 

	// Merge the temp arrays back into arr[l..r] 

	// Initial index of first subarray 
	int i = 0;  

	// Initial index of second subarray 
	int j = 0;  

	// Initial index of merged subarray 
	int k = l; 

	while (i < n1 && j < n2) 
	{ 
		if (L[i] <= R[j])  
		{ 
			arr[k] = L[i]; 
			i++; 
		} 
		else 
		{ 
			arr[k] = R[j]; 
			j++; 
		} 
		k++; 
	} 

	// Copy the remaining elements of 
	// L[], if there are any  
	while (i < n1)  
	{ 
		arr[k] = L[i]; 
		i++; 
		k++; 
	} 

	// Copy the remaining elements of 
	// R[], if there are any  
	while (j < n2) 
	{ 
		arr[k] = R[j]; 
		j++; 
		k++; 
	} 
}

void mergeSort(int arr[], int l, int r)
{

	if (l < r)
	{
		if(r-l<5)
		{
			int i, j, min_idx;

			// One by one move boundary of unsorted subarray
			for (i = l; i < r; i++)
			{
				// Find the minimum element in unsorted array
				min_idx = i;
				for (j = i+1; j < r+1; j++)
					if (arr[j] < arr[min_idx])
						min_idx = j;

				// Swap the found minimum element with the first element
				swap(&arr[min_idx], &arr[i]);

			}
		}
		else
		{

			// Same as (l+r)/2, but avoids
			// overflow for large l and h
			int m = l + (r - l) / 2;

			// Sort first and second halves
			mergeSort(arr, l, m);
			mergeSort(arr, m + 1, r);
			// wait until both are completed

			merge(arr, l, m, r);
		}

	}
}

void forkmergeSort( int * arr , int l , int r)
{
	if(l<r)
	{
		if(r-l<5)
		{
			int i, j, min_idx;

			// One by one move boundary of unsorted subarray
			for (i = l; i < r; i++)
			{
				// Find the minimum element in unsorted array
				min_idx = i;
				for (j = i+1; j < r+1; j++)
					if (arr[j] < arr[min_idx])
						min_idx = j;

				// Swap the found minimum element with the first element
				swap(&arr[min_idx], &arr[i]);

			}
		}

		else
		{
			int m = l + (r - l) / 2;

			int pid1=fork();
			int pid2;
			if(pid1==0){
				//sort the left half of the array
				forkmergeSort(arr, l, m);
				_exit(1);
			}
			else{
				pid2=fork();
				if(pid2==0){
					// sort the right half of the array
					forkmergeSort(arr, m+1, r);
					_exit(1);
				}
				else{
					// wait until both are completed
					int status;
					waitpid(pid1, &status, 0);
					waitpid(pid2, &status, 0);
					merge(arr, l , m , r); // call merge when both the process get exited
				}
			}
			return;

		}
	}

}
void *threaded_mergeSort(void *a)
{
	struct arg *args =  (struct arg*)a;

	int l = args->l;
	int r = args->r;
	int *arr = args->arr;
	if(l>r) return NULL;

	if( l < r )
	{
		if(r-l<5)
		{

			int i, j, min_idx;

			// One by one move boundary of unsorted subarray
			for (i = l; i < r; i++)
			{
				// Find the minimum element in unsorted array
				min_idx = i;
				for (j = i+1; j < r+1; j++)
					if (arr[j] < arr[min_idx])
						min_idx = j;

				// Swap the found minimum element with the first element
				swap(&arr[min_idx], &arr[i]);
			}

		}
		else
		{
			int m = l + (r - l) / 2;
			// sort the left half of the array
			struct arg a1;
			a1.l = l;
			a1.r = m;
			a1.arr = arr;
			pthread_t tid1;
			pthread_create(&tid1, NULL, threaded_mergeSort, &a1);

			//sort right half array
			struct arg a2;
			a2.l = m+1;
			a2.r = r;
			a2.arr = arr;
			pthread_t tid2;
			pthread_create(&tid2, NULL, threaded_mergeSort, &a2);

			//wait for the two halves to get sorted
			pthread_join(tid1, NULL);
			pthread_join(tid2, NULL);

			merge(arr , l , m ,r); // call merge when both the threads join back the main thread.
			
		}
	}


}	

void runSorts(long long int n){

	struct timespec ts;

	//getting shared memory
	int *arr = shareMem(sizeof(int)*(n+1));
	for(int i=0;i<n;i++) scanf("%d", arr+i);

	int brr[n+1];
	for(int i=0;i<n;i++) brr[i] = arr[i];
	printf("Running concurrent_mergesort for n = %lld\n", n);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	long double st = ts.tv_nsec/(1e9)+ts.tv_sec;

	//multiprocess mergesort
	forkmergeSort(arr, 0, n-1);
	for(int i=0; i<n; i++){
		printf("%d ",arr[i]);
	}
	printf("\n");
	//accessing the clock
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
	printf("time = %Lf\n", en - st);
	long double t1 = en-st;

	for(int i=0;i<n;i++) brr[i] = arr[i];

	pthread_t tid;
	struct arg a;
	a.l = 0;
	a.r = n-1;
	a.arr = brr;
	printf("Running threaded_concurrent_mergesort for n = %lld\n", n);
	//accessing the clock
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	st = ts.tv_nsec/(1e9)+ts.tv_sec;

	//multithreaded mergesort
	pthread_create(&tid, NULL, threaded_mergeSort, &a);
	pthread_join(tid, NULL);
	for(int i=0; i<n; i++){
		printf("%d ",a.arr[i]);
	}
	printf("\n");
	//accessing the clock
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	en = ts.tv_nsec/(1e9)+ts.tv_sec;
	printf("time = %Lf\n", en - st);
	long double t2 = en-st;


	printf("Running normal_Mergesort for n = %lld\n", n);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	st = ts.tv_nsec/(1e9)+ts.tv_sec;

	for(int i=0;i<n;i++) brr[i] = arr[i];
	// normal mergesort
	mergeSort(brr, 0, n-1);
	for(int i=0; i<n; i++){
		printf("%d ",brr[i]);
	}
	printf("\n");
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	en = ts.tv_nsec/(1e9)+ts.tv_sec;
	printf("time = %Lf\n", en - st);
	long double t3 = en - st;

	printf("normal_MergeSort ran:\n\t[ %Lf ] times faster than concurrent_Mergesort\n\t[ %Lf ] times faster than threaded_concurrent_Mergesort\n", t1/t3, t2/t3);
	shmdt(arr);
	return;


}

int main()
{
	long long int n;
	printf("Enter the value of n: ");
	scanf("%lld",&n);
	runSorts(n);
	return 0;

}
