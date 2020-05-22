#!/usr/bin/env sh

TMPFILE=$(tempfile)

for _FP in "$@"; do
    echo $_FP
    xmllint --format "$_FP" > $TMPFILE
    cp $TMPFILE "$_FP"
done
