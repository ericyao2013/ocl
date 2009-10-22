#include <rtt/PeriodicActivity.hpp>
#include <rtt/Ports.hpp>
#include <rtt/Toolkit.hpp>

#include "hardware/wrench/WrenchSensor.hpp"
#include <rtt/os/main.h>
#include <kdl/bindings/rtt/toolkit.hpp>

//User interface
#include "taskbrowser/TaskBrowser.hpp"

//Reporting
#include "reporting/FileReporting.hpp"



using namespace std;
using namespace RTT;
using namespace KDL;
using namespace Orocos;

/**
 * main() function
 */
int ORO_main(int arc, char* argv[])
{
    // Set log level more verbose than default,
    // such that we can see output :
    if ( Logger::log().getLogLevel() < Logger::Info ) {
        Logger::log().setLogLevel( Logger::Info );
        log(Info) << argv[0] << " manually raises LogLevel to 'Info' (5). See also file 'orocos.log'."<<endlog();
    }
    // import kdl toolkit
    Toolkit::Import( KDLToolkit );

    WrenchSensor a_task("ATask");

    PeriodicActivity periodicActivityA(0,0.1, a_task.engine() );
    FileReporting reporter("Reporting");
    reporter.connectPeers(&a_task);

    TaskBrowser browser( &a_task );

    browser.loop();

    periodicActivityA.stop();

    return 0;
}
