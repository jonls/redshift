#!/bin/sh

# Kondor Dániel, 2018, kondor.dani@gmail.com
# simple script to adjust OLED (or similar) display brightness
# based on Ivo's answer here: https://askubuntu.com/questions/824949/lenovo-thinkpad-x1-yoga-oled-brightness
# but simplified to not update the 'official' but useless setting under /sys
# this way it does not need root priviliges
# also, added the possibility to integrate with Redshift

# usage: ./yoga-brightness.sh up|down
# in my case I could map these to the brightness keys of my laptop under
#	GNOME's keyboard shortcuts settings


dir=$XDG_RUNTIME_DIR
test -d "$dir" || exit 0

test -e "$dir/brightness" || echo 1.0 > "$dir/brightness"

VAL=`cat "$dir/brightness"`

if [ "$1" = "down" ]; then
    VAL=`echo "$VAL"-0.05 | bc`
else
    VAL=`echo "$VAL"+0.05 | bc`
fi

if [ `echo "$VAL < 0.0" | bc` = "1" ]; then
    VAL=0.0
elif [ `echo "$VAL > 1.0" | bc` = "1" ]; then
    VAL=1.0
fi

if [ -p "$dir/redshift-brightness" ]; then
    echo $VAL > "$dir/redshift-brightness"
else 
    xrandr --output eDP-1 --brightness $VAL
fi

echo $VAL > "$dir/brightness"
