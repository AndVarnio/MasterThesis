#!/bin/sh
#Auto-generated embedded sdk uninstall script, generated Mi 9. Mai 11:51:48 CEST 2018.

SERVICESTARTOK=0
if test -e /usr/sbin/update-rc.d
then
    echo -n "Remove service start entry for ueyeusbdrc..."
    /usr/sbin/update-rc.d -f "ueyeusbdrc" remove
    echo "Done."
    SERVICESTARTOK=1
elif test -e /sbin/chkconfig
then
    echo -n "Remove service start entry for ueyeusbdrc..."
    /sbin/chkconfig --del "ueyeusbdrc"
    echo "Done."
    SERVICESTARTOK=1
fi

if test ${SERVICESTARTOK} -eq 0
then
    echo "Could not remove auto start entries."
fi


SERVICESTARTOK=0
if test -e /usr/sbin/update-rc.d
then
    echo -n "Remove service start entry for ueyeethdrc..."
    /usr/sbin/update-rc.d -f "ueyeethdrc" remove
    echo "Done."
    SERVICESTARTOK=1
elif test -e /sbin/chkconfig
then
    echo -n "Remove service start entry for ueyeethdrc..."
    /sbin/chkconfig --del "ueyeethdrc"
    echo "Done."
    SERVICESTARTOK=1
fi

if test ${SERVICESTARTOK} -eq 0
then
    echo "Could not remove auto start entries."
fi

rm -Rf "/usr/local/share/ueye/ueyeusbd/ueyeusbd"
rm -Rf "/usr/local/share/ueye/bin/ueyenotify"
rm -Rf "/etc/init.d/ueyeusbdrc"
rm -Rf "/etc/udev/rules.d/zz-ueyeusb.rules"
rm -Rf "/usr/local/share/ueye/bin/mkcfgfiles"
rm -Rf "/usr/local/share/ueye/ueyeethd/ueyeethd"
rm -Rf "/etc/init.d/ueyeethdrc"
rm -Rf "/etc/network/if-post-up.d/ueyeethdnotify"
rm -Rf "/etc/network/if-pre-down.d/ueyeethdnotify"
rm -Rf "/usr/lib/libueye_api.so.4.90"
rm -Rf "/usr/lib/libueye_api.so"
rm -Rf "/usr/lib/libueye_api.so.1"
rm -Rf "/usr/include/ueye.h"
rm -Rf "/usr/include/ueye_deprecated.h"
rm -Rf "/usr/include/uEye.h"
rm -Rf "/usr/local/share/ueye/bin/ueyesetid"
rm -Rf "/usr/bin/ueyesetid"
rm -Rf "/usr/local/share/ueye/bin/ueyesetip"
rm -Rf "/usr/bin/ueyesetip"
rm -Rf "/usr/local/share/ueye/bin/ueyefreeze"
rm -Rf "/usr/bin/ueyefreeze"
rm -Rf "/usr/local/share/ueye/bin/ueyelive"
rm -Rf "/usr/bin/ueyelive"
rm -Rf "/usr/local/share/ueye/bin/ueyedemo"
rm -Rf "/usr/bin/ueyedemo"
rm -Rf "/usr/local/share/ueye/bin/idscameramanager"
rm -Rf "/usr/bin/idscameramanager"
rm -Rf "/usr/bin/ueyecameramanager"
rm -Rf "/usr/local/share/ueye/licenses"
rm -Rf "/var/run/ueyed"
rm -Rf /usr/local/share/ueye

echo "Reload and retrigger udev to apply USB device rules..."
if test -e /sbin/udevadm
then
    /sbin/udevadm control --reload-rules
    /sbin/udevadm trigger --subsystem-match=usb --attr-match=idVendor=1409
elif test -e /sbin/udevcontrol -a -e /sbin/udevtrigger
then
    /sbin/udevcontrol reload_rules
    /sbin/udevtrigger --subsystem-match=usb --attr-match=idVendor=1409
else
    echo "Warning: Failed to reload and retrigger udev. Please reload udev rules manually and retrigger or reboot your system."
fi


echo "Run ldconfig to update library cache..."
if test -e /sbin/ldconfig
then
    /sbin/ldconfig
else
    echo "Warning: Failed to run ldconfig. Please run ldconfig manually or reboot your system."
fi

[ -e /tmp/sdkui.4925 ] && rm -Rf /tmp/sdkui.4925
#End of auto-generated embedded sdk uninstall script.
