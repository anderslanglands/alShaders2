## Cryptomatte
Cryptomatte in Arnold 5 sees significant changes. 

### Arnold 5 compared with Arnold 4 implementations. 
Cryptomatte is now an AOV shader, named simply "Cryptomatte". This allows Cryptomatte to support all shaders in your scene, without those shaders needing to be built for Cryptomatte, or using a passthrough. 

To install the shader in your scene, connect it to the AOV shaders port on the render globals. Check with the documentation for your *toA plugin for how to do this. 

### Global Cryptomatte Options
Global options are now on the shader. 

* Sidecar Manifests - Write the manifest to a sidecar .json file instead of into the header. Writing these is deferred until after the render, meaning that they work with deferred-loaded procedurals. 
* Cryptomatte Depth - Controls how many layers of Cryptomatte will be created, which is the number of matte-able objects that can exist per pixel. No case of users requiring more than 6 (default value) in production is known. 
* Strip Object Namespaces - Strips namespaces from objects in Maya or Softimage style naming. 
* Strip Material Namespaces - Strips namespaces from materials in Maya or Softimage style naming. 

The AOV ports define which AOVs the Cryptomattes are created in.

### Standard Cryptomattes Overrides
There are also override mechanisms for changing the names of objects. These are all controlled through user data. The user data may be per object, polygon, curve, instance, etc. Manifests work properly with these, with some exceptions, such as the case of integer offsets on instanced objects. 

#### String overrides 

    crypto_asset - Overrides the shape's name in the "asset" Cryptomatte, if defined. 
    crypto_object - Overrides the shape's name in the "object" Cryptomatte, if defined. 
    crypto_material - Overrides the shape's name in the "material" Cryptomatte, if defined. 

#### Integer "Offset" overrides 

    crypto_asset_offset
    crypto_object_offset
    crypto_material_offset

If one of these integer attributes is defined (on a shape, polygon, curve, instance, etc), it modifies the existing name. Unlike the string overrides, it only modifies the name, it does not replace it. The integer value of the user data is appended to the name. For example:

    object name: helmet
    crypto_object_offset: 4
    name in cryptomatte: helmet_4

On polygons or curves, this allows selection of certain clusters of polygons as if they were separate objects. On instances, it modifies the names of all the geometry in the instances. For example, if you have instances of tanks, and you put the user data onto the instance, then one tanks's thread material might be keyable as tread_material_1, and the next one might be keyable as tread_material_2.

## User-Defined Cryptomattes
In addition to the Cryptomattes made with name parsing, you can now create custom ones, driven by String user data. There are ports for defining four of them on the Cryptomatte shader. 

* Source Name: The name of the user data to be used. This must be string user data. If the user data is not present, the object will be part of the background. 
* AOV Name: The name of the AOV from which to create the Cryptomatte. This should be a custom RGBA AOV. 

Offset user data for user-defined cryptomattes is not supported. 

### Release notes
* New AOV shader named simply Cryptomatte with all global Cryptomatte options
  * Removed Arnold Render Options user data
* Added Sidecar manifests, which are deferred-written and work with deferred-loaded standins
* Support Integer Overrides
* Bug Fixes
  * Fixed Multiple cameras
  * Per-face shader assignments now work regardless of what shader is applied first (thanks to new AiShaderGlobalsGetShader() API from Solid Angle)
  * Matte objects work properly (omitted from Cryptomattes)
  * fixed issue with user cryptomattes behind transparency
* Internal changes
  * Uses float AOVs for less overhead
  * Opacity now uses built in opacity AOV rather than storing on own samples
  * cryptomatte, cryptomatte_filter, and cryptomatte_manifest_driver are all packaged into one executable
