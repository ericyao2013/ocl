/***************************************************************************
 tag: Erwin Aertbelien May 2006
                           -------------------
    begin                : Mon January 19 2004
    copyright            : (C) 2004 Peter Soetens
    email                : Erwin.Aertbelien@mech.kuleuven.ac.be

 based on the work of Johan Rutgeerts in LiASHardware.cpp

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

#include <hardware/lias/CRSnAxesVelocityController.hpp>

//#include <rtt/NonPreemptibleActivity.hpp>
//#include <rtt/BufferPort.hpp>
#include <rtt/Event.hpp>
#include <rtt/Logger.hpp>
//#include <rtt/Attribute.hpp>
#include <rtt/Command.hpp>
#include <rtt/DataPort.hpp>

#include <dev/IncrementalEncoderSensor.hpp>
#include <rtt/dev/AnalogOutput.hpp>
#include <rtt/dev/DigitalOutput.hpp>
#include <rtt/dev/DigitalInput.hpp>
#include <dev/AnalogDrive.hpp>
#include <dev/Axis.hpp>
#include <rtt/dev/AxisInterface.hpp>


#if defined (OROPKG_OS_LXRT)

#include "IP_Digital_24_DOutInterface.hpp"
#include "IP_Encoder_6_EncInterface.hpp"
#include "IP_FastDAC_AOutInterface.hpp"
#include "IP_OptoInput_DInInterface.hpp"
#include "CombinedDigitalOutInterface.hpp"

#include "LiASConstants.hpp"

#else

#include <dev/SimulationAxis.hpp>

#endif
namespace OCL {

	using namespace RTT;
	using namespace std;

#define NUM_AXES 6

#define DBG \
    log(Info) << endlog(); \
    log(Info) << __PRETTY_FUNCTION__ << endlog();


#define TRACE(x) \
    log(Info) << "TRACE " << #x << endlog(); \
    x;


using namespace Orocos;


CRSnAxesVelocityController::CRSnAxesVelocityController(const std::string& name,const std::string& propertyfilename)
  : TaskContext(name),
    driveValue(NUM_AXES),
    reference(NUM_AXES),
    positionValue(NUM_AXES),
    output(NUM_AXES),
    _propertyfile(propertyfilename),
    driveLimits("driveLimits","velocity limits of the axes, (rad/s)"),
    lowerPositionLimits("LowerPositionLimits","Lower position limits (rad)"),
    upperPositionLimits("UpperPositionLimits","Upper position limits (rad)"),
    initialPosition("initialPosition","Initial position (rad) for simulation or hardware"),
    signAxes("signAxes","Indicates the sign of each of the axes"),
 	offset  ("offset",  "offset to compensate for friction.  Should only partially compensate friction"),
    driveOutOfRange("driveOutOfRange"),
    positionOutOfRange("positionOutOfRange"),
    _num_axes("NUM_AXES",NUM_AXES),
	_homed(NUM_AXES),
    servoGain("servoGain","gain of the servoloop (no units)"),
    _servoGain(NUM_AXES),
    servoIntegrationFactor("servoIntegrationFactor","INVERSE of the integration time for the servoloop (s)"),
    _servoIntegrationFactor(NUM_AXES),
    servoFFScale(        "servoFFScale","scale factor on the feedforward signal of the servo loop "),
    _servoFFScale(NUM_AXES),
    servoDerivTime(      "servoDerivTime","Derivative time for the servo loop "),
    servoIntVel(NUM_AXES),
    servoIntError(NUM_AXES),
    servoInitialized(false),
	previousPos(NUM_AXES),
    _axes(NUM_AXES),
    _axesInterface(NUM_AXES)
{
  /**
   * Adding properties
   */
  properties()->addProperty( &driveLimits );
  properties()->addProperty( &lowerPositionLimits );
  properties()->addProperty( &upperPositionLimits  );
  properties()->addProperty( &initialPosition  );
  properties()->addProperty( &signAxes  );
  properties()->addProperty( &offset  );
  properties()->addProperty( &servoGain  );
  properties()->addProperty( &servoIntegrationFactor  );
  properties()->addProperty( &servoFFScale  );
  properties()->addProperty( &servoDerivTime  );
  attributes()->addConstant( &_num_axes);

  if (!marshalling()->readProperties(propertyfilename)) {
    log(Error) << "Failed to read the property file, continueing with default values." << endlog();
    throw 0;
  }
