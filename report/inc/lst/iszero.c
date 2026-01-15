static ssize_t find_first_nonzero(const unsigned char *buffer, size_t size)
{
    for (size_t i = 0; i < size; i++) 
    {
        if (buffer[i] != 0)
            return i;
    }
    return -1;
}