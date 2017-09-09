#!/bin/bash

echo "-- Preparing WWW files --"

[[ -e html ]] && rm -r html
mkdir -p html/img
mkdir -p html/js
mkdir -p html/css

cd front-end
sh ./build.sh
cd ..

cp front-end/js/app.js   html/js/
cp front-end/css/app.css html/css/

cp front-end/img/*       html/img/
cp front-end/favicon.ico html/favicon.ico

# cleanup
find html/ -name "*.orig" -delete
find html/ -name "*.xcf" -delete
find html/ -name "*~" -delete
