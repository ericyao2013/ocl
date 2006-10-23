/***************************************************************************
  tag: Peter Soetens  Thu Apr 22 20:40:59 CEST 2004  AxisSensor.hpp 

                        AxisSensor.hpp -  description
                           -------------------
    begin                : Thu April 22 2004
    copyright            : (C) 2004 Peter Soetens
    email                : peter.soetens@mech.kuleuven.ac.be
 
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
 
#ifndef AXES_COMPONENT_HPP
#define AXES_COMPONENT_HPP

#include <rtt/dev/AxisInterface.hpp>
#include <rtt/dev/DriveInterface.hpp>
#include <rtt/dev/DigitalInput.hpp>
#include <rtt/dev/DigitalOutput.hpp>

#include <map>
#include <utility>
#include <string>
#include <boost/tuple/tuple.hpp>
#include <rtt/Logger.hpp>
#include <rtt/TaskContext.hpp>
#include <rtt/DataPort.hpp>
#include <rtt/Property.hpp>
#include <rtt/RTT.hpp>

namespace Orocos
{
    /**
     * @brief A Component which allows you to control Axis objects.
     * There is no 'motion control' involved, merely making the
     * Axis objects available throught a Component interface. See
     * the nAxes* components for motion control.
     *
     * Each added Axis introduces ports and Digital Inputs/Outputs
     * which you can use to read the status.
     */
    class AxesComponent
        : public TaskContext
    {
    public:
        typedef std::vector<double> ChannelType;

        /**
         * @brief Create a Component with maximum \a max_axes virtual channels in the "InputValues" 
         * DataPort and \a max_axes virtual channels in the "OutputValues" DataPort.
         *
         */
        AxesComponent( int max_axes = 1, const std::string& name = "AxesComponent" );

        virtual void update();

        /**
         * @brief Add an AxisInterface object with a name.
         * You need to add an axis first before it can be put on a virtual channel. 
         */
        bool addAxis( const std::string& name, AxisInterface* axis_i );

        /** 
         * @brief Add an Axis object on a Channel.
         * The drive value for the axis is fetched from the 'OutputValues' data port,
         * the sensed valued for the axis is written to the 'InputValues' data port.
         * 
         * @param axis_name        The name of the previously added Axis
         * @param sensor_name      The name of a Sensor of the Axis
         * @param virtual_channel  The channel number where the Sensor must be added.
         * 
         * @return true if successfull, false otherwise.
         */
        bool addAxisOnChannel(const std::string& axis_name, const std::string& sensor_name, int virtual_channel );

        /** 
         * @brief Remove an axis from a virtual channel.
         * 
         * @param virtual_channel The channel to remove.
         * 
         */
        void removeAxisFromChannel(int virtual_channel);

        /**
         * @brief Remove a previously added Axis.
         */
        bool removeAxis( const std::string& name );

        /**
         * @name AxesComponent Methods.
         * Runtime inspection for the AxesComponent.
         * @{
         */

        /**
         * @brief Enable an Axis.
         */
        bool enableAxis( const std::string& name );

        /**
         * @brief Disable an Axis.
         */
        bool disableAxis( const std::string& name );

        /**
         * @brief Stop an Axis.
         */
        bool stopAxis( const std::string& name );

        /**
         * @brief Switch on a Digital Output.
         */
        bool switchOn( const std::string& name );
                    
        /**
         * @brief Switch off a Digital Output.
         */
        bool switchOff( const std::string& name );
           
        /**
         * @brief Inspect if an axis is enabled (equivalent to !isLocked()).
         */
        bool isEnabled( const std::string& name ) const;

        /**
         * @brief Inspect if an axis is 'driven' (in movement).
         */
        bool isDriven( const std::string& name ) const;

        /**
         * @brief Inspect if an axis is 'stopped' (electronical stand still).
         */
        bool isStopped( const std::string& name ) const;

        /**
         * @brief Inspect if an axis is 'locked' (mechanical stand still).
         */
        bool isLocked( const std::string& name ) const;

        /**
         * @brief Inspect the position of an Axis.
         */
        double position( const std::string& name ) const;

        /**
         * @brief Inspect the status of a Digital Input or
         * Digital Output.
         */
        bool isOn( const std::string& name ) const;

        /**
         * @brief Inspect a Sensor value of the Axis.
         */
        double readSensor( const std::string& name ) const;

        /**
         * Get the number of axes.
         */
        int getAxes() const;
        /**
         * @}
         */

        /**
         * @name AxesComponent Commands.
         * Runtime commands for the AxesComponent.
         * @{
         */
        
        /**
         * Calibrate a Sensor of the Axis.
         */
        bool calibrateSensor( const std::string& name );
        
        /**
         * Reset (uncalibrate) a Sensor of the Axis.
         */
        bool resetSensor( const std::string& name );
        
        /**
         * Checks if a Sensor is calibrated ( Completion Condition ).
         */
        bool isCalibrated( const std::string& name ) const;
        /** @} */
    protected:

        /////// SENSOR //////

        /**
         * Write Analog input to DataObject.
         */
        void drive_to_do( std::pair<std::string,std::pair<ORO_DeviceInterface::AxisInterface*,
                          ReadDataPort<double>* > > dd );
            
        void sensor_to_do( std::pair<std::string,std::pair< const SensorInterface<double>*,
                           WriteDataPort<double>* > > dd );
            
        Property<int> max_channels;

        std::vector< std::pair< const SensorInterface<double>*, ORO_DeviceInterface::AxisInterface* > > channels;
        ChannelType chan_meas;
        WriteDataPort<ChannelType> chan_sensor;

        std::map<std::string, const DigitalInput* > d_in;
        std::map<std::string, DigitalOutput* > d_out;

        typedef
        std::map<std::string, std::pair<ORO_DeviceInterface::AxisInterface*, DataObjectInterface<double>* > > DriveMap;
        DriveMap drive;
        typedef
        std::map<std::string, std::pair< SensorInterface<double>*, DataObjectInterface<double>* > > SensorMap;
        SensorMap sensor;
        typedef
        std::map<std::string, ORO_DeviceInterface::AxisInterface* > AxisMap;
        AxisMap axes;

        std::string axis_to_remove;
        int usingChannels;

        /////// EFFECTOR //////

        /**
         * Write to Data to AnalogDrives.
         */
        void write_to_drive( pair<std::string, pair<ORO_DeviceInterface::AxisInterface*, DataObjectInterface<double>* > > dd );

        /**
         * Check if the Output DataObject lacks a user requested AnalogDrive.
         */
        bool lacksDrive( pair<std::string,pair<ORO_DeviceInterface::AxisInterface*, DataObjectInterface<double>* > > dd );
            
        std::vector<double> chan_out;
        ReadDataPort< std::vector<double> > chan_drive;

        std::map<std::string, DigitalOutput* > d_out;

        std::map<std::string, pair<ORO_DeviceInterface::AxisInterface*, DataObjectInterface<double>* > > drive;
        std::map<std::string, pair<ORO_DeviceInterface::AxisInterface*, int> > axes;
        
    };
}

#endif
