int usb_hcd_link_urb_to_ep(struct usb_hcd *hcd, struct urb *urb)
{
	int		rc = 0;
    // обеспечение монопольного доступа
	spin_lock(&hcd_urb_list_lock);
	// отменён ли URB
	if (unlikely(atomic_read(&urb->reject))) {
		rc = -EPERM;
		goto done;
	}
    // могут ли URB быть отправлены на эту конечную точку
	if (unlikely(!urb->ep->enabled)) {
		rc = -ENOENT;
		goto done;
	}
    // может ли устройство отправлять запросы
	if (unlikely(!urb->dev->can_submit)) {
		rc = -EHOSTUNREACH;
		goto done;
	}
	// добавление urb в очередь конечной точки
	if (HCD_RH_RUNNING(hcd)) {
		urb->unlinked = 0;
		list_add_tail(&urb->urb_list, &urb->ep->urb_list);
	} else {
		rc = -ESHUTDOWN;
		goto done;
	}
done:
	spin_unlock(&hcd_urb_list_lock);
	return rc;
}