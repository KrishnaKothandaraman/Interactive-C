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



// global variables
long size = 40; 
long num_workers;  // size of the array
unsigned int * intarr; // array of random integers

int arr[] = {42, 98, 2, 31, 86, 87, 5, 13, 99, 44, 67, 37, 17, 7, 87, 3, 96, 71, 40, 19, 58, 13, 61, 77, 11, 13, 6, 81, 76, 18, 24, 14, 63, 59, 99, 17, 36, 84, 1, 48};
const num_threads = 4;
const Mxn = 1e8;
// every thread will push it's partitions here
long *global_partitions[num_threads][num_threads];
// keeps track of the length of each partition for each thread
long thread_partition_counter[num_threads][num_threads];
// tracks how many threads have pushed their partitions. Used for cond_wait
int partiton_counter[num_threads];
pthread_mutex_t mutex;
pthread_cond_t condPartition;

// struct that keeps track of start and stop data for a thread. Used for sorting subarrays
typedef struct start_stop_struct {
    // keeps track of which thread this is for partitioning purposes
    int *thread_count;
    // start idx of where its going to sort
    long start_idx;
    // how many it's going to sort
    long len;
    // samples after phase 1
    long *samples[num_threads];
    // pivots after phase 2
    long *pivots[num_threads - 1];
    // stores local partitions of a thread
    long *local_partitions[num_threads][Mxn];
    // combined partitions from each thread
    long combined_partitions[Mxn]; 
    // stores length of combined partition
    long combined_partition_length;
} start_stop_struct;

void *phase3_and_phase4(void* arg){
  start_stop_struct *s = (start_stop_struct*)arg;
  // stores local partitions
  long *local_partitions[num_threads][size/num_threads];
  int start = 0;
  int j;
  int thread_id = (s->start_idx)/(size/num_threads);

  printf("Thread %d running routine start idx %ld len %ld\n", thread_id ,s->start_idx, s->len);
  for (int i = 0; i < num_threads - 1; i++){
    j = start;
    while (j < 10 && (arr[s->start_idx + j] <= s->pivots[i])){
      j++;
    }
    printf("Found partition start: %d end %d\n", start, j-1);
    // set partition length of thread = num of elements in partition found
    thread_partition_counter[thread_id][i] = j - start;
    printf("Setting %d %d thread partition counter to %d\n", thread_id, i, j - start);
    for (int l = start; l <= j - 1; l++){
      printf("Setting index %d of local partition row %d = %ld\n", l - start, i, arr[s->start_idx + l]);
      s->local_partitions[i][l - start] = arr[s->start_idx + l];
    }
    start = j;
    }

    // set partition length of thread = num of elements in partition found
    printf("Setting %d %d thread partition counter to %d\n", thread_id, num_threads - 1, s->len - start);
    thread_partition_counter[thread_id][num_threads - 1] = s->len - start;
    printf("Found partition start: %d end %d\n", start, s->len - 1);
    for (int l = start; l <= s->len - 1; l++){
      printf("Setting index %d of local partition row %d = %ld\n", l - start, num_threads - 1, arr[s->start_idx + l]);
      s->local_partitions[num_threads - 1][l - start] = arr[s->start_idx + l];
    }

  pthread_mutex_lock(&mutex);
  for (int i = 0; i < num_threads; i++){
    // printf("Thread %d trying for lock!\n", (s->start_idx)/(size2/num_threads));
    // printf("Thread %d got lock!\n", (s->start_idx)/(size2/num_threads));
    // push ith partition to thread_count position of ith global partition row
    global_partitions[i][thread_id] = s->local_partitions[i];
    partiton_counter[i]++;
    pthread_cond_broadcast(&condPartition);
    // printf("Global partitions %d %d = %p\n", i, thread_id, local_partitions[i]);
    printf("Set global partitions %d %d\n", i, thread_id);
    // printf("Thread %d released lock!\n", (s->start_idx)/(size2/num_threads));
  }

  while (partiton_counter[thread_id] != 4){
    printf("Thread %d Waiting on partitioning to finish\n", thread_id);
    pthread_cond_wait(&condPartition, &mutex);
  }
  printf("Thread %d partitioning done!\n", thread_id);

  // calculate length of combined partitions
  for (int i = 0; i < num_threads; i++){
    s->combined_partition_length += thread_partition_counter[i][thread_id];
  }
  printf("Thread %d combined length %ld\n", thread_id, s->combined_partition_length);
  // buffer to store combined partitions
  // keeps track of end of combined partitions so far
  int partition_ctr = 0;
  for (int i = 0; i < num_threads ; i++){
    printf("Trying to access global_partitions %d %d\n", thread_id, i);
    long *part = global_partitions[thread_id][i];
    for (int k = 0; k < thread_partition_counter[i][thread_id]; k++){
      s->combined_partitions[partition_ctr] = part[k];
      partition_ctr++;
    }
  }
  qsort(s->combined_partitions, s->combined_partition_length, sizeof(long), compare);
  printf("Thread %d\n", thread_id);
  for (int i =0;  i< partition_ctr ; i++){
    printf("%ld ", s->combined_partitions[i]);
  }
  printf("\n");

  pthread_mutex_unlock(&mutex);

  pthread_exit(s);

}

