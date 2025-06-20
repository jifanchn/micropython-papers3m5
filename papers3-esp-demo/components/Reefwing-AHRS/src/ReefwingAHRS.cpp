/******************************************************************
  @file       ReefwingAHRS.cpp
  @brief      Attitude and Heading Reference System (AHRS)
  @author     David Such
  @copyright  Please see the accompanying LICENSE file.

  Code:        David Such
  Version:     2.3.6
  Date:        11/02/25

  1.0.0 Original Release.                         22/02/22
  1.1.0 Added NONE fusion option.                 25/05/22
  2.0.0 Changed Repo & Branding                   15/12/22
  2.0.1 Invert Gyro Values PR                     24/12/22
  2.1.0 Updated Fusion Library                    30/12/22
  2.2.0 Add support for Nano 33 BLE Sense Rev. 2  10/02/23
  2.3.0 Extended Kalman Filter added              20/11/24
  2.3.1 Madgwick filter bug fixed                 30/12/24
  2.3.2 Improved normalization for Madgwick       31/12/24
  2.3.3 Complementary update enhancements         05/01/25
  2.3.4 Corrected spelling for Mahony             09/01/25
  2.3.5 Fixed bug in complementaryUpdate          09/01/25
  2.3.6 Added support for Nano 33 BLE Rev. 2      11/02/25


  Credits: - The C++ code for our quaternion position update 
             using the Madgwick Filter is based on the paper, 
             "An efficient orientation filter for inertial and 
             inertial/magnetic sensor arrays" written by Sebastian 
             O.H. Madgwick in April 30, 2010.

******************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include "ReefwingAHRS.h"

/******************************************************************
 *
 * Extended Kalman Filter utilities - 
 * 
 ******************************************************************/

// Define state transition function
// state[0]: Roll angle in radians
// state[1]: Pitch angle in radians
// controlInput[0]: Roll rate from gyroscope (gx in rad/s)
// controlInput[1]: Pitch rate from gyroscope (gy in rad/s)
void imuStateTransitionFunc(const float* state, const float* controlInput, float deltaTime, float* newState) {
    newState[0] = state[0] + controlInput[0] * deltaTime; // Update roll
    newState[1] = state[1] + controlInput[1] * deltaTime; // Update pitch
}

// Define measurement function
// Assign roll and pitch from accelerometer
void imuMeasurementFunc(const float* state, float* measurement) {
    measurement[0] = state[0];  // Roll
    measurement[1] = state[1];  // Pitch
}

// Jacobian is a 2x2 identity matrix for a linear state transition
// The state variables are roll and pitch
// The control inputs are gyroscope rates
void imuStateJacobianFunc(const float* state, const float* controlInput, float deltaTime, float* jacobian) {
    jacobian[0] = 1.0f;  // d(roll')/d(roll)
    jacobian[1] = 0.0f;  // d(roll')/d(pitch)
    jacobian[2] = 0.0f;  // d(pitch')/d(roll)
    jacobian[3] = 1.0f;  // d(pitch')/d(pitch)
}

void imuMeasurementJacobianFunc(const float* state, float* jacobian) {
    // Jacobian is a 2x2 identity matrix for direct measurements
    jacobian[0] = 1.0f;  // d(measurement_roll)/d(roll)
    jacobian[1] = 0.0f;  // d(measurement_roll)/d(pitch)
    jacobian[2] = 0.0f;  // d(measurement_pitch)/d(roll)
    jacobian[3] = 1.0f;  // d(measurement_pitch)/d(pitch)
}

/******************************************************************
 *
 * ReefwingAHRS Implementation - 
 * 
 ******************************************************************/

ReefwingAHRS::ReefwingAHRS() { 
  _boardTypeStr[0] = "Nano";
  _boardTypeStr[1] = "Nano 33 BLE";
  _boardTypeStr[2] = "Nano 33 BLE Rev 2";
  _boardTypeStr[3] = "Nano 33 BLE Sense";
  _boardTypeStr[4] = "Nano 33 BLE Sense Rev 2";
  _boardTypeStr[5] = "Seeed XIAO nRF52840 Sense";
  _boardTypeStr[6] = "MKR Portenta H7";
  _boardTypeStr[7] = "MKR Vidor 4000";
  _boardTypeStr[8] = "Nano 33 IoT";
  _boardTypeStr[9] = "Undefined Board Type";
}

