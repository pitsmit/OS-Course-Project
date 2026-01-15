struct usb_device {
	int		devnum;
	char		devpath[16];
	enum usb_device_state	state;
    ...
	struct usb_device_descriptor descriptor;
    ...
	unsigned can_submit:1;
    ...
	char *product;
	char *manufacturer;
	char *serial;
    ...
};