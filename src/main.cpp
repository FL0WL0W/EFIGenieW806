/******************************************************************************
** 
 * \file        main.c
 * \author      IOsetting | iosetting@outlook.com
 * \date        
 * \brief       Demo code of PWM in independent mode
 * \note        This will drive 3 on-board LEDs to show fade effect
 * \version     v0.1
 * \ingroup     demo
 * \remarks     test-board: HLK-W806-KIT-V1.0
 *              PWM Frequency = 40MHz / Prescaler / (Period + 1)；
                Duty Cycle(Edge Aligned)   = (Pulse + 1) / (Period + 1)
                Duty Cycle(Center Aligned) = (2 * Pulse + 1) / (2 * (Period + 1))
 *
******************************************************************************/

#include "wm_regs.h"
#include "EmbeddedIOServiceCollection.h"
#include "EngineMain.h"
#include "CommunicationService_W80xUART.h"
#include "DigitalService_W80x.h"
#include "AnalogService_W80x.h"
#include "TimerService_W80x.h"
#include "PwmService_W80x.h"
#include "CommunicationHandlers/CommunicationHandler_EFIGenie.h"

using namespace EFIGenie;
using namespace EmbeddedIOServices;
using namespace EmbeddedIOOperations;
using namespace OperationArchitecture;

extern uint32_t _config;

extern "C"
{
    EmbeddedIOServiceCollection _embeddedIOServiceCollection = { 0, 0, 0, 0 };
    EngineMain *_engineMain = 0;
    CommunicationService_W80xUART *_uartService = 0;
    CommunicationHandler_EFIGenie *_efiGenieHandler = 0;
    communication_receive_callback_id_t _efiGenieHandlerCallbackID = 0;
    GeneratorMap<Variable> *_variableMap;
    tick_t prev;
    Variable *loopTime;

    void Setup(uint8_t startEngine);

    bool write(void *destination, const void *data, size_t length) {
        if(reinterpret_cast<size_t>(destination) >= 0x20000100 && reinterpret_cast<size_t>(destination) <= 0x20048000)
        {
            std::memcpy(destination, data, length);
        }
        else if(reinterpret_cast<size_t>(destination) >= 0x08002400 && reinterpret_cast<size_t>(destination) <= 0x08100000)
        {
            const size_t constDataAddress = reinterpret_cast<size_t>(data); //have to do this because the sdk has parameter as const even though they do not modify the data
            HAL_FLASH_Write(reinterpret_cast<uint32_t>(destination), reinterpret_cast<uint8_t *>(constDataAddress), length);
        }

        return true;
    }

    bool quit() {
        if(_engineMain != 0)
        {
            delete _engineMain;
            _engineMain = 0;
        }
        return true;
    }

    bool start() {
        if(_engineMain == 0)
        {
            Setup(1);
        }
        return true;
    }

    void Setup(uint8_t startEngine) 
    {
        if(_variableMap == 0)
            _variableMap = new GeneratorMap<Variable>();
        if(_uartService == 0)
            _uartService = CommunicationService_W80xUART::Create(0, 1024, 1024, 1000000, UART_WORDLENGTH_8B, UART_STOPBITS_1, UART_PARITY_NONE);

        if(_embeddedIOServiceCollection.DigitalService == 0)
            _embeddedIOServiceCollection.DigitalService = new DigitalService_W80x();
        if(_embeddedIOServiceCollection.AnalogService == 0)
            _embeddedIOServiceCollection.AnalogService = new AnalogService_W80x();
        if(_embeddedIOServiceCollection.TimerService == 0)
            _embeddedIOServiceCollection.TimerService = new TimerService_W80x(1,0,1000000);
        if(_embeddedIOServiceCollection.PwmService == 0)
            _embeddedIOServiceCollection.PwmService = new PwmService_W80x();

        const size_t configSize = _config + sizeof(uint32_t) + sizeof(uint32_t);
        size_t configgedSize;
        if(startEngine == 1)
            _engineMain = new EngineMain(&_config, configgedSize, &_embeddedIOServiceCollection, _variableMap);

        if(configSize != configgedSize)
        {
            delete _engineMain;
            _engineMain = 0;
        }

        if(_efiGenieHandler == 0)
        {
            _efiGenieHandler = new CommunicationHandler_EFIGenie(_variableMap, write, quit, start, &_config);
            _efiGenieHandlerCallbackID = _uartService->RegisterReceiveCallBack([&](communication_send_callback_t send, const void *data, size_t length){ return _efiGenieHandler->Receive(send, data, length); });
        }

        if(_engineMain != 0)
            _engineMain->Setup();
        loopTime = _variableMap->GenerateValue(250);
    }
    void Loop() 
    {
        const tick_t now = _embeddedIOServiceCollection.TimerService->GetTick();
        *loopTime = (float)(now-prev) / _embeddedIOServiceCollection.TimerService->GetTicksPerSecond();
        prev = now;
        _uartService->FlushReceive();

        if(_engineMain != 0)
            _engineMain->Loop();
    }


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
}