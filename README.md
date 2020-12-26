# Embedded Linux Assignment - Heart Rate Monitor

The goal of this assignment is implemanting a hearth rate monitor, which is made of two components:

- a Linux character-based driver (cDD) used to access a “virtual” Photopletismography (PPG) sensor
- a Linux user application (APP)

The APP performs an endless loop, where it samples the PPG sensor, and every 2048 acquired samples, it performs a 2048-points FFT,  computes the Power Spectral Density (PSD), identifies the base frequency of the signal (the frequency where the PSD is maximum), which is the heart rate in Hz, and it prints the heart rate in beat-per-minutes, according to the following specifications: 

- sampling frequency: 50 Hz
- number of samples to be acquired: 2048

The cDD provides access to the virtual PPG sensor. Each time the read function of the cDD is invoked, a pre-defined value is provided to the user application.

## How to test

Assuming a pocky Linux distribution has already been built and setup on your machine and you have already created a layer called ```meta-example``` ,  act as following:

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

```bitbake core-image-minimal```

and test the application running ```app``` from the command user interface.

## Timing

## Memory Usage

