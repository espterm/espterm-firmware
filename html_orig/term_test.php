<?php

require '_test_env.php';

$f = file_get_contents('term.html');

$f = str_replace('%screenData%',
'{
 "w": 26, "h": 10,
 "x": 0, "y": 0,
 "cv": 1,
 "screen": "70 t259"
}', $f);

$f = str_replace('window.location.host', json_encode(ESP_IP), $f);

echo $f;
