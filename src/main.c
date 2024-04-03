#include "Entry.h"
#include "wm_regs.h"

#define RCC ((RCC_TypeDef *)RCC_BASE)

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

// __attribute__((isr)) void Default_Handler(void)
// {
//     printf("\n\r\n\r***ERROR***\n\r");
//     while(1);
// }

// void Error_Handler(void)
// {
//     printf("\n\r\n\r***ERROR***\n\r");
//     while(1);
// }

// void assert_failed(uint8_t *file, uint32_t line)
// {
//     printf("\n\r\n\r***ERROR***\n\r");
//     while(1);
// }
