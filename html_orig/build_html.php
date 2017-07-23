<?php

require_once __DIR__ . '/base.php';

function process_html($s) {
	$pattern = '/<!--(.*)-->/Uis';
	$s = preg_replace($pattern, '', $s);

	$pattern = '/(?:(?:\/\*(?:[^*]|(?:\*+[^*\/]))*\*+\/)|(?:(?<!\:|\\\|\'|\")\/\/.*))/';
	$s = preg_replace($pattern, '', $s);

	$pattern = '/\s+/s';
	$s = preg_replace($pattern, ' ', $s);
	return $s;
}

ob_start();
foreach($_pages as $_k => $p) {
	if ($p->bodyclass == 'api') continue;
	echo "Generating: $_k ($p->title)\n";
	$_GET['page'] = $_k;
	ob_flush();                                   // print the message
	ob_clean();                                   // clean up
	include(__DIR__ . '/index.php');
	$s = ob_get_contents();                       // grab the output

	// remove newlines and comments
	// as tests have shown, it saves just a couple kilobytes,
	// making it not a very big improvement at the expense of ugly html.
//	$s = process_html($s);

	ob_clean();                                   // clean up
	$of = __DIR__ . '/../html/' . $_k . '.tpl';
	file_put_contents($of, $s);                   // write to a file
}

ob_flush();
