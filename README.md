![Cryptomatte Logo](/docs/header.png)

# Cryptomatte for Arnold 5

Version 1.0

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

This repo contains a test suite. Running it requires:

* Python 2.7
* Arnold 5.0.1+
* OpenImageIO (Python) 
* Build of AlShaders2 in ARNOLD_PLUGIN_PATH

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
