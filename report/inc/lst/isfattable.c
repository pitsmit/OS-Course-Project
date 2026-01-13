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