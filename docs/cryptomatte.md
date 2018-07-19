## Cryptomatte

Cryptomatte in Arnold 5 is an AOV shader. 

Usage varies by DCC. In all DCCs however, it involves setting up a global AOV shader, though in Maya and Cinema 4D this is done automatically. AOV shaders are shaders which run on all samples after the main shading network has evaluated. When Cryptomatte is present, and a Cryptomatte AOV is active, it will handle the rest of the setup itself inside of Arnold. The setup includes creating more outputs, writing metadata and manifests, and installing the proper filters. The Cryptomatte Filter sidecar manifest driver are bundled in the same plugin, but should not be used manually by users. 

The basic procedure is as follows: 
1. Activate a Cryptomatte AOV. This should be configured to write to an **RGB EXR file**.
2. Connect a Cryptomatte shader to AOV shaders in the render globals. (In Maya and Cinema4D this is automatic and can be skipped). 
3. Render to EXR to disk. You'll get the AOV expanded into all the Cryptomatte data, with metadata. 

### Per-DCC documentation:
* Solid Angle has provided documentation for usage in the various *toA plugins:
  * [MtoA (Maya)](https://support.solidangle.com/display/A5AFMUG/Cryptomatte)
  * [HtoA (Houdini)](https://support.solidangle.com/display/A5AFHUG/Cryptomatte)
  * [C4DtoA (Cinema 4D)](https://support.solidangle.com/display/A5AFCUG/Cryptomatte)
  * [3DSMAXtoA (3D Studio Max)](https://support.solidangle.com/display/A5AF3DSUG/Cryptomatte)
* Nuke and Fusion documentation found in the [Main Cryptomatte repo](https://github.com/Psyop/Cryptomatte), as are links to Fnordware ProEXR builds and others. 

### Global Cryptomatte Options
Global options are on the Cryptomatte shader. 

#### Cryptomatte Globals
* Sidecar Manifests - Write the manifest to a sidecar .json file instead of into the header. Writing these is deferred until after the render, meaning that they work with deferred-loaded procedurals. 
* Cryptomatte Depth - Controls how many layers of Cryptomatte will be created, which is the number of matte-able objects that can exist per pixel. 6 is always plenty.
* Strip Object Namespaces - Strips namespaces from objects in Maya or Softimage style naming. See name processing. 
* Strip Material Namespaces - Strips namespaces from materials in Maya or Softimage style naming. See name processing. 

#### Standard Cryptomatte AOVS
The AOV ports define which AOVs the Cryptomattes are created in. It's recommended to leave these alone, as some DCCs rely on the names being set to default for some auto-setup features to work. 

#### Advanced Options
* Preview in EXR: Preview AOVs are what the various tutorials say to look at, but they are no longer actually used by the decoders, so they are dead weight. By default this is turned off, which means they don't write to EXRs. (Recommended off). 
* Name processing options: See name processing. 

#### User Cryptomattes
See user defined Cryptomattes below. 

## Overrides
### Standard Cryptomattes Overrides
There are also override mechanisms for changing the names of objects. These are all controlled through user data. The user data may be per object, polygon, curve, instance, etc. Manifests work properly with these, with some exceptions, such as the case of integer offsets on instanced objects. 

Setting user data in DCCs:
* Maya: [Use attributes named mtoa_constant_* on shapes](https://support.solidangle.com/display/AFMUG/Ai+UserData+Color)
* Cinema4D: [See User Data](https://support.solidangle.com/display/A5AFCUG/User+Data)
* Houdini: [See Attributes](https://support.solidangle.com/display/A5AFHUG/Attributes)

#### Name overrides (type: string)

To override names used by Cryptomatte, user data of the following names may be used. They should be applied to the shape.

* `crypto_asset` - Overrides the shape's name in the "asset" Cryptomatte, if defined. 
* `crypto_object` - Overrides the shape's name in the "object" Cryptomatte, if defined. 
* `crypto_material` - Overrides the shape's name in the "material" Cryptomatte, if defined. 

_Maya example: to override asset name, add a string parameter named `mtoa_constant_crypto_asset` to a shape, and set that value to whatever you would like the name to be._

#### "Offset" overrides (type: integer)

`crypto_asset_offset`
`crypto_object_offset`
`crypto_material_offset`

If one of these integer attributes is defined (on a shape, polygon, curve, instance, etc), it modifies the existing name. Unlike the string overrides, it only modifies the name, it does not replace it. The integer value of the user data is appended to the name. For example:

`object name: helmet`
`crypto_object_offset: 4`
`name in cryptomatte: helmet_4`

On polygons or curves, this allows selection of certain clusters of polygons as if they were separate objects. On instances, it modifies the names of all the geometry in the instances. For example, if you have instances of tanks, and you put the user data onto the instance, then one tanks's thread material might be keyable as tread_material_1, and the next one might be keyable as tread_material_2.

_Maya example: to override asset name, add a integer parameter named `mtoa_constant_crypto_asset_offset` to a shape, and set that value to whatever you would like the offset to be._

## User-Defined Cryptomattes (driven by string user data)
In addition to the Cryptomattes made with name parsing, you can now create custom ones, driven by String user data. There are ports for defining four of them on the Cryptomatte shader. 

* Source Name: The name of the user data to be used. This must be string user data. If the user data is not present, the object will be part of the background. 
* AOV Name: The name of the AOV from which to create the Cryptomatte. This should be a custom RGBA AOV. 

"Offset" overrides for user-defined cryptomattes are not supported. 

_Maya example: to create a user-defined Cryptomatte:
(1) Add the Cryptomatte shader to the render globals (if there isn't already one in the scene). 
(2) Create a custom AOV (e.g. "MyCryptomatte"). 
(3) Define some user data (e.g. "MyUData"), On the Cryptomatte shader. 
(4) Set "Source Name" to "MyUData" and "AOV Name" to "MyCryptomatte"._

## Name Processing

All mattes are based on names. If two objects are named "horse", either through overrides or processed names, they will both belong to the same matte. Object names are material names are used for mattes. 

Asset matte might be better called "namespace". It contains, by default, information that is extracted from the object name. This is complicated because different DCCS have wildly different naming conventions, and may be mixed style in the same scenes when sharing ass files. 

```
Input name: `soldier:helmetShape`
Asset name: `soldier`
Object Name ("strip object namespaces" on): `helmetShape`
Object Name ("strip object namespaces" off): 'soldier:helmetShape`. 
Note: If "strip object namespaces" is on, soldier:helmetShape and soldier2:helmetShape would share the same object matte but not the same asset matte. 
```

## Basic Options
These options control whether namespaces are stripped. The advanced options control how namespaces are interpreted.
* Strip Object Namespaces: Strip namespaces from object names in object matte
* Strip Material Namespaces: Strip namespaces from material names in material matte

## Advanced Options
* Maya Names: (e.g. `namespace:obj`). If disabled, colons are considered as defining namespaces. 
* Path-Style names: (e.g. `/obj/path/name`). If enabled, anything before the last forward slashes are considered as defining namespaces.
* Obj Path Pipes: (e.g. `|obj|path|name`) Consider pipes (|) part of the object path. (For older C4D)
* Strip Material Pipes: (e.g. `/a/b/c|standard_surface -> /a/b/c`) In paths, cut material names after pipes (|). (For C4D). 
* Legacy Styles: Includes old C4D style, as well as Softimage.

## Arnold 5 compared with Arnold 4 implementations. 
Cryptomatte in Arnold 5 sees significant changes. Cryptomatte is now an AOV shader, named simply "Cryptomatte". This allows Cryptomatte to support all shaders in your scene, without those shaders needing to be built for Cryptomatte, or using a passthrough. 
