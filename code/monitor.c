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
    // FAT[0] должен быть 0xF8FFFFFF (little-endian)
    if (buffer[0] == 0xF8 && buffer[1] == 0xFF && 
        buffer[2] == 0xFF && buffer[3] == 0x0F) {
        // FAT[1] должен быть 0xFFFFFFFF (little-endian)
        if (buffer[4] == 0xFF && buffer[5] == 0xFF && 
            buffer[6] == 0xFF && buffer[7] == 0x0F) {
            return 1;  // Это начало FAT таблицы
        }
    }
    return 0;
}

static int is_likely_directory(const unsigned char *buf)
{
    // Первая запись: должна быть "." с атрибутами DIRECTORY
    if (buf[0] != 0x2e || buf[11] != 0x10) return 0;    
    // Проверяем что остальные символы имени - пробелы
    for (int i = 1; i < 8; i++) {
        if (buf[i] != 0x20) return 0;
    }    
    return 1;
}

static int is_free_region(const unsigned char *buffer) 
{    
    if (buffer[0] == 0x00 && buffer[1] == 0x00 && 
        buffer[2] == 0x00 && buffer[3] == 0x00) {
        if (buffer[4] == 0xFF && buffer[5] == 0xFF && 
            buffer[6] == 0xFF && buffer[7] == 0x0F) {
            return 1;
        }
    }
    return 0;
}

static void log_sg_data(struct urb *urb)
{
    struct scatterlist *sg;
    int i;
    printk(KERN_INFO "SG list: %d segments, total length: %u\n",
           urb->num_sgs, urb->transfer_buffer_length);    
    for_each_sg(urb->sg, sg, urb->num_sgs, i) {
        struct page *page;
        void *virt_addr;
        size_t len = sg->length;   
        printk(KERN_INFO "%zu %d", len, sg->dma_length);
        page = sg_page(sg);
        if (!page) {
            printk(KERN_INFO "  SG[%d]: NULL page\n", i);
            continue;
        }
        virt_addr = kmap_atomic(page);
        if (!virt_addr) {
            printk(KERN_INFO "  SG[%d]: kmap failed\n", i);
            continue;
        }
        virt_addr += sg->offset;
        printk(KERN_INFO "  SG[%d]: len=%zu\n", i, len);
        if (memcmp(virt_addr, VOLUME_NAME, strlen(VOLUME_NAME)) == 0)
            goto release;
        if (is_fat_table(virt_addr))
            goto release;
        if (is_likely_directory(virt_addr))
            goto release;
        if (is_free_region(virt_addr))
            goto release;

        print_hex_dump(KERN_INFO, "    Data: ", 
                      DUMP_PREFIX_OFFSET, 16, 1,
                      virt_addr, min(len, 512), true);

        unsigned char *data = virt_addr;
        for (size_t j = 0; j < len; j++) {
            data[j] ^= 0xAA;
        }

        print_hex_dump(KERN_INFO, "    Data-after: ", 
                      DUMP_PREFIX_OFFSET, 16, 1,
                      virt_addr, min(len, 512), true);
        if (urb->dev && urb->dev->bus && urb->dev->bus->sysdev) {
            dma_sync_single_for_device(urb->dev->bus->sysdev,
                                   sg_dma_address(sg),
                                   len,
                                   DMA_TO_DEVICE);
        }
release:
        virt_addr -= sg->offset;
        kunmap_atomic(virt_addr);
    }
}

static int handler_usb_hcd_link_urb_to_ep_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct urb *urb = (struct urb *)regs->si;
    if (!urb || !urb->dev || !urb->pipe)
        return 0;
    struct usb_device *udev = urb->dev;

    if (udev->descriptor.idVendor == VENDOR_ID && 
        udev->descriptor.idProduct == PRODUCT_ID && 
        usb_urb_dir_out(urb) == 1) {
        struct bulk_cb_wrap *cbw = (struct bulk_cb_wrap *)urb->transfer_buffer;
        if (cbw && cbw->Signature && le32_to_cpu(cbw->Signature) == US_BULK_CB_SIGN) {
            return 0;
        }
        log_sg_data(urb);
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