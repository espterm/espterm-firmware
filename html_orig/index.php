<?php

require_once __DIR__ . '/base.php';

if (!isset($_GET['page'])) $_GET['page'] = 'term';

$_GET['PAGE_TITLE'] = $_pages[$_GET['page']]->title . ' :: ' . APP_NAME;
$_GET['BODYCLASS'] = $_pages[$_GET['page']]->bodyclass;

if (!function_exists('tplSubs')) {
	function tplSubs($str, $reps)
	{
		return preg_replace_callback('/%(j:|js:|h:|html:)?([a-z0-9-_.]+)%/i', function ($m) use ($reps) {
			$key = $m[2];
			if (array_key_exists($key, $reps)) {
				$val = $reps[$key];
			} else {
				$val = '';
			}
			switch ($m[1]) {
				case 'j:':
				case 'js:':
					$v = json_encode($val);
					return substr($v, 1, strlen($v) - 2);
				case 'h:':
				case 'html:':
					return htmlspecialchars($val);
				default:
					return $val;
			}
		}, $str);
	}
}

require __DIR__ . '/pages/_head.php';
$_pf = __DIR__ . '/pages/'.$_GET['page'].'.php';
if (file_exists($_pf)) {
	$f = file_get_contents($_pf);

	if (DEBUG)
		$str = tplSubs($f,  require(__DIR__ . '/_debug_replacements.php'));
	else $str = $f;

	// special symbols
	$str = str_replace('\,', '&#8239;', $str);
	$str = preg_replace('/(?<=\w)~(?=\w)/', '&nbsp;', $str);

	include_str($str);
} else {
	echo "404";
}

require __DIR__ . '/pages/_tail.php';
