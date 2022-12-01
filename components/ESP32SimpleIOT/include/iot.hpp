#pragma once

#include "config.hpp"

#define __IOT__
#include "global.hpp"

#undef __IOT__

class IoT
{
  public:
    /// States of the finite state machine
    ///
    /// On battery power (*DEEP_SLEEP* feature is set), only the following
    /// states will have networking capability:
    ///
    ///    *STARTUP*, *PROCESS_EVENT*, *END_EVENT*, *HOURS_24*
    ///
    /// @startuml{state_uml.png} "IOT State Diagram" width=5cm
    /// hide empty description
    /// [*] --> STARTUP
    /// STARTUP --> WAIT_FOR_EVENT : COMPLETED
    /// STARTUP --> STARTUP : NOT-COMPLETED
    /// WAIT_FOR_EVENT -right-> PROCESS_EVENT : NEW_EVENT
    /// WAIT_FOR_EVENT --> HOURS_24
    /// HOURS_24 --> WAIT_FOR_EVENT
    /// PROCESS_EVENT --> WAIT_END_EVENT : COMPLETED
    /// PROCESS_EVENT --> PROCESS_EVENT : NOT_COMPLETED
    /// PROCESS_EVENT -left-> WAIT_FOR_EVENT : ABORTED
    /// PROCESS_EVENT --> HOURS_24
    /// HOURS_24 --> PROCESS_EVENT
    /// WAIT_END_EVENT --> END_EVENT : COMPLETED
    /// WAIT_END_EVENT --> PROCESS_EVENT : RETRY
    /// WAIT_END_EVENT --> HOURS_24
    /// WAIT_END_EVENT --> WAIT_END_EVENT : NOT_COMPLETED
    /// HOURS_24 --> WAIT_END_EVENT
    /// END_EVENT --> WAIT_FOR_EVENT : COMPLETED
    /// END_EVENT --> END_EVENT : NOT_COMPLETED
    /// @enduml
    enum State : int8_t {
      STARTUP        =  1, ///< The device has just been reset
      WAIT_FOR_EVENT =  2, ///< Wait for an event to occur
      PROCESS_EVENT  =  4, ///< An event is being processed
      WAIT_END_EVENT =  8, ///< The device is waiting for the end of the event to occur
      END_EVENT      = 16, ///< The end of an event has been detected
      HOURS_24       = 32  ///< This event occurs every 24 hours
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
      RETRY          ///< From WAIT_END_EVENT, go back to PROCESS_EVENT
    };

    /// Application defined process handling function. To be supplied as a parameter
    /// to the IOT::init() function.
    /// @param[in] _state The current state of the Finite State Machine.
    /// @return The status of the user process execution.  private:

    typedef UserResult ProcessHandler(State _state);

  private:
    static constexpr char const * TAG = "IoT Class";

    ProcessHandler * process_handler;

  public:
    esp_err_t                   init(ProcessHandler * handler);
    void                     process();
    esp_err_t prepare_for_deep_sleep();
};