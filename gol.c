/*
* Swarthmore College, CS 31
* Copyright (c) 2021 Swarthmore College Computer Science Department,
* Swarthmore PA

STUDENTS: Nader Ahmed & Ibrahim Hassouna
LAB 6: GAME OF LIFE
DATE: 11/2/2021

* To run:
* ./gol file1.txt  0  # run with config file file1.txt, do not print board
* ./gol file1.txt  1  # run with config file file1.txt, ascii animation
* ./gol file1.txt  2  # run with config file file1.txt, ParaVis animation
*/
#include <pthreadGridVisi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "colors.h"

#include <pthreadGridVisi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

/****************** Definitions **********************/
/* Three possible modes in which the GOL simulation can run */
#define OUTPUT_NONE   0   // with no animation
#define OUTPUT_ASCII  1   // with ascii animation
#define OUTPUT_VISI   2   // with ParaVis animation

/* Used to slow down animation run modes: usleep(SLEEP_USECS);
* Change this value to make the animation run faster or slower
*/
#define SLEEP_USECS    100000
/* A global variable to keep track of the number of live cells in the
* world (this is the ONLY global variable you may use in your program)
*/
static int total_live = 0;

/* This struct represents all the data you need to keep track of your GOL
* simulation.  Rather than passing individual arguments into each function,
* we'll pass in everything in just one of these structs.
* this is passed to play_gol, the main gol playing loop
*/
struct gol_data {
  int rows;  // the row dimension
  int cols;  // the column dimension
  int iters; // number of iterations to run the gol simulation
  int output_mode; // set to:  OUTPUT_NONE, OUTPUT_ASCII, or OUTPUT_VISI
  int* board;
  int* newBoard; // the board of cells
  int aliveCells; // number of cells currently alive
  visi_handle handle; //paravis library field
  color3 *image_buff; //paravis library field
  int numThreads;
  int rowsOrColumns;
  int divisionsPerThread;
  int leftOver;
  int printPartition;
};

struct threadArgs {
  int tid;
  struct gol_data* golData;
};

/****************** Function Prototypes **********************/
/* the main gol game playing loop (prototype must match this) */
void* play_gol(void * args);
void update_colors(struct gol_data *data,int startRow,int startCol,int endRow,int endCol) ;
/* init gol data from the input file and run mode cmdline args */
int init_game_data_from_args(struct gol_data *data, char *argv[]);
/*create the board and initialize the values*/
void createBoard(struct gol_data *data, int* x, int* y);
/* print board to the terminal (for OUTPUT_ASCII mode) */
void print_board(struct gol_data *data, int round);
/* function to simulate one round of the game*/
void oneRound(struct gol_data *data,int statRow,int startCol,int endRow,int endCol);
/*function to calculate the number of living neighbors of a givin cell*/
int liveNeighbors(struct gol_data *data, int i, int j);
void * thread_function(void * args);
// function that gives the thread what part of the grid to work on.
void divide(struct gol_data* threadsData, int id, int * start,int* end);
/************ Definitions for using ParVisi library ***********/
/* register animation with Paravisi library (DO NOT MODIFY) */
//int connect_animation(void (*applfunc)(struct gol_data *data),
//struct gol_data* data);
/* name for visi (you may change the string value if you'd like) */
static char visi_name[] = "GOL!";
pthread_mutex_t mutex;
pthread_barrier_t barrier;
/**********************************************************/

