void dma_sync_single_for_device(struct device *dev, dma_addr_t addr,
		size_t size, enum dma_data_direction dir)
{
    // Получение операций DMA для конкретного устройства
	const struct dma_map_ops *ops = get_dma_ops(dev);

	BUG_ON(!valid_dma_direction(dir));
    // прямое отображение
	if (dma_map_direct(dev, ops))
		dma_direct_sync_single_for_device(dev, addr, size, dir);
    // косвенное отображение через операции устройства
	else if (ops->sync_single_for_device)
		ops->sync_single_for_device(dev, addr, size, dir);
	debug_dma_sync_single_for_device(dev, addr, size, dir);
}