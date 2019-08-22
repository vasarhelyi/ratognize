# ratognize

Ratognize is an OpenCV-based video-processing tool for colored blob tracking, optimized for simple and fast execution on large scale footages with many individual blobs to track.

It was developed at Eötvös University, Department of Biological Physics, throughout the [EU ERC COLLMOT Research Grant](https://hal.elte.hu/flocking) for tracking painted animals (rats first, hence the name) for several hours, days or even months (don't worry, they were safe and treated well, we are still friends).

**Disclaimer**: The code was created for researchers by researchers, sorry if it does not meet your coding standards. Customer service is available though 0-24h ;)


# install

## prerequisites

* The code relies on OpenCv 2.x, you should install that first (yes, I know, it is old, I will upgrade only if you really need it).
Hint: [this github gist](https://gist.github.com/arthurbeggs/06df46af94af7f261513934e56103b30) seems to be a smooth way to go (on linux).

* Using [CUDA](https://developer.nvidia.com/cuda-zone) is also recommended for speedup.

* Finally, [ffmpeg](https://ffmpeg.org/) is also needed to get proper video outputs. 

The code base was tested both under Linux (with gcc) and Windows (with Visual Studio) and should run smoothly, although I know it never does. Feel free to ask and please help me if you have suggestions for improvement. 

## linux 

To use ffmpeg with CMake smoothly, you should first install the followings:

```
sudo apt install -y libavcodec-dev libavformat-dev libavdevice-dev libavfilter-dev
```

If you installed all prerequisits, just run `bootstrap.sh`, go to the `build` directory and run `make`.

## windows

For Windows, code was only tested in Visual Studio. There you need to setup your environment properly. The file called `user_macros.props` might be of help.


# usage

Ratognize is the second element of a toolchain that should be used properly to perform multiple colored object tracking on multiple videos (possibly on supercomputers) simultaneously and smoothly. Here is the total recommended workflow:

1. Create your color definitions with [ColorWheelHSV](https://github.com/vasarhelyi/ColorWheelHSV)
2. Setup and run [ratognize](https://github.com/vasarhelyi/ratognize) to get `.blob` data for all your defined colors.
3. Run [trajognize](https://github.com/vasarhelyi/trajognize) to get `.barcode` data for all the detected blobs.
4. Run **trajognize statistics** to analyze your data automatically.
5. If you have multiple video files to analyze, run **trajognize sum** to summarize data for all individual threads.

For minimal usage instructions of ratognize, run `ratognize --help`.

To setup ratognize properly, read and update the sample ini file `etc/configs/ratognize.ini`.

For any further questions on usage please contact.

## definitions

* **blob** - a blob is a single solid-colored area on an image that can be detected by ratognize with high efficiency.

* **barcode** - a barcode consists of several blobs next to each other in a row.
  each object you want to track must have a colored barcode on it, consisting
  of 3-4-5 blobs, depending on the total number of objects to be tracked.
  Barcodes should be unique, even if read backwards (RGB == BGR).