int main(int argc, char *argv[]) {
  int ret;
  struct gol_data data;
  struct timeval start_time, stop_time;
  double secs,start,startMicro,end,endMicro;
  pthread_mutex_init(&mutex, NULL);
  /* check number of command line arguments */
  if (argc != 6 ) {
    printf("usage: ./gol <infile> output_mode[0,1,2] num_threads partition[0,1] print_partition[0,1] \n");
    printf("(output mode - 0: with no visi, 1: with ascii visi, 2: with ParaVis visi)\n");
    exit(1);
  }

  /* Initialize game state (all fields in data) from information
  * read from input file */
  ret = init_game_data_from_args(&data, argv);
  if(ret != 0) {
    printf("Error init'ing with file %s, mode %s\n", argv[1], argv[2]);
    exit(1);
  }

  data.numThreads = atoi(argv[3]);
  data.rowsOrColumns = atoi(argv[4]);
  data.printPartition =atoi(argv[5]);

  if (data.rowsOrColumns == 0) {
    data.divisionsPerThread = data.rows / data.numThreads;
    data.leftOver = data.rows - (data.numThreads * data.divisionsPerThread);
  } else {
    data.divisionsPerThread = data.cols / data.numThreads;
    data.leftOver = data.cols - (data.numThreads * data.divisionsPerThread);
  }
  pthread_barrier_init(&barrier, 0, data.numThreads);

  /* Invoke play_gol in different ways based on the run mode */

  ret = gettimeofday(&start_time, NULL);
  if(ret) {
    printf("ERROR time not measured)");
    exit(1);
  }

  start = (double)start_time.tv_sec;
  startMicro = (double)(start_time.tv_usec)/(10000000);

  struct threadArgs * thread_args = malloc(sizeof(struct threadArgs) * data.numThreads);
  pthread_t *thread_array= malloc(sizeof(pthread_t) * data.numThreads);

  if(data.output_mode == OUTPUT_NONE) {  // run with no animation
    for(int i=0; i<data.numThreads; i++) {
      thread_args[i].tid = i;
      thread_args[i].golData = &data;
    }
    for(int i=0; i<data.numThreads; i++) {
       pthread_create(&thread_array[i], NULL, play_gol , &thread_args[i]);
    }
    for (int i = 0; i < data.numThreads; i++) {
      pthread_join(thread_array[i], NULL);
    }
  }
  else if (data.output_mode == OUTPUT_ASCII) { // run with ascii animation
    for(int i=0; i<data.numThreads; i++) {
      thread_args[i].tid = i;
      thread_args[i].golData = &data;
    }
    for(int i=0; i<data.numThreads; i++) {
       pthread_create(&thread_array[i], NULL, play_gol , &thread_args[i]);
    }
    for (int i = 0; i < data.numThreads; i++) {
      pthread_join(thread_array[i], NULL);
    }
    if(system("clear")) { perror("clear"); exit(1); }
    print_board(&data, data.iters);
  }
  else {  // OUTPUT_VISI: run with ParaVisi animation
    // call init_pthread_animation and get visi_handle
    data.handle = init_pthread_animation(data.numThreads, data.rows, data.cols, visi_name);
    if(data.handle == NULL) {
      printf("ERROR init_pthread_animation\n");
      exit(1);
    }
    // get the animation buffer
    data.image_buff = get_animation_buffer(data.handle);
    if(data.image_buff == NULL) {
      printf("ERROR get_animation_buffer returned NULL\n");
      exit(1);
    }
    // connect ParaVisi animation to main application loop function
    for(int i=0; i<data.numThreads; i++) {
      thread_args[i].tid = i;
      thread_args[i].golData = &data;
    }
    for(int i=0; i<data.numThreads; i++) {
       pthread_create(&thread_array[i], NULL, play_gol , &thread_args[i]);
    }
    // start ParaVisi animation
    run_animation(data.handle, data.iters);
    for (int i = 0; i < data.numThreads; i++) {
      pthread_join(thread_array[i], NULL);
    }
  }
  ret = gettimeofday(&stop_time, NULL);
  if(ret) {
    printf("ERROR time not measured)");
    exit(1);
  }
  end = (double)stop_time.tv_sec ;
  endMicro =(double)(stop_time.tv_usec)/(10000000);
  secs = (end-start)+(endMicro-startMicro);

  if (data.output_mode != OUTPUT_VISI) {

    fprintf(stdout, "Total time: %0.3f seconds\n", secs);
    fprintf(stdout, "Number of live cells after %d rounds: %d\n\n",
    data.iters, total_live);
  }
  free(data.board);
  free(data.newBoard);
  free(thread_args);
  free(thread_array);
  return 0;
}
/**********************************************************/
/* initialize the gol game state from command line arguments
*       argv[1]: name of file to read game config state from
*       argv[2]: run mode value
* data: pointer to gol_data void calculate(struct threads_data threadsData, int id, int * start,int* end)struct to initialize
* argv: command line args
*       argv[1]: name of file to read game config state from
*       argv[2]: run mode
* returns: 0 on success, 1 on error
*/
void divide(struct gol_data* threadsData, int id, int * start,int* end){
   if (id < threadsData->leftOver){
     *start=id*(threadsData->divisionsPerThread +1);
     *end= *start +(threadsData->divisionsPerThread);
   }
   else{
     *start=(threadsData->leftOver)*(threadsData->divisionsPerThread +1) + (id-threadsData->leftOver)*(threadsData->divisionsPerThread);
     *end= *start +(threadsData->divisionsPerThread)-1;
   }

}
int init_game_data_from_args(struct gol_data *data, char *argv[]) {
  char* filename = argv[1];
  FILE* myFile;
  int runMode = atoi(argv[2]);
  myFile = fopen(filename,"r");
  int ret;

  ret = fscanf(myFile,"%d",&(data->rows));
  if(!ret) {
    return ret;
  }

  ret = fscanf(myFile,"%d",&(data->cols));
  if(!ret) {
    return ret;
  }

  ret = fscanf(myFile,"%d",&(data->iters));
  if(!ret) {
    return ret;
  }

  if ((runMode != 0) &&  (runMode!= 1) && (runMode!=2)) {
    return 1;
  }

  ret = fscanf(myFile,"%d",&(data->aliveCells));
  if(!ret) {
    return ret;
  }
  total_live=data->aliveCells;

  int *x = malloc(sizeof(int)*data->aliveCells);
  int *y = malloc(sizeof(int)*data->aliveCells);

  for(int k=0; k<data->aliveCells; k++) {
    ret = fscanf(myFile,"%d",&(x[k]));
    if(!ret) {
      return ret;
    }

    ret = fscanf(myFile,"%d",&(y[k]));
    if(!ret) {
      return ret;
    }
  }
  data->output_mode = runMode;
  createBoard(data,x,y);
  free(x);
  free(y);
  fclose(myFile);
  return 0;
}