void ReefwingAHRS::begin() {
  //  Detect Board Hardware & Associated IMU
  setBoardType(BoardType::NOT_DEFINED);
  setImuType(ImuType::UNKNOWN);
  setDOF(DOF::DOF_6);

  #if defined(ARDUINO_ARDUINO_NANO33BLE)    //  Nano 33 BLE found - 4 possible variants
  
    byte error;
    
    Wire1.begin();
    Wire1.beginTransmission(HTS221_ADDRESS);
    error = Wire1.endTransmission();

    if (error == 0) {
      setBoardType(BoardType::NANO33BLE_SENSE_R1);
      setImuType(ImuType::LSM9DS1);
      setDOF(DOF::DOF_9);
    }
    else {
      Wire1.beginTransmission(HS3003_ADDRESS);
      error = Wire1.endTransmission();

      if (error == 0) {
        setBoardType(BoardType::NANO33BLE_SENSE_R2);
        setImuType(ImuType::BMI270_BMM150);
        setDOF(DOF::DOF_9);
      }
      else {
        Wire1.beginTransmission(LSM9DS1AG_ADDRESS);
        error = Wire1.endTransmission();

        if (error == 0) {
          setBoardType(BoardType::NANO33BLE);
          setImuType(ImuType::LSM9DS1);
          setDOF(DOF::DOF_9);
        }
        else {
          setBoardType(BoardType::NANO33BLE_R2);
          setImuType(ImuType::BMI270_BMM150);
          setDOF(DOF::DOF_9);
        }
      }
    }
  #elif defined(ARDUINO_AVR_NANO)   
    setBoardType(BoardType::NANO);
  #elif defined(ARDUINO_PORTENTA_H7_M7) 
    setBoardType(BoardType::PORTENTA_H7);
  #elif defined(ARDUINO_SAMD_MKRVIDOR4000)
    setBoardType(BoardType::VIDOR_4000);
  #elif defined(ARDUINO_SAMD_NANO_33_IOT)
    setBoardType(BoardType::NANO33IOT);
  #elif defined(BOARD_NAME)
    if (strncmp(BOARD_NAME, _boardTypeStr[5], 25) == 0) {
      setBoardType(BoardType::XIAO_SENSE);
      setImuType(ImuType::LSM6DS3);
      setDOF(DOF::DOF_6);
    }
  #endif

  //  Default Sensor Fusion Co-Efficients - see README.md
  _gyroMeasError = 40.0f * DEG_TO_RAD;   // gyroscope measurement error in rads/s (start at 40 deg/s)
  _alpha = 0.98; // default complementary filter coefficient
  _beta = sqrt(3.0f / 4.0f) * _gyroMeasError;   // compute beta
  _Kp = 2.0f * 5.0f; // These are the free parameters in the Mahony filter and fusion scheme, _Kp for proportional feedback, _Ki for integral
  _Ki = 0.0f;

  //  Set default sensor fusion algorithm
  setFusionAlgorithm(SensorFusion::MADGWICK);

  //  Set default magnetic declination - Sydney, NSW, AUSTRALIA
  setDeclination(12.717);

  //  Init CLASSIC linear complementary filter
  angles.pitch = 0.0f;
  angles.roll = 0.0f;
  angles.yaw = 0.0f;

  // Init Extended Kalman Filter
  state[0] = 0.0f; // Roll
  state[1] = 0.0f; // Pitch

  // Initialize the covariance, process noise, and measurement noise matrices
  float initialState[2] = {0.0f, 0.0f};
  float initialCovariance[4] = {0.01f, 0.0f, 0.0f, 0.01f}; // Small initial uncertainty


  ekf.initialize(initialState, initialCovariance, 2, 2);
  ekf.setStateTransition(imuStateTransitionFunc);
  ekf.setMeasurementFunction(imuMeasurementFunc);
  ekf.setStateTransitionJacobian(imuStateJacobianFunc);
  ekf.setMeasurementJacobian(imuMeasurementJacobianFunc);
}

