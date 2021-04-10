/******************************************************************************
SparkFunMPU9250-DMP.cpp - MPU-9250 Digital Motion Processor Arduino Library 
Jim Lindblom @ SparkFun Electronics
original creation date: November 23, 2016
https://github.com/sparkfun/SparkFun_MPU9250_DMP_Arduino_Library

This library implements motion processing functions of Invensense's MPU-9250.
It is based on their Emedded MotionDriver 6.12 library.
	https://www.invensense.com/developers/software-downloads/

Development environment specifics:
Arduino IDE 1.6.12
SparkFun 9DoF Razor IMU M0

Supported Platforms:
- ATSAMD21 (Arduino Zero, SparkFun SAMD21 Breakouts)
******************************************************************************/
#include "SparkFunMPU9250-DMP.h"
#include "MPU9250_RegisterMap.h"
#define PI 3.1415926f
#include <math.h>

//extern "C" {
#include "util/inv_mpu.h"
//}

const signed char defaultOrientation[9] = {
	1, 0, 0,
	0, 1, 0,
	0, 0, 1
};

int ax, ay, az;
int gx, gy, gz;
int mx, my, mz;
long qw, qx, qy, qz;
long temperature;
unsigned long time;
float pitch, roll, yaw;
float heading;

unsigned short _aSense = 0.0f;
float _gSense = 0.0f;
float _mSense = 6.665f;

static unsigned char mpu9250_orientation;
static unsigned char tap_count;
static unsigned char tap_direction;
static bool _tap_available;
static void orient_cb(unsigned char orient);
static void tap_cb(unsigned char direction, unsigned char count);

inv_error_t MPU9250_DMP_begin(void)
{
	inv_error_t result;
    struct int_param_s int_param;
	
	result = mpu_init(&int_param);
	
	if (result)
		return result;
	
	mpu_set_bypass(1); // Place all slaves (including compass) on primary bus
	
	MPU9250_DMP_setSensors(INV_XYZ_GYRO | INV_XYZ_ACCEL | INV_XYZ_COMPASS);
	
	_gSense = MPU9250_DMP_getGyroSens();
	_aSense = MPU9250_DMP_getAccelSens();
	
	return result;
}

inv_error_t MPU9250_DMP_enableInterrupt(unsigned char enable)
{
	return set_int_enable(enable);
}

inv_error_t MPU9250_DMP_setIntLevel(unsigned char active_low)
{
	return mpu_set_int_level(active_low);
}

inv_error_t MPU9250_DMP_setIntLatched(unsigned char enable)
{
	return mpu_set_int_latched(enable);
}

short MPU9250_DMP_getIntStatus(void)
{
	short status;
	if (mpu_get_int_status(&status) == INV_SUCCESS)
	{
		return status;
	}
	return 0;
}

// Accelerometer Low-Power Mode. Rate options:
// 1.25 (1), 2.5 (2), 5, 10, 20, 40, 
// 80, 160, 320, or 640 Hz
// Disables compass and gyro
inv_error_t MPU9250_DMP_lowPowerAccel(unsigned short rate)
{
	return mpu_lp_accel_mode(rate);
}

inv_error_t MPU9250_DMP_setGyroFSR(unsigned short fsr)
{
	inv_error_t err;
	err = mpu_set_gyro_fsr(fsr);
	if (err == INV_SUCCESS)
	{
		_gSense = MPU9250_DMP_getGyroSens();
	}
	return err;
}

inv_error_t MPU9250_DMP_setAccelFSR(unsigned char fsr)
{
	inv_error_t err;
	err = mpu_set_accel_fsr(fsr);
	if (err == INV_SUCCESS)
	{
		_aSense = MPU9250_DMP_getAccelSens();
	}
	return err;
}

unsigned short MPU9250_DMP_getGyroFSR(void)
{
	unsigned short tmp;
	if (mpu_get_gyro_fsr(&tmp) == INV_SUCCESS)
	{
		return tmp;
	}
	return 0;
}

unsigned char MPU9250_DMP_getAccelFSR(void)
{
	unsigned char tmp;
	if (mpu_get_accel_fsr(&tmp) == INV_SUCCESS)
	{
		return tmp;
	}
	return 0;	
}

