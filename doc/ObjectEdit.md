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
* **Animation Speed**: Never tested
* **Block type**: no idea what this does
* **filename**: The filename stored in the rsw file. This can be changed, but it won't be visualized in browedit (for now)
* **Cast Shadow**: This is a browedit custom property, and is not saved in the .rsw file but in the .extra.json file. Indicates if this model should be used in shadow casting

#### Effects

#### Sounds

#### Lights

See [Lightmapping](Lightmapping.md)

### Object Picker


### Main Menu




<!--stackedit_data:
eyJoaXN0b3J5IjpbMTU1MDkyMjM4NSwtMTY3NjIwMDEyMV19
-->