#pragma once

#include <esp_sleep.h>

#include "config.hpp"

#define __IOT__
#include "global.hpp"

#undef __IOT__

extern uint32_t error_count;

class IoT
{
  public:
    /// States of the finite state machine
    ///
    /// On battery power (*DEEP_SLEEP* feature is set), only the following
    /// states will have networking capability:
    ///
    ///    *STARTUP*, *PROCESS_EVENT*, *END_EVENT*, *WATCHDOG*
    ///
    /// @startuml{state_uml.png} "IOT State Diagram" width=5cm
    /// hide empty description
    /// [*] --> STARTUP
    /// STARTUP --> WAIT_FOR_EVENT : COMPLETED
    /// STARTUP --> STARTUP : NOT-COMPLETED
    /// WAIT_FOR_EVENT -right-> PROCESS_EVENT : NEW_EVENT
    /// WAIT_FOR_EVENT --> WATCHDOG
    /// WATCHDOG --> WAIT_FOR_EVENT
    /// PROCESS_EVENT --> WAIT_END_EVENT : COMPLETED
    /// PROCESS_EVENT --> PROCESS_EVENT : NOT_COMPLETED
    /// PROCESS_EVENT -left-> WAIT_FOR_EVENT : ABORTED
    /// PROCESS_EVENT --> WATCHDOG
    /// WATCHDOG --> PROCESS_EVENT
    /// WAIT_END_EVENT --> END_EVENT : COMPLETED
    /// WAIT_END_EVENT --> PROCESS_EVENT : RETRY
    /// WAIT_END_EVENT --> WATCHDOG
    /// WAIT_END_EVENT --> WAIT_END_EVENT : NOT_COMPLETED
    /// WATCHDOG --> WAIT_END_EVENT
    /// END_EVENT --> WAIT_FOR_EVENT : COMPLETED
    /// END_EVENT --> END_EVENT : NOT_COMPLETED
    /// @enduml
    enum State : int8_t {
      STARTUP        =  1, ///< The device has just been reset
      WAIT_FOR_EVENT =  2, ///< Wait for an event to occur
      PROCESS_EVENT  =  4, ///< An event is being processed
      WAIT_END_EVENT =  8, ///< The device is waiting for the end of the event to occur
      END_EVENT      = 16, ///< The end of an event has been detected
      WATCHDOG       = 32  ///< Transmit a watchdog packet
    };

    /// A UserResult is returned by the user process function to indicate
    /// how to proceed with the finite state machine transformation. Please
    /// look at the State enumeration description where a state diagram show the
    /// relationship between states and values returned by the user process function.

    enum UserResult : uint8_t {
      COMPLETED = 1, ///< The processing for the current state is considered completed
      NOT_COMPLETED, ///< The current state still require some processing in calls to come
      ABORTED,       ///< The event vanished and requires no more processing (in a *PROCESS_EVENT* state)
      NEW_EVENT,     ///< A new event occurred (in a *WAIT_FOR_EVENT* state)
      RETRY,         ///< From WAIT_END_EVENT, go back to PROCESS_EVENT
    };

    enum RestartReason : uint8_t {
      RESET,
      DEEP_SLEEP_AWAKE
    };

    enum DeepSleepAwakeReason : uint8_t {
      TIMEOUT,
      OTHER
    };

    /// Application defined process handling function. To be supplied as a parameter
    /// to the IOT::init() function.
    /// @param[in] _state The current state of the Finite State Machine.
    /// @return The status of the user process execution.  private:

    typedef UserResult ProcessHandler(State state);

  private:
    static constexpr char const * TAG = "IoT Class";

    QueueHandle_t send_queue_handle;
    ProcessHandler * process_handler;

    RestartReason      restart_reason;
    esp_sleep_source_t deep_sleep_wakeup_reason;
    
    /// @brief Deep Sleep Duration bypass
    ///
    /// The **deep_sleep_duration** variable is used to bypass the default 
    /// deep sleep duration that is based on the need to send a watchdog
    /// message every 24 hours.
    ///
    /// The values can be:
    ///
    /// -1 : Deep Sleep is disabled
    ///  0 : Use the default duration for the watchdog transmission
    /// >0 : Deep sleep for the number of seconds identified
    ///
    /// Every time the process method is run, the deep_sleep_duration
    /// is resetted to its default value of 0.
    ///
    int32_t          deep_sleep_duration;

    State check_if_24_hours_time(State the_state);

  public:
    esp_err_t                      init(ProcessHandler * handler);
    void                        process();
    void                       send_msg(const char * msg_type, const char * other_field = nullptr);
    inline void set_deep_sleep_duration(int32_t seconds) { deep_sleep_duration = seconds; }
    inline void   increment_error_count() { error_count += 1; }
    inline bool               was_reset() { return restart_reason == RestartReason::RESET; }
    inline bool  was_deep_sleep_timeout() { return deep_sleep_wakeup_reason == ESP_SLEEP_WAKEUP_TIMER; }
    esp_err_t    prepare_for_deep_sleep();
};