unsigned short MPU9250_DMP_getMagFSR(void)
{
	unsigned short tmp;
	if (mpu_get_compass_fsr(&tmp) == INV_SUCCESS)
	{
		return tmp;
	}
	return 0;
}

inv_error_t MPU9250_DMP_setLPF(unsigned short lpf)
{
	return mpu_set_lpf(lpf);
}

unsigned short MPU9250_DMP_getLPF(void)
{
	unsigned short tmp;
	if (mpu_get_lpf(&tmp) == INV_SUCCESS)
	{
		return tmp;
	}
	return 0;
}

inv_error_t MPU9250_DMP_setSampleRate(unsigned short rate)
{
    return mpu_set_sample_rate(rate);
}

unsigned short MPU9250_DMP_getSampleRate(void)
{
	unsigned short tmp;
	if (mpu_get_sample_rate(&tmp) == INV_SUCCESS)
	{
		return tmp;
	}
	return 0;
}

inv_error_t MPU9250_DMP_setCompassSampleRate(unsigned short rate)
{
	return mpu_set_compass_sample_rate(rate);
}

unsigned short MPU9250_DMP_getCompassSampleRate(void)
{
	unsigned short tmp;
	if (mpu_get_compass_sample_rate(&tmp) == INV_SUCCESS)
	{
		return tmp;
	}
	
	return 0;
}

float MPU9250_DMP_getGyroSens(void)
{
	float sens;
	if (mpu_get_gyro_sens(&sens) == INV_SUCCESS)
	{
		return sens;
	}
	return 0;
}
	
unsigned short MPU9250_DMP_getAccelSens(void)
{
	unsigned short sens;
	if (mpu_get_accel_sens(&sens) == INV_SUCCESS)
	{
		return sens;
	}
	return 0;
}

float MPU9250_DMP_getMagSens(void)
{
	return 0.15; // Static, 4915/32760
}

unsigned char MPU9250_DMP_getFifoConfig(void)
{
	unsigned char sensors;
	if (mpu_get_fifo_config(&sensors) == INV_SUCCESS)
	{
		return sensors;
	}
	return 0;
}

inv_error_t MPU9250_DMP_configureFifo(unsigned char sensors)
{
	return mpu_configure_fifo(sensors);
}

inv_error_t MPU9250_DMP_resetFifo(void)
{
	return mpu_reset_fifo();
}

unsigned short MPU9250_DMP_fifoAvailable(void)
{
	unsigned char fifoH, fifoL;
	
	if (mpu_read_reg(MPU9250_FIFO_COUNTH, &fifoH) != INV_SUCCESS)
		return 0;
	if (mpu_read_reg(MPU9250_FIFO_COUNTL, &fifoL) != INV_SUCCESS)
		return 0;
	
	return (fifoH << 8 ) | fifoL;
}

inv_error_t MPU9250_DMP_updateFifo(void)
{
	short gyro[3], accel[3];
	unsigned long timestamp;
	unsigned char sensors, more;
	
	if (mpu_read_fifo(gyro, accel, &timestamp, &sensors, &more) != INV_SUCCESS)
		return INV_ERROR;
	
	if (sensors & INV_XYZ_ACCEL)
	{
		ax = accel[X_AXIS];
		ay = accel[Y_AXIS];
		az = accel[Z_AXIS];
	}
	if (sensors & INV_X_GYRO)
		gx = gyro[X_AXIS];
	if (sensors & INV_Y_GYRO)
		gy = gyro[Y_AXIS];
	if (sensors & INV_Z_GYRO)
		gz = gyro[Z_AXIS];
	
	time = timestamp;
	
	return INV_SUCCESS;
}

inv_error_t MPU9250_DMP_setSensors(unsigned char sensors)
{
	return mpu_set_sensors(sensors);
}

bool MPU9250_DMP_dataReady()
{
	unsigned char intStatusReg;
	
	if (mpu_read_reg(MPU9250_INT_STATUS, &intStatusReg) == INV_SUCCESS)
	{
		return (intStatusReg & (1<<INT_STATUS_RAW_DATA_RDY_INT));
	}
	return false;
}