#if defined (OROPKG_OS_LXRT)
    log(Info) << "LXRT version of CRSnAxesVelocityController has started" << endlog();

    _IP_Digital_24_DOut = new IP_Digital_24_DOutInterface("IP_Digital_24_DOut");
    // \TODO : Set this automatically to the correct value :
    _IP_Encoder_6_task  = new IP_Encoder_6_Task(LiAS_ENCODER_REFRESH_PERIOD);
    _IP_FastDac_AOut    = new IP_FastDAC_AOutInterface("IP_FastDac_AOut");
    _IP_OptoInput_DIn   = new IP_OptoInput_DInInterface("IP_OptoInput_DIn");

    _IP_Encoder_6_task->start();

    //Set all constants
    double driveOffsets[6] = LiAS_OFFSETSinVOLTS;
    double radpsec2volt[6] = LiAS_RADproSEC2VOLT;
    int    encoderOffsets[6] = LiAS_ENCODEROFFSETS;
    double ticks2rad[6] = LiAS_TICKS2RAD;
    double jointspeedlimits[6] = LiAS_JOINTSPEEDLIMITS;


    _enable = new DigitalOutput( _IP_Digital_24_DOut, LiAS_ENABLE_CHANNEL, true);
    _enable->switchOff();

    _combined_enable_DOutInterface = new CombinedDigitalOutInterface( "enableDOutInterface", _enable, 6, OR );


    _brake = new DigitalOutput( _IP_Digital_24_DOut, LiAS_BRAKE_CHANNEL, false);
    _brake->switchOn();

    _combined_brake_DOutInterface = new CombinedDigitalOutInterface( "brakeDOutInterface", _brake, 2, OR);


    for (unsigned int i = 0; i < LiAS_NUM_AXIS; i++)
    {
		_homed[i] = false;
        //Setting up encoders
        _encoderInterface[i] = new IP_Encoder_6_EncInterface( *_IP_Encoder_6_task, i ); // Encoder 0, 1, 2, 3, 4 and 5.
        _encoder[i] = new IncrementalEncoderSensor( _encoderInterface[i], 1.0 / ticks2rad[i], encoderOffsets[i], -180, 180, LiAS_ENC_RES );

        _vref[i]   = new AnalogOutput( _IP_FastDac_AOut, i + 1 );
        _combined_enable[i] = new DigitalOutput( _combined_enable_DOutInterface, i );
        _drive[i] = new AnalogDrive( _vref[i], _combined_enable[i], 1.0 / radpsec2volt[i], driveOffsets[i] / radpsec2volt[i]);

        _reference[i] = new DigitalInput( _IP_OptoInput_DIn, i + 1 ); // Bit 1, 2, 3, 4, 5, and 6.


        _axes[i] = new Axis( _drive[i] );
        log(Error) << "_axes[i]->limitDrive( jointspeedlimits[i] ) has been disabled." << endlog();
        //_axes[i]->limitDrive( jointspeedlimits[i] );
        //_axes[i]->setLimitDriveEvent( maximumDrive );  \\TODO I prefere to handle this myself.
        _axes[i]->setSensor( "Position", _encoder[i] );
        // not used any more :_axes[i]->setSensor( "Velocity", new VelocityReaderLiAS( _axes[i], jointspeedlimits[i] ) );

        // Axis 2 and 3 get a combined brake
        if ((i == 1)||(i == 2))
        {
            _combined_brake[i-1] = new DigitalOutput( _combined_brake_DOutInterface, i-1);
            _axes[i]->setBrake( _combined_brake[i-1] );
        }

        _axesInterface[i] = _axes[i];
    }
  #else  // ifndef   OROPKG_OS_LXRT
    log(Info) << "GNU-Linux simulation version of CRSnAxesVelocityController has started" << endlog();
    /*_IP_Digital_24_DOut            = 0;
    _IP_Encoder_6_task             = 0;
    _IP_FastDac_AOut               = 0;
    _IP_OptoInput_DIn              = 0;
    _combined_enable_DOutInterface = 0;
    _enable                        = 0;
    _combined_brake_DOutInterface  = 0;
    _brake                         = 0;*/

    for (unsigned int i = 0; i <NUM_AXES; i++) {
	  _homed[i] = true;
      _axes[i] = new SimulationAxis(
					0.0,
					lowerPositionLimits.value()[i],
					upperPositionLimits.value()[i]);
      _axes[i]->setMaxDriveValue( driveLimits.value()[i] );
      _axesInterface[i] = _axes[i];
    }
  #endif

  _deactivate_axis3 = false;
  _deactivate_axis2 = false;
  _activate_axis2   = false;
  _activate_axis3   = false;

  /*
   *  Command Interface
   */
  typedef CRSnAxesVelocityController MyType;

  this->commands()->addCommand(
    command( "startAxis", &MyType::startAxis,         &MyType::startAxisCompleted, this),
    "start axis, initializes drive value to zero and starts updating the drive-value \
     with the drive-port (only possible if axis is unlocked","axis","axis to start" );
  this->commands()->addCommand( command( "stopAxis", &MyType::stopAxis,          &MyType::stopAxisCompleted, this),
    "stop axis, sets drive value to zero and disables the update of the drive-port, \
    (only possible if axis is started","axis","axis to stop" );
  this->commands()->addCommand( command( "lockAxis", &MyType::lockAxis,          &MyType::lockAxisCompleted, this),
    "lock axis, enables the brakes (only possible if axis is stopped","axis","axis to lock" );
  this->commands()->addCommand( command( "unlockAxis", &MyType::unlockAxis,        &MyType::unlockAxisCompleted, this),
    "unlock axis, disables the brakes and enables the drive (only possible if \
     axis is locked","axis","axis to unlock" );
  this->commands()->addCommand( command( "startAllAxes", &MyType::startAllAxes,      &MyType::startAllAxesCompleted, this), "start all axes"  );
  this->commands()->addCommand( command( "stopAllAxes", &MyType::stopAllAxes,       &MyType::stopAllAxesCompleted, this), "stops all axes"  );
  this->commands()->addCommand( command( "lockAllAxes", &MyType::lockAllAxes,       &MyType::lockAllAxesCompleted, this), "locks all axes"  );
  this->commands()->addCommand( command( "unlockAllAxes", &MyType::unlockAllAxes,     &MyType::unlockAllAxesCompleted, this), "unlock all axes"  );
  this->commands()->addCommand( command( "prepareForUse", &MyType::prepareForUse,     &MyType::prepareForUseCompleted, this), "prepares the robot for use"  );
  this->commands()->addCommand( command( "prepareForShutdown", &MyType::prepareForShutdown,&MyType::prepareForShutdownCompleted, this), "prepares the robot for shutdown"  );
  this->commands()->addCommand( command( "addDriveOffset"    , &MyType::addDriveOffset,    &MyType::addDriveOffsetCompleted, this),  "adds an offset to the drive value of axis","axis","axis to add offset to","offset","offset value in rad/s" );
  this->commands()->addCommand( command( "initPosition"     , &MyType::initPosition,      &MyType::initPositionCompleted, this),  "changes position value to the initial position","axis","axis to initialize" );
  this->commands()->addCommand( command( "changeServo"     , &MyType::changeServo,         &MyType::changeServoCompleted, this),  "Apply the changed properties of the servo loop" );

  this->methods()->addMethod( method( "isDriven", &MyType::isDriven, this),  "checks wether axis is driven","axis","axis to check"  );

  /**
   * Creating and adding the data-ports
   */
  for (int i=0;i<NUM_AXES;++i) {
      char buf[80];
      sprintf(buf,"driveValue%d",i);
      driveValue[i] = new ReadDataPort<double>(buf);
      ports()->addPort(driveValue[i]);
      sprintf(buf,"reference%d",i);
      reference[i] = new WriteDataPort<bool>(buf);
      ports()->addPort(reference[i]);
      sprintf(buf,"positionValue%d",i);
      positionValue[i]  = new WriteDataPort<double>(buf);
      ports()->addPort(positionValue[i]);
      sprintf(buf,"output%d",i);
      output[i]  = new WriteDataPort<double>(buf);
      ports()->addPort(output[i]);
  }

  /**
   * Adding the events :
   */
   events()->addEvent( &driveOutOfRange, "Each axis that is out of range throws a seperate event.", "A", "Axis", "V", "Value" );
   events()->addEvent( &positionOutOfRange, "Each axis that is out of range throws a seperate event.", "A", "Axis", "P", "Position"  );

   /**
    * Initializing servoloop
    */
    for (int axis=0;axis<NUM_AXES;++axis) {
        // state      :
        servoIntVel[axis]   = initialPosition.value()[axis];
        previousPos[axis]   = initialPosition.value()[axis];
        servoIntError[axis] = 0;  // for now.  Perhaps store it and reuse it in a property file.
        // parameters :
        _servoGain[axis]              = servoGain.value()[axis];
        _servoIntegrationFactor[axis] = servoIntegrationFactor.value()[axis];
        _servoFFScale[axis]           = servoFFScale.value()[axis];
    }
}

