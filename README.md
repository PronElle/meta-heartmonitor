# Embedded Linux Assignment - Heart Rate Monitor

The goal of this assignment is implemanting a hearth rate monitor, which is made of two components:

- a Linux character-based driver (cDD) used to access a “virtual” Photopletismography (PPG) sensor
- a Linux user application (APP)

The APP performs an endless loop, where it samples the PPG sensor, and every 2048 acquired samples, it performs a 2048-points FFT,  computes the Power Spectral Density (PSD), identifies the base frequency of the signal (the frequency where the PSD is maximum), which is the heart rate in Hz, and it prints the heart rate in beat-per-minutes, according to the following specifications: 

- sampling frequency: 50 Hz
- number of samples to be acquired: 2048

The cDD provides access to the virtual PPG sensor. Each time the read function of the cDD is invoked, a pre-defined value is provided to the user application.

## How to use

Assuming that you already built and setup a ```pocky``` Linux distribution on your machine for RaspberryPi 4 or qemuarm, you can either 

- use a bash script I wrote to automate the whole process
- do the whole setup by hand as described in the following sections

### Raspberry PI 4

Assuming your poky directory is located at ```~/``` , create a bash file called ```setup.sh``` 

```bash
touch ~/setup.sh
```

and copy the following code in it

**WARNING:** Make sure it fits your system before using. In particular, make sure your output device is actually called ```/dev/sdb``` otherwise modify last line with the appropriate one. Also, if your build directory has a different name, just replace ```build_rpi4``` with the correct one. 

````bash
#!/bin/bash

cd poky
git clone https://github.com/PronElle/OSESAssignment

shopt -s expand_aliases
source oe-init-build-env build_rpi4

bitbake-layers add-layer ../OSESAssignment

echo "IMAGE_INSTALL_append = \" app\"" >> conf/local.conf
echo "IMAGE_INSTALL_append = \" ppgmod\"" >> conf/local.conf
echo "KERNEL_MODULE_AUTOLOAD += \"ppgmod\"" >> conf/local.conf

bitbake core-image-full-cmdline
sudo dd if=tmp/deploy/images/raspberrypi4/core-image-full-cmdline-raspberrypi4.rpi-sdimg of=/dev/sdb bs=1M
````

Then, make it executable 

```bash
sudo chmod u+x setup.sh
```

and run it

```bash
./setup.sh
```

After login, type ```app``` to test the application. 

Alternatively, do it by hand as described below.

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

Eventually, build the new image

```bash
bitbake core-image-full-cmdline
```

now burn the image to the SD Card.

**WARNING:** Make sure your output device is actually called ```/dev/sdb``` otherwise modify ```of=<outdevname>``` properly (check with ```sudo fdisk -l```).

```bash
sudo dd if=tmp/deploy/images/raspberrypi4/core-image-full-cmdline-raspberrypi4.rpi-sdimg of=/dev/sdb bs=1M
```

After login,  test the application running ```app``` from the command user interface.

### qemuarm 

As for the real target, here's a script

```bash
#!/bin/bash

cd poky
git clone https://github.com/PronElle/OSESAssignment

shopt -s expand_aliases
source oe-init-build-env build_qemuarm

bitbake-layers add-layer ../OSESAssignment

echo "IMAGE_INSTALL_append = \" app\"" >> conf/local.conf
echo "IMAGE_INSTALL_append = \" ppgmod\"" >> conf/local.conf
echo "KERNEL_MODULE_AUTOLOAD += \"ppgmod\"" >> conf/local.conf

bitbake core-image-minimal
runqemu qemuarm
```

Make it executable and run it as described for the rpi4. 

Alternatively, reach your ```poky``` root directory

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

```bash
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

**NOTICE:** qemuarm is not that accurate when it comes to timing. Refer to a real target for better performances. 

## Why using a pipe 

- it embeds a FIFO mechanism, which proves useful as samples keep getting acquired and accumulated even during bpm computation, thus no data get lost;
- it doesn't require any synchronization mechanism (no semaphores, locks or mutexs) because pipes are blocking, so that a read is performed only when the pipe is not empty;
- it's simple, efficient and low in terms of memory occupation.