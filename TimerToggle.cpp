#include "tinyfsm.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

struct Off; // forward declaration

// ----------------------------------------------------------------------------
// 1. Event Declarations
//
struct Toggle : tinyfsm::Event { };
struct TimerExpired : tinyfsm::Event { };

// ----------------------------------------------------------------------------
// 2. State Machine Base Class Declaration
//
struct Switch : tinyfsm::Fsm<Switch>
{
    virtual void react(Toggle const &) { };
    virtual void react(TimerExpired const &) { };

    virtual void entry(void) { };  /* entry actions in some states */
    void         exit(void)  { };  /* no exit actions */

    // alternative: enforce entry actions in all states (pure virtual)
    //virtual void entry(void) = 0;
};


// ----------------------------------------------------------------------------
// 3. State Declarations
//
struct On : Switch
{
    void entry() override
    {
        std::cout << "* Switch is ON" << std::endl;
        timer_start = std::chrono::steady_clock::now();
        is_timer_started = true;
    };
    void react(Toggle const &) override
    {
        transit<Off>();
        is_timer_started = false;
    };
    void react(TimerExpired const &) override
    {
        std::cout << "* Switch is turned off due to timeout" << std::endl;
        transit<Off>();
        is_timer_started = false;
    }

    std::chrono::time_point<std::chrono::steady_clock> timer_start;
    bool is_timer_started = false;
};

struct Off : Switch
{
    void entry() override
    {
        std::cout << "* Switch is OFF" << std::endl;
    };
    void react(Toggle const &) override
    {
        transit<On>();
    };
};

FSM_INITIAL_STATE(Switch, Off)

// ----------------------------------------------------------------------------
// 4. State Machine List Declaration (dispatches events to multiple FSM's)
//
// In this example, we only have a single state machine, no need to use FsmList<>:
//using Switch = tinyfsm::FsmList< Switch >;

void timer(std::atomic<bool> &shouldExit)
{
    TimerExpired timerExpired;
    // Check if timer has expired

    while(!shouldExit){
        if (Switch::is_in_state<On>() && Switch::state<On>().is_timer_started)
        {
            std::chrono::duration elapsed_time = std::chrono::steady_clock::now() - Switch::state<On>().timer_start;
            std::cout << "> Timer: " << elapsed_time.count()/1000000  << "ms" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (elapsed_time > std::chrono::milliseconds(3000))
            {
                std::cout << "> Timer expired!" << std::endl;
                Switch::dispatch(timerExpired);
            }
        }
    }
}

void interact(std::atomic<bool> &shouldExit){
    Toggle toggle;
    while(!shouldExit){
        char c;
        std::cout << std::endl << "t=Toggle, q=Quit ? ";
        std::cin >> c;
        switch(c) {
            case 't':
                std::cout << "> Toggling switch..." << std::endl;
                Switch::dispatch(toggle);
                // alternative: instantiating causes no overhead (empty declaration)
                //Switch::dispatch(Toggle());
                break;
            case 'q':
                shouldExit = true;
                break;
            default:
                std::cout << "> Invalid input" << std::endl;
        };
    }
}

// ----------------------------------------------------------------------------
// Main
//
int main()
{
    std::atomic<bool> shouldExit = false;

    // instantiate events
    std::thread interact_thread(interact, ref(shouldExit));
    std::thread timer_thread(timer, ref(shouldExit));
    Switch::start();

    timer_thread.join();
    interact_thread.join();

    return 0;
}
