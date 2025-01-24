#include "Entry.h"
#include "wm_regs.h"

#define RCC ((RCC_TypeDef *)RCC_BASE)

uint8_t startEngine = 1;

int main(void)
{
    RCC->CLK_EN &= ~0x3FFFFF;
    RCC->BBP_CLK = 0x0F;
    RCC->CLK_DIV = 0x83060302;

    Setup(startEngine);

    while (1)
    {
        Loop();
    }
}

__attribute__((isr)) void Default_Handler(void)
{
    startEngine = 0;
    main();
}

void Error_Handler(void)
{
    startEngine = 0;
    main();
}

void assert_failed(uint8_t *file, uint32_t line)
{
    startEngine = 0;
    main();
}
