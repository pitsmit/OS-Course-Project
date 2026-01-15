unmap:
                virt_addr -= sg->offset;
                kunmap_local(virt_addr);
            }
            else 
            {
                printk(KERN_INFO "  SG[%d]: kmap failed\n", i);
            }
        }
        else 
        {
            printk(KERN_INFO "  SG[%d]: NULL page\n", i);
        }

    }
}

static int handler_usb_hcd_link_urb_to_ep_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct urb *urb = (struct urb *)regs->si;
    if (!urb || !urb->dev)
        return 0;
    struct usb_device *udev = urb->dev;

    if (udev->descriptor.idVendor == VENDOR_ID && 
        udev->descriptor.idProduct == PRODUCT_ID && 
        usb_urb_dir_out(urb) == 1) 
        {

        int is_flag = 0;
        if (urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP) 
        {
            is_flag = 1;
        }

        printk(KERN_INFO "%d, %d, %d, %s\n", is_flag, urb->transfer_buffer_length, urb->num_sgs, (char*) urb->transfer_buffer);