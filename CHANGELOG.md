### CryptomatteArnold 1.0 release notes: 

* Renamed repo from AlShaders2 to CryptomatteArnold
* Code changes
  * Removed html doc generation
  * Moved documentation to this /docs

### (AlShaders2) Beta 4 release notes: 

Bug fixes:
* Fixed crash with multiple Cryptomatte shaders in scene. 
* Added error detection for issue where Cryptomatte filters do not get set up correctly, (reported by one user by not reproducible). 

### (AlShaders2) Beta 3 release notes:

Features
* Support Arnold 5.1 adaptive sampling
* Support for mixed bit-depth EXR files
* Support long object names (up to 2048 characters)
* Added preview_in_exr control
  * False (default): Omit preview channels when EXR drivers are being used
  * True: Keep behavior as usual
* Support C4DtoA Arnold 5.1 naming changes
* Add controls to disable various kinds of name processing
* Switched to Daniel Schmidt's faster hash to float code

Bug fixes
* Crash when rendering with non-aov shaders in Arnold 5.1
* Skip disabled nodes in manifest

### (AlShaders2) Beta 2 release notes:

Features
* Test suite for developers
* Added Cinema 4D name parsing (Sen Haerens)
* aov_shader metadata for KtoA and MtoA
* Unicode and special characters in names are now tested and supported

Bug fixes
* Restored Maya name in special cases (ns1:obj1|ns2:obj2)
* Fixed some crashes with malformed names, added tests for this
* Fixed metadata when rendering multiple Cryptomattes into one EXR file (Sen Haerens)
* Fixed metadata with multiple drivers if one driver did not support metadata (Sen Haerens)
* Fixed manifests in custom Cryptomattes
* Fixed manifests for standard cryptomattes with array (per poly) overrides 
* Fixed special characters (quotes, slashes) in manifests
* Manifests no longer include list aggregate nodes (Sen Haerens)
* Fixed issue passing AtStrings to variadic arguments in gcc older than 4.8.x (Sachin Shrestha)
* Statically link the stdc++ lib (Sachin Shrestha)
