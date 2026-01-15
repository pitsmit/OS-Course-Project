int usb_submit_urb(struct urb *urb, gfp_t mem_flags)
{
    int xfertype;
    struct usb_device *dev;
    struct usb_host_endpoint *ep;
    dev = urb->dev;
    // определение конечной точки передачи
    ep = usb_pipe_endpoint(dev, urb->pipe);
    if (!ep)
        return -ENOENT;
    urb->ep = ep;
    // установка статуса URB - в процессе выполнения
    urb->status = -EINPROGRESS;
    urb->actual_length = 0;
    // определение типа передачи
    xfertype = usb_endpoint_type(&ep->desc);
    if (xfertype == USB_ENDPOINT_XFER_CONTROL) {
        ...
    }
    // обработка контроллером хоста
    return usb_hcd_submit_urb(urb, mem_flags);
}