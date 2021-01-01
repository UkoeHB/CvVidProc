# README

<!-- must update setup.py version as well -->
version 0.3.0

Dependencies:

- OpenCV
- CMake
- compiler with c++14 support


## Install Python module cvvidproc

As simple as (can use `pip` or `pip3`):
```
pip install .
```

Note: rerunning `pip install .` will uninstall the existing installation and reinstall the new one, which makes it easy to install new versions (`git pull` -> `git merge origin/master` -> `pip install .`). Use `git remote add upstream 'upstream repo url'` to connect local repo with upstream/origin (upstream is developer repo, origin is remote clone i.e. in Github).


## Building and Running

Run cmake for debug or release:

```
// in command line, for debug build (slow version)
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Debug

// in command line, for release build (fast version)
cmake -S . -B BuildR -DCMAKE_BUILD_TYPE=Release
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


## License

[MIT license](https://opensource.org/licenses/MIT)

























