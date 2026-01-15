        struct bulk_cb_wrap *cbw = (struct bulk_cb_wrap *)urb->transfer_buffer;
        if (cbw && 
            cbw->Signature && 
            le32_to_cpu(cbw->Signature) == US_BULK_CB_SIGN) 
        {
            return 0;
        }
        process(urb);
    }
    return 0;
}

static int __init monitor_init(void)
{  
    kp_submit.symbol_name = "usb_hcd_link_urb_to_ep";
    kp_submit.pre_handler = handler_usb_hcd_link_urb_to_ep_pre;    
    int ret = register_kprobe(&kp_submit);
    if (ret < 0) 
    {
        printk(KERN_ERR "Failed kprobe: %d\n", ret);
        return ret;
    } 
    printk(KERN_INFO "USB FAKER: Loaded\n");   
    return 0;
}

static void __exit monitor_exit(void)
{
    unregister_kprobe(&kp_submit);
    printk(KERN_INFO "USB FAKER: Unloaded\n");
}

module_init(monitor_init);
module_exit(monitor_exit);
MODULE_LICENSE("GPL");