void ReefwingAHRS::reset() {
    // Reset timing variables
    _lastUpdate = 0;

    // Reset AHRS parameters
    angles = {0.0f, 0.0f, 0.0f};             // Reset roll, pitch, and yaw
    configAngles = {0.0f, 0.0f, 0.0f};       // Reset configuration angles

    _q = Quaternion(1.0f, 0.0f, 0.0f, 0.0f); // Reset quaternion
    _eInt[0] = _eInt[1] = _eInt[2] = 0.0f;   // Reset Mahony integral error
    _att[0] = 1.0f; _att[1] = 0.0f;          // Reset complementary filter state
    _att[2] = 0.0f; _att[3] = 0.0f;

    // Reset Extended Kalman Filter
    state[0] = 0.0f; // Reset Roll
    state[1] = 0.0f; // Reset Pitch

    float initialState[2] = {0.0f, 0.0f};
    float initialCovariance[4] = {0.01f, 0.0f, 0.0f, 0.01f}; // Small initial uncertainty

    ekf.initialize(initialState, initialCovariance, 2, 2); // Reinitialize EKF
    ekf.setStateTransition(imuStateTransitionFunc);        // Reattach transition function
    ekf.setMeasurementFunction(imuMeasurementFunc);        // Reattach measurement function
    ekf.setStateTransitionJacobian(imuStateJacobianFunc);  // Reattach state Jacobian
    ekf.setMeasurementJacobian(imuMeasurementJacobianFunc); // Reattach measurement Jacobian

    // Reset other variables if required (e.g., specific to filters)
    _fusion = SensorFusion::MADGWICK; // Default filter, can be overridden later
}

void ReefwingAHRS::update() {
  long now = micros();
  float deltaT = ((now - _lastUpdate)/1000000.0f); // Time elapsed since last update in seconds

  _lastUpdate = now;

  switch(_fusion) {
    case SensorFusion::MADGWICK:
      madgwickUpdate(gyroToRadians(), deltaT);
      angles = _q.toEulerAngles(_declination);
    break;
    case SensorFusion::MAHONY:
      mahonyUpdate(gyroToRadians(), deltaT);
      angles = _q.toEulerAngles(_declination);
    break;
    case SensorFusion::COMPLEMENTARY:
      complementaryUpdate(gyroToRadians(), deltaT);
      angles = _q.toEulerAngles(_declination);
    break;
    case SensorFusion::CLASSIC:
      updateEulerAngles(deltaT);
      classicUpdate();
      if (_dof == DOF::DOF_9) {
        tiltCompensatedYaw();
      }
    break;
    case SensorFusion::EXTENDED_KALMAN:
      extendedKalmanUpdate(_data, deltaT);
      if (_dof == DOF::DOF_9) {
        tiltCompensatedYaw();
      }
    break;
    case SensorFusion::NONE:
      updateEulerAngles(deltaT);
      if (_dof == DOF::DOF_9) {
        tiltCompensatedYaw();
      }
    break;
  }
}

SensorData ReefwingAHRS::gyroToRadians() {
  //  Convert gyro data from DPS to radians/sec.
  SensorData filterData = _data;

  filterData.gx = _data.gx * DEG_TO_RAD;
  filterData.gy = _data.gy * DEG_TO_RAD;
  filterData.gz = _data.gz * DEG_TO_RAD;

  return filterData;
}

void ReefwingAHRS::formatAnglesForConfigurator() {
  //  Adjust angle signs for consistent display
  //  in the Reefwing Configurator
  configAngles = angles;
  
  switch(_fusion) {
    case SensorFusion::MADGWICK:
      configAngles.roll = -angles.roll;
      configAngles.pitch = -angles.pitch;
      configAngles.pitchRadians = -angles.pitchRadians;
      configAngles.rollRadians = -angles.rollRadians;
    break;
    case SensorFusion::MAHONY:
      configAngles.roll = -angles.roll;
      configAngles.pitch = -angles.pitch;
      configAngles.pitchRadians = -angles.pitchRadians;
      configAngles.rollRadians = -angles.rollRadians;
      break;
    case SensorFusion::COMPLEMENTARY:
      configAngles.yaw = -angles.yaw;
      configAngles.yawRadians = -angles.yawRadians;
      break;
    case SensorFusion::CLASSIC:
      break;
    case SensorFusion::EXTENDED_KALMAN:
      break;
    case SensorFusion::NONE:
      break;
    default:
      break;
  }
}

BoardType ReefwingAHRS::getBoardType() {
  return _boardType;
}

const char* ReefwingAHRS::getBoardTypeString() {
  uint8_t index = (uint8_t)_boardType;

  return _boardTypeStr[index];
}

void ReefwingAHRS::setFusionAlgorithm(SensorFusion algo) {
  _fusion = algo;
}

void ReefwingAHRS::setAlpha(float a) {
  _alpha = a;
}