CRSnAxesVelocityController::~CRSnAxesVelocityController()
{
   DBG;
  // make sure robot is shut down
  prepareForShutdown();

  for (unsigned int i = 0; i < LiAS_NUM_AXIS; i++)
  {
    // brake, drive, encoders are deleted by each axis
    delete _axes[i];
    #if defined (OROPKG_OS_LXRT)
        delete _reference[i];
        delete _encoderInterface[i];
    #endif
  }
    #if defined (OROPKG_OS_LXRT)
    delete _combined_enable_DOutInterface;
    delete _enable;
    delete _combined_brake_DOutInterface;
    delete _brake;
    if (_IP_Encoder_6_task!=0) _IP_Encoder_6_task->stop();

    delete _IP_Digital_24_DOut;
    delete _IP_Encoder_6_task;
    delete _IP_FastDac_AOut;
    delete _IP_OptoInput_DIn;
    #endif
}

bool
CRSnAxesVelocityController::isDriven(int axis)
{
  DBG;
  if (!(axis<0 || axis>NUM_AXES-1))
    return _axes[axis]->isDriven();
  else{
    log(Error) <<"Axis "<< axis <<"doesn't exist!!"<<endlog();
    return false;
  }
}

bool
CRSnAxesVelocityController::startAxis(int axis)
{
  DBG;
  if (!(axis<0 || axis>NUM_AXES-1))
    return _axes[axis]->drive(0.0);
  else{
    log(Error) <<"Axis "<< axis <<"doesn't exist!!"<<endlog();
    return false;
  }
}

