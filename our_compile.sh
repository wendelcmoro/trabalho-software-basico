#!/usr/bin/env bash
export PATH=$PATH:~/cmips/cMIPS/bin
cd ./cMIPS ; build.sh
cd ../
cp ./main.c ./cMIPS/tests/
cp ./handlerUART.s ./cMIPS/tests/
cd ./cMIPS/tests/
compile.sh main.c && mv prog.bin data.bin .. && (cd .. ; run.sh)
