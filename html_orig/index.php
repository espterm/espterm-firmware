<?php

require_once __DIR__ . '/base.php';

if (!isset($_GET['page'])) $_GET['page'] = 'term';

$_GET['PAGE_TITLE'] = $_pages[$_GET['page']]->title . ' :: ' . APP_NAME;
$_GET['BODYCLASS'] = $_pages[$_GET['page']]->bodyclass;

require __DIR__ . '/pages/_head.php';
$_pf = __DIR__ . '/pages/'.$_GET['page'].'.php';

$include_re = '/<\?php\s*(require|include)\s*\(?\s*?(?:__DIR__\s*\.)?\s*(["\'])(.*?)\2\s*\)?;\s*\?>/';

if (file_exists($_pf)) {
	$f = file_get_contents($_pf);

	// Resolve requires inline - they wont work after dumping the resulting file to /tmp for eval
	$f = preg_replace_callback($include_re, function ($m) use ($_pf) {
		$n = dirname($_pf).'/'.$m[3];
		if (file_exists($n)) {
			return file_get_contents($n);
		} else {
			return "<p>NOT FOUND: $n</p>";
		}
	}, $f);

	if (DEBUG)
		$str = tplSubs($f,  require(__DIR__ . '/_debug_replacements.php'));
	else $str = $f;

	// special symbols
	$str = str_replace('\,', '&#8239;', $str);
	$str = preg_replace('/(?<=[^ \\\\])~(?=[^ ])/', '&nbsp;', $str);
	$str = str_replace('\~', '~', $str);
	$str = preg_replace('/(?<![\w\\\\])`([^ `][^`]*?[^ `]|[^ `])`(?!\w)/', '<code>$1</code>', $str);
	$str = preg_replace('/(?<![\w\\\\])_([^ _][^_]*?[^ _]|[^ _])_(?!\w)/', '<i>$1</i>', $str);
	$str = preg_replace('/(?<![\w\\\\])\*([^ *][^*]*?[^ *]|[^ *])\*(?!\w)/', '<b>$1</b>', $str);

	$str = preg_replace("/\s*(\\\\\\\\)[\n \t]+/", '<br>', $str);
	include_str($str);
} else {
	echo "404";
}

require __DIR__ . '/pages/_tail.php';