bool
CRSnAxesVelocityController::startAxisCompleted(int axis) const {
    DBG;
    return _axes[axis]->isDriven();
    return true;
}


bool
CRSnAxesVelocityController::startAllAxes()
{
  DBG;
  bool result = true;
  for (int axis=0;axis<NUM_AXES;++axis) {
    result &= _axes[axis]->drive(0.0);
  }
  return result;
}


bool CRSnAxesVelocityController::startAllAxesCompleted()const
{
  bool _return = true;
  for(unsigned int axis = 0;axis<NUM_AXES;axis++)
   _return &= _axes[axis]->isDriven();
  return _return;
}


bool
CRSnAxesVelocityController::stopAxis(int axis)
{
   DBG;
  if (!(axis<0 || axis>NUM_AXES-1))
    return _axes[axis]->stop();
  else{
    log(Error) <<"Axis "<< axis <<" doesn't exist!!"<<endlog();
    return false;
  }
}

bool
CRSnAxesVelocityController::stopAxisCompleted(int axis) const {
    DBG;
    return _axes[axis]->isStopped();
}


bool
CRSnAxesVelocityController::stopAllAxes()
{
  DBG;
  bool result = true;
  for (int axis=0;axis<NUM_AXES;++axis) {
    result &= _axes[axis]->stop();
  }
  return result;
}

bool
CRSnAxesVelocityController::stopAllAxesCompleted() const
{
  return true;
}




