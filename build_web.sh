#!/bin/bash

echo "-- Preparing WWW files --"

[[ -e html ]] && rm -r html

ESP_PROD=1 front-end/build.sh

echo "Copying from submodule..."

cp -r front-end/out html
