# fxload from Linux Hotplug project

Imported sources of `fxload` project from Linux Hotplug project. Fetched
from:

	http://downloads.sourceforge.net/project/linux-hotplug/fxload/2008_10_13/fxload-2008_10_13.tar.gz

	MD5 (/Users/wk/Downloads/fxload-2008_10_13.tar.gz) = 4477a2457f064228bef4a93ba2f21692

The `master` branch represents the project as is, from the `tar.gz`.

In the `multios` branch one has a fixed `fxload`, where I brought a
"backend" functionality--an ability to pick a way `fxload` gets to USB. One
can now pick LibUSB backend. On top of that, with LibUSB backend, once can
also pick a device by specifying vendor ID, instead of a path to the USB
device in the filesystem.

# How to use

Provided is the description for the `multios` branch:

	git clone https://github.com/wkoszek/fxload.git
	git checkout multios
	make
	./fxload -B
