<?php

$f = file_get_contents('wifi.html');

$f = str_replace('%StaSSID%', 'Chlivek', $f);
$f = str_replace('%StaIP%', '192.168.0.15', $f);
$f = str_replace('%WiFiModeNum%', '1', $f);
$f = str_replace('%WiFiMode%', 'Client', $f);
$f = str_replace('%WiFiChannel%', '1', $f);

$f = str_replace('window.location.host', '"192.168.0.15"', $f);

echo $f;
