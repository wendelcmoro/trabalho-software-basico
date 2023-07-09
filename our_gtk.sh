#!/usr/bin/env bash
export PATH=$PATH:~/cmips/cMIPS/bin
cd ./cMIPS/tests
(cd .. ; run.sh -w -v v_tx.sav)&
