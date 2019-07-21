#!/bin/bash

#echo "$(ls)"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

TXTFILE="$DIR/test.txt"

echo $DIR
echo $TXTFILE

if [ -e $TXTFILE ]
then
  if ( ! grep -q "masterMain.py" $TXTFILE;) then
    echo "not in there"
  else
    echo "is in there"
  fi
else
  echo "File Does Not Exist"
fi
