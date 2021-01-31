# README

<!-- must update setup.py version as well -->
version 0.5.1

Dependencies:

- CMake
- compiler with c++14 support

Dependencies that will autoinstall if absent:
- OpenCV
- NumPy
- pybind11


## Install Python module `cvvidproc`

As simple as:
```
pip3 install .
```

- Rerunning this command after files are changed will update the installation. E.g. `git pull origin master` -> `git merge origin/master` -> `pip3 install .`. Use `git remote add upstream 'upstream repo url'` to connect local repo with upstream/origin (upstream is developer repo, origin is remote clone e.g. in Github).
- If your project has a `_skbuild` directory you might need to delete it for a successful re-install.
- If you need to use a manually installed OpenCV, the only way right now is to go in `CMakeLists.txt` and directly edit `set(CV_DIR path/to/...)` and `set(CV_INSTALL_DIR path/to/...)`.


## Building and Running as standalone program

Run cmake for debug or release:

```
// in command line, for debug build (slow version)
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Debug

// in command line, for release build (fast version)
cmake -S . -B BuildR -DCMAKE_BUILD_TYPE=Release

// add these flags if relying on a manually installed version of OpenCV
-D CV_DIR=path/to/location/of/opencv/library	// e.g. ~/MyLibs/
-D CV_INSTALL_DIR=path/to/opencv/installation 	// e.g. ~/MyLibs/OpenCV/opencv-1.2.3/release/
```

Build:

```
// in command line, for debug build (slow version)
cmake --build Build

// in command line, for release build (fast version)
cmake --build BuildR
```

Execute program.:

```
./Build(R)/vid_bg --vid=vid.mp4
```

Notes:
- place videos in `Videos/` directory to make them easily findable by program, or pass the entire video path to `--vid_path=path/to/vid/your_vid.vid_ext`
- use tag `--help` to learn command line options


## Installing OpenCV on Ubuntu

If you have Linux, see [this resource](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html) for installing OpenCV. It may be necessary to use the `ninja` build generator instead of `make`. If, for example, you install OpenCV to the directory "`~`", then the command for generating a debug build system will be:

```
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Debug -D CV_DIR=/~/ -D CV_INSTALL_DIR=/~/opencv/build/
```

Ubuntu users may need to download and set up an 'X server for windows' by following [this tutorial](https://seanthegeek.net/234/graphical-linux-applications-bash-ubuntu-windows/). If your Ubuntu doesn't already have it, install GTK3 for backend GUI with:

```
sudo apt-get install libgtk-3-0
```


## License

[MIT license](https://opensource.org/licenses/MIT)

























