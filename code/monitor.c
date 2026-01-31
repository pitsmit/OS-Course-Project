#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/usb.h>
#include <linux/scatterlist.h>
#include <linux/dma-buf.h>
#include <linux/highmem.h> 
#include <linux/usb/storage.h>
#include <linux/msdos_fs.h>
#include <scsi/scsi_proto.h>

#define VOLUME_NAME "MYFLASH"

#define IS_VALID_CBW(cbw) \
    ((cbw) && (le32_to_cpu((cbw)->Signature) == US_BULK_CB_SIGN))

#define IS_MEANINGFUL_CBW(cbw) \
    ((((cbw)->Flags & 0x3F) == 0) && \
     ((cbw)->Lun < 1) && \
     ((cbw)->Length >= 1 && (cbw)->Length <= 16) && \
     (((cbw)->Flags & 0x40) == 0))

static struct kprobe kp_submit;

static int is_fsinfo(const struct fat_boot_fsinfo *buffer) 
{
    return IS_FSINFO(buffer);
}

static int is_root_dir(const struct msdos_dir_entry *dir)
{
    return (dir->attr & ATTR_VOLUME) && 
           (strncmp(dir->name, VOLUME_NAME, strlen(VOLUME_NAME)) == 0);
}

#define BYTES_SIZE_ENTRY 4

static int is_fat_table(const u32 *fat_entries, size_t size) 
{
    u32 prev = fat_entries[0];
    for (size_t i = 1; i < size / BYTES_SIZE_ENTRY; i++) 
    {
        if (fat_entries[i] == EOF_FAT32) break;
        if (fat_entries[i] == prev && prev == 0)
        {
            continue;
        }
        if (fat_entries[i] - 1 != prev && prev != EOF_FAT32)
        {
            return 0;
        }
        prev = fat_entries[i];
    }
    return 1;
}

static int is_path(const char *buf)
{
    return strncmp(buf, MSDOS_DOT, MSDOS_NAME) == 0;
}

static int is_empty(char *buf, size_t size)
{
    for (size_t i = 0; i < size; i++)
        if (buf[i] != 0) return 0;
    return 1;
}

static int is_trash_info(const char *buf) 
{
    return memcmp(buf, "[Trash Info]", 12) == 0;
}

static void process(struct urb *urb)
{
    struct scatterlist *sg;
    int i;
    void *virt_addr;
    struct page *page;
    size_t len;

    for_each_sg(urb->sg, sg, urb->num_sgs, i) 
    {
        len = sg->length;   
        page = sg_page(sg);
        if (page) 
        {
            virt_addr = kmap_local_page(page);
            if (virt_addr) 
            {
                virt_addr += sg->offset;
                printk(KERN_INFO "  SG[%d]: len=%zu\n", i, len);
                print_hex_dump(KERN_INFO, "    Data: ", 
                    DUMP_PREFIX_OFFSET, 16, 1,
                    virt_addr, min(len, 1024), true);                
                if (
                    is_root_dir(virt_addr) ||
                    is_fat_table(virt_addr, len) || 
                    is_fsinfo(virt_addr) ||
                    is_path(virt_addr) ||
                    is_empty(virt_addr, len) ||
                    is_trash_info(virt_addr)
                )
                {   
                    goto unmap;
                }

                get_random_bytes(virt_addr, len);

                if (urb->dev->bus && urb->dev->bus->sysdev) 
                {
                    dma_sync_single_for_device(
                        urb->dev->bus->sysdev,
                        sg_dma_address(sg),
                        len,
                        DMA_TO_DEVICE);
                }
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

static void print_urb(struct urb *urb) {
    printk(KERN_INFO "---- URB_INFO ----\n");
    pr_info("transfer_buffer_length: %d;\n"
            "transfer_buffer: %d;\n"
            "num_sgs: %d;\n"
            "URB_NO_TRANSFER_DMA_MAP: %d;\n"
            "transfer_dma: %llu;\n",
        urb->transfer_buffer_length, 
        urb->transfer_buffer ? 1 : 0,
        urb->num_sgs,
        urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP,
        urb->transfer_dma
    );
}

static void print_cdb(struct bulk_cb_wrap *bcb)
{
    printk(KERN_INFO "CDB (Length=%d):", bcb->Length);
    for (int i = 0; i < bcb->Length && i < 16; i++) {
        printk(KERN_CONT " %02x", bcb->CDB[i]);
    }
    printk(KERN_CONT "\n");
}

static int is_write_operation(struct bulk_cb_wrap *cbw)
{
    switch (cbw->CDB[0])
    {
    case WRITE_6:
    case WRITE_10:
    case WRITE_12:
    case WRITE_VERIFY_12:
    case WRITE_LONG_2:
    case WRITE_16:
    case WRITE_VERIFY_16:
    case WRITE_32:
    case WRITE_VERIFY_32:
        return 1;
    default:
        return 0;
    }
}

static int handler_usb_hcd_link_urb_to_ep_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct urb *urb = (struct urb *)regs->si;
    if (!urb || !urb->dev)
    {
        return 0;
    }
    struct usb_device *udev = urb->dev;
    struct usb_host_config *c = udev->config;
    struct usb_interface_descriptor *d = &c->intf_cache[0]->altsetting->desc;

    if (d->bInterfaceClass == USB_CLASS_MASS_STORAGE && 
        d->bInterfaceProtocol == USB_PR_BULK && 
        d->bInterfaceSubClass == USB_SC_SCSI &&
        usb_urb_dir_out(urb)) 
    {         
        if (urb->transfer_buffer) {
            if (urb->transfer_buffer_length > 0) {
                printk(KERN_INFO "Process TransferBuffer\n");
                struct bulk_cb_wrap *cbw = (struct bulk_cb_wrap *)urb->transfer_buffer;
                if (IS_VALID_CBW(cbw)       && 
                    is_write_operation(cbw) &&
                    IS_MEANINGFUL_CBW(cbw)  && 
                    cbw->DataTransferLength
                ) 
                {
                    print_cdb(cbw);
                    pr_info("It is a write command." 
                            "Due to BulkOnly, next URB will be with real data\n");
                }
                else {
                    pr_info("It is not a cbw in transfer_buffer." 
                            "Need to process(transfer_buffer)\n");
                }
            }
            else goto invalidURB;
        }
        else {
            if (urb->num_sgs > 0) {
                printk(KERN_INFO "Process SG\n");
                process(urb);
            }
            else {
invalidURB:
                printk(KERN_INFO "Invalid URB\n");
            }
        }
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