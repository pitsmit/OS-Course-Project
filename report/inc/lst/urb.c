struct urb {
	...
	struct usb_device *dev;	
	...
	int status;	
	unsigned int transfer_flags;
	void *transfer_buffer;
	dma_addr_t transfer_dma;
	struct scatterlist *sg;
	int num_sgs;
	u32 transfer_buffer_length;
	u32 actual_length;
	...
	usb_complete_t complete;
};