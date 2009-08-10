/***************************************************************************
  tag: Peter Soetens  Thu Jul 3 15:34:31 CEST 2008  DeploymentComponent.hpp

                        DeploymentComponent.hpp -  description
                           -------------------
    begin                : Thu July 03 2008
    copyright            : (C) 2008 Peter Soetens
    email                : peter.soetens@fmtc.be

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef OCL_DEPLOYMENTCOMPONENT_HPP
#define OCL_DEPLOYMENTCOMPONENT_HPP

#include <rtt/RTT.hpp>
#include <rtt/TaskContext.hpp>
#include <rtt/Properties.hpp>
#include <rtt/Attribute.hpp>
#include <rtt/Ports.hpp>
#include <ocl/OCL.hpp>
#include <vector>
#include <map>
#include <rtt/marsh/PropertyDemarshaller.hpp>

// Suppress warnings in ocl/ComponentLoader.hpp
#ifndef OCL_STATIC
#define OCL_STATIC
#include <ocl/ComponentLoader.hpp>
#undef OCL_STATIC
#else
#include <ocl/ComponentLoader.hpp>
#endif

namespace OCL
{

    /**
     * A Component for deploying (configuring) other components in an
     * application. It allows to create connections between components,
     * load the properties and scripts for components and setup
     * component activities.
     *
     * The main idea is to load an XML file as described in the
     * Deployment Component Manual. It dictates the libraries to load,
     * the components to create, configure and start. Every aspect of
     * the XML file can be expressed in a program script as well. If
     * you want this component to execute a program script, assign it
     * a periodic activity using either a 'site local' XML script (see below),
     * or by listing the DeploymentComponent in your main
     * XML file.
     *
     * @section sect-site-local Automatically loading a site local XML file
     * It is possible to store site local settings in a separate
     * XML configuration file which will be automatically loaded
     * when the DeploymentComponent is created. The default name of
     * this file is 'this->getName() + "-site.cpf"'. It is only looked
     * for and loaded in the constructor of this class.
     *
     * @section sect-conf-depl Configuring the DeploymentComponent itself.
     * When reading an XML file (for example, when using kickStart() or
     * loadComponents() ) the DeploymentComponent checks if a section
     * is devoted to itself by comparing the listed component name with its own
     * name ( this->getName() ). If it matches, it applies the configuration
     * instructions to itself in the same manner as it would configure
     * other components.
     *
     */
    class OCL_API DeploymentComponent
        : public RTT::TaskContext
    {
    protected:
        /**
         * This bag stores the current configuration.
         * It is the cumulation of all loadConfiguration() calls.
         */
        RTT::PropertyBag root;
        RTT::Property<std::string> compPath;
        RTT::Property<bool> autoUnload;
        RTT::Attribute<bool> validConfig;
        RTT::Constant<int> sched_RT;
        RTT::Constant<int> sched_OTHER;
        RTT::Constant<int> lowest_Priority;
        RTT::Constant<int> highest_Priority;
        RTT::Property<std::string> target;

        /**
         * Each configured component is stored in a struct like this.
         * We need this to keep track of: 1. if we created an activity for it.
         * 2. if we loaded it.
         */
        struct ComponentData {
            ComponentData()
                : instance(0), act(0), loaded(false),
                  autostart(false), autoconf(false),
                  autoconnect(false),  autosave(false),
                  proxy(false), server(false),
                  use_naming(true),type(""),
                  configfile("")
            {}
            /**
             * The component instance. This is always a valid pointer.
             */
            RTT::TaskContext* instance;
            /**
             * The activity created by DeploymentComponent.
             * May be null or an activity that we own.
             */
            ActivityInterface* act;
            /**
             * True if it was loaded and created by DeploymentComponent.
             * If true, instance may be deleted during
             * unloadComponent.
             */
            bool loaded;
            bool autostart, autoconf, autoconnect, autosave;
            bool proxy, server, use_naming;
            std::string type, configfile;
        };

        /**
         * Assembles all ports which share a connection.
         */
        struct ConnectionData {
            typedef std::vector<RTT::PortInterface*> Ports;
            typedef std::vector<RTT::TaskContext*>   Owners;
            Ports ports;
            Owners owners;
        };

        /**
         * This maps connection names to associated ports.
         */
        typedef std::map<std::string, ConnectionData> ConMap;
        ConMap conmap;

        /**
         * This vector holds the dynamically loaded components.
         */
        typedef std::map<std::string, ComponentData> CompList;
        CompList comps;

        /**
         * Keep a list of all loaded libraries such that double
         * loads are avoided during import/loadLibrary.
         */
        class LoadedLib{
	    public:
            LoadedLib(std::string n, void* h)
            {
                name = n;
                handle = h;
            }
            std::string name;
            void* handle;
            std::vector<std::string> components_type;
        };

        static std::vector< LoadedLib > loadedLibs;

        /**
         * Handle of last loaded library.
         */
        void* handle;
        /**
         * Name of last loaded library.
         */
        std::string libname;

        /**
         * This function imports available plugins from
         * the path formed by the expression
         *  @code ComponentPath + "/rtt/"+ Target + "/plugins" @endcode
         * @return always true.
         */
        bool configureHook();

        /**
         * This method removes all references to the component hold in \a cit,
         * on the condition that it is not running.
         */
        bool unloadComponentImpl( CompList::iterator cit );


        /**
         * Hook function for subclasses. Allows a subclass
         * to abort or extend the loading of a component.
         * By default, this function returns true.
         * @return false if the component should be unloaded again,
         * true if loading was succesful.
         */
        virtual bool componentLoaded(TaskContext* c);
    public:
        /**
         * Constructs and configures this component.
         *
         * The constructor looks for the site local configuration XML
         * file ('\a name + "-site.cpf"') and if found, kickStart()'s
         * it. You need to set AutoConf to true in order to force a
         * call to configureHook(). In case this file is not present
         * in the current working directory, the component is configured
         * and is thus constructed in the Stopped state. Using a site file
         * does not prevent you from kickstarting or loading other XML files
         * lateron.
         *
         * @param name The name of this component. By default: \a Deployer
         * @param siteFile The site-specific XML file which, if found, will be used
         * for a site-specific kickStart. If left empty, the value becomes
         * by default: \a name + \a "-site.cpf"
         * @see kickStart
         * @see configureHook
         */
        DeploymentComponent(std::string name = "Deployer", std::string siteFile = "");

        /**
         * Cleans up all configuration related information.
         * If the property 'AutoUnload' is set to true, it will
         * also call kickOutAll(), otherwise, the loaded
         * components are left as-is.
         */
        ~DeploymentComponent();

        TaskContext* myGetPeer(std::string name) {return comps[ name ].instance; }

        /**
         * Establish a bidirectional connection between two tasks.
         *
         * @param one The first task to connect
         * @param other The second task to connect
         *
         * @return true if both tasks are peers of this component and
         * could be connected.
         */
        bool connectPeers(const std::string& one, const std::string& other);

        /**
         * Establish a data flow connection between two tasks. The direction
         * of the connection is determined by the read/write port types.3B
         *
         * @param one The first task to connect
         * @param other The second task to connect
         *
         * @return true if both tasks are peers of this component and
         * data ports could be connected.
         */
        bool connectPorts(const std::string& one, const std::string& other);

        /**
         * Connect two named ports of components. The direction
         * of the connection is determined by the read/write port types.
         *
         * @param one Name of the first component
         * @param one_port Name of the port of the first component to connect to
         * \a other_port
         * @param other Name of the second component
         * @param other_port Name of the port of the second component to connect
         * to \a one_port
         *
         * @return true if the ports are present and could be connected, false otherwise.
         */
        bool connectPorts(const std::string& one, const std::string& one_port,
                          const std::string& other, const std::string& other_port);

        /**
         * Establish a uni directional connection form one task to another
         *
         * @param from The component where the connection starts.
         * @param to The component where the connection ends.
         *
         * @return true if both tasks are peers of this component and
         * a connection could be created.
         */
        bool addPeer(const std::string& from, const std::string& to);

        using TaskContext::addPeer;
        using TaskContext::connectPeers;

        /**
         * Import a library or all libraries in a given directory.
         * This function calls loadLibrary on each found shared library in \a path.
         */
        bool import(const std::string& path);

        /**
         * Use this command to load a dynamic library into the memory of the
         * current process.
         *
         * @param name an absolute or relative path to a loadable library.
         *
         * @return True if it could be loaded, false otherwise.
         */
        bool loadLibrary(const std::string& name);

        /**
         * Load a new component in the current process. It wil appear
         * as a peer with name \a name of this component.
         *
         * @param name Name the new component will receive.
         * @param type The type of the component. This is usually a library (.dll or .so)
         * name.
         *
         * @return True if the component could be created, false if \a name
         * is already in use or \a type was not an Orocos library.
         */
        bool loadComponent(const std::string& name, const std::string& type);

        /**
         * Unload a loaded component from the current process. It may not
         * be running.
         *
         * @param name The name of a component loaded with loadComponent().
         *
         * @return true if \a name was not running and could be unloaded.
         */
        bool unloadComponent(const std::string& name);

        /**
         * This function prints out the component types this DeploymentComponent
         * can create.
         * @see loadComponent()
         */
        void displayComponentTypes() const;

        /**
         * (Re-)set the activity of a component with a periodic activity.
         *
         * @param comp_name The name of the component to change.
         * @param period    The period of the activity.
         * @param priority  The scheduler priority (OS dependent).
         * @param scheduler The scheduler type \a ORO_SCHED_RT or \a ORO_SCHED_OTHER.
         *
         * @return false if one of the parameters does not match or if the
         * component is running.
         */
        bool setPeriodicActivity(const std::string& comp_name,
                                 double period, int priority,
                                 int scheduler);

        /**
         * (Re-)set the activity of a component with a non periodic activity.
         *
         * @param comp_name The name of the component to change.
         * @param priority  The scheduler priority (OS dependent).
         * @param scheduler The scheduler type \a ORO_SCHED_RT or \a ORO_SCHED_OTHER.
         *
         * @return false if one of the parameters does not match or if the
         * component is running.
         */
        bool setNonPeriodicActivity(const std::string& comp_name,
                                    int priority,
                                    int scheduler);

        /**
         * (Re-)set the activity of a component with a (stand alone) slave activity.
         *
         * @param comp_name The name of the component to change.
         * @param period    The period of the activity.
         *
         * @return false if one of the parameters does not match or if the
         * component is running.
         */
        bool setSlaveActivity(const std::string& comp_name,
                              double period);

        /**
         * (Re-)set the activity of a component with a slave activity with master.
         *
         * @param comp_name The name of the component to change.
         * @param master_name The name of the master component.
         *
         * @return false if one of the components is not found or \a comp_name is running.
         */
        bool setMasterSlaveActivity(const std::string& comp_name,
                                    const std::string& master_name);

        /**
         * (Re-)set the activity of a component.
         *
         * @param comp_name The name of the component to change.
         * @param act_type  The Activity type: 'PeriodicActivity', 'NonPeriodicActivity' or 'SlaveActivity'.
         * @param priority  The scheduler priority (OS dependent).
         * @param period    The period of the activity.
         * @param scheduler The scheduler type \a ORO_SCHED_RT or \a ORO_SCHED_OTHER.
         * @param master_name The name of the master component in case of a SlaveActivity with a master.
         *
         * @return false if one of the parameters does not match or if the
         * component is running.
         */
        bool setActivity(const std::string& comp_name,
                         const std::string& act_type,
                         double period, int priority,
                         int scheduler, const std::string& master_name = "");

        /**
         * Load a (partial) application XML configuration from disk. The
         * necessary components are located or loaded, but no
         * component configuration is yet applied. One can load
         * multiple configurations and call configureComponents() once
         * to apply all settings. In case of duplicate information is
         * the latest loaded configuration option used.
         *
         * @see configureComponents to configure the components with
         * the loaded configuration and startComponents to start them.
         * @param config_file A file on local disk containing the XML configuration.
         * @return true if the configuration could be read and was valid.
         */
        bool loadComponents(const std::string& config_file);

        /**
         * Configure the components with loaded configuration(s). This
         * function connects components and data ports, reads
         * properties for the components, attaches activities and
         * loads program and state machine scripts.  If a component
         * XML entry has the AutoConf element, configure() will be
         * called upon this component as well.  If the configuration
         * fails halfway, the system is configured as complete as
         * possible. You can try to reconfigure by loading a new
         * configuration (using loadConfiguration ) and call
         * configureComponents again to resolve remaining issues.
         *
         * This function tries to apply the configuration with a best effort.
         * For example, if a program must be loaded in the component, and a program
         * with that same name is already present, the present one is unloaded and the
         * new one is attempted to be loaded. If that fails, the configuration process
         * leaves the scripts as-is and proceeds with further configuration steps of
         * the same component and other components.
         *
         * The order of configuration depends on the order of components during
         * loadConfiguration. The first encountered component is configured first.
         * If additional loadConfiguration operations refer to the same component,
         * the configuration order is not changed.
         *
         * @return true if all components could be succesfully configured.
         */
        bool configureComponents();

        /**
         * Start all components in the current configuration which have AutoStart
         * set to true.
         * @return true if all components could be succesfully started.
         */
        bool startComponents();

        /**
         * Clear all loaded configuration options.
         * This does not alter any component.
         */
        void clearConfiguration();

        /**
         * Stop all loaded and running components.
         */
        bool stopComponents();

        /**
         * Cleanup all loaded and not running components.
         */
        bool cleanupComponents();

        /**
         * Unload all loaded and not running components.
         */
        bool unloadComponents();

        /**
         * This function runs loadComponents, configureComponents and startComponents
         * in a row, given no failures occur along the way.
         */
        bool kickStart(const std::string& file_name);

        /**
         * Stop, cleanup and unload a single component which were loaded by this component.
         * @param comp_name name of the component.
         * @return true if successfully stopped, cleaned and unloaded
         */
        bool kickOutComponent(const std::string& comp_name);

        /**
         * Identical to \a kickOutAll, but it reads the name of the Components to kickOut from an XML file.
         * @param config_file name of an XML file (probably the same used by loadComponents() or kickStart() ).
         */
        void kickOut(const std::string& config_file);

        /**
         * Stop, cleanup and unload all components loaded by the DeploymentComponent.
         */
        bool kickOutAll();


        using TaskCore::configure;

        /**
         * Configure a component by loading the property file 'name.cpf' for component with
         * name \a name.
         * @param name The name of the component to configure.
         * The file used will be 'name.cpf'.
         * @return true if the component is a peer of this component and the file could be
         * read.
         */
        bool configure(const std::string& name);

        /**
         * Configure a component by loading a property file.
         *
         * @param name The name of the component to configure
         * @param filename The filename where the configuration is in.
         *
         * @return true if the component is a peer of this component and the file could be
         * read.
         */
        bool configureFromFile(const std::string& name, const std::string& filename);

        /**
         * Load a (partial) application XML configuration from disk. The
         * necessary components are located or loaded, but no
         * component configuration is yet applied. One can load
         * multiple configurations and call configureComponents() once
         * to apply all settings. In case of duplicate information is
         * the latest loaded configuration option used.
         *
         * @deprecated by loadComponents.
         * @see configureComponents to configure the components with
         * the loaded configuration.
         * @param config_file A file on local disk containing the XML configuration.
         * @return true if the configuration could be read and was valid.
         */
        bool loadConfiguration(const std::string& config_file);

        /**
         * Identical to \a loadConfiguration, but reads the XML from a string
         * instead of a file.
         * @param config_text A string containing the XML configuration.
         * @return true if the configuration string could be read and was valid.
         */
        bool loadConfigurationString(const std::string& config_text);

        /**
         * Returns the factory singleton which creates all types of components
         * for the DeploymentComponent.
         */
        FactoryMap& getFactories();

        /**
         * Stop a single loaded and running component.
         * @param instance instance pointer of the component.
         * @return true if successfully stopped.
         */
        bool stopComponent(RTT::TaskContext *instance);

        /**
         * Stop a single loaded and running components.
         * @param comp_name name of the component.
         * @return true if successfully stopped
         */
        bool stopComponent(const std::string& comp_name)
        {
            return this->stopComponent(  this->getPeer(comp_name) );
        }

        /**
         * Cleanup a single loaded and not running component.
         * @param instance instance pointer of the component.
         * @return true if successfully cleaned up
         */
        bool cleanupComponent(RTT::TaskContext *instance);

        /**
         * Cleanup a single loaded and not running component.
         * @param comp_name name of the component.
         * @return true if successfully cleaned up
         */
        bool cleanupComponent(const std::string& comp_name)
        {
            return this->cleanupComponent( this->getPeer(comp_name) );
        }

    };


}
#endif
