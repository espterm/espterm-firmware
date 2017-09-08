#!/bin/bash

echo "Building css..."

npm run sass -- --output-style compressed sass/app.scss > css/app.css
