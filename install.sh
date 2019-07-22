#!/bin/bash


insertBefore ()
{
    local file="$1" line="$2" newText="$3"
    sudo sed -i -e "/^$line$/i"'\\n'"$newText"'\n' "$file"
}


DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
TXTFILE="$DIR/test.txt"
REQFILE="$DIR/requirements.txt"
RCFILE="/etc/rc.local"

MATCHSTRING="masterMain.py"
EXITSTRING="exit 0"
PYTHONCOMMAND="sudo python3 $DIR/masterMain.py &"

if [ -e $RCFILE ]
then
  if ( ! grep -q $MATCHSTRING $RCFILE;) then
    echo "Adding startup command to $RCFILE"
    insertBefore $RCFILE "$EXITSTRING" "$PYTHONCOMMAND"
  else
    echo "$RCFILE is properly configured."
  fi
else
  echo "$RCFILE Does Not Exist"
fi

####################

#pip3 install -r $REQFILE