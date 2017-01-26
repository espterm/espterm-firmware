<!DOCTYPE html>
<html>
<head>
	<meta charset="UTF-8">
	<title>ESP8266 Remote Terminal</title>
	<meta name="viewport" content="width=device-width,shrink-to-fit=no,user-scalable=no,initial-scale=1.0,maximum-scale=1.0,minimum-scale=1.0">

	<link rel="stylesheet" href="/css/app.css">
	<script src="/js/app.js"></script>
</head>
<body class="page-term">

<h1 onclick="location.href='/wifi'">ESP8266 Remote Terminal</h1>

<div id="termwrap">
	<div id="screen"></div>

	<div id="buttons">
		<button data-n="1" class="btn-blue">1</button><!--
		--><button data-n="2" class="btn-blue">2</button><!--
		--><button data-n="3" class="btn-blue">3</button><!--
		--><button data-n="4" class="btn-blue">4</button><!--
		--><button data-n="5" class="btn-blue">5</button>
	</div>
</div>

<script>
	_root = window.location.host;
	termInit(%screenData%);
</script>

</body>
</html>
