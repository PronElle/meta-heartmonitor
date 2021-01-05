# Embedded Linux Assignment - Heart Rate Monitor

The goal of this assignment is implemanting a hearth rate monitor, which is made of two components:

- a Linux character-based driver (cDD) used to access a “virtual” Photopletismography (PPG) sensor
- a Linux user application (APP)

The APP performs an endless loop, where it samples the PPG sensor, and every 2048 acquired samples, it performs a 2048-points FFT,  computes the Power Spectral Density (PSD), identifies the base frequency of the signal (the frequency where the PSD is maximum), which is the heart rate in Hz, and it prints the heart rate in beat-per-minutes, according to the following specifications: 

- sampling frequency: 50 Hz
- number of samples to be acquired: 2048

The cDD provides access to the virtual PPG sensor. Each time the read function of the cDD is invoked, a pre-defined value is provided to the user application.

## How to use

Assuming that you already built and setup a ```pocky``` Linux distribution on your machine for RaspberryPi 4 or qemuarm, open up a new terminal and act as follows, according to what target you want to the test it on (rpi or qemuarm).

### Raspberry Pi 4

Reach your ```poky``` root directory

```bash
cd poky
```

then, clone this repository as follows

```bash
git clone https://github.com/PronElle/OSESAssignment
```

and initialize the environment

```bash
source oe-init-build-env build_rpi4
```

now the layer needs to be added to the configuration

```
bitbake-layers add-layer ../OSESAssignment
```

At this point, application and the kernel module need to be added to the Linux distro configurations. Edit the following file with your favorite text editor, e.g. ```vi``` 

```bash
vi conf/local.conf
```

and add the following lines at the end of the file

```bash
IMAGE_INSTALL_append = " app"
IMAGE_INSTALL_append = " ppgmod"
KERNEL_MODULE_AUTOLOAD += "ppgmod"
```

Then, the layer needs to be added to the Linux distro

```
vi conf/bblayers.conf
```

and replace its content with the following lines

```bash
BBPATH = "${TOPDIR}"
BBFILES ?= ""
BBLAYERS ?= " \
    /home/elle/poky/meta \
    /home/elle/poky/meta-poky \
    /home/elle/poky/meta-yocto-bsp \
    /home/elle/poky/meta-openembedded/meta-oe \
    /home/elle/poky/meta-openembedded/meta-multimedia \
    /home/elle/poky/meta-openembedded/meta-networking \
    /home/elle/poky/meta-openembedded/meta-python \
    /home/elle/poky/meta-raspberrypi \
    /home/elle/poky/OSESAssignment \
    "
```

Eventually, build the new image

```bash
bitbake core-image-full-cmdline
```

and test the application running ```app``` from the command user interface.

### qemuarm 

Reach your ```poky``` root directory

```bash
cd poky
```

then, clone this repository as follows

```bash
git clone https://github.com/PronElle/OSESAssignment
```

and initialize the environment

```bash
source oe-init-build-env build_qemuarm
```

now the layer needs to be added to the configuration

```
bitbake-layers add-layer ../OSESAssignment
```

At this point, application and the kernel module need to be added to the Linux distro configurations. Edit the following file with your favorite text editor, e.g. ```vi``` 

```bash
vi conf/local.conf
```

and add the following lines at the end of the file

```bash
IMAGE_INSTALL_append = " app"
IMAGE_INSTALL_append = " ppgmod"
KERNEL_MODULE_AUTOLOAD += "ppgmod"
```

Eventually, build the new image

```bash
bitbake core-image-minimal
```

and run ```qemuarm```

```bash
runqemu qemuarm
```

test the application typing ```app``` from the command user interface.

## Timing

A sampling frequency of 50 Hz is equal to 20 ms sampling period. To ensure the proper timing, an alarm-based approach relying on ```SIGALARM``` and ```setitimer``` function was employed.

Timer values are defined by the following structures

```C
struct itimerval {
    struct timeval it_interval; /* next value */
    struct timeval it_value;    /* current value */
};

struct timeval {
    time_t      tv_sec;         /* seconds */
    suseconds_t tv_usec;        /* microseconds */
};
```

## Why using a pipe 

- it embeds a FIFO mechanism, which proves useful as samples keep getting acquired and accumulated even during bpm computation, thus no data get lost;
- it doesn't require any synchronization mechanism (no semaphores, locks or mutexs) because pipes are blocking, so that a read is performed only when the pipe is not empty;
- it's simple, efficient and low in terms of memory occupation.