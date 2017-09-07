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

$no_tpl_files = ['help', 'cfg_wifi_conn'];

$dest = ESP_DEMO ? __DIR__ . '/../html_demo/' : __DIR__ . '/../html/';

ob_start();
foreach($_pages as $_k => $p) {
	if ($p->bodyclass == 'api') {
		if (ESP_DEMO) {
			$target = 'term.html';
			echo "Generating: ~$_k.html -> $target\n";
			$s = "<!DOCTYPE HTML><meta http-equiv=\"refresh\" content=\"0;url=$target\">";
		} else {
			continue;
		}
	} else {
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
		ob_clean();
	}                                 // clean up
	$of = $dest . $_k . ((in_array($_k, $no_tpl_files)||ESP_DEMO) ? '.html' : '.tpl');
	file_put_contents($of, $s);                   // write to a file
}

ob_flush();
