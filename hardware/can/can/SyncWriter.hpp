/***************************************************************************
 tag: Peter Soetens  Mon Jan 19 14:11:20 CET 2004  SyncWriter.hpp

 SyncWriter.hpp -  description
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

#ifndef ORO_SYNCWRITER_HPP
#define ORO_SYNCWRITER_HPP

#include "CAN.hpp"
#include <rtt/RunnableInterface.hpp>
#include "CANBus.hpp"

namespace RTT
{
    namespace CAN
    {
        /**
         * A class which writes a sync message to the bus when
         * it is run.
         */
        class SyncWriter: public RunnableInterface
        {
            CANMessage syncMsg;
            CANBus* bus;
        public:
            SyncWriter(CANBus* _bus) :
                bus(_bus)
            {
                syncMsg.setStdId(0x80);
            }
            bool initialize()
            {
                return true;
            }
            void step()
            {
                bus->sync();
            }
            void finalize()
            {
            }
            void sync()
            {
                bus->write(&syncMsg);
            }

        };
    }
}

#endif
