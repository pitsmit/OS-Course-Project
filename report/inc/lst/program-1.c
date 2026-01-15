#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/usb.h>
#include <linux/scatterlist.h>
#include <linux/dma-buf.h>
#include <linux/highmem.h> 
#include <linux/usb/storage.h>
#include <linux/msdos_fs.h>

#define VENDOR_ID   0x0dd8
#define PRODUCT_ID  0x3801
#define VOLUME_NAME "MYFLASH"
static struct kprobe kp_submit;

static ssize_t find_first_nonzero(const unsigned char *buffer, size_t size)
{
    for (size_t i = 0; i < size; i++) 
    {
        if (buffer[i] != 0)
            return i;
    }
    return -1;
}

static int is_fat_table(const unsigned char *buffer) 
{    
    if (buffer[0] == 0xF8 && 
        buffer[1] == 0xFF && 
        buffer[2] == 0xFF && 
        buffer[3] == 0x0F &&
        buffer[4] == 0xFF && 
        buffer[5] == 0xFF && 
        buffer[6] == 0xFF && 
        buffer[7] == 0x0F) return 1;
    return 0;
}