void ReefwingAHRS::setBeta(float b) {
  _beta = b;
}

void ReefwingAHRS::setGyroMeasError(float gme) {
  _gyroMeasError = gme;
}

void ReefwingAHRS::setKp(float p) {
  _Kp = p;
}

void ReefwingAHRS::setKi(float i) {
  _Ki = i;
}

void ReefwingAHRS::setDeclination(float dec) {
  _declination = dec;
}

void ReefwingAHRS::setData(SensorData d, bool axisAlign) {
  //  If required, convert IMU data to a 
  //  consistent Reference Frame.
  _data = d;

  if (axisAlign) {
    switch(_imuType) {
      case ImuType::LSM9DS1:
        _data.mx = -d.mx;
      break;
      case ImuType::LSM6DS3:
      break;
      case ImuType::BMI270_BMM150:
        _data.my = -d.my;
      break;
      case ImuType::MPU6050:
      break;
      case ImuType::MPU6500:
      break;
      case ImuType::UNKNOWN:
      break;
      default:
      break;
    }
  }
}

void ReefwingAHRS::setDOF(DOF d) {
  _dof = d;
}

void ReefwingAHRS::setBoardType(BoardType b) {
  _boardType = b;
}

void ReefwingAHRS::setImuType(ImuType i) {
  _imuType = i;
}

Quaternion ReefwingAHRS::getQuaternion() {
  return _q;
}

/******************************************************************
 *
 * ReefwingAHRS - Update Methods 
 * 
 ******************************************************************/

void ReefwingAHRS::extendedKalmanUpdate(SensorData d, float deltaT) {
  // Pre-process sensor data
  float accRoll = atan2(d.ay, d.az);
  float accPitch = atan2(-d.ax, sqrt(d.ay * d.ay + d.az * d.az));

  // Define control input (gyroscope rates in rad/s)
  float controlInput[2] = {(float)(d.gx * DEG_TO_RAD), (float)(d.gy * DEG_TO_RAD)};

  // Define measurement (accelerometer angles)
  float measurement[2] = {accRoll, accPitch};

  // Call the EKF predict and update steps
  ekf.predict(controlInput, deltaT);
  ekf.update(measurement);

  // Extract the updated state (roll and pitch)
  const float* state = ekf.getState();
  float roll = state[0];  
  float pitch = state[1];

  // Update angles
  angles.roll = roll * RAD_TO_DEG;
  angles.pitch = pitch * RAD_TO_DEG;
  angles.rollRadians = roll;
  angles.pitchRadians = pitch;
}

void ReefwingAHRS::classicUpdate() {
  // Validate denominators for angle calculations
  float rollDenominator = sqrt(pow(_data.ax, 2) + pow(_data.az, 2));
  float pitchDenominator = sqrt(pow(_data.ay, 2) + pow(_data.az, 2));

  if (rollDenominator < 1e-6 || pitchDenominator < 1e-6) { return; }

  // Convert from force vector to angle using 3-axis formula - result in radians
  float accRollAngle  = atan(-1 * _data.ay / rollDenominator);
  float accPitchAngle = -atan(-1 * _data.ax / pitchDenominator);

  // Combine gyro and acc angles using a complementary filter
  angles.pitch = _alpha * angles.pitch + (1.0f - _alpha) * accPitchAngle * RAD_TO_DEG;
  angles.roll = _alpha * angles.roll + (1.0f - _alpha) * accRollAngle * RAD_TO_DEG;
  angles.pitchRadians = angles.pitch * DEG_TO_RAD;
  angles.rollRadians = angles.roll * DEG_TO_RAD;
}

void ReefwingAHRS::tiltCompensatedYaw() {
  //  Calculate yaw using magnetometer & derived roll and pitch
  float mag_x_compensated = _data.mx * cos(angles.pitchRadians) + _data.mz * sin(angles.pitchRadians);
  float mag_y_compensated = _data.mx * sin(angles.rollRadians) * sin(angles.pitchRadians) + _data.my * cos(angles.rollRadians) - _data.mz * sin(angles.rollRadians) * cos(angles.pitchRadians);

  angles.yawRadians = atan2(mag_y_compensated, mag_x_compensated);
  angles.yaw =  angles.yawRadians * RAD_TO_DEG;    //  Yaw compensated for tilt
  
  angles.heading = angles.yaw - _declination;
  angles.heading = fmod(angles.heading + 360.0, 360.0);  // Normalize to [0, 360)
}

