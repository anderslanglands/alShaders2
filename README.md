# alShaders2

For Cryptomatte documentation, see this repo's wiki. 

### Beta 2 release notes:

Features
* Test suite for developers
* Added Cinema 4D name parsing (Sen Haerens)
* aov_shader metadata for KtoA and MtoA

Bugs
* Restored Maya name in special cases (ns1:obj1|ns2:obj2)
* Fixed some crashes with malformed names, added tests for this
* Fixed metadata when rendering multiple Cryptomattes into one EXR file (Sen Haerens)
* Fixed metadata with multiple drivers if one driver did not support metadata (Sen Haerens)
* Fixed manifests in custom Cryptomattes
* Manifests no longer include list aggregate nodes (Sen Haerens)
* Fixed issue passing AtStrings to variadic arguments in gcc older than 4.8.x (Sachin Shrestha)
* Statically link the stdc++ lib (Sachin Shrestha)

### Developer info

This repo now contains a test suite. In addition to being useful during development, when submitting pull requests for new features, add tests for those features is encouraged as well. 

Running it requires:

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

