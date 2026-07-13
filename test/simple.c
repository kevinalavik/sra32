void _start(void)
{
    *(volatile unsigned char *)0xFFF0 = 'H';
    *(volatile unsigned char *)0xFFF0 = 'I';
    *(volatile unsigned char *)0xFFF0 = '!';
    *(volatile unsigned char *)0xFFF0 = '\n';
    for (;;)
        ;
}