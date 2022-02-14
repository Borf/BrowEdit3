# Lightmapping

The BrowEdit lightmapper is a custom written, and is basically a software raytracer to calculate the light intensity on the ground in RO. Every tile (and wall) has a tiny 8x8 lightmap, where the edges of this lightmap are shared with the tiles around it. This means the actual lightmap information is only 6x6. Let's call these 6v6 lightmap pixels **luxels**. BrowEdit calculates the distance of every luxel to the lights around it, and if there is a 3d model inbetween the light and that luxel, this light won't be used. This calculation is not the same as the original lightmap calculations, so this page will try to explain how to set it up and how to get good lighting.

The lighting is based on different lights. 
* Sunlight  
  The sunlight is the basic method of lighting normal, outdoor maps, but is important for all maps. It can be configured using 4 settings. To change the lighting, click on the RSW map in *object edit* mode. In here, you can change the `Latitude`, `Longitude`, `Lightmap Ambient` and `Lightmap Intensity`.  The `Latitude` and `Longitude` are stored in the .rsw file and are used by the game to calculate the lighting ingame as well, the `Lightmap Ambient`and `Lightmap Intensity` are stored in the .extra.json file, and won't be used by the game. You can use `Latitude` and `Longitude` to control the direction the light is coming from. `Lightmap Ambient` controls the dark levels. `Lightmap Ambient` is a value between 0 and 1. Setting it to 0 means places in shadow are completely dark, setting it to 1 means the shadows are completely light. `Lightmap Intensity` is the value that is added on top of the `Lightmap Ambient` for areas that are lit by the sun. For a map that is properly lit, usually `Lightmap Ambient` and `Lightmap Intensity` added together should be 1
* Point light  
  Point lights are the lights you can drag around and add for a local light. They can be placed around the map and have a few important properties for lighting; `Position`, `Color`, `Range`, `Intensity` and `Cutoff`.
  * Range: this is what determines how far the light will reach.
  * Color: The color is used for the colormap. Leave it black for just shadow/light, like with the sunlight, but set it to a color to add color to the lighting. If the result is too bright, you can change the color to a darker tone of the same color to reduce its brightness
  * Light falloff type: There are several different presets, that can give you different effects. The further something is from a light, the less light they receive. Basically, the strengh of light decreases exponentially based on the distance of the light (basically something twice the distance receives half the light, though it is more complicated than that). When calculating lightmaps, BrowEdit calculates the distance of the ground to a point, and then calculates the light intensity based on that distance. This falloff type determines the algorithm used to calculate the light intensity
    * Exponential: 
      The default light setting. With an exponential light, you can set up a factor that determines how fast the light will fall off. 
    * Linear Tweak:  
      You can make a custom graph to set up the falloff. You should get a small graph with a line and some circles. The lines will go straight from circle to circle. The circles can be dragged to move them, you can click on the graph somewhere to add a point, and rightclick to remove a point
    * Lagrange Tweak:
      Similar to linear tweak, except the points are interpolated using the lagrange interpolation method. This gives smoother transitions, but can be a bit unstable when adding too many points (the graph will go up and down a lot)
    * Spline Tweak:
      Similar to Lagrange tweak, except the graph is more stable. This one is recommended for most control over your lights.
    * Magic: I'll explain what those values mean later, but you can read about it on [ogldev](https://ogldev.org/www/tutorial36/tutorial36.html) and [this blog](https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/). warning, it has math. This kind of light is supposed to be more realistic, but it is a bit hard to tweak and control

The lightmapper has some settings you can use to tweak it.
* Quality: this can improve the quality of your lightmap, with supersampling. With a quality of 1, exactly 1 ray is cast from the texel to the different lights. With a quality of 2, 4 rays are cast. quality of 3 gives 9 rays, 4 gives 16 rays, etc. The results of the rays are averaged for smoother shadow edges. 
* Thread count: The lightmapper is multithread optimized. This means that it can run on multiple computer threads at the same time. Tweak this value to be the amount of cores on your PC for faster calculations. If you don't want your computer to use up all its resources or overheat, lower the amount of cores. Using more threads than you have cores will work too, but it probably won't make it faster
* Debug Points: this shows the points that were sampled for the lightmapping process
* Range X, Range Y: you can use these to control the part of the map that is being lightmapped. Leave to default for the full map
<!--stackedit_data:
eyJoaXN0b3J5IjpbMTY2NDIxMjA1NV19
-->