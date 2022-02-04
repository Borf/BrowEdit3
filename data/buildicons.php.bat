"c:\program files\php\php.exe" %0
pause
<?php

$img = imagecreatetruecolor(800,800);
imagesavealpha($img, true);
imagefill($img, 0,0,imagecolorallocatealpha($img,0,0,0,0));
imagealphablending($img, false);

$files = scandir("icons");

$order = array(
	"edit_height.png", "edit_texture.png", "edit_object.png", "edit_wall.png", "edit_gat.png", "","","",
	"show_shadowmap.png", "show_lightmap.png", "show_texture.png", "show_gat.png", "show_lights.png", "lights.png", "grid.png","show_object.png",
	"undo.png", "redo.png", "copy.png", "paste.png", "move.png", "rotate.png", "scale.png", "",
	"camera_ortho.png","camera_perspective.png","","","","","","",
	"","","","","","","","",
	"","","","","","","","",
	"","","","","","","","",
	"","","","","","","","missing.png",
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