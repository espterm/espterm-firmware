#!/usr/bin/env bash

xtensa-lx106-elf-gcc -E -Iuser -Ilibesphttpd/include -Iesp_iot_sdk_v1.5.2/include -Iinclude $@
