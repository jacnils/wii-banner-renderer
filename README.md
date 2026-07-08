# wii-banner-renderer

[Showcase](https://www.youtube.com/watch?v=_UVSLDDvSd0)

**Note: Experimental, no support.**

This program allows you to render a Wii banner to a video file. It is based on the work of the Wii Banner Player team. See code files for license and copyright details.

## Usage

Call the program using the path to a *.wad*, *00000000.app* or *opening.bnr* as a parameter. It should generate an output.mp4 file in your working directory.

## Dependencies

- GLEW
- OpenGL
- OpenSSL
- EGL (Linux only)

## Building (Windows)

1. Install MSYS MINGW and set up an environment
2. Install dependencies
3. mkdir -p build
4. cd build
5. cmake ..
6. cmake --build .

## Building (Linux)

1. Install dependencies
2. mkdir -p build
3. cd build
4. cmake ..
5. cmake --build .

## Known bugs

- Text rendering is broken (or rather, not implemented fully)
- Animations don't quite work
- In some cases you might get an unplayable video file on some banners, cause is unknown.

## License

See specific code files.
