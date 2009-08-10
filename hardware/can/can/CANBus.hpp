/***************************************************************************
 tag: Peter Soetens  Mon Jan 19 14:11:21 CET 2004  CANBus.hpp

 CANBus.hpp -  description
 -------------------
 begin                : Mon January 19 2004
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

#ifndef CANBUS_HPP
#define CANBUS_HPP

#include "CANBusInterface.hpp"
#include "CANControllerInterface.hpp"
#include "CANMessage.hpp"

#include <list>
#include <algorithm>
#include <rtt/os/rtstreams.hpp>

namespace RTT
{
    namespace CAN
    {
        using std::list;
        using std::find;

        /**
         * A CAN  Bus in its simplest (but effective) form, making use of
         * nodeId() and other CAN functionalities to optimise and interpret
         * the data flow.
         */
        class CANBus: public CANBusInterface
        {

        public:
            /**
             * Create a CANBus instance with no devices attached to it.
             * A controller device needs to be added to put on and receive
             * messages from the bus.
             *
             * @param _controller The Controller of the bus.
             */
            CANBus() :
                controller(0)
            {
            }

            void setController(CANControllerInterface* contr)
            {
                controller = contr;
            }

            virtual bool addDevice(CANDeviceInterface* dev)
            {
                if (devices.size() < MAX_DEVICES)
                    devices.push_back(dev);
                else
                    return false;
                return true;
            }

            virtual bool addListener(CANListenerInterface* dev)
            {
                if (listeners.size() < MAX_DEVICES)
                    listeners.push_back(dev);
                else
                    return false;
                return true;
            }

            virtual void removeDevice(CANDeviceInterface* dev)
            {
                list<CANDeviceInterface*>::iterator itl;
                itl = find(devices.begin(), devices.end(), dev);
                if (itl != devices.end())
                    devices.erase(itl);
            }

            virtual void removeListener(CANListenerInterface* dev)
            {
                list<CANListenerInterface*>::iterator itl;
                itl = find(listeners.begin(), listeners.end(), dev);
                if (itl != listeners.end())
                    listeners.erase(itl);
            }

            virtual void write(const CANMessage *msg)
            {
                //rt_std::cout <<"Writing...";
                // All listeners are notified.
                list<CANListenerInterface*>::iterator itl = listeners.begin();
                while (itl != listeners.end())
                {
                    list<CANListenerInterface*>::iterator next = (++itl)--;
                    (*itl)->process(msg); // eventually check the node id (in CANBus)
                    itl = next;
                }

                if (controller == 0)
                    return;
                if (msg->origin != controller)
                {
                    //rt_std::cout <<"to controller !\n";
                    controller->process(msg);
                }
                else
                {
                    //rt_std::cout <<"to devicelist !\n";
                    list<CANDeviceInterface*>::iterator itd = devices.begin();
                    while (itd != devices.end())
                    {
                        list<CANDeviceInterface*>::iterator _next = (++itd)--;
                        (*itd)->process(msg); // eventually check the node id (in CANBus)
                        itd = _next; // XXX What if itd just removed _next (or everything)?
                        // can be 'solved' if only self-removal is allowed.
                    }
                }
            }

            CANControllerInterface* getController()
            {
                return controller;
            }

            static const unsigned int MAX_DEVICES = 127;
        protected:
            list<CANDeviceInterface*> devices;
            list<CANListenerInterface*> listeners;

            CANControllerInterface* controller;
        };

    }
}

#endif
