Versions in the file are noted as hex. 0x0203 means major version 2, minor version 3.

RoStr data type is a fixed number of bytes, with a 0-terminated string inside.
Vector3 data type are 3 4-byte floats (12 bytes)

# Main Structure

| Field         | Size     | Type                  | Version   | Description                                                                                                          |   |
|---------------|----------|-----------------------|-----------|----------------------------------------------------------------------------------------------------------------------|---|
| Header        | 4        | 4 chars               | All       | 4 characters GRSW                                                                                                    |   |
| Version       | 2        | 2 bytes               | All       | 2 byte version. First byte is the minor version, second byte is the major version.                                   |   |
| Build Number  | 1        | 1 byte                | >= 0x0202 | Build number. Seems to be another version byte                                                                       |   |
| Unknown       | 4        | 1 int                 | >= 0x0205 | Unknown value                                                                                                        |   |
| iniFile       | 40       | RoStr(40)             | All       | Unused nowadays. Used to have an ini file in the old Ro alpha / arcturus                                             |   |
| gndFile       | 40       | RoStr(40)             | All       | The GND file linked with this RSW file                                                                               |   |
| gatFile       | 40       | RoStr(40)             | > 0x0104  | The GAT file linked with this RSW file                                                                               |   |
| iniFile?      | 40       | RoStr(40)             | All       | Unused nowadays. Used to have an ini file in the old Ro alpha / arcturus                                             |   |
| Water Info    | ??       | [Water](#Water)       | < 0x0206  | Information about the water style on this map. This has been removed from the rsw, to the GND file in later versions |   |
| Lighting Info | 16 or 20 | [Lighting](#Lighting) | >= 0x105  | Information about the light.                                                                                         |   |
| BoundingBox   | 16       | 4 int                 | >= 0x106  | 4 bytes, probably indicating the bounds of the map. On a lot of maps these are (-500,-500) and (500,500)             |   |
| ObjectCount   | 4        | 1 int                 | All       | The number of objects in this RSW file                                                                               |   |
| Objects       | ??       | [Object](#Object)     | All       | A list of RSW Objects. These define the models, effects, lights and sounds on a map                                  |   |
| Quadtree      | ??       | [Quadtree](#Quadtree) | ??        | The quadtree information of this model                                                                               |   |


# Water

| Field                   | Size | Type  | Version   | Description                                                                                                                 |   |
|-------------------------|------|-------|-----------|-----------------------------------------------------------------------------------------------------------------------------|---|
| Height                  | 4    | Float | >= 0x0103 | The height of the water level                                                                                               |   |
| Type                    | 4    | Int   | >= 0x108  | The texture of the water                                                                                                    |   |
| Amplitude               | 4    | Float | >= 0x108  | The height of the waves. Is multiplied with a cos, so if height is 10, and amplitude is 5, the water waves between 5 and 15 |   |
| Wave Speed              | 4    | Float | >= 0x108  | The speed at which the waves travel                                                                                         |   |
| Wave Pitch              | 4    | Float | >= 0x108  | The width of the waves                                                                                                      |   |
| Texture Animation Speed | 4    | Float | >= 0x109  | The speed the water textures are switched. This is the number of frames (at 60FPS) it takes for the next frame to appear.   |   |


# Lighting

| Field          | Size | Type    | Version | Description                                                                                                                       |   |
|----------------|------|---------|---------|-----------------------------------------------------------------------------------------------------------------------------------|---|
| Longitude      | 4    | int     | All     | The longitude of the light                                                                                                        |   |
| Latitude       | 4    | int     | All     | The longitude of the light                                                                                                        |   |
| Diffuse Color  | 12   | Vector3 | All     | The diffuse color of the light                                                                                                    |   |
| Ambient Color  | 12   | Vector3 | All     | The ambient color of the light                                                                                                    |   |
| Shadow Opacity | 4    | Float   | All     | A factor that indicates how much the shadow affects lighting. At the moment in browedit this only affects ambient lighting though |   |


# Object

Objects first start with a number to identify what type of object it is.

## Models

| Field           | Size | Type    | Version                      | Description                        |   |
|-----------------|------|---------|------------------------------|------------------------------------|---|
| Type            | 4    | Int     | All                          | 1 for models                       |   |
| Name            | 40   | RoStr   | >= 0x0103                    | The name of this model             |   |
| Animation Type  | 4    | Int     | >= 0x0103                    |                                    |   |
| Animation Speed | 4    | Float   | >= 0x0103                    |                                    |   |
| Block Type      | 4    | Int     | >= 0x0103                    |                                    |   |
| Unknown         | 1    | Byte    | >= 0x0206, buildNumber > 161 | unknown value                      |   |
| Filename        | 40   | RoStr   | All                          | The name of the RSM model to place |   |
| Object name     | 80   | RoStr   | All                          | An extra name?                     |   |
| Position        | 12   | Vector3 | All                          | The position of this model         |   |
| Rotation        | 12   | Vector3 | All                          | The rotation of this model         |   |
| Scale           | 12   | Vector3 | All                          | The scale of this model            |   |



## Lights

| Field    | Size | Type     | Version   | Description                            |   |
|----------|------|----------|-----------|----------------------------------------|---|
| Type     | 4    | int      | All       | 2 for lights                           |   |
| Name     | 40   | RoStr    | >= 0x0103 | The name of this light                 |   |
| Position | 12   | Vector3  | All       | The position of this light             |   |
| Unknown  | 10   | 10 bytes | All       | Unknown values                         |   |
| Color    | 12   | Vector3  | All       | The color of this light                |   |
| Range    | 4    | Float    | All       | Supposed to be the range of this light |   |


## Sounds

| Field    | Size | Type    | Version                               | Description                       |   |
|----------|------|---------|---------------------------------------|-----------------------------------|---|
| Type     | 4    | int     | All                                   | 3 for sounds                      |   |
| Name     | 80   | All     | The name of this audio source         |                                   |   |
| Filename | 40   | All     | The filename of the wave file to load |                                   |   |
| Unknown  | 4    | Float   | All                                   | Unknown                           |   |
| Unknown  | 4    | Float   | All                                   | Unknown                           |   |
| Rotation | 12   | Vector3 | All                                   | The rotation of this audio source |   |
| Scale    | 12   | Vector3 | All                                   | The scale of this audio source    |   |
| Unknown  | 8    | 8 bytes | All                                   | Unknown values                    |   |
| Position | 12   | Vector3 | All                                   | The position of this audio source |   |
| Volume   | 4    | Float   | All                                   | Volume of this audio source       |   |
| Width    | 4    | Float   | All                                   | Width of this audio source        |   |
| Height   | 4    | Float   | All                                   | Height of this audio source       |   |
| Range    | 4    | Float   | All                                   | Range of this audio source        |   |


## Effects

| Field    | Size | Type    | Version                 | Description                                 |   |
|----------|------|---------|-------------------------|---------------------------------------------|---|
| Type     | 4    | int     | All                     | 4 for effects                               |   |
| Name     | 80   | All     | The name of this effect |                                             |   |
| Position | 12   | Vector3 | All                     | The position of this audio source           |   |
| Id       | 4    | Int     | All                     | The ID of this effect. 974 for 'lub effect' |   |
| Loop     | 4    | Float   | All                     | Looping of the effect                       |   |
| Unknown  | 4    | Float   | All                     | Looping of the effect                       |   |
| Unknown  | 4    | Float   | All                     |                                             |   |
| Unknown  | 4    | Float   | All                     |                                             |   |
| Unknown  | 4    | Float   | All                     |                                             |   |




# Quadtree