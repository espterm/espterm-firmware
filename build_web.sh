#!/bin/bash

mkdir -p html
yuicompressor html_orig/style.css > html/style.css
yuicompressor html_orig/script.js > html/script.js
minify --type=html html_orig/index.html -o html/term.tpl
