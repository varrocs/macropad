
#define ENDP_ADDR_SRL_DATA_OUT  0x01
#define ENDP_ADDR_SRL_DATA_IN   0x82
#define ENDP_ADDR_SRL_COMM_IN   0x83
#define IFACE_NUMBER_SRL_COMM		0
#define IFACE_NUMBER_SRL_DATA		1
#define ENDP_ADDR_HID_IN        0x84
#define IFACE_NUMBER_HID		2
#define ENDP_ADDR_MKEYS_IN        0x85
#define IFACE_NUMBER_MKEYS		3

#define USB_STRINGS_NUM 3

extern const char *usb_strings[USB_STRINGS_NUM];
extern const struct usb_config_descriptor usb_config;
extern const struct usb_device_descriptor usb_device_desc;
