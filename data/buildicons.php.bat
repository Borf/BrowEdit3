"c:\program files\php\php.exe" %0
pause
<?php

$img = imagecreatetruecolor(800,800);
imagesavealpha($img, true);
imagefill($img, 0,0,imagecolorallocatealpha($img,0,0,0,0));
imagealphablending($img, false);

$files = scandir("icons");

$order = array(
	"edit_height.png",	"edit_texture.png",	"edit_object.png", "edit_wall.png",							//0-3
		"edit_gat.png", "","","",																		//4-7
	"shadowmap_on.png",	"shadowmap_off.png","colormap_on.png", "colormap_off.png",						//8-11
		"tilecolor_on.png", "tilecolor_off.png", "toggle_lighting_on.png", "toggle_lighting_off.png",	//12-
	"texture_on.png", "texture_off.png", "smooth_color_on.png", "smooth_color_off.png",					//16-
		"emptytile_on.png", "emptytile_off.png", "all_lights_on.png", "all_lights_off.png",				//20-
	"lightsphere_on.png", "lightsphere_off.png", "","",													//24-
		"","","","",																					//28-
	"snap_grid_on.png","snap_grid_off.png","viewoptions.png","",														//32-
		"undo.png", "redo.png", "copy.png", "paste.png",												//36-
	"move.png", "rotate.png", "scale.png", "",															//40-
		"camera_ortho.png","camera_perspective.png","","",												//44-
	"pivot_rotation01.png","pivot_rotation02.png","","",												//48-
		"object_window_opened.png","object_window_closed.png","","",																					//52-
	"","","","",																						//56-
		"","","","missing.png",																			//60-63
);

foreach($files as $file)
	if(is_file("icons/" . $file) && !in_array($file, $order))
		echo "Warning! " . $file . " not used!\n";

for($i = 0; $i < count($order); $i++)
{
	if($order[$i] == "")
		continue;
	$sub = imagecreatefrompng("icons/" . $order[$i]);
	
	$x = ($i%8) * 100;
	$y = floor($i/8) * 100;

	imagecopy($img, $sub, $x, $y, 0,0,imagesx($sub), imagesy($sub));
}

print_r($order);
imagepng($img, "icons.png");