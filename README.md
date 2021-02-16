# README

<!-- must update setup.py version as well -->
version 0.7.3

Dependencies:

- CMake
- compiler with c++14 support
- Python3

Dependencies that will autoinstall if absent:
- OpenCV
- NumPy
- pybind11


## Table Of Contents

- [About](#About)
- [Install Python Module](#Install-Python-module-`cvvidproc`)
- [Using Demo Program](#Building-and-Running-as-standalone-program)
- [Installing OpenCV on Ubuntu](#Installing-OpenCV-on-Ubuntu)
- [License](#License)


## About

`cvvidproc` is a small library with utilities for processing OpenCV videos. It was developed to support [Andy Ylitalo](https://github.com/andylitalo)'s bubble-tracking project with faster implementations of video processing than can be achieved in pure Python.

The available methods are:
- `GetVideoBackground()`: Obtains the 'background' of a video by computing the median of each pixel. Currently only a brute-force histogram algorithm is implemented, albeit in a multithreaded manner.
- `TrackBubbles()`: Tracks bubbles in a video, and reports back a dictionary of all bubbles encountered. Frame processing is implemented in C++, while bubble identification and tracking rely on a python function that must be injected by the caller.


## Install Python module `cvvidproc`

As simple as:
```
python3 setup.py clean && pip3 install .
```

For manually installed OpenCV, you must define these environment variables:
```
CV_DIR=path/to/location/of/opencv/library	// e.g. ~/MyLibs/
CV_INSTALL_DIR=path/to/opencv/installation 	// e.g. ~/MyLibs/OpenCV/opencv-1.2.3/release/

# full call for installing python module
python3 setup.py clean && \
	CV_DIR=~/MyLibs/ \
	CV_INSTALL_DIR=~/MyLibs/OpenCV/opencv-1.2.3/release/ \
	pip3 install .
```

- Rerunning this command after files are changed will update the installation. E.g. `git pull origin master` -> `git merge origin/master` -> `python3 setup.py ... (etc.)`. Use `git remote add upstream 'upstream repo url'` to connect local repo with upstream/origin (upstream is developer repo, origin is remote clone e.g. in Github).


## Building and Running as standalone program

Run cmake for debug or release:

```
// in command line, for release build (fast version)
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release

// in command line, for debug build (slow version)
cmake -S . -B BuildD -DCMAKE_BUILD_TYPE=Debug

// the OpenCV directory flags must have -D in this case
-DCV_DIR=~/MyLibs/
-DCV_INSTALL_DIR=~/MyLibs/OpenCV/opencv-1.2.3/release/

// for example
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release -DCV_DIR=~/MyLibs/ -DCV_INSTALL_DIR=~/MyLibs/OpenCV/opencv-1.2.3/release/
```

Build:

```
// in command line, for release build (fast version)
cmake --build Build

// in command line, for debug build (slow version)
cmake --build BuildD
```

Execute program.:

```
./Build/cvvidproc --vid=vid.mp4
./BuildD/cvvidproc --vid=vid.mp4
```

Notes:
- place videos in `Videos/` directory to make them easily findable by program, or pass the entire video path to `--vid_path=path/to/vid/your_vid.vid_ext`
- use tag `--help` to learn command line options


## Installing OpenCV on Ubuntu

If you have Linux, see [this resource](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html) for installing OpenCV. It may be necessary to use the `ninja` build generator instead of `make`. If, for example, you install OpenCV to the directory "`~`", then the command for generating a debug build system will be:

```
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release -D CV_DIR=/~/ -D CV_INSTALL_DIR=/~/opencv/build/
```

Ubuntu users may need to download and set up an 'X server for windows' by following [this tutorial](https://seanthegeek.net/234/graphical-linux-applications-bash-ubuntu-windows/). If your Ubuntu doesn't already have it, install GTK3 for backend GUI with:

```
sudo apt-get install libgtk-3-0
```


## License

[MIT license](https://opensource.org/licenses/MIT)

























