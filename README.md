# README

<!-- must update setup.py and cmake versions as well -->
version 0.12.1

Dependencies:

- CMake
- compiler with c++14 support
- Python3
- scikit-build (`pip3 install scikit-build`)
- OpenCV (>= 4.2.0)
    - MacOS: `brew install opencv`
    - Windows: [tutorial](https://docs.opencv.org/master/d3/d52/tutorial_windows_install.html)
    - Linux: [Installing OpenCV on Ubuntu](#Installing-OpenCV-on-Ubuntu)
- NumPy (`pip3 install numpy`)
- pybind11 (`pip3 install pybind11`)


## Table Of Contents

- [About](#About)
- [Install Python Module](#Install-Python-module-`cvvidproc`)
- [Using Demo Program](#Building-and-Running-as-standalone-program)
- [Installing OpenCV on Ubuntu](#Installing-OpenCV-on-Ubuntu)
- [License](#License)


## About

`cvvidproc` is a small library with utilities for processing OpenCV videos. It was developed to support [Andy Ylitalo](https://github.com/andylitalo)'s bubble-tracking project with faster implementations of video processing than can be achieved in pure Python. See [API.md](./API.md) for the Python API.


## Install Python module `cvvidproc`

As simple as:
```
pip3 install .
```

For manually installed OpenCV, you must define these environment variables:
```
CV_DIR=path/to/location/of/opencv/library   // e.g. ~/MyLibs/
CV_INSTALL_DIR=path/to/opencv/installation  // e.g. ~/MyLibs/OpenCV/opencv-1.2.3/release/

# full call for installing python module
CV_DIR=~/MyLibs/ \
    CV_INSTALL_DIR=~/MyLibs/OpenCV/opencv-1.2.3/release/ \
    pip3 install .
```

- Rerunning this command after files are changed will update the installation. E.g. `git pull origin master` -> `git merge origin/master` -> `pip3 install .`. Use `git remote add upstream 'upstream repo url'` to connect local repo with upstream/origin (upstream is developer repo, origin is remote clone e.g. in Github).


## Building and Running as standalone program

Generate the build system:

```
// in command line
cmake -S . -B Build

// the OpenCV directory flags must have -D in this case
-DCV_DIR=~/MyLibs/
-DCV_INSTALL_DIR=~/MyLibs/OpenCV/opencv-1.2.3/release/

// for example
cmake -S . -B Build -DCV_DIR=~/MyLibs/ -DCV_INSTALL_DIR=~/MyLibs/OpenCV/opencv-1.2.3/release/
```

Build the project:

```
cmake --build Build
```

Execute program:

```
./Build/cvvidproc --vid=vid.mp4
```

Notes:
- place videos in `Videos/` directory to make them easily findable by program, or pass the entire video path to `--vid_path=path/to/vid/your_vid.vid_ext`
- use tag `--help` to learn command line options


## Installing OpenCV on Ubuntu

If you have Linux, here are all the recommended packages to install first, and steps to take.

- start by updating your existing packages
```
sudo apt-get update
sudo apt-get upgrade
```

- for video I/O
```
sudo apt-get install build-essential cmake unzip pkg-config
sudo apt-get install libjpeg-dev libpng-dev libtiff-dev
sudo apt-get install libavcodec-dev libavformat-dev libswscale-dev libv4l-dev
sudo apt-get install libxvidcore-dev libx264-dev
```

- for GTK as GUI backend
```
sudo apt-get install libgtk-3-dev
```

- for mathematical optimizations
```
sudo apt-get install libatlas-base-dev gfortran 
```

- FFMPEG and supporting packages for video support
```
sudo apt install ffmpeg libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
```

- gstreamer for opening and viewing mp4 files (not required for opencv, but nice to have if dealing with mp4s)
```
sudo apt install gstreamer1.0-libav
```

- packages for gstreamer to be found by cmake during OpenCV build
```
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
```

- download and unzip opencv source (from [OpenCV install tutorial](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html))
```
wget -O opencv.zip https://github.com/opencv/opencv/archive/master.zip
unzip opencv.zip
mv opencv-master opencv
mkdir -p build && cd build
```

- combined CMake flags from [here](https://github.com/UkoeHB/CvVidProc/issues/6), [here](https://www.pyimagesearch.com/2018/08/15/how-to-install-opencv-4-on-ubuntu/), [here](https://stackoverflow.com/questions/37678324/compiling-opencv-with-gstreamer-cmake-not-finding-gstreamer), and [here](https://stackoverflow.com/questions/28776053/opencv-gtk2-x-error)
```
cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local/ -D INSTALL_C_EXAMPLES=OFF -D BUILD_TEST=OFF -D WITH_FFMPEG=ON -D WITH_OPENCL=OFF -D BUILD_PERF_TESTS=OFF -D WITH_GTK=ON -D WITH_GSTREAMER=ON ..
```

- assuming you are in directory `opencv/build`, first build then install OpenCV (note: installing is optional, see [the tutorial's warning](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html) on that subject)
```
make -j4
sudo make install
```

If you decline to install with `sudo make install`, then you must manually link OpenCV to `cvvidproc`. If, for example, you build OpenCV in the directory "`~`", then the command for generating a build system will be:

```
cmake -S . -B Build -D CV_DIR=/~/ -D CV_INSTALL_DIR=/~/opencv/build/
```

Ubuntu users may need to download and set up an 'X server for Windows' by following [this tutorial](https://seanthegeek.net/234/graphical-linux-applications-bash-ubuntu-windows/). If your Ubuntu doesn't already have it, install GTK3 for backend GUI with:

```
sudo apt-get install libgtk-3-0
```


## License

[MIT license](https://opensource.org/licenses/MIT)
