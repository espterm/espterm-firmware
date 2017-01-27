#!/bin/bash

echo "-- Preparing WWW files --"

# Join scripts
DD=html_orig/jssrc
cat $DD/chibi.js \
	$DD/utils.js \
	$DD/modal.js \
	$DD/appcommon.js \
	$DD/term.js \
	$DD/wifi.js > html/js/app.js

sass --sourcemap=none html_orig/sass/app.scss html_orig/css/app.css

# No need to compress CSS and JS now, we run YUI on it later
cp html_orig/css/app.css    html/css/app.css
cp html_orig/term.html      html/term.tpl
cp html_orig/wifi.html      html/wifi.tpl
cp html_orig/wifi_conn.html html/wifi_conn.tpl
cp html_orig/img/loader.gif html/img/loader.gif
cp html_orig/favicon.ico    html/favicon.ico
