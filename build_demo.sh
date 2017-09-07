#!/bin/bash

echo "-- Preparing WWW files --"

[[ -e html_demo ]] && rm -r html_demo
mkdir -p html_demo/img
mkdir -p html_demo/js
mkdir -p html_demo/css

cd html_orig
sh ./packjs.sh
ESP_DEMO=1 php ./build_html.php
cd ..

cp html_orig/js/app.js html_demo/js/

sass html_orig/sass/app.scss html_demo/css/app.css
rm html_demo/css/app.css.map

cp html_orig/img/* html_demo/img/
cp html_orig/favicon.ico html_demo/favicon.ico

# cleanup
find html_demo/ -name "*.orig" -delete
find html_demo/ -name "*.xcf" -delete
find html_demo/ -name "*~" -delete
