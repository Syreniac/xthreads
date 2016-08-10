/*
 * test.c
 *
 *  Created on: 5 Jul 2016
 *      Author: Matthew
 */

#include "pthreads.h"
#include <stdio.h>
#include <stdlib.h>

#define M 4
#define K 2
#define N 4
#define NUM_THREADS 10

int A [M][K] = { {1,5}, {2,6}, {3,7}, {4,8} };
int B [K][N] = { {8,7,6,5}, {4,3,2,1} };
int C [M][N];

struct v {
   int i; /* row */
   int j; /* column */
};

void runner(void *param); /* the thread */

/*int main(int argc, char *argv[]) {

    int i,j, count = 0;

   for(i = 0; i < M; i++) {
      for(j = 0; j < N; j++) {
         //Assign a row and column for each thread
         struct v *data = (struct v *) malloc(sizeof(struct v));
         data->i = i;
         data->j = j;
         // Now create the thread passing it data as a parameter
         pthread_t tid;       //Thread ID
         pthread_attr_t attr; //Set of thread attributes

          //To demonstrate that both synchronised and unsynchronised threads work,
          //I've hacked this together.
          //The final thread is a synchronised one, that will allow the main thread to wait
          //until the others are completed in a not-totally-horrid way.


         attr.detachState = XTHREADS_CREATE_UNDETACHED;
         pthread_create(&tid,&attr,runner,data);
         pthread_detach(tid);
         count++;
      }
   }

   //Print out the resulting matrix
   for(i = 0; i < M; i++) {
      for(j = 0; j < N; j++) {
         printf("%d ", C[i][j]);
      }
      printf("\n");
   }
}*/

//The thread will begin control in this function
void runner(void *param) {
   struct v *data = param; // the structure that holds our data
   int n, sum = 0; //the counter and sum

   //Row multiplied by column
   for(n = 0; n< K; n++){
      sum += A[data->i][n] * B[n][data->j];
   }
   //assign the sum to its coordinate
   C[data->i][data->j] = sum;

   //Exit the thread
   pthread_exit(0);
}