/* the gol application main loop function:
*  runs rounds of GOL,
*    * updates program state for next round (world and total_live)
*    * performs any animation step based on the output/run mode
*
*   data: pointer to a struct gol_data  initialized with
*         all GOL game playing sta  int x=(int) args;te
*/
void* play_gol(void * args) {
  struct threadArgs * myargs = (struct threadArgs *) args;
  int startRow, endRow, startCol, endCol;
  struct gol_data * data=myargs->golData;

  // printf("%d     %d:%d             %d \n",myargs->tid, start,end, end-start);

  if(data->rowsOrColumns==0) {
    divide(data,myargs->tid,&startRow,&endRow);
    startCol = 0;
    endCol = data->cols-1;
  } else {
    divide(data,myargs->tid,&startCol,&endCol);
    startRow = 0;
    endRow = data->rows-1;
  }

  if (data->printPartition){
     printf("tid  %d  rows: %d : %d  (%d)  cols: %d : %d  (%d) \n"    ,myargs->tid, startRow, endRow,endRow-startRow+1,startCol, endCol,endCol-startCol+1);
  }

  int numRounds = data->iters;

  int i;
  for (i=0;i<numRounds;i++) {
    if (data->output_mode == 0) {
      pthread_barrier_wait(&barrier);
      oneRound(data,startRow,startCol,endRow,endCol);
      pthread_barrier_wait(&barrier);
      if (myargs->tid==0){
        int * temp=data->board;
        data->board=data->newBoard;
        data->newBoard =temp;
      }
    } else if (data->output_mode==1) {
      pthread_barrier_wait(&barrier);
      oneRound(data,startRow,startCol,endRow,endCol);
      pthread_barrier_wait(&barrier);
      if (myargs->tid==0){
        int * temp=data->board;
        data->board=data->newBoard;
        data->newBoard =temp;
        print_board(data,i);
        usleep(SLEEP_USECS);
        system("clear");
      }

    } else if (data->output_mode==2) {
      oneRound(data,startRow,startCol,endRow,endCol);
      pthread_barrier_wait(&barrier);
      if (myargs->tid==0){
        int * temp=data->board;
        data->board=data->newBoard;
        data->newBoard =temp;
      }
      pthread_barrier_wait(&barrier);
      update_colors(data,startRow,startCol,endRow,endCol);
      draw_ready(data->handle);
      usleep(450000);
    }
  }
  data=NULL;
  return NULL;
}
/*
Creates board using malloc. It is a 2d array initialized with 0s to represent
dead cells, and 1s to represent alive ones (as given by the input file).
*/

void createBoard(struct gol_data *data, int* x,int* y) {
  data->board = malloc(sizeof(int)*data->cols*data->rows);
  data->newBoard=malloc(sizeof(int)*data->cols*data->rows);
  for (int i=0;i<data->rows;i++) {
    for(int j=0;j<data->cols;j++) {
      data->board[i*data->cols + j]=0;
    }
         for(int i=0;i<data->aliveCells;i++) {
      data->board[x[i]*data->cols+y[i]]=1;
  }
}
}

