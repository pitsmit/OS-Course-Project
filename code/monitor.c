#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/usb.h>
#include <linux/scatterlist.h>
#include <linux/dma-buf.h>
#include <linux/highmem.h> 
#include <linux/usb/storage.h>

#define VENDOR_ID   0x0dd8
#define PRODUCT_ID  0x3801
#define VOLUME_NAME "MYFLASH"
static struct kprobe kp_submit;

static int is_fat_table(const unsigned char *buffer) 
{    
    if (buffer[0] == 0xF8 && 
        buffer[1] == 0xFF && 
        buffer[2] == 0xFF && 
        buffer[3] == 0x0F &&
        buffer[4] == 0xFF && 
        buffer[5] == 0xFF && 
        buffer[6] == 0xFF && 
        buffer[7] == 0x0F)
            return 1;
    return 0;
}

static void process(struct urb *urb)
{
    struct scatterlist *sg;
    int i;
    //printk(KERN_INFO "SG list: %d segments, total length: %u\n",
    //       urb->num_sgs, urb->transfer_buffer_length);    
    for_each_sg(urb->sg, sg, urb->num_sgs, i) {
        struct page *page;
        void *virt_addr;
        size_t len = sg->length;   
        page = sg_page(sg);
        if (page) {
            virt_addr = kmap_atomic(page);
            if (virt_addr) {
                virt_addr += sg->offset;
                //printk(KERN_INFO "  SG[%d]: len=%zu\n", i, len);
                if (memcmp(virt_addr, VOLUME_NAME, strlen(VOLUME_NAME)) == 0 ||
                    is_fat_table(virt_addr) ||        // fat table?
                    ((char*)virt_addr)[11] == 0x10 || // directory?
                    ((char*)virt_addr)[0] == 0xE5)    // is free element?
                    goto unmap;

                //print_hex_dump(KERN_INFO, "    Data: ", 
                //            DUMP_PREFIX_OFFSET, 16, 1,
                //            virt_addr, min(len, 512), true);

                unsigned char *data = virt_addr;
                for (size_t j = 0; j < len; j++) {
                    data[j] ^= 0xAA;
                }

                //print_hex_dump(KERN_INFO, "    Data-after: ", 
                //            DUMP_PREFIX_OFFSET, 16, 1,
                //            virt_addr, min(len, 512), true);
                if (urb->dev->bus && urb->dev->bus->sysdev) {
                    dma_sync_single_for_device(urb->dev->bus->sysdev,
                                        sg_dma_address(sg),
                                        len,
                                        DMA_TO_DEVICE);
                }
unmap:
                virt_addr -= sg->offset;
                kunmap_atomic(virt_addr);
            }
            else {
                printk(KERN_INFO "  SG[%d]: kmap failed\n", i);
            }
        }
        else {
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
        usb_urb_dir_out(urb) == 1) {

        int res = 0;
        if (urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP) {
            res = 1;
            //printk(KERN_INFO "URB_NO_TRANSFER_DMA_MAP is here! %d", urb->transfer_buffer_length);
        }

        printk(KERN_INFO "%d, %d, %d, %s\n", res, urb->transfer_buffer_length, urb->num_sgs, (char*) urb->transfer_buffer);
        
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

static int __init monitor_init(void)
{  
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

static void __exit monitor_exit(void)
{
    unregister_kprobe(&kp_submit);
    printk(KERN_INFO "USB FAKER: Unloaded\n");
}

module_init(monitor_init);
module_exit(monitor_exit);
MODULE_LICENSE("GPL");