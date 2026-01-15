static struct kprobe kp_submit;
static int __init monitor_init(void) {  
    kp_submit.symbol_name = "usb_hcd_link_urb_to_ep";
    kp_submit.pre_handler = handler_usb_hcd_link_urb_to_ep_pre;    
    int ret = register_kprobe(&kp_submit);
    if (ret < 0) {
        printk(KERN_ERR "Failed kprobe: %d\n", ret);
        return ret;
    } 
    printk(KERN_INFO "USB FAKER: Loaded\n");   
    return 0;
}
static void __exit monitor_exit(void) {
    unregister_kprobe(&kp_submit);
    printk(KERN_INFO "USB FAKER: Unloaded\n");
}