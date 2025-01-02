#!/bin/bash

if [ "$#" != "2" ]; then
  echo "Specifica file di input e nome della classe"
  exit 1
fi
echo "#ifndef $2_H" > "$2.h"
echo "#define $2_H" >> "$2.h"
# echo "std::vector<unsigned char> $2 = {" >> "$2.h"
ffmpeg -i "$1" -ac 1 -ar 32000 -acodec pcm_s8 -f s8 - | xxd -n "$2" -i >> "$2.h"
# echo "};" >> "$2.h"
echo "#endif" >> "$2.h"
