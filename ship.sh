#!/usr/bin/env bash

echo -n -e "\e[1;36m Version number (for file names): \e[0m"
read vers
echo

if [ -z "$vers" ]
    then
        echo -e "\e[0;31m Aborted.\e[0m"
        echo
        exit
fi

function buildlang() {
    lang=$1

    echo -e "\n\e[33;1m------ Building \"${lang}\" package ------\e[0m\n"

    make clean
    ESP_LANG=${lang} make web
    ESP_LANG=${lang} make actual_all -B -j4

    cp firmware/0x00000.bin release/0x00000.bin
    cp firmware/0x40000.bin release/0x40000.bin
    cd release

    destdir="$vers-$lang"
    file0=${vers}-0x00000-${lang}.bin
    file4=${vers}-0x40000-${lang}.bin
    [ -e ${destdir} ] && rm -r ${destdir}
    mkdir ${destdir}
    cp ../firmware/0x00000.bin ${destdir}/${file0}
    cp ../firmware/0x40000.bin ${destdir}/${file4}

    flashsh=${destdir}/flash.sh
    cp ../rel-tpl/flash.sh ${flashsh}
    sed -i s/%FILE0%/${file0}/ ${flashsh}
    sed -i s/%FILE4%/${file4}/ ${flashsh}
    sed -i s/%VERS%/${vers}/ ${flashsh}
    sed -i s/%LANG%/${lang}/ ${flashsh}
    chmod +x ${flashsh}

    readmefil=${destdir}/README.txt
    cp ../rel-tpl/README.txt ${readmefil}
    sed -i s/%VERS%/${vers}/ ${readmefil}
    sed -i s/%LANG%/${lang}/ ${readmefil}
    dt=$(LC_TIME=en_US.UTF-8 date '+%c %Z')
    sed -i "s#%DATETIME%#${dt}#" ${readmefil}
    unix2dos ${readmefil}

    cd ${destdir}
    sha256sum ${file0} ${file4} README.txt flash.sh > checksums.txt
    cd ..

    targetfile=espterm-${vers}-${lang}.zip
    [[ -e ${targetfile}.zip ]] && rm ${targetfile}.zip
    pwd
    zip -9 ${targetfile} ${destdir}/*
    #rm -r ${destdir}

    rm 0x00000.bin 0x40000.bin
    cd ..
}

buildlang cs
buildlang en
buildlang de
