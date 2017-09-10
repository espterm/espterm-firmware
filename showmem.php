#!/bin/env php
<?php

$dump = trim(shell_exec('xtensa-lx106-elf-size build/httpd_app.a -A'));

$pieces = explode("\n\n", $dump);

$all_sections = [];

$table = [];
foreach ($pieces as $piece) {
  $rows = explode("\n", trim($piece));
  if (!$rows) continue;
  list($name,) = explode(' ', trim($rows[0]));
  $entries = array_map(function($s) {
    $pcs = preg_split('/\s+/', $s);
    return [$pcs[0], $pcs[1]];
  }, array_slice($rows, 3, count($rows)-4));
  foreach($entries as $e) {
    list($space, $bytes) = $e;
    if (!in_array($space, $all_sections)) $all_sections[] = $space;
    $table[$name][$space] = $bytes;
  }
}

if ($argv[1]) {
  $key = $argv[1];
  uasort($table, function($a, $b) use ($key) {
    $av = $a[$key] ?? 0;
    $bv = $b[$key] ?? 0;
    return -($av <=> $bv); 
  });
}

$all_sections = array_diff($all_sections, [
  '.comment', 
  '.xtensa.info',
  '.xt.lit',
  '.xt.prop',
  '.literal',
  '.irom0.literal',
  '.data',
]);

$widths = [];
foreach($all_sections as $k) {
  $widths[$k] = max(8, strlen($k));
  printf("%".($widths[$k])."s ", $k);
}
echo "\n";//.str_repeat('-',array_sum($widths))."\n";

foreach ($table as $file => $map) {
  foreach ($all_sections as $space) {
    $b = isset($map[$space]) ? $map[$space] : 0;
    printf("%".$widths[$space]."d ", $b);
  }
  echo " $file\n";
}