void ReefwingAHRS::updateEulerAngles(float deltaT) {
  // Auxiliary variables to avoid repeated calculation
  float sinPHI = sin(angles.rollRadians);
  float cosPHI = cos(angles.rollRadians);
  float cosTHETA = cos(angles.pitchRadians);
  float tanTHETA = tan(angles.pitchRadians);

  //  Convert gyro rates to Euler rates (ground reference frame)
  //  Euler Roll Rate, ϕ ̇= p + sin(ϕ)tan(θ) × q + cos(ϕ)tan(θ) × r
  //  Euler Pitch Rate, θ ̇= cos(ϕ) × q - sin(ϕ) × r
  //  Euler Yaw Rate, ψ ̇= [sin(ϕ) × q]/cos(θ) + cos(ϕ)cos(θ) × r

  float eulerRollRate = _data.gy + sinPHI * tanTHETA * _data.gx + cosPHI * tanTHETA * _data.gz;
  float eulerPitchRate = cosPHI * _data.gx - sinPHI * _data.gz;
  float eulerYawRate = 0.0f;

  if (fabs(cosTHETA) > 1e-6) {
    // Avoid division by zero in yaw rate calculation
    eulerYawRate = (sinPHI * _data.gx) / cosTHETA + cosPHI * cosTHETA * _data.gz;
  }

  angles.rollRadians  += eulerRollRate * deltaT;    // Angle around the X-axis
  angles.pitchRadians += eulerPitchRate * deltaT;   // Angle around the Y-axis
  angles.roll = angles.rollRadians * RAD_TO_DEG;
  angles.pitch = angles.pitchRadians * RAD_TO_DEG;
  angles.yawRadians   += eulerYawRate * deltaT;     // Angle around the Z-axis 
  angles.yaw = angles.yawRadians * RAD_TO_DEG;
}

void ReefwingAHRS::complementaryUpdate(SensorData d, float deltaT) {
  //  Roll (Theta) and Pitch (Phi) from accelerometer
  float rollAcc = atan2(d.ay, d.az);
  float pitchAcc = atan2(-d.ax, sqrt(pow(d.ay, 2) + pow(d.az, 2)));

  // Auxiliary variables to avoid repeated arithmetic
  float _halfdT = deltaT * 0.5f;
  float _cosTheta = cos(rollAcc);
  float _cosPhi = cos(pitchAcc);
  float _sinTheta = sin(rollAcc);
  float _sinPhi = sin(pitchAcc);
  float _halfTheta = rollAcc * 0.5f;
  float _halfPhi = pitchAcc * 0.5f;
  float _cosHalfTheta = cos(_halfTheta);
  float _cosHalfPhi = cos(_halfPhi);
  float _sinHalfTheta = sin(_halfTheta);
  float _sinHalfPhi = sin(_halfPhi);

  //  Calculate Attitude Quaternion
  //  ref: https://ahrs.readthedocs.io/en/latest/filters/complementary.html
  //  amended based on suggestion of Martin Budden
  const float qDot0 = - d.gx * _att[1] - d.gy * _att[2] - d.gz * _att[3];
  const float qDot1 = + d.gx * _att[0] - d.gy * _att[3] + d.gz * _att[2];
  const float qDot2 = + d.gx * _att[3] + d.gy * _att[0] - d.gz * _att[1];
  const float qDot3 = - d.gx * _att[2] + d.gy * _att[1] + d.gz * _att[0];

  _att[0] += qDot0 * _halfdT;
  _att[1] += qDot1 * _halfdT;
  _att[2] += qDot2 * _halfdT;
  _att[3] += qDot3 * _halfdT;

  //  Calculate Tilt Vector [bx by bz] and tilt adjusted yaw (Psi) using accelerometer data
  float bx = d.mx * _cosTheta + d.my * _sinTheta * _sinPhi + d.mz * _sinTheta * _cosPhi;
  float by = d.my * _cosPhi - d.mz * _sinPhi;
  // float bz = -d.mx * _sinTheta + d.my * _cosTheta * _sinPhi + d.mz * _cosTheta * _cosPhi;

  float yaw = atan2(-by, bx);

  // More auxiliary variables to avoid repeated arithmetic
  float _halfPsi = yaw * 0.5f;
  float _cosHalfPsi = cos(_halfPsi);
  float _sinHalfPsi = sin(_halfPsi);

  //  Convert Accelerometer & Magnetometer roll, pitch & yaw to quaternion (qam)
  float qam[4];  

  qam[0] = _cosHalfPhi * _cosHalfTheta * _cosHalfPsi + _sinHalfPhi * _sinHalfTheta * _sinHalfPsi;
  qam[1] = _sinHalfPhi * _cosHalfTheta * _cosHalfPsi - _cosHalfPhi * _sinHalfTheta * _sinHalfPsi;
  qam[2] = _cosHalfPhi * _sinHalfTheta * _cosHalfPsi + _sinHalfPhi * _cosHalfTheta * _sinHalfPsi;
  qam[3] = _cosHalfPhi * _cosHalfTheta * _sinHalfPsi - _sinHalfPsi * _sinHalfTheta * _cosHalfPsi;

  //  Fuse attitude quaternion (_att) with qam using complementary filter
  _q.q0 = _alpha * _att[0] + (1 - _alpha) * qam[0];
  _q.q1 = _alpha * _att[1] + (1 - _alpha) * qam[1];
  _q.q2 = _alpha * _att[2] + (1 - _alpha) * qam[2];
  _q.q3 = _alpha * _att[3] + (1 - _alpha) * qam[3];

  //  Normalize quaternion
  //  Without normalization, errors can accumulate over time, leading to drift or instability.
  float norm = sqrt(_q.q0 * _q.q0 + _q.q1 * _q.q1 + _q.q2 * _q.q2 + _q.q3 * _q.q3);

  _q.q0 /= norm;
  _q.q1 /= norm;
  _q.q2 /= norm;
  _q.q3 /= norm;
}

