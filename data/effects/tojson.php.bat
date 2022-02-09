"c:\program files\php\php.exe" %0
pause
<?php

$data = file("effect_list.md");

$started = false;


$out = json_decode('{
    "id": 44,
    "loop": 40.0,
    "param1": 35.0,
    "param2": 35.0,
    "param3": 0.0,
    "param4": 0.0,
    "type": "rsweffect"
   }', true);

foreach($data as $line)
{
    $line = trim($line);
    if(substr(trim($line),0,3) == "--:")
        $started = true;
    else if($started)
    {
        $fields = explode("|", $line);
        for($i = 0; $i < count($fields); $i++)
            $fields[$i] = trim($fields[$i]);
        if(count($fields) != 3)
            continue;

        $out["id"] = $fields[0]+0;
        $out["desc"] = $fields[2];
        file_put_contents($fields[0] . " " . $fields[1] . ".json", json_encode($out));
    }
}