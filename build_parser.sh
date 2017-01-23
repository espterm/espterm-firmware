#!/bin/bash

echo "-- Building parser from Ragel source --"

ragel -L -G0 user/ansi_parser.rl -o user/ansi_parser.c
