![version](https://img.shields.io/github/v/tag/Reefwing-Software/Reefwing-imuTypes) ![license](https://img.shields.io/badge/license-MIT-green) ![release](https://img.shields.io/github/release-date/Reefwing-Software/Reefwing-imuTypes?color="red") ![open source](https://badgen.net/badge/open/source/blue?icon=github)

# Reefwing IMU Types
 
 Common structs and enums used by the following Reefwing Libraries:

 - [ReefwingAHRS](https://github.com/Reefwing-Software/Reefwing-AHRS)
 - [ReefwingLSM9DS1](https://github.com/Reefwing-Software/Reefwing-LSM9DS1)
 - [ReefwingMPU6x00](https://github.com/Reefwing-Software/MPU6x00)
 - [ReefwingMPU6050](https://github.com/Reefwing-Software/MPU6050)
 - [Reefwing_xIMU3](https://github.com/Reefwing-Software)

 This common library header is to prevent duplicate definition of similar types, classes and enums. It also ensures that changes will flow through to all of the libraries.

## Structs Defined in Library

- EulerAngles
- InertialMessage
- RawData
- ScaledData
- TempData
- SensorData
- VectorData
- Ping
- NetworkAnnouncement

## Classes Defined in Library

- Quaternion
