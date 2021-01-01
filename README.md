# README

<!-- must update setup.py version as well -->
version 0.4.0

Dependencies:

- OpenCV
- CMake
- compiler with c++14 support


## Install Python module `cvvidproc`

As simple as (can use `python` or `python3`):
```
python3 setup.py --force-cmake --quiet install 
```

Note: rerunning this command after files are changed will update the installation. E.g. `git pull origin master` -> `git merge origin/master` -> `python setup.py ...`. Use `git remote add upstream 'upstream repo url'` to connect local repo with upstream/origin (upstream is developer repo, origin is remote clone i.e. in Github).


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

