bool
CRSnAxesVelocityController::unlockAxis(int axis)
{
  DBG;
  if (!(axis<0 || axis>LiAS_NUM_AXIS-1))
  {
      _axes[axis]->unlock();
      return true;
  } else{
    log(Error) <<"Axis "<< axis <<"doesn't exist!!"<<endlog();
    return false;
  }
}

bool
CRSnAxesVelocityController::unlockAxisCompleted(int axis) const {
    DBG;
    return true;
}


bool
CRSnAxesVelocityController::lockAxis(int axis)
{
  DBG;
  if (!(axis<0 || axis>LiAS_NUM_AXIS-1))
  {
      _axes[axis]->lock();
      return true;
  } else {
    log(Error) <<"Axis "<< axis <<"doesn't exist!!"<<endlog();
    return false;
  }
}

bool
CRSnAxesVelocityController::lockAxisCompleted(int axis) const {
    DBG;
    return true;
}


bool
CRSnAxesVelocityController::lockAllAxes() {
    DBG;
    bool result=true;
    for (int axis=0;axis<NUM_AXES;axis++) {
       _axes[axis]->lock();
    }
    return result;
}

bool
CRSnAxesVelocityController::lockAllAxesCompleted() const {
    DBG;
    return true;
}


bool
CRSnAxesVelocityController::unlockAllAxes() {
    DBG;
    for (int axis=0;axis<NUM_AXES;axis++) {
         _axes[axis]->unlock();
    }
    return true;
}


bool
CRSnAxesVelocityController::unlockAllAxesCompleted() const {
    DBG;
    return true;
}

bool
CRSnAxesVelocityController::addDriveOffset(int axis, double offset)
{
  DBG;
  if (!(axis<0 || axis>NUM_AXES-1)) {
       #if defined (OROPKG_OS_LXRT)
       _axes[axis]->getDrive()->addOffset(offset);
       #endif
       return true;
  } else {
    log(Error) <<"Axis "<< axis <<"doesn't exist!!"<<endlog();
    return false;
  }
}

bool
CRSnAxesVelocityController::addDriveOffsetCompleted(int axis, double ) const {
    DBG;
    return true;
}


bool
CRSnAxesVelocityController::initPosition(int axis)
{
  DBG;
  if (!(axis<0 || axis>NUM_AXES-1)) {
	   _homed[axis] = true;
       #if defined (OROPKG_OS_LXRT)
       _encoder[axis]->writeSensor(initialPosition.value()[axis]);
       servoIntVel[axis] = initialPosition.value()[axis];
       previousPos[axis] = servoIntVel[axis];
       #else
       #endif
       return true;
  } else {
    log(Error) <<"Axis "<< axis <<"doesn't exist!!"<<endlog();
    return false;
  }
}

bool
CRSnAxesVelocityController::initPositionCompleted(int) const {
    DBG;
    return true;
}


bool CRSnAxesVelocityController::changeServo() {
    for (int axis=0;axis<NUM_AXES;++axis) {
        servoIntVel[axis] *= _servoGain[axis] / servoGain.value()[axis];
        servoIntVel[axis] *= _servoIntegrationFactor[axis] / servoIntegrationFactor.value()[axis];
        _servoGain[axis]  = servoGain.value()[axis];
        _servoIntegrationFactor[axis] = servoIntegrationFactor.value()[axis];
    }
    return true;
}

bool CRSnAxesVelocityController::changeServoCompleted() const {
    DBG;
    return true;
}


/**
 *  This function contains the application's startup code.
 *  Return false to abort startup.
 **/
bool CRSnAxesVelocityController::startup() {
    DBG;
    // Initialize the servo loop
  for (int axis=0;axis<NUM_AXES;++axis) {
      servoIntVel[axis] = _axes[axis]->getSensor("Position")->readSensor();
	  previousPos[axis] = servoIntVel[axis];
  }
  return true;
}

/**
 * This function is periodically called.
 */
