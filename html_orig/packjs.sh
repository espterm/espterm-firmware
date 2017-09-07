#!/bin/bash

echo "Packing js..."

cat jssrc/chibi.js \
	jssrc/keymaster.js \
	jssrc/utils.js \
	jssrc/modal.js \
	jssrc/notif.js \
	jssrc/appcommon.js \
	jssrc/lang.js \
	jssrc/wifi.js \
	jssrc/term_* \
	jssrc/term.js > js/app.js
