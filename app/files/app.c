#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

#define q	  11		    /* for 2^11 points */
#define N 	(1 << q)	/* N-point FFT, iFFT */

#define Ts  20     /* sampling time [us] */

typedef float real;
typedef struct{ 
   real Re; 
   real Im;
} complex;

#ifndef PI
# define PI	3.14159265358979323846264338327950288
#endif

typedef struct{
	pthread_t id;
	complex v[N];
}mythread_s;


static int fd = -1;
int counter = 0;
complex v[N];


/**
 *  @brief evaluates fast Fourier transform
 *  @param v: array of samples
 *  @param n: number of samples
 *  @param tmp
 **/ 
void fft( complex *v, int n, complex *tmp )
{
  if(n > 1) {			/* otherwise, do nothing and return */
    int k, m;    
    complex z, w, *vo, *ve;
    ve = tmp; vo = tmp+n/2;
 
    for(k=0; k<n/2; k++) {
      ve[k] = v[2*k];
      vo[k] = v[2*k+1];
    }
 
    fft( ve, n/2, v );		/* FFT on even-indexed elements of v[] */
    fft( vo, n/2, v );		/* FFT on odd-indexed elements of v[] */
 
    for(m = 0; m < n/2; m++) {
      w.Re = cos(2*PI*m/(double)n);
      w.Im = -sin(2*PI*m/(double)n);
      z.Re = w.Re*vo[m].Re - w.Im*vo[m].Im;	/* Re(w*vo[m]) */
      z.Im = w.Re*vo[m].Im + w.Im*vo[m].Re;	/* Im(w*vo[m]) */
      v[  m  ].Re = ve[m].Re + z.Re;
      v[  m  ].Im = ve[m].Im + z.Im;
      v[m+n/2].Re = ve[m].Re - z.Re;
      v[m+n/2].Im = ve[m].Im - z.Im;
    }
  }
  return;
}


/** 
 *  @brief thread function to evaluate bpm using fft
 *  @param v: array of samples
 **/
void* bpm_thread(complex *v){
  complex scratch[N];
  float abs[N];
  int k, m;
  int minIdx, maxIdx;
 
// FFT computation
  fft( v, N, scratch );

// PSD computation
  for(k=0; k<N; k++) {
	abs[k] = (50.0/2048)*((v[k].Re*v[k].Re)+(v[k].Im*v[k].Im)); 
  }

  minIdx = (0.5*2048)/50;   // position in the PSD of the spectral line corresponding to 30 bpm
  maxIdx = 3*2048/50;       // position in the PSD of the spectral line corresponding to 180 bpm

// Find the peak in the PSD from 30 bpm to 180 bpm
  m = minIdx;
  for(k=minIdx; k<(maxIdx); k++) {
    if( abs[k] > abs[m] )
	    m = k;
  }

  // Print the heart beat in bpm
  printf("bpm: %d\n", m*60*50/2048);
}


/**
 *  @brief SIGN_INT Handler (Ctrl + C)
 *         allowing user to stop execution.
 **/
void SignIntHandler(){
  printf("[INFO] Terminating Program\n");
  if(fd != -1) close(fd); // if file was actually opened

  exit(EXIT_SUCCESS);
}

/**
 * @brief setups a repetitive alarm every ts in us
 *        look at README for more
 * @param ts : time in seconds
 **/
void setReAlarm(time_t ts){
  struct itimerval itv;

  itv.it_value.tv_usec = ts *1000;
  itv.it_value.tv_sec = ts / 1000;
  itv.it_interval = itv.it_value; // repetitive

  setitimer(ITIMER_REAL, &itv, NULL);
  return;
}




/**
 * @brief SIGALRM Handler: accesses drive file 
 *        to get a sample every 20 ms
 **/
void sampleHandler(){ 
  int val ;

  read(fd, (char*)&(val), sizeof(int));
  //printf("[INFO] read: %d\n", val);

  v[counter].Re = val;
  v[counter++].Im = 0;

  if(counter == N){ // if all samples gathered
	  mythread_s thr;
	  for(int i = 0; i < N; ++i) thr.v[i] = v[i]; // copying N samples

	  // detaching a thread handling the N samples
	  if(pthread_create(&(thr.id), NULL, (void*)bpm_thread, (void*)thr.v) != 0){
	    fprintf(stderr,"[ERROR] Unable to create thread: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	  }
	  counter = 0;
  }

}



int main(void)
{
  char* dev_name = "/dev/ppgmod_dev";
  printf("[INFO] Application started\n");

  // attacching Ctrl + C to Handler
  signal(SIGINT, SignIntHandler);

  // opening driver file
  if((fd = open(dev_name,  O_RDWR)) < 0){
    fprintf(stderr,"[ERROR] Unable to open %s\n: %s\n", dev_name, strerror(errno));
    exit(EXIT_FAILURE);
  }


  // attach SIGALARM to Handler
  if(signal(SIGALRM, sampleHandler) == SIG_ERR){
    fprintf(stderr, "[ERROR] Unable to handle SIGALARM: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }


  // set ripetitive alarm every Ts ms
  setReAlarm(Ts);

  while(1) pause();

  exit(EXIT_SUCCESS);
}
