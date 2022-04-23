"c:\program files\php\php.exe" %0
pause
<?php

$img = imagecreatetruecolor(1000,1000);
imagesavealpha($img, true);
imagefill($img, 0,0,imagecolorallocatealpha($img,0,0,0,0));
imagealphablending($img, false);

$files = scandir("icons");

$order = array(
	"edit_height.png",		"edit_texture.png",			"edit_object.png",			"edit_wall.png",		"edit_gat.png",			//00-04
	"",						"",							"",							"",						"",						//05-09
	"shadowmap_on.png",		"shadowmap_off.png",		"colormap_on.png",			"colormap_off.png",		"tilecolor_on.png",		//10-14
	"tilecolor_off.png",	"toggle_lighting_on.png",	"toggle_lighting_off.png",	"texture_on.png",		"texture_off.png",		//15-19
	"smooth_color_on.png",	"smooth_color_off.png",		"emptytile_on.png",			"emptytile_off.png",	"all_lights_on.png",	//20-24
	"all_lights_off.png",	"lightsphere_on.png",		"lightsphere_off.png",		"",						"",						//25-29
	"view_model_on.png",	"view_model_off.png",		"view_effect_on.png",		"view_effect_off.png",	"view_light_on.png",	//30-34
	"view_light_off.png",	"view_sound_on.png",		"view_sound_off.png",		"view_water_on.png",	"view_water_off.png",	//35-39
	"snap_grid_on.png",		"snap_grid_off.png",		"viewoptions.png",			"",						"",						//40-44
	"undo.png",				"redo.png",					"copy.png",					"paste.png",			"",						//45-49
	"move.png",				"rotate.png",				"scale.png",				"rotate_right.png",		"",						//50-54
	"camera_ortho.png",		"camera_perspective.png",	"mirror_horizontal.png",	"mirror_vertical.png",	"",						//55-59
	"pivot_rotation01.png",	"pivot_rotation02.png",		"",							"",						"",						//60-64
	"object_window_opened.png","object_window_closed.png","texture_window_opened.png","texture_window_closed.png","",				//65-69
	"texture_stamp.png",	"texture_bucket.png",		"texture_select.png",		"",						"",						//70-74
	"",						"",							"",							"",						"",						//75-79
	"",						"",							"",							"",						"",						//80-84
	"",						"",							"",							"",						"",						//85-89
	"",						"",							"",							"",						"",						//90-94
	"",						"",							"",							"",						"missing.png"			//95-99
);

foreach($files as $file)
	if(is_file("icons/" . $file) && !in_array($file, $order))
		echo "Warning! " . $file . " not used!\n";

for($i = 0; $i < count($order); $i++)
{
	if($order[$i] == "")
		continue;
	$sub = imagecreatefrompng("icons/" . $order[$i]);
	
	$x = ($i%10) * 100;
	$y = floor($i/10) * 100;

	imagecopy($img, $sub, $x, $y, 0,0,imagesx($sub), imagesy($sub));
}

print_r($order);
imagepng($img, "icons.png");