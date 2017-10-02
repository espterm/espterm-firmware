#!/usr/bin/env bash

# The parameters ESPTOOL, ESPPORT and ESPBAUD can be customized
# - export your preferred values in .bashrc

echo -e "\e[32;1mFlashing ESPTerm %VERS% (%LANG%)\e[0m"

if [ -z ${ESPTOOL} ]; then
	ESPTOOL='esptool'
	which ${ESPTOOL} &>/dev/null
	if [ $? -ne 0 ]; then
		ESPTOOL='esptool.py'
		which ${ESPTOOL} &>/dev/null
		if [ $? -ne 0 ]; then
			echo -e '\e[31;1mesptool not found!\e[0m'
			exit 1
		fi
	fi
fi

[ -z ESPPORT ] && ESPPORT=/dev/ttyUSB0
[ -z ESPBAUD ] && ESPBAUD=460800

set -x
${ESPTOOL} --port ${ESPPORT} --baud ${ESPBAUD} \
	write_flash 0x00000 '%FILE0%' 0x40000 '%FILE4%'
