#include "EngineMain.h"
#include "EmbeddedIOServiceCollection.h"
#include "TimerService_W80x.h"
#include "CommunicationHandler_Prefix.h"
#include "CommunicationService_W80xUART.h"
#include "AnalogService_W80x.h"
#include "DigitalService_W80x.h"
#include "PwmService_W80x.h"
#include "DigitalService_W80x.h"
#include "CommunicationHandlers/CommunicationHandler_EFIGenie.h"
#include "Variable.h"
#include "Config.h"
#include "wm_internal_flash.h"
#include "CRC.h"

using namespace EFIGenie;
using namespace EmbeddedIOServices;
using namespace EmbeddedIOOperations;
using namespace OperationArchitecture;

extern char _config;

extern "C"
{
    EmbeddedIOServiceCollection _embeddedIOServiceCollection;
    EngineMain *_engineMain = 0;
    CommunicationService_W80xUART *_uartService;
    CommunicationHandler_EFIGenie *_efiGenieHandler;
    GeneratorMap<Variable> *_variableMap;
    tick_t prev;
    Variable *loopTime;
    

    bool write(void *destination, const void *data, size_t length) {
        if(reinterpret_cast<size_t>(destination) >= 0x20000100 && reinterpret_cast<size_t>(destination) <= 0x20048000)
        {
            std::memcpy(destination, data, length);
        }
        else if(reinterpret_cast<size_t>(destination) >= 0x08002400 && reinterpret_cast<size_t>(destination) <= 0x08100000)
        {
            const size_t constDataAddress = reinterpret_cast<size_t>(data); //have to do this because the sdk does not have parameter as const even though they do not modify the data
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
            size_t configSize = 0;
            _engineMain = new EngineMain(&_config, configSize, &_embeddedIOServiceCollection, _variableMap);

            _engineMain->Setup();
        }
        return true;
    }

    void Setup() 
    {
        _variableMap = new GeneratorMap<Variable>();
        _uartService = CommunicationService_W80xUART::Create(0, 1024, 1024, 2000000, UART_WORDLENGTH_8B, UART_STOPBITS_1, UART_PARITY_NONE);

        _embeddedIOServiceCollection.DigitalService = new DigitalService_W80x();
        _embeddedIOServiceCollection.AnalogService = new AnalogService_W80x();
        _embeddedIOServiceCollection.TimerService = new TimerService_W80x(1,0);
        _embeddedIOServiceCollection.PwmService = new PwmService_W80x();

        size_t configSize = 0;
        _engineMain = new EngineMain(&_config, configSize, &_embeddedIOServiceCollection, _variableMap);

        _efiGenieHandler = new CommunicationHandler_EFIGenie(_variableMap, write, quit, start, reinterpret_cast<const void*>(&_config));
        _uartService->RegisterReceiveCallBack([&](communication_send_callback_t send, const void *data, size_t length){ return _efiGenieHandler->Receive(send, data, length); });

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
}