void CRSnAxesVelocityController::update() {
#if !defined (OROPKG_OS_LXRT)
	for (int axis=0;axis<NUM_AXES;axis++) {
		double measpos = signAxes.value()[axis]*_axes[axis]->getSensor("Position")->readSensor();
/*        if((
            (measpos < lowerPositionLimits.value()[axis])
          ||(measpos > upperPositionLimits.value()[axis])
          ) && _homed[axis]) {
            // emit event.
			positionOutOfRange(axis, measpos);
        }*/
		double setpoint = signAxes.value()[axis]*driveValue[axis]->Get();
        _axes[axis]->drive(setpoint);
        positionValue[axis] ->Set(measpos);
		output[axis]->Set(setpoint);
        bool ref = measpos > initialPosition.value()[axis];
		if (axis==3) ref = !ref;
        reference[axis]->Set(ref);
	}
#else
    double dt;
    // Determine sampling time :
    if (servoInitialized) {
        dt              = TimeService::Instance()->secondsSince(previousTime);
        previousTime    = TimeService::Instance()->getTicks();
    } else {
        dt = 0.0;
    }
    previousTime        = TimeService::Instance()->getTicks();
    servoInitialized    = true;

    double outputvel[NUM_AXES];

    for (int axis=0;axis<NUM_AXES;axis++) {
        double measpos;
        double setpoint;
        // Ask the position and perform checks in joint space.
        measpos = signAxes.value()[axis]*_encoder[axis]->readSensor();
        positionValue[axis] ->Set(  measpos );
        if((
            (measpos < lowerPositionLimits.value()[axis])
          ||(measpos > upperPositionLimits.value()[axis])
          ) && _homed[axis]) {
            // emit event.
			positionOutOfRange(axis, measpos);
        }
        if (_axes[axis]->isDriven()) {
            setpoint = driveValue[axis]->Get();
        	// perform control action ( dt is zero the first time !) :
        	servoIntVel[axis]      += dt*setpoint;
        	double error            = servoIntVel[axis] - measpos;
        	servoIntError[axis]    += dt*error;
            double deriv;
            if (dt < 1E-4) {
                deriv = 0.0;
            } else {
                deriv = servoDerivTime.value()[axis]*(previousPos[axis]-measpos)/dt;
            }
        	outputvel[axis]         = _servoGain[axis]*
                (error + _servoIntegrationFactor[axis]*servoIntError[axis] + deriv)
                + _servoFFScale[axis]*setpoint;
            // check direction of motion or desired motion :
            double offsetsign;
            if (previousPos[axis] < measpos) {
                offsetsign = 1.0;
            } else if ( previousPos[axis] > measpos ) {
                offsetsign = -1.0;
            } else {
                offsetsign = outputvel[axis] < 0.0 ? -1.0 : 1.0;
            }
            outputvel[axis] += offsetsign*offset.value()[axis];
		} else {
			outputvel[axis] = 0.0;
		}
		previousPos[axis] = measpos;
    }
    for (int axis=0;axis<NUM_AXES;axis++) {
        // send the drive value to hw and performs checks
        if (outputvel[axis] < -driveLimits.value()[axis])  {
            // emit event.
			//driveOutOfRange(axis, outputvel[axis]);
			// saturate
            outputvel[axis] = -driveLimits.value()[axis];
        }
        if (outputvel[axis] >  driveLimits.value()[axis]) {
            // emit event.
			//driveOutOfRange(axis, outputvel[axis]);
			// saturate
            outputvel[axis] = driveLimits.value()[axis];
        }
        output[axis]->Set(outputvel[axis]);
        _axes[axis]->drive(signAxes.value()[axis]*outputvel[axis]);
        // ask the reference value from the hw
        reference[axis]->Set( _reference[axis]->isOn());
    }
	#endif
}

bool CRSnAxesVelocityController::prepareForUse() {
    DBG;
    return true;
}

bool
CRSnAxesVelocityController::prepareForUseCompleted() const {
    DBG;
    return true;
}


bool CRSnAxesVelocityController::prepareForShutdown() {
    DBG;
    return true;
}

bool
CRSnAxesVelocityController::prepareForShutdownCompleted() const {
    DBG;
    return true;
}


/**
 * This function is called when the task is stopped.
 */
void CRSnAxesVelocityController::shutdown() {
    DBG;
    prepareForShutdown();
    //marshalling()->writeProperties(_propertyfile);
}


} // end of namespace Orocos

//#endif //OROPKG_OS_LXRT
