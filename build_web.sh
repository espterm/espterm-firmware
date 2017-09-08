#!/bin/bash

echo "-- Preparing WWW files --"

[[ -e html ]] && rm -r html
mkdir -p html/img
mkdir -p html/js
mkdir -p html/css

cd html_orig
sh ./packjs.sh
sh ./packcss.sh
php ./build_html.php
cd ..

cp html_orig/js/app.js html/js/
cp html_orig/css/app.css html/css/

cp html_orig/img/* html/img/
cp html_orig/favicon.ico html/favicon.ico

# cleanup
find html/ -name "*.orig" -delete
find html/ -name "*.xcf" -delete
find html/ -name "*~" -delete
