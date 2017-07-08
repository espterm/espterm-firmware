<?php

require '_test_env.php';

$f = file_get_contents('wifi.html');

$f = str_replace('%StaSSID%', 'Chlivek', $f);
$f = str_replace('%StaIP%', json_encode(ESP_IP), $f);
$f = str_replace('%WiFiModeNum%', '1', $f);
$f = str_replace('%WiFiMode%', 'Client', $f);
$f = str_replace('%WiFiChannel%', '1', $f);

$f = str_replace('window.location.host', json_encode(ESP_IP), $f);

echo $f;
