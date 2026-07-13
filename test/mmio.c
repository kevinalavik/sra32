void putc(int ch)
{
    *(volatile unsigned char *)0xFFF0 = (unsigned char)ch;
}

void puts(const char *str)
{
    while (*str)
        putc((unsigned char)*str++);
}

void _start(void)
{
    puts("Hello, World!\n");
    for (;;)
        ;
}