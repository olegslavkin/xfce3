#!/bin/sh
#
# Start-up stuff from ~/Desktop/Autostart (as it seems to be the new standard)
#
for i in `ls ${HOME}/Desktop/Autostart/`; do
  if test -x $HOME/Desktop/Autostart/$i; then
     $HOME/Desktop/Autostart/$i &
  fi
done
#
# Start the window manager 
#
mwm&
# Start "xfbd" for backdrop management
xfbd&
# Start "xfmouse" for userdefined mouse settings
xfmouse&
# And at last, start "xfce"
xfce
