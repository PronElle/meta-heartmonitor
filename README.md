# Embedded Linux Assignment - Heart Rate Monitor

The goal of this assignment is implemanting a hearth rate monitor, which is made of two components:

- a Linux character-based driver (cDD) used to access a “virtual” Photopletismography (PPG) sensor
- a Linux user application (APP)

The APP performs an endless loop, where it samples the PPG sensor, and every 2048 acquired samples, it performs a 2048-points FFT,  computes the Power Spectral Density (PSD), identifies the base frequency of the signal (the frequency where the PSD is maximum), which is the heart rate in Hz, and it prints the heart rate in beat-per-minutes, according to the following specifications: 

- sampling frequency: 50 Hz
- number of samples to be acquired: 2048

The cDD provides access to the virtual PPG sensor. Each time the read function of the cDD is invoked, a pre-defined value is provided to the user application.

## How to use

Assuming that you already 

-  built and setup a ```pocky``` Linux distribution on your machine for Raspberry Pi 4 or qemuarm
-  created a layer called ```meta-example``` and  a directory called ```recipes-example``` inside it to store your recipes

open up a new terminal and act as follows, according to what target you want to the test it on (rpi or qemuarm).

### Raspberry Pi 4

```bash
cd poky
source oe-init-build-env build_rpi4
```

then, clone this repository as follows

```bash
cd ../meta-example/recipes-example/
git clone https://github.com/PronElle/OSESAssignment
```

At this point, application and the kernel module need to be added to the Linux distro configurations

```bash
vi ../../build_rpi4/conf/local.conf
```

and add the following lines at the end of the file

```bash
IMAGE_INSTALL_append = " app"
IMAGE_INSTALL_append = " ppgmod"
KERNEL_MODULE_AUTOLOAD += "ppgmod"
```

Then, the layer needs to be added to the Linux distro

```
vi ../../build_rpi4/conf/bblayers.conf
```

and replace its content with the following lines

```bash
BBPATH = "${TOPDIR}"
BBFILES ?= ""
BBLAYERS ?= " \
    /opt/poky/meta \
    /opt/poky/meta-poky \
    /opt/poky/meta-yocto-bsp \
    /opt/poky/meta-openembedded/meta-oe \
    /opt/poky/meta-openembedded/meta-multimedia \
    /opt/poky/meta-openembedded/meta-networking \
    /opt/poky/meta-openembedded/meta-python \
    /opt/poky/meta-raspberrypi \
    "
```

Eventually, build the new image

```bash
bitbake core-image-full-cmdline
```

and test the application running ```app``` from the command user interface.

### Quemuarm 

```bash
cd poky
source oe-init-build-env build_qemuarm
```

then, clone this repository as follows

```bash
cd ../meta-example/recipes-example/
git clone https://github.com/PronElle/OSESAssignment
```

At this point, application and the kernel module need to be added to the Linux distro configurations

```bash
vi ../../build_qemuarm/conf/local.conf
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

and run ```quemuarm```

```bash
runqemu qemuarm
```

test the application typing ```app``` from the command user interface.

## Timing

A sampling frequency of 50 Hz is equal to 20 ms sampling period. To ensure the proper timing, an alarm-based approach relying on ```SIGALARM``` was employed. This solution is rather better than a sleep-based approach because it both  reduces CPU usage and improves timing // TODO: to be explained better

## Memory Usage

//TODO: to be written