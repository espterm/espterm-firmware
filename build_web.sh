#!/bin/bash

echo "-- Preparing WWW files --"

[[ -e html ]] && rm -r html

cd front-end
ESP_PROD=1 sh ./build.sh
cd ..

echo "Copying from submodule..."

cp -r front-end/out html