void *sort_subarr(void* arg){
  // parsing argument given into struct
  start_stop_struct *s = (start_stop_struct*)arg;
  printf("Thread %d Start sorting at %ld!\n", s->thread_count ,s->start_idx);
  // sort array of size length starting from index
  qsort(arr + s->start_idx, s->len, sizeof(int), compare);

  for (int i = 0; i< num_threads; i++){
    s->samples[i] = arr[s->start_idx + (i*size)/(num_threads*num_threads)];
  }
  pthread_exit(s);
}

int main(){

  pthread_t th[num_threads];
  start_stop_struct* st = malloc(num_threads * sizeof(start_stop_struct));
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&condPartition, NULL);
  for (int i = 0; i < num_threads; i++){
    st[i].thread_count = i;
    st[i].start_idx = i*(size/num_threads);
    if ((i == num_threads - 1) && (size % num_threads != 0)){
      st[i].len = size - st[i].start_idx;
      // printf("%d: len = %d\n", i, st[i].len);
    }
    else{
      st[i].len = (size/num_threads);
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

  // for (int i = 0; i< num_threads ;i++){
  //   printf("Thread %d Sample values\n", st[i].thread_count);
  //   for (int j =0; j<num_threads; j++){
  //     printf("Sample %d: %ld\n", j, st[i].samples[j]);
  //   }
  // }

  // sort pivots
  long pivot_combined[num_threads*num_threads];

  // save pivots into combined array
  for (int i = 0; i < num_threads; i++){
    for(int j=0; j<num_threads;j++){
      pivot_combined[i*num_threads + j] = st[i].samples[j];
    }
  }
  // sort pivots in main
  qsort(pivot_combined, num_threads*num_threads, sizeof(long), compare);
  // printf("Sorted samples: ");
  // for (int i=0;i<num_threads*num_threads;i++){
  //   printf("%ld ", pivot_combined[i]);
  // }
  // printf("\n");

  // choose pivot values from p + (p/2) - 1, 2p + (p/2) -1 .... (p-1)p + (p/2) - 1

  long *pivots_main[num_threads];

  // pick p-1 pivots
  for (int i =0; i < num_threads; i++){
    pivots_main[i] = pivot_combined[(i+1)*num_threads + (num_threads/2) - 1];
  }

  // printf("Pivots\n");
  // for (int i = 0; i < num_threads - 1; i++){
  //   printf("Pivot main %d: %ld\n", i, pivots_main[i]);
  // }
  // assign pivots to structs
  for (int i = 0; i < num_threads; i++){
    for (int j =0; j < num_threads - 1; j++){
      st[i].pivots[j] = pivots_main[j];
    }
  }

  // printf("Thread 1");
  // for (int i = 0; i < num_threads - 1; i++){
  //   printf("Thread 1 pivots: %ld\n", st[1].pivots[i]);
  // }

  for (int i = 0; i < num_threads; i++){
    pthread_create(&th[i], NULL, &phase3_and_phase4, &st[i]);
  }
  // wait phase 3 and 4 to complete
  for (int i = 0 ; i < num_threads; i++){
    // pthread_join(th[i], &st[i]);
    pthread_join(th[i], &st[i]);
  }

  for (int i =0 ;i< num_threads; i++){
    printf("length of combined for thread %d = %ld\n", i , st[i].combined_partition_length);
  }
  // keeps track of where are are in the array currently
  long arr_ptr = 0;
  for (int i = 0; i < num_threads; i ++){
    printf("Thread %d\n", i);
    for (int j = 0; j < st[i].combined_partition_length; j++){
      printf("Saving arr index %d value %d\n", arr_ptr + j, st[i].combined_partitions[j]);
      arr[arr_ptr + j] = st[i].combined_partitions[j];
    }
    arr_ptr += st[i].combined_partition_length;
  }

  for (int i =0;i < size; i++){
    printf("%d ", arr[i]);
  }
  printf("\n");

  if (!checking(arr, size)) {
    printf("The array is not in sorted order!!\n");
  }
  else{
    printf("SORTED!\n");
  }
  // for(int i = 0; i < size2; i++){
  //   printf("%d: %d\n", i, arr[i]);
  // }
  
  for(int i=0;i<num_threads;i++){
    printf("Partition counter %d = %d\n", i, partiton_counter[i]);
  }

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&condPartition);
  free(st);
  return 0;
}



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