void ReefwingAHRS::madgwickUpdate(SensorData d, float deltaT) {
  float norm;
  float hx, hy, _2bx, _2bz, _bx, _bz;
  float s0, s1, s2, s3;
  float qDot0, qDot1, qDot2, qDot3;

  // Auxiliary variables to avoid repeated arithmetic
  float _2q0mx;
  float _2q0my;
  float _2q0mz;
  float _2q1mx;
  float _4bx;
  float _4bz;
  float _2q0 = 2.0f * _q.q0;
  float _2q1 = 2.0f * _q.q1;
  float _2q2 = 2.0f * _q.q2;
  float _2q3 = 2.0f * _q.q3;
  float _2q0q2 = 2.0f * _q.q0 * _q.q2;
  float _2q2q3 = 2.0f * _q.q2 * _q.q3;
  float q0q0 = _q.q0 * _q.q0;
  float q0q1 = _q.q0 * _q.q1;
  float q0q2 = _q.q0 * _q.q2;
  float q0q3 = _q.q0 * _q.q3;
  float q1q1 = _q.q1 * _q.q1;
  float q1q2 = _q.q1 * _q.q2;
  float q1q3 = _q.q1 * _q.q3;
  float q2q2 = _q.q2 * _q.q2;
  float q2q3 = _q.q2 * _q.q3;
  float q3q3 = _q.q3 * _q.q3;

  // Normalise accelerometer measurement
  norm = sqrt(d.ax * d.ax + d.ay * d.ay + d.az * d.az);
  if (norm != 0.0f) {
    norm = 1.0f/norm;
    d.ax *= norm;
    d.ay *= norm;
    d.az *= norm;
  }

  // Normalise magnetometer measurement
  norm = sqrt(d.mx * d.mx + d.my * d.my + d.mz * d.mz);
  if (norm != 0.0f) {
    norm = 1.0f/norm;
    d.mx *= norm;
    d.my *= norm;
    d.mz *= norm;
  }

  // Reference direction of Earth's magnetic field
  _2q0mx = 2.0f * _q.q0 * d.mx;
  _2q0my = 2.0f * _q.q0 * d.my;
  _2q0mz = 2.0f * _q.q0 * d.mz;
  _2q1mx = 2.0f * _q.q1 * d.mx;

  hx = d.mx * q0q0 - _2q0my * _q.q3 + _2q0mz * _q.q2 + d.mx * q1q1 + _2q1 * d.my * _q.q2 + _2q1 * d.mz * _q.q3 - d.mx * q2q2 - d.mx * q3q3;
  hy = _2q0mx * _q.q3 + d.my * q0q0 - _2q0mz * _q.q1 + _2q1mx * _q.q2 - d.my * q1q1 + d.my * q2q2 + _2q2 * d.mz * _q.q3 - d.my * q3q3;
  _bx = sqrt(hx * hx + hy * hy);
  _bz = -_2q0mx * _q.q2 + _2q0my * _q.q1 + d.mz * q0q0 + _2q1mx * _q.q3 - d.mz * q1q1 + _2q2 * d.my * _q.q3 - d.mz * q2q2 + d.mz * q3q3;
  _2bx = 2.0f * _bx;
  _2bz = 2.0f * _bz;
  _4bx = 2.0f * _2bx;
  _4bz = 2.0f * _2bz;

  // Gradient decent algorithm corrective step
  s0 = -_2q2 * (2.0f * q1q3 - _2q0q2 - d.ax) + _2q1 * (2.0f * q0q1 + _2q2q3 - d.ay) - _2bz * _q.q2 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - d.mx) + (-_2bx * _q.q3 + _2bz * _q.q1) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - d.my) + _2bx * _q.q2 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - d.mz);
  s1 = _2q3 * (2.0f * q1q3 - _2q0q2 - d.ax) + _2q0 * (2.0f * q0q1 + _2q2q3 - d.ay) - 4.0f * _q.q1 * (1.0f - 2.0f * q1q1 - 2.0f * q2q2 - d.az) + _2bz * _q.q3 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - d.mx) + (_2bx * _q.q2 + _2bz * _q.q0) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - d.my) + (_2bx * _q.q3 - _4bz * _q.q1) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - d.mz);
  s2 = -_2q0 * (2.0f * q1q3 - _2q0q2 - d.ax) + _2q3 * (2.0f * q0q1 + _2q2q3 - d.ay) - 4.0f * _q.q2 * (1.0f - 2.0f * q1q1 - 2.0f * q2q2 - d.az) + (-_4bx * _q.q2 - _2bz * _q.q0) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - d.mx) + (_2bx * _q.q1 + _2bz * _q.q3) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - d.my) + (_2bx * _q.q0 - _4bz * _q.q2) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - d.mz);
  s3 = _2q1 * (2.0f * q1q3 - _2q0q2 - d.ax) + _2q2 * (2.0f * q0q1 + _2q2q3 - d.ay) + (-_4bx * _q.q3 + _2bz * _q.q1) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - d.mx) + (-_2bx * _q.q0 + _2bz * _q.q2) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - d.my) + _2bx * _q.q1 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - d.mz);
  
  norm = sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);    // normalise step magnitude
  norm = 1.0f/norm;
  s0 *= norm;
  s1 *= norm;
  s2 *= norm;
  s3 *= norm;

  // Compute rate of change of quaternion
  qDot0 = 0.5f * (-_q.q1 * d.gx - _q.q2 * d.gy - _q.q3 * d.gz) - _beta * s0;
  qDot1 = 0.5f * (_q.q0 * d.gx + _q.q2 * d.gz - _q.q3 * d.gy) - _beta * s1;
  qDot2 = 0.5f * (_q.q0 * d.gy - _q.q1 * d.gz + _q.q3 * d.gx) - _beta * s2;
  qDot3 = 0.5f * (_q.q0 * d.gz + _q.q1 * d.gy - _q.q2 * d.gx) - _beta * s3;

  // Integrate to yield quaternion
  _q.q0 += qDot0 * deltaT;
  _q.q1 += qDot1 * deltaT;
  _q.q2 += qDot2 * deltaT;
  _q.q3 += qDot3 * deltaT;

  norm = sqrt(_q.q0 * _q.q0 + _q.q1 * _q.q1 + _q.q2 * _q.q2 + _q.q3 * _q.q3);    // normalise quaternion
  norm = 1.0f/norm;

  _q.q0 = _q.q0 * norm;
  _q.q1 = _q.q1 * norm;
  _q.q2 = _q.q2 * norm;
  _q.q3 = _q.q3 * norm;
}

