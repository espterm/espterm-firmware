<!doctype html>
<html>
<head>
	<meta charset="utf-8">
	<meta http-equiv="X-UA-Compatible" content="IE=edge">
	<meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">
	<title><?= PAGE_TITLE ?></title>
	<link href="/css/app.css" rel="stylesheet">
	<script src="/js/app.js"></script>
	<script>var _root = <?= json_encode(LIVE_ROOT) ?>;</script>
</head>
<body class="<?= BODYCLASS ?>">
<div id="outer">
<?php
$cfg = false;
if ($_pages[CUR_PAGE]->bodyclass == 'cfg') {
	$cfg = true;
	require __DIR__ . '/_cfg_menu.php';
}
?>

<div id="content">
<img src="/img/loader.gif" alt="Loading…" id="loader">
<?php if ($cfg): ?>
<h1><?= tr('menu.' . CUR_PAGE) ?></h1>
<?php endif; ?>
