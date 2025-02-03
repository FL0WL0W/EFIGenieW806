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
#include "Entry.h"

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
}