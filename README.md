![Cryptomatte Logo](/docs/header.png)

# Cryptomatte for Arnold 5

This is the Arnold 5 implementation of [Cryptomatte](https://github.com/Psyop/Cryptomatte), by Jonah Friedman, Andy Jones, and Anders Langlands. Cryptomatte creates ID mattes automatically with support for motion blur, transparency, and depth of field, using names available in the Arnold scene at render time. 

Version 1.0.0. See [changelog](CHANGELOG.md) for version history. 

## Requirements

* Arnold 5.0.1 or later
* (Windows) Visual Studio 2015 Redistributable

## Documentation for Users

* [Main Cryptomatte for Arnold 5 shader documentation](/docs/cryptomatte.md)
* Usage in \*toA plugins (Helpfully provided by Solid Angle):
  * [MtoA (Maya)](https://support.solidangle.com/display/A5AFMUG/Cryptomatte)
  * [HtoA (Houdini)](https://support.solidangle.com/display/A5AFHUG/Cryptomatte)
  * [C4DtoA (Cinema 4D)](https://support.solidangle.com/display/A5AFCUG/Cryptomatte)
  * [3DSMaxToA (3D Studio Max)](https://support.solidangle.com/display/A5AF3DSUG/Cryptomatte)
* Nuke and Fusion documentation found in the [Main Cryptomatte repo](https://github.com/Psyop/Cryptomatte), as are links to Fnordware ProEXR builds and others.

## Documentation for Developers

### Building from source

This project uses CMake >= 2.8 to build. It has been tested on Mac OS X >=10.7, Windows 7 with MSVC++2015 and Centos6 with gcc4.4. On Windows, cmake-gui is recommended. 

Distributed builds are built with the following:
* (Windows): MSVC 14 (2015) Win64
* (Linux): Centos6.9 + Developer Toolset 7
* (OS X): OS X 10.9 

#### Set ARNOLD_ROOT and (optionally) install directories

In order to set it up to build in your environment you need to tell CMake where Arnold API is installed. Specify ARNOLD_ROOT in one of the following ways:
* Set ARNOLD_ROOT in your environment before running CMake
* Pass ARNOLD_ROOT to cmake directly as in:
  * `> cmake -DARNOLD_ROOT=<path> ..`
* Create a local.cmake file and set it in there
* On Windows, set in cmake-gui (see below)

By default, the shaders will be installed to build/dist. From there you can copy the files to the appropriate paths on your system. If you would like to install directly to a specific path you can set INSTALL_DIR as described for ARNOLD_ROOT above to install to ${INSTALL_DIR}/bin etc. Alternatively setting INSTALL_ROOT instead will install to ${INSTALL_ROOT}/${CM_VERSION}/ai${ARNOLD_VERSION}

#### Build (Linux and Mac OS X)

Once those variables are set, cd to the top-level CryptomatteArnold directory:

```
> mkdir build
> cd build
> cmake ..
> make
> make install
```

#### Build (Windows)

On Windows use CMakeGUI. 
1. Set the source directory to where you unpacked the source files
2. Set the build directory to be the same but with "\build" on the end. 
3. Use "Add Entry" to add `ARNOLD_ROOT`, set to point to the root directory of the Arnold API. 
4. Click "Configure", Select "Yes" when it asks if you want to create the build directory, and choose a Win64 compiller option, e.g. `Visual Studio 14 2015 Win64`. 
5. Click "Generate". This will create a Visual Studio project files in the build directory
6. Open a "Developer Command Prompt for VS2015"
7. cd to the top-level CryptomatteArnold directory
8. `> msbuild build\INSTALL.vcxproj /p:Configuration=Release`

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

## Thanks to

Many people have contributed to Cryptomatte for Arnold with code contributions, bug reports, reproductions, and technical advice. This list is certain to be incomplete. 

* Tony Barbieri
* Kevin Beason
* Benoit Leveau
* Alon Gibli
* Gaetan Guidet
* Sen Haerens
* Michael Heberlein
* Martin Kindl
* Martin Ögren
* Jean-Francois Panisset
* Sachin Shrestha
* Vahan Sosoyan
* Solid Angle
  * Stephen Blair
  * Marcos Fajardo
  * Mike Farnsworth
  * Angel Jimenez
  * Peter Horvath
  * Brecht Van Lommel
  * Frederic Servant
  * Ramon Montoya Vozmediano
* The Cryptomatte Committee
