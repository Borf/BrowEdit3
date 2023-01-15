# Object Edit

Object Edit mode is for editing objects. There are 4 types of objects; models, sounds, effects and lights.

## General editing of objects

In object edit mode, you will have the `Objects` window, the `Properties` window, and the `Object Picker` window, besides the main viewport, the toolbar and the main menubar

### Main Viewport

Click to select an object. Shift+click to select multiple objects. After selecting an object, a gadget will appear at the center of the selected models. You can drag the arrows on this gadget to manipulate the model. In the toolbar are 3 buttons (labeled move, rotate and scale) to set the effect of the gadget. Hold shift while manipulating to snap to the grid (see toolbar-grid)

There is also a rightclick menu available with the options
* **Set to floor height**: sets the selected items to the floor
* **Copy**: copies the selected items to the clipboard
* **Paste**: pastes the selected items from the clipboard
* **Select same**: see main menu-select same
* **Select near**: see main menu-select near

### Toolbar

The toolbar has some buttons specific for object editing.

### Grid

The snap to grid button (also enabled by holding shift) will enable the grid, and show grid options. There are 2 settings for the grid, the grid size and global or local grid. Enabling the option makes the grid local, disabling makes the grid global (this will be replaced with 2 different icons later). The local grid, is a grid relative to the original position or rotation of the model. If a model is at X-position 5, and the grid is set to units of 10, in local mode, it will snap at positions -5, 5, 15, 25... In global mode, the grid will snap to 0,10,20,30..., so it will snap relative to the world origin. 

### Objects Window

The objects window gives an overview of the loaded maps, and the objects on these maps. You can select models here, or doubleclick to go to them. Use ctrl+click to select multiple models

### Properties Window

The properties window is used to show the properties of the current selection. At the moment, there is a small difference in code for if a single object is selected, or when multiple objects are selected, so features might differ if one or multiple objects are selected.

Different objects will give different properties, but editing should work mostly the same.

By rightclicking certain properties, you can copy the values of a single propertly, like the position. Then select another model, and you can paste the properties into the other object. It will only copy 1 property at the same time, so either position, rotation or scale, not but at the same time.

#### Models
Models have a few properties
* **Animation Type**: no idea what this does
* **Animation Speed**: Never tested, but should set the animation speed?
* **Block type**: no idea what this does
* **filename**: The filename stored in the rsw file. This can be changed, but it won't be visualized in browedit (for now)
* **Cast Shadow**: This is a browedit custom property, and is not saved in the .rsw file but in the .extra.json file. Indicates if this model should be used in shadow casting

#### Effects
Effects are used by RO to show certain effects ingame. There are only a limited number of effects actually used by RO's maps. See [Object Picker](#Object_Picker) for information about the effect IDs. 
* **Type**: The ID of the effect
* **Loop**: time in milliseconds for it to loop
* **Param 1**: no idea what this does
* **Param 2**: no idea what this does
* **Param 3**: no idea what this does
* **Param 4**: no idea what this does

#### Sounds
Sounds are played by RO and can be used to give some more atmosphere to your maps. 
* **Filename**: The filename of the audio file. Should be a wav-file
* **Volume**: The volume of this audio effect. Is a value between 0 and 1, where 1 is the maximum volume
* **Width**: no idea what this does
* **Height**: no idea what this does
* **Range**: no idea what this does
* **Cycle**: no idea what this does
* **Unknown6**: no idea what this does
* **Unknown7**: no idea what this does
* **Unknown8**: no idea what this does

#### Lights

See [Lightmapping](Lightmapping.md)

### Object Picker

The object picker is a window used to place new objects on the map. The object picker can be opened using the ![ObjectPicker](../data/icons/object_window_closed.png) button in the toolbar. Browedit will automatically scan your rsm and wav files. For lights and effects, BrowEdit has a set of json files in the browedit folder that get scanned to easily add effects and sounds. The folders are scanned on startup for efficiency, so placing new files won't show up immediately. The tree on the left shows different categories and some structured folders, as they are structured in your grf files / data. Objects can be right-clicked for a popup with more information and some options to use these models.

To quickly find models, a filter and tagging-system is used. Every object can have tags attached to them, and the tags can be searched through quickly. To edit tags, rightclick the object, type the tag in the inputbox and hit the add button. 
Objects can automatically be tagged, based on the map. The button 'Rescan map filters', will open all maps in your grf/datafolder, scan which objects are used on them, and tag these objects with the map:\<name\> tag. This way, when searching for the map:prontera tag, you get all objects used on the prontera map. Rescanning the maps with the 'rescan map filters' button will remove all map:\<name\> tags, but it will keep all your custom tags.

Rightclicking will give some options

* **Add to map**: Add this object to the map
* **Replace selected models**: Replaces all selected rsm models with this model. You can use this to quickly replace a big number of models
* **Select this model**: Selects all of this model on the map. Used to easily find models

### Main Menu




<!--stackedit_data:
eyJoaXN0b3J5IjpbMTU1MDkyMjM4NSwtMTY3NjIwMDEyMV19
-->