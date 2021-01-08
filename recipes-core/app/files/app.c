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

#define q 11       /* for 2^11 points */
#define N (1 << q) /* N-point FFT, iFFT */
#define Ts 0x4E20  /* sampling time [us] */

#ifndef PI
#define PI 3.14159265358979323846264338327950288
#endif

/*Uncomment if you want to see samples read printed and their timing*/
//#define DEBUG_MODE()

#ifdef DEBUG_MODE
struct timeval tv1, tv2;
#define CAPTURE_TIME(tv) gettimeofday(tv, NULL)

double get_sample_time(struct timeval *tvx, struct timeval *tvy)
{
  return (double)(tvy->tv_usec - tvx->tv_usec) / 1000000 +
         (double)(tvy->tv_sec - tvx->tv_sec);
}
#endif

typedef float real;

typedef struct
{
  real Re;
  real Im;
} complex;

/* shared vars */
complex v[N];
pthread_t bpm_thread_id;
static int fd_pipe[2];
static int fd = -1;

/**
 *  @brief evaluates fast Fourier transform
 *  @param v: array of samples
 *  @param n: number of samples
 *  @param tmp
 **/
void fft(complex *v, int n, complex *tmp)
{
  if (n > 1)
  { /* otherwise, do nothing and return */
    int k, m;
    complex z, w, *vo, *ve;
    ve = tmp;
    vo = tmp + n / 2;

    for (k = 0; k < n / 2; k++)
    {
      ve[k] = v[2 * k];
      vo[k] = v[2 * k + 1];
    }

    fft(ve, n / 2, v); /* FFT on even-indexed elements of v[] */
    fft(vo, n / 2, v); /* FFT on odd-indexed elements of v[] */

    for (m = 0; m < n / 2; m++)
    {
      w.Re = cos(2 * PI * m / (double)n);
      w.Im = -sin(2 * PI * m / (double)n);
      z.Re = w.Re * vo[m].Re - w.Im * vo[m].Im; /* Re(w*vo[m]) */
      z.Im = w.Re * vo[m].Im + w.Im * vo[m].Re; /* Im(w*vo[m]) */
      v[m].Re = ve[m].Re + z.Re;
      v[m].Im = ve[m].Im + z.Im;
      v[m + n / 2].Re = ve[m].Re - z.Re;
      v[m + n / 2].Im = ve[m].Im - z.Im;
    }
  }
  return;
}

/** 
 *  @brief thread function to evaluate bpm using fft
**/
void *bpm_thread()
{
  complex scratch[N];
  float abs[N];
  int k, m, val;
  int minIdx, maxIdx;
  unsigned short counter = 0;

  while (1)
  {
    read(fd_pipe[0], &val, sizeof(val)); // it's blocking !

    v[counter].Re = val;
    v[counter++].Im = 0;

    if (counter == N)
    { // if all samples gathered
      counter = 0;

      // FFT computation
      fft(v, N, scratch);

      // PSD computation
      for (k = 0; k < N; k++)
        abs[k] = (50.0 / N) * ((v[k].Re * v[k].Re) + (v[k].Im * v[k].Im));

      minIdx = (0.5 * N) / 50; // position in the PSD of the spectral line corresponding to 30 bpm
      maxIdx = 3 * N / 50;     // position in the PSD of the spectral line corresponding to 180 bpm

      // Find the peak in the PSD from 30 bpm to 180 bpm
      m = minIdx;
      for (k = minIdx; k < maxIdx; k++)
        if (abs[k] > abs[m]) m = k;

      // Print the heart beat in bpm
      printf("bpm: %d\n", m * 60 * 50 / N);
    }
  }

  pthread_exit(NULL);
}

/**
 *  @brief SIGN_INT Handler (Ctrl + C)
 *         allowing user to stop execution.
 **/
void SignIntHandler()
{
  pthread_cancel(bpm_thread_id);
  if (fd != -1)
    close(fd); // if file was actually opened

  printf("Terminating Program\n");
  exit(EXIT_SUCCESS);
}

/**
 * @brief setups a repetitive alarm every ts in us
 *        look at README for more
 * @param ts : time in us
 **/
void setReAlarm(time_t ts)
{
  struct itimerval itv;

  itv.it_value.tv_usec = ts;
  itv.it_value.tv_sec = ts / 1000000;
  itv.it_interval = itv.it_value; // repetitive

  setitimer(ITIMER_REAL, &itv, NULL);
  return;
}

/**
 * @brief SIGALRM Handler: accesses drive file 
 *        to get a sample every 20 ms
 **/
void sampleHandler()
{
  int val;

#ifdef DEBUG_MODE
  CAPTURE_TIME(&tv2);
#endif

  read(fd, (char *)&(val), sizeof(int)); // reading from mod

#ifdef DEBUG_MODE
  printf("[time = %f s]\tread: %d\n", get_sample_time(&tv1, &tv2), val);
  CAPTURE_TIME(&tv1);
#endif

  write(fd_pipe[1], &val, sizeof(val)); // send to thread
}

int main(void)
{
  char *dev_name = "/dev/ppgmod_dev";

  // attacching Ctrl + C to Handler
  signal(SIGINT, SignIntHandler);

  // opening driver file
  if ((fd = open(dev_name, O_RDWR)) < 0)
  {
    fprintf(stderr, "Unable to open %s\n: %s\n", dev_name, strerror(errno));
    exit(EXIT_FAILURE);
  }

  // creating the FIFO shared channel
  if (pipe(fd_pipe) < 0)
  {
    fprintf(stderr, "Unable to create FIFO channel: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // attach SIGALARM to Handler
  if (signal(SIGALRM, sampleHandler) == SIG_ERR)
  {
    fprintf(stderr, "Unable to handle SIGALARM: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // detaching a thread handling the N samples
  if (pthread_create(&bpm_thread_id, NULL, (void *)bpm_thread, NULL) != 0)
  {
    fprintf(stderr, "Unable to create thread: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // set ripetitive alarm every Ts ms
  setReAlarm(Ts);

#ifdef DEBUG_MODE
  CAPTURE_TIME(&tv1);
#endif

  printf("Application started\n");
  while (1) pause();

  return 0;
}