inv_error_t MPU9250_DMP_update(unsigned char sensors)
{
	inv_error_t aErr = INV_SUCCESS;
	inv_error_t gErr = INV_SUCCESS;
	inv_error_t mErr = INV_SUCCESS;
	inv_error_t tErr = INV_SUCCESS;
	
	if (sensors & UPDATE_ACCEL)
		aErr = MPU9250_DMP_updateAccel();
	if (sensors & UPDATE_GYRO)
		gErr = MPU9250_DMP_updateGyro();
	if (sensors & UPDATE_COMPASS)
		mErr = MPU9250_DMP_updateCompass();
	if (sensors & UPDATE_TEMP)
		tErr = MPU9250_DMP_updateTemperature();
	
	return aErr | gErr | mErr | tErr;
}

int MPU9250_DMP_updateAccel(void)
{
	short data[3];
	
	if (mpu_get_accel_reg(data, &time))
	{
		return INV_ERROR;		
	}
	ax = data[X_AXIS];
	ay = data[Y_AXIS];
	az = data[Z_AXIS];
	return INV_SUCCESS;
}

int MPU9250_DMP_updateGyro(void)
{
	short data[3];
	
	if (mpu_get_gyro_reg(data, &time))
	{
		return INV_ERROR;		
	}
	gx = data[X_AXIS];
	gy = data[Y_AXIS];
	gz = data[Z_AXIS];
	return INV_SUCCESS;
}

int MPU9250_DMP_updateCompass(void)
{
	short data[3];
	
	if (mpu_get_compass_reg(data, &time))
	{
		return INV_ERROR;		
	}
	mx = data[X_AXIS];
	my = data[Y_AXIS];
	mz = data[Z_AXIS];
	return INV_SUCCESS;
}

inv_error_t MPU9250_DMP_updateTemperature(void)
{
	return mpu_get_temperature(&temperature, &time);
}

int MPU9250_DMP_selfTest(unsigned char debug)
{
	long gyro[3], accel[3];
	return mpu_run_self_test(gyro, accel);
}

inv_error_t MPU9250_DMP_dmpBegin(unsigned short features, unsigned short fifoRate)
{
	unsigned short feat = features;
	unsigned short rate = fifoRate;

	if (MPU9250_DMP_dmpLoad() != INV_SUCCESS)
		return INV_ERROR;
	
	// 3-axis and 6-axis LP quat are mutually exclusive.
	// If both are selected, default to 3-axis
	if (feat & DMP_FEATURE_LP_QUAT)
	{
		feat &= ~(DMP_FEATURE_6X_LP_QUAT);
		dmp_enable_lp_quat(1);
	}
	else if (feat & DMP_FEATURE_6X_LP_QUAT)
		dmp_enable_6x_lp_quat(1);
	
	if (feat & DMP_FEATURE_GYRO_CAL)
		dmp_enable_gyro_cal(1);
	
	if (MPU9250_DMP_dmpEnableFeatures(feat) != INV_SUCCESS)
		return INV_ERROR;
	
	//rate = constrain(rate, 1, 200);
	if (MPU9250_DMP_dmpSetFifoRate(rate) != INV_SUCCESS)
		return INV_ERROR;
	
	return mpu_set_dmp_state(1);
}

inv_error_t MPU9250_DMP_dmpLoad(void)
{
	return dmp_load_motion_driver_firmware();
}

unsigned short MPU9250_DMP_dmpGetFifoRate(void)
{
	unsigned short rate;
	if (dmp_get_fifo_rate(&rate) == INV_SUCCESS)
		return rate;
	
	return 0;
}

inv_error_t MPU9250_DMP_dmpSetFifoRate(unsigned short rate)
{
	if (rate > MAX_DMP_SAMPLE_RATE) rate = MAX_DMP_SAMPLE_RATE;
	return dmp_set_fifo_rate(rate);
}

inv_error_t MPU9250_DMP_dmpUpdateFifo(void)
{
	short gyro[3];
	short accel[3];
	long quat[4];
	unsigned long timestamp;
	short sensors;
	unsigned char more;
	
	if (dmp_read_fifo(gyro, accel, quat, &timestamp, &sensors, &more)
		   != INV_SUCCESS)
    {
	   return INV_ERROR;
    }
	
	if (sensors & INV_XYZ_ACCEL)
	{
		ax = accel[X_AXIS];
		ay = accel[Y_AXIS];
		az = accel[Z_AXIS];
	}
	if (sensors & INV_X_GYRO)
		gx = gyro[X_AXIS];
	if (sensors & INV_Y_GYRO)
		gy = gyro[Y_AXIS];
	if (sensors & INV_Z_GYRO)
		gz = gyro[Z_AXIS];
	if (sensors & INV_WXYZ_QUAT)
	{
		qw = quat[0];
		qx = quat[1];
		qy = quat[2];
		qz = quat[3];
	}
	
	time = timestamp;
	
	return INV_SUCCESS;
}

