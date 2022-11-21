// COMP3230 Programming Assignment Two
// The sequential version of the sorting using qsort

/*
# Filename: 
# Student name and No.:
# Development platform:
# Remark:
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

int checking(unsigned int *, long);
int compare(const void *, const void *);

// global variables
long size; 
long num_workers;  // size of the array
unsigned int * intarr; // array of random integers

int main2 (int argc, char **argv)
{
  long i, j;
  struct timeval start, end;

  if ((argc <= 2))
  {
    printf("Usage: p_sort <number> [<no_of_workers>]\n");
    exit(0);
  }

  if ((argc == 3)){
    num_workers = atol(argv[2]);
    if (num_workers <= 1){
      printf("Error: Worker thread count must be greater than 1\n");
      exit(0);
    }
  }
  size = atol(argv[1]);
  intarr = (unsigned int *)malloc(size*sizeof(unsigned int));
  if (intarr == NULL) {perror("malloc"); exit(0); }
  
  // set the random seed for generating a fixed random
  // sequence across different runs
  char * env = getenv("RANNUM");  //get the env variable
  if (!env)                       //if not exists
    srandom(3230);
  else
    srandom(atol(env));
  
  for (i=0; i<size; i++) {
    intarr[i] = random();
  }
  
  // measure the start time
  gettimeofday(&start, NULL);
  
  // just call the qsort library
  // replace qsort by your parallel sorting algorithm using pthread
  qsort(intarr, size, sizeof(unsigned int), compare);

  // measure the end time
  gettimeofday(&end, NULL);
  
  if (!checking(intarr, size)) {
    printf("The array is not in sorted order!!\n");
  }
  
  printf("Total elapsed time: %.4f s\n", (end.tv_sec - start.tv_sec)*1.0 + (end.tv_usec - start.tv_usec)/1000000.0);
    
  free(intarr);
  return 0;
}

int compare(const void * a, const void * b) {
  return (*(unsigned int *)a>*(unsigned int *)b) ? 1 : ((*(unsigned int *)a==*(unsigned int *)b) ? 0 : -1);
}

int checking(unsigned int * list, long size) {
  long i;
  printf("First : %d\n", list[0]);
  printf("At 25%%: %d\n", list[size/4]);
  printf("At 50%%: %d\n", list[size/2]);
  printf("At 75%%: %d\n", list[3*size/4]);
  printf("Last  : %d\n", list[size-1]);
  for (i=0; i<size-1; i++) {
    if (list[i] > list[i+1]) {
      return 0;
    }
  }
  return 1;
}



int arr[] = {42, 98, 2, 31, 86, 87, 5, 13, 99, 44, 67, 37, 17, 7, 87, 3, 96, 71, 40, 19, 58, 13, 61, 77, 11, 13, 6, 81, 76, 18, 24, 14, 63, 59, 99, 17, 36, 84, 1, 48};
int size2 = 40;
const num_threads = 4;

// struct that keeps track of start and stop data for a thread. Used for sorting subarrays
typedef struct start_stop_struct {
    long start_idx;
    long len;
    long pivot_ctr;
    long *pivots[num_threads];
} start_stop_struct;

void *sort_subarr(void* arg){
  // parsing argument given into struct
  start_stop_struct *s = (start_stop_struct*)arg;
  printf("Start sorting at %ld!\n", s->start_idx);
  // sort array of size length starting from index
  qsort(arr + s->start_idx, s->len, sizeof(int), compare);

  for (int i = 0; i< num_threads; i++){
    s->pivots[i] = arr[s->start_idx + (i*size2)/(num_threads*num_threads)];
  }
  pthread_exit(s);
}

int main(){

  pthread_t th[num_threads];
  start_stop_struct* st = malloc(num_threads * sizeof(start_stop_struct));

  for (int i = 0; i < num_threads; i++){
    st[i].start_idx = i*(size2/num_threads);
    st[i].pivot_ctr = num_threads;
    if ((i == num_threads - 1) && (size2 % num_threads != 0)){
      st[i].len = size2 - st[i].start_idx;
      printf("%d: len = %d\n", i, st[i].len);
    }
    else{
      st[i].len = (size2/num_threads);
    }
  }

  // create buckets and sort each bucket
  for (int i = 0; i < num_threads; i++){
    pthread_create(&th[i], NULL, &sort_subarr, &st[i]);
  }
  // wait for buckets to sort. Collect pivots in the struct
  for (int i =0; i < num_threads; i++){
    pthread_join(th[i], &st[i]);
  }

  for (int i = 0; i< num_threads ;i++){
    printf("Thread %d pivot values\n", i);
    for (int j =0; j<num_threads; j++){
      printf("Pivot %d: %ld\n", j, st[i].pivots[j]);
    }
  }

  // sort pivots
  long pivot_combined[num_threads*num_threads];

  // save pivots into combined array
  for (int i = 0; i < num_threads; i++){
    for(int j=0; j<num_threads;j++){
      pivot_combined[i*num_threads + j] = st[i].pivots[j];
    }
  }
  // sort pivots in main
  qsort(pivot_combined, num_threads*num_threads, sizeof(long), compare);
  printf("Sorted pivots: ");
  for (int i=0;i<num_threads*num_threads;i++){
    printf("%ld ", pivot_combined[i]);
  }
  printf("\n");

  if (!checking(arr, (size2/num_threads))) {
    printf("The array is not in sorted order!!\n");
  }
  else{
    printf("SORTED!\n");
  }
  // for(int i = 0; i < size2; i++){
  //   printf("%d: %d\n", i, arr[i]);
  // }

  free(st);
  return 0;
}