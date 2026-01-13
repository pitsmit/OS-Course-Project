static int handler_usb_hcd_link_urb_to_ep_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct urb *urb = (struct urb *)regs->si;
    if (!urb || !urb->dev)
        return 0;
    struct usb_device *udev = urb->dev;

    if (udev->descriptor.idVendor == VENDOR_ID && 
        udev->descriptor.idProduct == PRODUCT_ID && 
        usb_urb_dir_out(urb) == 1) {
        struct bulk_cb_wrap *cbw = (struct bulk_cb_wrap *)urb->transfer_buffer;
        if (cbw && 
            cbw->Signature && 
            le32_to_cpu(cbw->Signature) == US_BULK_CB_SIGN) {
            return 0;
        }
        process(urb);
    }
    return 0;
}