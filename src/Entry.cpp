#include "EFIGenieMain.h"
#include "EmbeddedIOServiceCollection.h"
#include "TimerService_W806.h"
#include "AnalogService_W806.h"
#include "DigitalService_W806.h"
#include "PwmService_W806.h"
#include "DigitalService_W806.h"

using namespace EFIGenie;
using namespace EmbeddedIOServices;
using namespace EmbeddedIOOperations;

// extern char _config;
extern "C"
{
    EmbeddedIOServiceCollection _embeddedIOServiceCollection;
    EFIGenieMain *_engineMain;
    Task *ledTask;
    Task *led2Task;
    Task *ledPWMTask;
    tick_t ledInterval;
    tick_t led2Interval;
    tick_t ledPWMInterval;
    #define DUTY_MAX 70
    #define DUTY_MIN 0
    int i, j, m[3] = {0}, d[3] = {DUTY_MIN, (DUTY_MIN + DUTY_MAX) / 2, DUTY_MAX - 1};
    void Setup() 
    {
        _embeddedIOServiceCollection.DigitalService = new DigitalService_W806();
        _embeddedIOServiceCollection.DigitalService->InitPin(34, Out);
        _embeddedIOServiceCollection.DigitalService->AttachInterrupt(0, [](){
            _embeddedIOServiceCollection.DigitalService->WritePin(34, _embeddedIOServiceCollection.DigitalService->ReadPin(0));
        });
        _embeddedIOServiceCollection.DigitalService->WritePin(34, _embeddedIOServiceCollection.DigitalService->ReadPin(0));
        // _embeddedIOServiceCollection.DigitalService->InitPin(32, Out);
        _embeddedIOServiceCollection.DigitalService->InitPin(33, Out);
        _embeddedIOServiceCollection.AnalogService = new AnalogService_W806();
        _embeddedIOServiceCollection.TimerService = new TimerService_W806(0,1);
        ledInterval = _embeddedIOServiceCollection.TimerService->GetTicksPerSecond();
        led2Interval = ledInterval - 100;
        ledPWMInterval = 20001;
        ledTask = new Task([](){
            _embeddedIOServiceCollection.DigitalService->WritePin(33, !_embeddedIOServiceCollection.DigitalService->ReadPin(33));
        });
        led2Task = new Task([](){
            // _embeddedIOServiceCollection.DigitalService->WritePin(32, !_embeddedIOServiceCollection.DigitalService->ReadPin(32));
        });
        ledPWMTask = new Task([](){
            for (uint8_t i = 0; i < 3; i++)
            {
                if (m[i] == 0) // Increasing
                {
                    d[i]++;
                    if (d[i] == DUTY_MAX)
                    {
                        m[i] = 1;
                    }
                }
                else // Decreasing
                {
                    d[i]--;
                    if (d[i] == DUTY_MIN)
                    {
                        m[i] = 0;
                    }
                }
                PwmValue value = { 1/100000.0f, d[i] / (100 * 100000.0f) };

                _embeddedIOServiceCollection.PwmService->WritePin(i+32, value);
            }
        });
        _embeddedIOServiceCollection.TimerService->ScheduleTask(ledTask, _embeddedIOServiceCollection.TimerService->GetTick()-1000);
        while(ledTask->Scheduled);
        _embeddedIOServiceCollection.TimerService->ScheduleTask(led2Task, ledTask->ExecutedTick-500000);
        _embeddedIOServiceCollection.TimerService->ScheduleTask(ledPWMTask, ledTask->ExecutedTick+ledPWMInterval);
        _embeddedIOServiceCollection.PwmService = new PwmService_W806();
        _embeddedIOServiceCollection.PwmService->InitPin(32, Out, 1000);
        // _embeddedIOServiceCollection.PwmService->InitPin(33, Out, 1000);
        // _embeddedIOServiceCollection.PwmService->InitPin(34, Out, 1000);
        // char nullconf[50] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		// size_t _configSize = 0;
        // _engineMain = new EFIGenieMain(reinterpret_cast<void*>(nullconf), _configSize, &_embeddedIOServiceCollection);
    }
    void Loop() 
    {
        if(!ledTask->Scheduled)
            _embeddedIOServiceCollection.TimerService->ScheduleTask(ledTask, ledTask->ScheduledTick + ledInterval);
        if(!led2Task->Scheduled)
            _embeddedIOServiceCollection.TimerService->ScheduleTask(led2Task, led2Task->ScheduledTick + led2Interval);
        if(!ledPWMTask->Scheduled)
            _embeddedIOServiceCollection.TimerService->ScheduleTask(ledPWMTask, ledPWMTask->ScheduledTick + ledPWMInterval);
        // _engineMain->Loop();
    }
}