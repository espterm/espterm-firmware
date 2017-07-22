#!/bin/bash

echo "Packing js..."

cat jssrc/chibi.js \
	jssrc/utils.js \
	jssrc/modal.js \
	jssrc/notif.js \
	jssrc/appcommon.js \
	jssrc/term.js > js/app.js