inv_error_t MPU9250_DMP_dmpEnableFeatures(unsigned short mask)
{
	unsigned short enMask = 0;
	enMask |= mask;
	// Combat known issue where fifo sample rate is incorrect
	// unless tap is enabled in the DMP.
	enMask |= DMP_FEATURE_TAP; 
	return dmp_enable_feature(enMask);
}

unsigned short MPU9250_DMP_dmpGetEnabledFeatures(void)
{
	unsigned short mask;
	if (dmp_get_enabled_features(&mask) == INV_SUCCESS)
		return mask;
	return 0;
}

inv_error_t MPU9250_DMP_dmpSetTap(
        unsigned short xThresh, unsigned short yThresh, unsigned short zThresh,
        unsigned char taps, unsigned short tapTime, unsigned short tapMulti)
{
	unsigned char axes = 0;
	if (xThresh > 0)
	{
		axes |= TAP_X;
		//xThresh = constrain(xThresh, 1, 1600);
		if (dmp_set_tap_thresh(1<<X_AXIS, xThresh) != INV_SUCCESS)
			return INV_ERROR;
	}
	if (yThresh > 0)
	{
		axes |= TAP_Y;
		//yThresh = constrain(yThresh, 1, 1600);
		if (dmp_set_tap_thresh(1<<Y_AXIS, yThresh) != INV_SUCCESS)
			return INV_ERROR;
	}
	if (zThresh > 0)
	{
		axes |= TAP_Z;
		//zThresh = constrain(zThresh, 1, 1600);
		if (dmp_set_tap_thresh(1<<Z_AXIS, zThresh) != INV_SUCCESS)
			return INV_ERROR;
	}
	if (dmp_set_tap_axes(axes) != INV_SUCCESS)
		return INV_ERROR;
	if (dmp_set_tap_count(taps) != INV_SUCCESS)
		return INV_ERROR;
	if (dmp_set_tap_time(tapTime) != INV_SUCCESS)
		return INV_ERROR;
	if (dmp_set_tap_time_multi(tapMulti) != INV_SUCCESS)
		return INV_ERROR;
	
    dmp_register_tap_cb(tap_cb);
	
	return INV_SUCCESS;
}

unsigned char MPU9250_DMP_getTapDir(void)
{
	_tap_available = false;
	return tap_direction;
}

unsigned char MPU9250_DMP_getTapCount(void)
{
	_tap_available = false;
	return tap_count;
}

bool MPU9250_DMP_tapAvailable(void)
{
	return _tap_available;
}

inv_error_t MPU9250_DMP_dmpSetOrientation(const signed char * orientationMatrix)
{
	unsigned short scalar;
	scalar = MPU9250_DMP_orientation_row_2_scale(orientationMatrix);
	scalar |= MPU9250_DMP_orientation_row_2_scale(orientationMatrix + 3) << 3;
	scalar |= MPU9250_DMP_orientation_row_2_scale(orientationMatrix + 6) << 6;
	
    dmp_register_android_orient_cb(orient_cb);
	
	return dmp_set_orientation(scalar);
}

unsigned char MPU9250_DMP_dmpGetOrientation(void)
{
	return mpu9250_orientation;
}

inv_error_t MPU9250_DMP_dmpEnable3Quat(void)
{
	unsigned short dmpFeatures;
	
	// 3-axis and 6-axis quat are mutually exclusive
	dmpFeatures = MPU9250_DMP_dmpGetEnabledFeatures();
	dmpFeatures &= ~(DMP_FEATURE_6X_LP_QUAT);
	dmpFeatures |= DMP_FEATURE_LP_QUAT;
	
	if (MPU9250_DMP_dmpEnableFeatures(dmpFeatures) != INV_SUCCESS)
		return INV_ERROR;
	
	return dmp_enable_lp_quat(1);
}
	
