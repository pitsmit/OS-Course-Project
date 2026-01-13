static void process(struct urb *urb){
    struct scatterlist *sg;
    int i; 
    for_each_sg(urb->sg, sg, urb->num_sgs, i) {
        size_t len = sg->length;   
        struct page *page = sg_page(sg);
        if (page) {
            void *virt_addr = kmap_atomic(page);
            if (virt_addr) {
                virt_addr += sg->offset;
                if (memcmp(virt_addr, VOLUME_NAME, strlen(VOLUME_NAME)) == 0 ||
                    is_fat_table(virt_addr) ||        
                    ((char*)virt_addr)[11] == 0x10 || 
                    ((char*)virt_addr)[0] == 0xE5)    
                    goto unmap;
                unsigned char *data = virt_addr;
                for (size_t j = 0; j < len; j++)
                    data[j] ^= 0xAA;
                if (urb->dev->bus && urb->dev->bus->sysdev) {
                    dma_sync_single_for_device(
                        urb->dev->bus->sysdev,
                        sg_dma_address(sg),
                        len, DMA_TO_DEVICE);
                }
unmap:
                virt_addr -= sg->offset;
                kunmap_atomic(virt_addr);
            }
            else printk(KERN_INFO "  SG[%d]: kmap failed\n", i);
        }
        else printk(KERN_INFO "  SG[%d]: NULL page\n", i);
    }
}