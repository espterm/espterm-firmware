#!/bin/bash

./build_demo.sh

cp -r html_demo/* ../espterm.github.io/

cd ../espterm.github.io/

echo "Enter to deploy (^C to abort):"
read

git add --all
git commit -m "Deploy updates"
git push
