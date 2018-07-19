![Cryptomatte Logo](/docs/header.png)

# Cryptomatte for Arnold 5

Version 1.0.0

This is the Arnold 5 implementation of [Cryptomatte](https://github.com/Psyop/Cryptomatte). 

## User documentation

* [Main Cryptomatte for Arnold 5 shader documentation](/docs/cryptomatte.md)
* Solid Angle has helpfully provided documentation for usage in the *toA plugins
  * [MtoA (Maya)](https://support.solidangle.com/display/A5AFMUG/Cryptomatte)
  * [HtoA (Houdini)](https://support.solidangle.com/display/A5AFHUG/Cryptomatte)
  * [C4DtoA (Cinema 4D)](https://support.solidangle.com/display/A5AFCUG/Cryptomatte)
  * [3DSMaxToA (3D Studio Max)](https://support.solidangle.com/display/A5AF3DSUG/Cryptomatte)
* Nuke, Fusion, and links to After Effects compositor implementations are found in the [Main Cryptomatte repo](https://github.com/Psyop/Cryptomatte). 

## Change Log / Release Notes

See [changelog.](CHANGELOG.md)

## Developer info

### Building from source

This project uses CMake >= 2.8 to build. It has been tested on Mac OS X >=10.7, Windows 7 with MSVC++2015 and Centos6.5 with gcc4.2.1. On Windows, cmake-gui is recommended. 

#### Set ARNOLD_ROOT and (optionally) install directories

In order to set it up to build in your environment you need to tell CMake where Arnold is installed. Specify ARNOLD_ROOT in one of the following ways:
1. Set ARNOLD_ROOT in your environment before running CMake
2. Pass ARNOLD_ROOT to cmake directly as in:
> cmake -DARNOLD_ROOT=<path> ..
3. Create a local.cmake file and set it in there

By default, the shaders will be installed to build/dist. From there you can copy the files to the appropriate paths on your system. If you would like to install directly to a specific path you can set INSTALL_DIR as described for ARNOLD_ROOT above to install to ${INSTALL_DIR}/bin etc. Alternatively setting INSTALL_ROOT instead will install to ${INSTALL_ROOT}/${CM_VERSION}/ai${ARNOLD_VERSION}

On Windows, in cmake-gui, ARNOLD_ROOT and INSTALL_DIR can be set in the UI. 

#### Build (Linux and Mac OS X)

Once those variables are set, cd to the top-level CryptomatteArnold directory:
> mkdir build
> cd build
> cmake ..
> make
> make install

#### Build (Windows)

On Windows use CMakeGUI. 
1. Set the source directory to where you unpacked the source files
2. Set the build directory to be the same but with "\build" on the end. 
3. Click "Configure" and select "Yes" when it asks if you want to create the build directory.
4. Click "Generate". This will create a Visual Studio project file in the build directory which you can use to build the library
5. Open a `Developer Command Prompt for VS2015` 
6. cd to the top-level CryptomatteArnold directory
> msbuild build\INSTALL.vcxproj /p:Configuration=Release

### Tests

This repo contains a test suite. Running it requires:

* Python 2.7
* Arnold 5.0.1+
* OpenImageIO (Python) 
* Build of CryptomatteArnold in ARNOLD_PLUGIN_PATH

To run the unit and integration tests, cd to the root directory of this repo, and run: 

```
python tests
```

This will do several renders at low resolution, and compare the results to known right answers for
pixel values, metadata, and sidecar files. These tests should be robust enough to continue passing 
with changed sampling seeds, making them suitable for testing with newer versions of Arnold. The 
test suite has been designed to run quickly to allow fast iteration, though this means packing 
multiple tests into each render. 

This will also run unit tests written in C++. 

To run a subset of tests, wildcard filters can be used:

```
python tests -f Cryptomatte01*
```
