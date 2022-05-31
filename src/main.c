#include "Entry.h"
#include "wm_regs.h"

#define RCC ((RCC_TypeDef *)RCC_BASE)

//undefined function so defining it
size_t write(int fd, const void *buf, size_t count)
{
    return -1;
}

int main(void)
{
    RCC->CLK_EN &= ~0x3FFFFF;
    RCC->BBP_CLK = 0x0F;
    RCC->CLK_DIV = 0x83060302;

    Setup();

    while (1)
    {
        Loop();
    }
}