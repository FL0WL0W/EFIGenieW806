#include "EFIGenieMain.h"
#include "EmbeddedIOServiceCollection.h"
#include "TimerService_W80x.h"
#include "CommunicationHandler_Prefix.h"
#include "CommunicationService_W80xUART.h"
#include "AnalogService_W80x.h"
#include "DigitalService_W80x.h"
#include "PwmService_W80x.h"
#include "DigitalService_W80x.h"
#include "CommunicationHandlers/CommunicationHandler_GetVariable.h"
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
    EFIGenieMain *_engineMain = 0;
    CommunicationService_W80xUART *_uartService;
    CommunicationHandler_Prefix *_prefixHandler;
    CommunicationHandler_GetVariable *_getVariableHandler;
    GeneratorMap<Variable> *_variableMap;
    tick_t prev;
    Variable *loopTime;
    const char doneResponseText[10] = " (Done)\n\r";
    const void *_metadata;
    #define DUTY_MAX 70
    #define DUTY_MIN 0
    int i, j, m[3] = {0}, d[3] = {DUTY_MIN, (DUTY_MIN + DUTY_MAX) / 2, DUTY_MAX - 1};
    void Setup() 
    {
        _variableMap = new GeneratorMap<Variable>();
        _uartService = CommunicationService_W80xUART::Create(0, 1024, 1024, 115200, UART_WORDLENGTH_8B, UART_STOPBITS_1, UART_PARITY_NONE);
        _uartService->RegisterReceiveCallBack([](communication_send_callback_t send, void *data, size_t length){ return _prefixHandler->Receive(send, data, length);});

        const char responseText1[32] = "Initializing EmbeddedIOServices";
        _uartService->Send(responseText1, strlen(responseText1));
        _embeddedIOServiceCollection.DigitalService = new DigitalService_W80x();
        // _embeddedIOServiceCollection.DigitalService->InitPin(0, In);
        // _embeddedIOServiceCollection.DigitalService->InitPin(34, Out);
        // _embeddedIOServiceCollection.DigitalService->WritePin(34, _embeddedIOServiceCollection.DigitalService->ReadPin(0));
        // _embeddedIOServiceCollection.DigitalService->AttachInterrupt(0, [](){
        //     _embeddedIOServiceCollection.DigitalService->WritePin(34, _embeddedIOServiceCollection.DigitalService->ReadPin(0));
        // });
        _embeddedIOServiceCollection.AnalogService = new AnalogService_W80x();
        _embeddedIOServiceCollection.TimerService = new TimerService_W80x(1,0);
        _embeddedIOServiceCollection.PwmService = new PwmService_W80x();
        _uartService->Send(doneResponseText, strlen(doneResponseText));

		size_t configSize = 0;
        const char responseText3[24] = "Initializing EngineMain";
        _uartService->Send((uint8_t*)responseText3, strlen(responseText3));
        _engineMain = new EFIGenieMain(&_config, configSize, &_embeddedIOServiceCollection, _variableMap);
        _metadata = Config::OffsetConfig(&_config, configSize);
        _uartService->Send((uint8_t*)doneResponseText, strlen(doneResponseText));
        
        _getVariableHandler = new CommunicationHandler_GetVariable(_variableMap);
        _prefixHandler->RegisterReceiveCallBack([](communication_send_callback_t send, void *data, size_t length){ return _getVariableHandler->Receive(send, data, length);}, "g", 1);
        _prefixHandler->RegisterReceiveCallBack([](communication_send_callback_t send, void *data, size_t length)
        { 
            const size_t minDataSize = sizeof(void *) + sizeof(size_t);
            if(length < minDataSize)
                return static_cast<size_t>(0);

            void *readData = *reinterpret_cast<void **>(data);
            size_t readDataLength = *reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(data) + sizeof(void *));
            send(readData, readDataLength);

            return minDataSize;
        }, "r", 1);
        _prefixHandler->RegisterReceiveCallBack([](communication_send_callback_t send, void *data, size_t length)
        { 
            const size_t minDataSize = sizeof(void *) + sizeof(size_t);
            if(length < minDataSize)
                return static_cast<size_t>(0);

            void *writeDataDest = *reinterpret_cast<void **>(data);
            size_t writeDataLength = *reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(data) + sizeof(void *));
            void *writeData = reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(data) + sizeof(void *) + sizeof(size_t));

            if(length < minDataSize + writeDataLength)
                return static_cast<size_t>(0);

            if(reinterpret_cast<size_t>(writeDataDest) >= 0x20000100 && reinterpret_cast<size_t>(writeDataDest) <= 0x20048000)
            {
                std::memcpy(writeDataDest, writeData, writeDataLength);
            }
            else if(reinterpret_cast<size_t>(writeDataDest) >= 0x08002400 && reinterpret_cast<size_t>(writeDataDest) <= 0x08100000)
            {
                HAL_FLASH_Write(reinterpret_cast<uint32_t>(writeDataDest), reinterpret_cast<uint8_t *>(writeData), writeDataLength);
            }

            char ack[1] = {6};
            send(ack, sizeof(ack));

            return minDataSize + writeDataLength;
        }, "w", 1);
        _prefixHandler->RegisterReceiveCallBack([](communication_send_callback_t send, void *data, size_t length)
        { 
            if(_engineMain != 0)
            {
                const char responseText[19] = "Quiting EngineMain";
                send((uint8_t*)responseText, strlen(responseText));
                delete _engineMain;
                _engineMain = 0;
                send((uint8_t*)doneResponseText, strlen(doneResponseText));
            }
            else
            {
                const char responseText[25] = "EngineMain not running\n\r";
                send((uint8_t*)responseText, strlen(responseText));
            }
            return static_cast<size_t>(0);
        }, "q", 1, false);
        _prefixHandler->RegisterReceiveCallBack([](communication_send_callback_t send, void *data, size_t length)
        { 
            if(_engineMain == 0)
            {
                size_t configSize = 0;
                const char responseText1[24] = "Initializing EngineMain";
                send((uint8_t*)responseText1, strlen(responseText1));
                _engineMain = new EFIGenieMain(&_config, configSize, &_embeddedIOServiceCollection, _variableMap);
                _metadata = Config::OffsetConfig(&_config, configSize);
                send((uint8_t*)doneResponseText, strlen(doneResponseText));

                const char responseText2[22] = "Setting Up EngineMain";
                send((uint8_t*)responseText2, strlen(responseText2));
                _engineMain->Setup();
                send((uint8_t*)doneResponseText, strlen(doneResponseText));
            }
            else
            {
                const char responseText[29] = "EngineMain already started\n\r";
                send((uint8_t*)responseText, strlen(responseText));
            }
            return static_cast<size_t>(0);
        }, "s", 1, false);
        _prefixHandler->RegisterReceiveCallBack([](communication_send_callback_t send, void *data, size_t length)
        { 
            if(length < sizeof(uint32_t))//make sure there are enough bytes to process a request
                return static_cast<size_t>(0);
            uint8_t offset = *reinterpret_cast<uint32_t *>(data); //grab offset from data

            send(reinterpret_cast<const uint8_t *>(_metadata) + offset * 64, 64);

            return static_cast<size_t>(sizeof(uint32_t));//return number of bytes handled
        }, "m", 1);
        _prefixHandler->RegisterReceiveCallBack([](communication_send_callback_t send, void *data, size_t length)
        { 
            size_t configLocation[1] = { reinterpret_cast<size_t>(&_config) };
            send(configLocation, sizeof(configLocation));
            
            return static_cast<size_t>(0);
        }, "c", 1, false);

        const char responseText5[22] = "Setting Up EngineMain";
        _uartService->Send((uint8_t*)responseText5, strlen(responseText5));
        _engineMain->Setup();
        _uartService->Send((uint8_t*)doneResponseText, strlen(doneResponseText));
        loopTime = _variableMap->GenerateValue(250);
    }
    void Loop() 
    {            
        const tick_t now = _embeddedIOServiceCollection.TimerService->GetTick();
        loopTime->Set((float)(now-prev) / _embeddedIOServiceCollection.TimerService->GetTicksPerSecond());
        prev = now;
        _uartService->FlushReceive();

        if(_engineMain != 0)
            _engineMain->Loop();
    }
}