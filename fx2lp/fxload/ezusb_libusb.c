/*
 * Copyright (c) 2010 Linux Hotplug Project
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
/*
 * This code is based on fxload-libusb for Microsoft Windows project done
 * by Claudio Favi.
 */
# if !defined(__linux__) || defined(LIBUSB_SUPPORT)
#ident "$Id$"

# include  <assert.h>
# include  <stdio.h>
# include  <errno.h>
# include  <limits.h>

# include  <sys/ioctl.h>

# include  <usb.h>

# include "ezusb.h"

static int ezusb_libusb_debug = 0;
#define DEBUG(...) do {					\
      if (ezusb_libusb_debug) {				\
	  printf("%s(%d): ", __func__, __LINE__);	\
	  printf(__VA_ARGS__);				\
	  printf("\n");					\
      }							\
} while (0)

/*
 * Issue a control request to the specified device.
 * This is O/S specific ...
 */
int ctrl_msg_libusb (
    struct ezusb_backend		*backend,
    unsigned char			requestType,
    unsigned char			request,
    unsigned short			value,
    unsigned short			index,
    unsigned char			*data,
    size_t				length
) {
    usb_dev_handle *device;
    int error;

    assert(backend != NULL && "internal error: no backend");
    assert(backend->priv_data != NULL && "no private data!");

    device = backend->priv_data;
    if (requestType == EZUSB_INPUT)
	requestType = USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
    else if (requestType == EZUSB_OUTPUT)
	requestType = USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE;

    error = usb_control_msg(device, (int)requestType, (int)request,
        (int)value, (int)index, (char*)data, (int)length, 10000);
    if (error < 0)
        logerror("problem with usb_control_msg(), returned %d, errno=%d\n",
	    error, errno);
    return error;
}

int ezusb_libusb_open(struct ezusb_backend *be, const char *device, int mode)
{
      struct usb_bus *bus = NULL;
      struct usb_device *dev = NULL;
      struct usb_device *found_dev = NULL;
      usb_dev_handle *devhandle = NULL;
      int ret = 0;
      unsigned int vid, pid;

      /*
       * For this backend, format for device is "vid=<VID>,pid=<PID>"
       */
      ret = sscanf(device, "vid=%x,pid=%x", &vid, &pid);
      if (ret != 2)
          ret = sscanf(device, "pid=%x,vid=%x", &pid, &vid);
      if (ret != 2) {
          logerror("wrong format; expected 'vid=<VID>,pid=<PID>' but got"
              " '%s'\n", device);
	  return -EINVAL;
      }

      usb_init();
      usb_set_debug(0);
      usb_find_busses();
      usb_find_devices();
      for (bus = usb_get_busses(); bus; bus = bus->next) {
           for (dev = bus->devices; dev; dev = dev->next) {
                DEBUG("USB device found: vendor: %#x product: %#x",
                        dev->descriptor.idVendor,
                        dev->descriptor.idProduct);
	        if (dev->descriptor.idProduct == pid &&
		    dev->descriptor.idVendor == vid) {
		    found_dev = dev;
		    break;
		}
           }
      }
      if (found_dev == NULL) {
          logerror("no device found");
	  return -ENOENT;
      }

      devhandle = usb_open(found_dev);
      if (devhandle == NULL) {
          logerror("couldn't open device handler!");
	  return -ENXIO;
      }
      be->priv_data = devhandle;
      return (0);
}

int ezusb_libusb_close(struct ezusb_backend *be)
{
      usb_dev_handle *devhandle = NULL;
      int error;

      assert(be != NULL && "no backend pointer (NULL)");
      assert(be->priv_data != NULL && "priv_data can't be NULL here");
      devhandle = be->priv_data;
      error = usb_close(devhandle);
      return error;
}

struct ezusb_backend ezusb_backend_libusb = {
      .name = "libusb",
      .priv_data = NULL,
      .open = ezusb_libusb_open,
      .close = ezusb_libusb_close,
      .ctrl_msg = ctrl_msg_libusb,
};

#endif /* LIBUSB_SUPPORT */
