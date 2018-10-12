#!/bin/sh
#Auto-generated embedded sdk setup script, generated Mi 9. Mai 11:51:48 CEST 2018.
chown root:root "/usr/local/share/ueye/ueyeusbd/ueyeusbd"
chown root:root "/usr/local/share/ueye/bin/ueyenotify"
chown root:root "/etc/init.d/ueyeusbdrc"
chown root:root "/etc/udev/rules.d/zz-ueyeusb.rules"
chown root:root "/usr/local/share/ueye/bin/mkcfgfiles"
chown root:root "/usr/local/share/ueye/ueyeethd/ueyeethd"
chown root:root "/etc/init.d/ueyeethdrc"
chown root:root "/etc/network/if-post-up.d/ueyeethdnotify"
chown root:root "/etc/network/if-pre-down.d/ueyeethdnotify"
chown root:root "/usr/lib/libueye_api.so.4.90"
chown root:root "/usr/include/ueye.h"
chown root:root "/usr/include/ueye_deprecated.h"
chown root:root "/usr/local/share/ueye/firmware/usb3" -R
chown root:root "/usr/local/share/ueye/firmware/usb3_addon" -R
chown root:root "/usr/local/share/ueye/bin/ueyesetid"
chown root:root "/usr/local/share/ueye/bin/ueyesetip"
chown root:root "/usr/local/share/ueye/bin/ueyefreeze"
chown root:root "/usr/local/share/ueye/bin/ueyelive"
chown root:root "/usr/local/share/ueye/bin/ueyedemo"
chown root:root "/usr/local/share/ueye/bin/idscameramanager"
chown root:root "/usr/local/share/ueye/licenses"
mkdir -p "/var/run/ueyed"
chown root:root "/var/run/ueyed"

SERVICESTARTOK=0
if test -e /usr/sbin/update-rc.d
then
    echo -n "Creating service start entry for ueyeusbdrc..."
    /usr/sbin/update-rc.d "ueyeusbdrc" defaults
    echo "Done."
    SERVICESTARTOK=1
elif test -e /sbin/chkconfig
then
    echo -n "Creating service start entry for ueyeusbdrc..."
    /sbin/chkconfig --add "ueyeusbdrc"
    echo "Done."
    SERVICESTARTOK=1
fi

if test ${SERVICESTARTOK} -eq 0
then
    echo "Could not create auto start entries. Please consult README.TXT."
fi


SERVICESTARTOK=0
if test -e /usr/sbin/update-rc.d
then
    echo -n "Creating service start entry for ueyeethdrc..."
    /usr/sbin/update-rc.d "ueyeethdrc" defaults
    echo "Done."
    SERVICESTARTOK=1
elif test -e /sbin/chkconfig
then
    echo -n "Creating service start entry for ueyeethdrc..."
    /sbin/chkconfig --add "ueyeethdrc"
    echo "Done."
    SERVICESTARTOK=1
fi

if test ${SERVICESTARTOK} -eq 0
then
    echo "Could not create auto start entries. Please consult README.TXT."
fi


echo "Generate ueyeusbd config..."
/usr/local/share/ueye/bin/mkcfgfiles -n /var/run/ueyed -o /usr/local/share/ueye/ueyeusbd/ueyeusbd.conf -u /usr/local/share/ueye -V usb

echo "Generate ueyeethd config..."
/usr/local/share/ueye/bin/mkcfgfiles -n /var/run/ueyed -o /usr/local/share/ueye/ueyeethd/ueyeethd.conf -u /usr/local/share/ueye -V eth

echo "Generate libueye_api config..."
/usr/local/share/ueye/bin/mkcfgfiles -n /var/run/ueyed -o /usr/local/share/ueye/libueye_api/machine.conf -u /usr/local/share/ueye -V api

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

#End of auto-generated embedded sdk setup script.
