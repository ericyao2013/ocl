#include <string>

#include <ocl/HMIConsoleOutput.hpp>
#include <timer/TimerComponent.hpp>
#include <taskbrowser/TaskBrowser.hpp>

#include <rtt/NonPeriodicActivity.hpp>
#include <rtt/PeriodicActivity.hpp>
#include <rtt/Method.hpp>
#include <iostream>
#include <rtt/os/main.h>

using namespace std;
using namespace Orocos;
using namespace RTT;

// test TimerComponent when used by state machine (ie via Orocos interface)
class TestStateMachine
    : public TaskContext
{
    Handle h;
	// log a message
	RTT::Method<void(std::string)>					log_mtd;

public:
    TestStateMachine(std::string name) : 
            TaskContext(name, PreOperational),
            log_mtd("log", &TestStateMachine::doLog, this)
    {
        methods()->addMethod(&log_mtd, "Log a message",
                             "message", "Message to log");
    }

    bool startHook()
    {
        bool 				rc = false;		// prove otherwise
        StateMachinePtr 	p;

        Logger::In			in(getName());
        std::string         machineName = this->getName();
        p = engine()->states()->getStateMachine(machineName);
        if (p)
        {
            if (p->activate())
            {	
                if (p->start())
                {
                    rc = true;
                }
                else
                {
                    log(Error) << "Unable to start state machine: " << machineName << endlog();
                }
            }
            else
            {
                log(Error) << "Unable to activate state machine: " << machineName << endlog();
            }
        }
        else
        {
            log(Error) << "Unable to find state machine: " << machineName << endlog();
        }
        return rc;
    }

    void doLog(std::string message)
    {
        Logger::In			in(getName());
        log(Info) << message << endlog();
    }
};

int ORO_main( int argc, char** argv)
{
    // Set log level more verbose than default,
    // such that we can see output :
    if ( Logger::log().getLogLevel() < Logger::Info ) {
        Logger::log().setLogLevel( Logger::Info );
        log(Info) << argv[0]
		      << " manually raises LogLevel to 'Info' (5). See also file 'orocos.log'."<<endlog();
    }

    HMIConsoleOutput hmi("hmi");
    hmi.setActivity( new NonPeriodicActivity(ORO_SCHED_RT, OS::HighestPriority) );

    TimerComponent tcomp("Timer");
    NonPeriodicActivity act(ORO_SCHED_RT, OS::HighestPriority, tcomp.engine() );

    TestStateMachine peer("testWithStateMachine");  // match filename
    PeriodicActivity p_act(ORO_SCHED_RT, OS::HighestPriority, 0.1, peer.engine() );

    peer.addPeer(&tcomp);
    peer.addPeer(&hmi);

    std::string name = "testWithStateMachine.osd";
	if (!peer.scripting()->loadStateMachines(name))
    {
        log(Error) << "Unable to load state machine: '" << name << "'" << endlog();
        return -1;
    }

    TaskBrowser tb( &peer );

    peer.configure();
    peer.start();
    tcomp.configure();
    tcomp.start();
    hmi.start();

    tb.loop();

    tcomp.stop();
    peer.stop();

    return 0;
}

