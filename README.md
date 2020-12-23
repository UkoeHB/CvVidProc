# README

version 0.2.0

Dependencies:

- OpenCV
- CMake
- compiler with c++14 support


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

























