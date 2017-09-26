#!/bin/bash

echo "-- Building parser from Ragel source --"

ragel -L -G0 user/ansi_parser.rl -o user/ansi_parser.c

sed -i "s/static const char _ansi_actions\[\]/static const char _ansi_actions\[\] ESP_CONST_DATA/" user/ansi_parser.c
sed -i "s/static const char _ansi_eof_actions\[\]/static const char _ansi_eof_actions\[\] ESP_CONST_DATA/" user/ansi_parser.c