void ReefwingAHRS::mahonyUpdate(SensorData d, float deltaT) {
  float norm;
  float hx, hy, bx, bz;
  float vx, vy, vz, wx, wy, wz;
  float ex, ey, ez;
  float pa, pb, pc;

  // Auxiliary variables to avoid repeated arithmetic
  float q0q0 = _q.q0 * _q.q0;
  float q0q1 = _q.q0 * _q.q1;
  float q0q2 = _q.q0 * _q.q2;
  float q0q3 = _q.q0 * _q.q3;
  float q1q1 = _q.q1 * _q.q1;
  float q1q2 = _q.q1 * _q.q2;
  float q1q3 = _q.q1 * _q.q3;
  float q2q2 = _q.q2 * _q.q2;
  float q2q3 = _q.q2 * _q.q3;
  float q3q3 = _q.q3 * _q.q3;   

  // Normalise accelerometer measurement
  norm = sqrtf(d.ax * d.ax + d.ay * d.ay + d.az * d.az);
  if (norm != 0.0f) {
    norm = 1.0f / norm;        // use reciprocal for division
    d.ax *= norm;
    d.ay *= norm;
    d.az *= norm;
  }

  // Normalise magnetometer measurement
  norm = sqrtf(d.mx * d.mx + d.my * d.my + d.mz * d.mz);
  if (norm != 0.0f) {
    norm = 1.0f / norm;        // use reciprocal for division
    d.mx *= norm;
    d.my *= norm;
    d.mz *= norm;
  }

  // Reference direction of Earth's magnetic field
  hx = 2.0f * d.mx * (0.5f - q2q2 - q3q3) + 2.0f * d.my * (q1q2 - q0q3) + 2.0f * d.mz * (q1q3 + q0q2);
  hy = 2.0f * d.mx * (q1q2 + q0q3) + 2.0f * d.my * (0.5f - q1q1 - q3q3) + 2.0f * d.mz * (q2q3 - q0q1);
  bx = sqrtf((hx * hx) + (hy * hy));
  bz = 2.0f * d.mx * (q1q3 - q0q2) + 2.0f * d.my * (q2q3 + q0q1) + 2.0f * d.mz * (0.5f - q1q1 - q2q2);

  // Estimated direction of gravity and magnetic field
  vx = 2.0f * (q1q3 - q0q2);
  vy = 2.0f * (q0q1 + q2q3);
  vz = q0q0 - q1q1 - q2q2 + q3q3;
  wx = 2.0f * bx * (0.5f - q2q2 - q3q3) + 2.0f * bz * (q1q3 - q0q2);
  wy = 2.0f * bx * (q1q2 - q0q3) + 2.0f * bz * (q0q1 + q2q3);
  wz = 2.0f * bx * (q0q2 + q1q3) + 2.0f * bz * (0.5f - q1q1 - q2q2);  

  // Error is cross product between estimated direction and measured direction of gravity
  ex = (d.ay * vz - d.az * vy) + (d.my * wz - d.mz * wy);
  ey = (d.az * vx - d.ax * vz) + (d.mz * wx - d.mx * wz);
  ez = (d.ax * vy - d.ay * vx) + (d.mx * wy - d.my * wx);
  if (_Ki > 0.0f) {
    _eInt[0] += ex;      // accumulate integral error
    _eInt[1] += ey;
    _eInt[2] += ez;
  }
  else {
    _eInt[0] = 0.0f;     // prevent integral wind up
    _eInt[1] = 0.0f;
    _eInt[2] = 0.0f;
  }

  // Apply feedback terms
  d.gx = d.gx + _Kp * ex + _Ki * _eInt[0];
  d.gy = d.gy + _Kp * ey + _Ki * _eInt[1];
  d.gz = d.gz + _Kp * ez + _Ki * _eInt[2];

  // Integrate rate of change of quaternion
  pa = _q.q1;
  pb = _q.q2;
  pc = _q.q3;
  _q.q0 = _q.q0 + (-_q.q1 * d.gx - _q.q2 * d.gy - _q.q3 * d.gz) * (0.5f * deltaT);
  _q.q1 = pa + (_q.q0 * d.gx + pb * d.gz - pc * d.gy) * (0.5f * deltaT);
  _q.q2 = pb + (_q.q0 * d.gy - pa * d.gz + pc * d.gx) * (0.5f * deltaT);
  _q.q3 = pc + (_q.q0 * d.gz + pa * d.gy - pb * d.gx) * (0.5f * deltaT);

  // Normalise quaternion
  norm = sqrtf(_q.q0 * _q.q0 + _q.q1 * _q.q1 + _q.q2 * _q.q2 + _q.q3 * _q.q3);
  norm = 1.0f / norm;
  _q.q0 = _q.q0 * norm;
  _q.q1 = _q.q1 * norm;
  _q.q2 = _q.q2 * norm;
  _q.q3 = _q.q3 * norm;
}