/*
Performs the game of life round. This function will be repeatedly called for
several rounds.
*/
void oneRound(struct gol_data *data,int startRow,int startCol,int endRow,int endCol) {
  int newLiving=0;
  int newDead=0;
  for (int i=startCol;i<=endCol;i++) {
    for(int j=startRow;j<=endRow;j++) {
      int numberOfLN=liveNeighbors(data,i,j);
      if (data->board[i*data->cols +j]==1){
        if (numberOfLN==1 || numberOfLN==0 || numberOfLN>=4){
          data->newBoard[i*data->cols +j]=0;
          newDead++;
        }
        else{
          data->newBoard[i*data->cols +j]=1;
        }
      }
      else{
        if (numberOfLN==3){
          data->newBoard[i*data->cols +j]=1;
          newLiving++;
        }
        else{
          data->newBoard[i*data->cols +j]=0;
        }
      }
    }
  }
  pthread_mutex_lock(&mutex);
  total_live=total_live+newLiving-newDead;
  pthread_mutex_unlock(&mutex);
}

/*
Function which calculates the number of alive neighbors with the wrap-around
properties specified.
*/
int liveNeighbors(struct gol_data *data, int i, int j) {
  int up, down, left, right;
  int neighbors=0;

  if(i==0) {
    left = (data->cols)-1;
  } else {
    left = i-1;
  }

  if(i==(data->cols)-1) {
    right = 0;
  } else {
    right = i+1;
  }

  if(j==0) {
    up = (data->rows)-1;
  } else {
    up = j-1;
  }

  if(j==data->rows-1) {
    down = 0;
  } else {
    down = j+1;
  }

  if(data->board[i*data->cols+up] == 1) {
    neighbors++;
  }
  if(data->board[right*data->cols+up] == 1) {
    neighbors++;
  }

  if(data->board[left*data->cols+up] == 1) {
    neighbors++;
  }

  if(data->board[right*data->cols+j] == 1) {
    neighbors++;
  }

  if(data->board[left*data->cols+j] == 1) {
    neighbors++;
  }

  if(data->board[right*data->cols+down] == 1) {
    neighbors++;
  }

  if(data->board[i*data->cols+down] == 1) {
    neighbors++;
  }

  if(data->board[left*data->cols+down] == 1) {
    neighbors++;
  }

  return neighbors;

}
/**********************************************************/

/**********************************************************/
/* Print the board to the terminal.
*   data: gol game specific data
*   round: the current round number
*
*/
void update_colors(struct gol_data *data,int startRow,int startCol,int endRow,int endCol) {

  int i, j, r, c, index, buff_i;
  color3 *buff;

  buff = data->image_buff;  // just for readability
  r = data->rows;
  c = data->cols;

  for(i=startRow; i <= endRow; i++) {
    for(j=startCol; j <= endCol; j++) {
      index = i*c + j;
      // translate row index to y-coordinate value
      // in the image buffer, r,c=0,0 is assumed to be the _lower_ left
      // in the grid, r,c=0,0 is _upper_ left.
      buff_i = (r - (i+1))*c + j;
      // update animation buffer

      if(data->board[index]==1) {
        buff[buff_i].r = (data->board[index]) % 256;
        buff[buff_i].g = (int)(data->board[index] + 1.*j/c) % 256;
        buff[buff_i].b = 200;
      } else {
        buff[buff_i].r = (data->board[index]) % 256;
        buff[buff_i].g = (int)(data->board[index] + 1.*j/c) % 256;
        buff[buff_i].b = 10;
      }
    }
  }
}
void print_board(struct gol_data *data, int round) {

  int i, j;

  /* Print the round number. */
  fprintf(stderr, "Round: %d\n", round);

  for (i = 0; i < data->rows; ++i) {
    for (j = 0; j < data->cols; ++j) {
      if (data->board[i*data->cols +j]==1){
        fprintf(stderr, " @");
      }
      else{
        fprintf(stderr, " .");
      }
    }
    fprintf(stderr, "\n");
  }

  /* Print the total number of live cells. */
  fprintf(stderr, "Live cells: %d\n\n", total_live);
}


/**********************************************************/
/***** START: DO NOT MODIFY THIS CODE *****/
/* sequential wrapper functions around ParaVis library functions */
void (*mainloop)(struct gol_data *data);

void* seq_do_something(void * args){
  mainloop((struct gol_data *)args);
  return 0;
}

int connect_animation(void (*applfunc)(struct gol_data *data), struct gol_data* data)
{
  pthread_t pid;

  mainloop = applfunc;
  if( pthread_create(&pid, NULL, seq_do_something, (void *)data) ) {
    printf("pthread_created failed\n");
    return 1;
  }
  return 0;
}
/***** END: DO NOT MODIFY THIS CODE *****/
/******************************************************/
