/******************************************************************
  @file       Reefwing_imuTypes.h
  @brief      Common structs, enums and classes for the Reefwing IMU 
              Libraries
  @author     David Such
  @copyright  Please see the accompanying LICENSE file

  Code:        David Such
  Version:     2.0.4
  Date:        20/11/24

  1.0.0     Original Release.                 19/04/23
  1.0.1     Minor documentation changes.      24/04/23
  2.0.0     Modified Quaternion class.        27/05/23
  2.0.1     Added I2C addresses               09/06/23
  2.0.2     Added xiao Sense support          17/06/23
  2.0.3     Vector Data Type added            02/08/23
  2.0.4     Optimised Quaternion calculations 20/11/24

  There are two conventions for quaternions, Hamilton and JPL. 
  The difference between the two conventions is the relation 
  between the three imaginary bases. In the Hamilton convention, 
  ijk = −1, while JPL defines ijk = 1. As consequences, the 
  multiplication of quaternions and the transformation between 
  quaternions and other rotation parameterizations differ with 
  the quaternion convention used.

  We use the Hamilton Convention.

  Credit - Uses a modified version of the Madgwick Quaternion Class.
           (http://www.x-io.co.uk/quaternions/)

******************************************************************/

#ifndef REEFWING_IMUTYPES_H
#define REEFWING_IMUTYPES_H

#include <Arduino.h>

/******************************************************************
 *
 * I2C IMU Device Addresses - 
 * 
 ******************************************************************/

#define LSM9DS1AG_ADDRESS 0x6B  //  Address of accelerometer & gyroscope
#define LSM9DS1M_ADDRESS  0x1E  //  Address of magnetometer 
#define HTS221_ADDRESS    0x5F  //  Nano 33 BLE Sense Rev 1 Sensor - temp/humidity
#define HS3003_ADDRESS    0x44  //  Nano 33 BLE Sense Rev 2 Sensor - temp/humidity
#define LSM6DS3_ADDRESS   0x6A  //  Seeed Studios xiao Sense gyro/accelerometer
#define MPU6000_ADDRESS   0x68  //  TDK InvenSense MPU6x00 gyro/accelerometer
#define MPU6050_ADDRESS   0x68  //  TDK InvenSense MPU6050 gyro/accelerometer

/******************************************************************
 *
 * Structs - 
 * 
 ******************************************************************/

struct EulerAngles {
  float roll;     /* rotation around x axis in degrees */
  float pitch;    /* rotation around y axis in degrees */
  float yaw;      /* rotation around z axis in degrees */
  float heading;  /* rotation relative to magnetic north */
  float rollRadians, pitchRadians, yawRadians;
  uint32_t  timeStamp;
};

struct InertialMessage {
  float ax, ay, az;
  float gx, gy, gz;
  uint32_t timeStamp;
};

struct RawData {
  int16_t rx, ry, rz;
  uint32_t  timeStamp;
};

struct ScaledData {
  float sx, sy, sz;
  uint32_t  timeStamp;
};

struct TempData {
  float celsius;
  uint32_t  timeStamp;
};

struct SensorData {
  float ax, ay, az;
  float gx, gy, gz;
  float mx, my, mz;
  uint32_t gTimeStamp, aTimeStamp, mTimeStamp;
};

struct VectorData {
  float x, y, z;
};

/******************************************************************
 *
 * Quaternion Class Definition - 
 * 
 ******************************************************************/

class Quaternion {
    public:
        Quaternion();
        Quaternion(float w, float x, float y, float z);
        Quaternion(float yaw, float pitch, float roll);

        Quaternion getConjugate(void);
        EulerAngles getEulerAngles();
        EulerAngles toEulerAngles(float declination = 0.0);

        void reset();

        float q0, q1, q2, q3;      //  Euler Parameters
        uint32_t  timeStamp;

    private:
        float radiansToDegrees(float radians);

        float eInt[3] = {0.0f, 0.0f, 0.0f};       //  Vector to hold integral error for Mahony filter
        float att[4] = {1.0f, 0.0f, 0.0f, 0.0f};  //  Attitude quaternion for complementary filter
};

#endif