unsigned long MPU9250_DMP_dmpGetPedometerSteps(void)
{
	unsigned long steps;
	if (dmp_get_pedometer_step_count(&steps) == INV_SUCCESS)
	{
		return steps;
	}
	return 0;
}

inv_error_t MPU9250_DMP_dmpSetPedometerSteps(unsigned long steps)
{
	return dmp_set_pedometer_step_count(steps);
}

unsigned long MPU9250_DMP_dmpGetPedometerTime(void)
{
	unsigned long walkTime;
	if (dmp_get_pedometer_walk_time(&walkTime) == INV_SUCCESS)
	{
		return walkTime;
	}
	return 0;
}

inv_error_t MPU9250_DMP_dmpSetPedometerTime(unsigned long time)
{
	return dmp_set_pedometer_walk_time(time);
}

float MPU9250_DMP_calcAccel(int axis)
{
	return (float) axis / (float) _aSense;
}

float MPU9250_DMP_calcGyro(int axis)
{
	return (float) axis / (float) _gSense;
}

float MPU9250_DMP_calcMag(int axis)
{
	return (float) axis / (float) _mSense;
}

float MPU9250_DMP_calcQuat(long axis)
{
	return MPU9250_DMP_qToFloat(axis, 30);
}
	
float MPU9250_DMP_qToFloat(long number, unsigned char q)
{
	unsigned long mask = 0;
	for (int i=0; i<q; i++)
	{
		mask |= (1<<i);
	}
	return (number >> q) + ((number & mask) / (float) (2<<(q-1)));
}

void MPU9250_DMP_computeEulerAngles(bool degrees)
{
    float dqw = MPU9250_DMP_qToFloat(qw, 30);
    float dqx = MPU9250_DMP_qToFloat(qx, 30);
    float dqy = MPU9250_DMP_qToFloat(qy, 30);
    float dqz = MPU9250_DMP_qToFloat(qz, 30);
    
    float ysqr = dqy * dqy;
    float t0 = -2.0f * (ysqr + dqz * dqz) + 1.0f;
    float t1 = +2.0f * (dqx * dqy - dqw * dqz);
    float t2 = -2.0f * (dqx * dqz + dqw * dqy);
    float t3 = +2.0f * (dqy * dqz - dqw * dqx);
    float t4 = -2.0f * (dqx * dqx + ysqr) + 1.0f;
  
	// Keep t2 within range of asin (-1, 1)
    t2 = t2 > 1.0f ? 1.0f : t2;
    t2 = t2 < -1.0f ? -1.0f : t2;
  
    pitch = asin(t2) * 2;
    roll = atan2(t3, t4);
    yaw = atan2(t1, t0);
	
	if (degrees)
	{
		pitch *= (180.0f / PI);
		roll *= (180.0f / PI);
		yaw *= (180.0f / PI);
		if (pitch < 0) pitch = 360.0f + pitch;
		if (roll < 0) roll = 360.0f + roll;
		if (yaw < 0) yaw = 360.0f + yaw;	
	}
}

float MPU9250_DMP_computeCompassHeading(void)
{
	if (my == 0)
		heading = (mx < 0) ? PI : 0;
	else
		heading = atan2(mx, my);
	
	if (heading > PI) heading -= (2 * PI);
	else if (heading < -PI) heading += (2 * PI);
	else if (heading < 0) heading += 2 * PI;
	
	heading*= 180.0f / PI;
	
	return heading;
}

unsigned short MPU9250_DMP_orientation_row_2_scale(const signed char *row)
{
    unsigned short b;

    if (row[0] > 0)
        b = 0;
    else if (row[0] < 0)
        b = 4;
    else if (row[1] > 0)
        b = 1;
    else if (row[1] < 0)
        b = 5;
    else if (row[2] > 0)
        b = 2;
    else if (row[2] < 0)
        b = 6;
    else
        b = 7;		// error
    return b;
}
		
static void tap_cb(unsigned char direction, unsigned char count)
{
	_tap_available = true;
	tap_count = count;
	tap_direction = direction;
}

static void orient_cb(unsigned char orient)
{
	mpu9250_orientation = orient;
}