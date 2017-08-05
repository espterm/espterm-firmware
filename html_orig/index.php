<?php

require_once __DIR__ . '/base.php';

if (!isset($_GET['page'])) $_GET['page'] = 'term';

$_GET['PAGE_TITLE'] = $_pages[$_GET['page']]->title . ' :: ' . APP_NAME;
$_GET['BODYCLASS'] = $_pages[$_GET['page']]->bodyclass;

require __DIR__ . '/pages/_head.php';
$_pf = __DIR__ . '/pages/'.$_GET['page'].'.php';
if (file_exists($_pf)) {
	$f = file_get_contents($_pf);
	$reps = DEBUG ? require(__DIR__ . '/_debug_replacements.php') : [];
	$str = str_replace(array_keys($reps), array_values($reps), $f);

	// special symbols
	$str = str_replace('\,', '&#8239;', $str);
	$str = preg_replace('/(?<=\w)~(?=\w)/', '&nbsp;', $str);

	include_str($str);
} else {
	echo "404";
}

require __DIR__ . '/pages/_tail.php';
