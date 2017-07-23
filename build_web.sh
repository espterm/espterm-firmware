#!/bin/bash

echo "-- Preparing WWW files --"

[[ -e html ]] && rm -r html
mkdir -p html/img
mkdir -p html/js
mkdir -p html/css

cd html_orig
sh ./packjs.sh
php ./build_html.php
cd ..

cp html_orig/js/app.js html/js/

sass --sourcemap=none html_orig/sass/app.scss html/css/app.css

cp html_orig/img/* html/img/

