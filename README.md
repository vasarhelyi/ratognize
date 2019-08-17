# ratognize

Ratognize is an OpenCV-based cpp tool for colored barcode tracking on video inputs, optimized for simple and fast execution on large scale footages.

It was developed at Eötvös University, Department of Biological Physics, throughout the [EU ERC COLLMOT Research Grant](https://hal.elte.hu/flocking) for tracking painted animals (rats first, hence the name) for several hours, days or even months (don't worry, they were safe and treated well, we are still friends).

Ratognize is the second element of a toolchain that should be used together to perform multiple colored object tracking on multiple videos (possibly on supercomputers) simultaneously and smoothly:

1. Create your color definitions with [ColorWheelHSV](https://github.com/vasarhelyi/ColorWheelHSV)
2. Setup and run _ratognize_ to get .blob data for all your defined colors.
3. Run _trajognize_ to get .barcode data for all the detected blobs.
4. Run _trajognize statistics_ to analyze your data automatically.
5. If you have multiple video files to analyze, run _trajognize sum_ to
   summarize data for all individual threads.


# install

The code relies on OpenCv 2.x, you should install that first (yes, I know, it is old, I will upgrade only if you really need it).

The code base was tested both under Linux (with gcc) and Windows (with Visual Studio) and should run smoothly.

However, the code was created for researchers by researchers, so sorry if it does not meet your coding standards... 

Any recommendation or help is welcome!


#usage

To run ratognize, it is recommended to read and setup the sample ini file 'ratognize.ini', run ratognize and then check the output files.


##definitions

* _blob_ - a blob is a single solid-colored area on an image that can be detected by ratognize with high efficiency.

* _barcode_ - a barcode consists of several blobs next to each other in a row.
  each object you want to track must have a colored barcode on it, consisting
  of 3-4-5 blobs, depending on the total number of objects to be tracked.
  Barcodes should be unique, even if read backwards (RGB == BGR).