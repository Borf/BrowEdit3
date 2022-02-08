"c:\program files\php\php.exe" %0
pause
<?php

$data = file("effect_list.md");

$started = false;
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
        file_put_contents($fields[0] . " " . $fields[1] . ".json", "{}");
    }
}