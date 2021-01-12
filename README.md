# Embedded Linux Assignment - Heart Rate Monitor

The goal of this assignment is implementing a heart rate monitor, which is made of two components:

- a Linux character-based driver (cDD) used to access a “virtual” Photopletismography (PPG) sensor
- a Linux user application (APP)

The APP performs an endless loop, where it samples the PPG sensor, and every 2048 acquired samples, it performs a 2048-points FFT,  computes the Power Spectral Density (PSD), identifies the base frequency of the signal (the frequency where the PSD is maximum), which is the heart rate in Hz, and it prints the heart rate in beat-per-minutes, according to the following specifications: 

- sampling frequency: 50 Hz
- number of samples to be acquired: 2048

The cDD provides access to the virtual PPG sensor. Each time the read function of the cDD is invoked, a pre-defined value is provided to the user application.

## How to use

Assuming you already built and setup a ```pocky``` Linux distribution on your machine for Raspberry PI 4 (or any other version) , you can either 

- use a bash script I wrote to automate the whole process
- do the whole setup by hand as described in the following sections

### Bash script based setup

Assuming your poky directory is located at ```~/``` , create a bash file called ```setup.sh``` 

```bash
touch ~/setup.sh
```

and copy the following code in it

**WARNING:** Make sure it fits your system before using. In particular, make sure your output device is actually called ```/dev/sdb``` otherwise modify last line with the appropriate one (check using ```sudo fdisk -l```) . Also, if your build directory has a different name, just replace ```build_rpi4``` with the correct one. 

````bash
#!/bin/bash

cd poky
git clone https://github.com/PronElle/OSESAssignment

shopt -s expand_aliases
source oe-init-build-env build_rpi4

bitbake-layers add-layer ../OSESAssignment

echo "IMAGE_INSTALL_append = \" heartbeat\"" >> conf/local.conf
echo "IMAGE_INSTALL_append = \" ppgmod\"" >> conf/local.conf
echo "KERNEL_MODULE_AUTOLOAD += \"ppgmod\"" >> conf/local.conf

bitbake core-image-full-cmdline
sudo dd if=tmp/deploy/images/raspberrypi4/core-image-full-cmdline-raspberrypi4.rpi-sdimg of=/dev/sdb bs=1M
````

Then, make it executable 

```bash
sudo chmod u+x setup.sh
```

Plug your SD card reader and run the script

```bash
./setup.sh
```

Eventually, connect via ssh to your rpi (or use a serial to USB cable or HDMI cable plus external monitor) and type ```heartbeat``` to test the application. 

Alternatively, do the setup by hand as described below.

### Setup by hand

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
IMAGE_INSTALL_append = " heartbeat"
IMAGE_INSTALL_append = " ppgmod"
KERNEL_MODULE_AUTOLOAD += "ppgmod"
```

and build the new image

```bash
bitbake core-image-full-cmdline
```

Plug the SD card reader in your computer and burn the image to the SD card.

**WARNING:** Make sure your output device is actually called ```/dev/sdb``` otherwise modify ```of=<outdevname>``` properly (check with ```sudo fdisk -l```).

```bash
sudo dd if=tmp/deploy/images/raspberrypi4/core-image-full-cmdline-raspberrypi4.rpi-sdimg of=/dev/sdb bs=1M
```

Eventually, login to your machine (either via ssh, serial port or using an HDMI cable and an external monitor) and  test the application running ```heartbeat``` from the command user interface.

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
- using another thread to read and a semaphore to force it to wait while computing the bpm doesn't allow you to read the sensor while this action is performed. 
- it's simple, efficient and low in terms of memory occupation.

## Extra: qemuarm

In case you own no real target, use the following script to deploy your application on qemuarm.

```bash
#!/bin/bash

cd poky
git clone https://github.com/PronElle/OSESAssignment

shopt -s expand_aliases
source oe-init-build-env build_qemuarm

bitbake-layers add-layer ../OSESAssignment

echo "IMAGE_INSTALL_append = \" heartbeat\"" >> conf/local.conf
echo "IMAGE_INSTALL_append = \" ppgmod\"" >> conf/local.conf
echo "KERNEL_MODULE_AUTOLOAD += \"ppgmod\"" >> conf/local.conf

bitbake core-image-minimal
runqemu qemuarm
```

**NOTICE:** qemuarm is far from precise in terms of timing. Do not rely on it if you want to test the application from that point of view.