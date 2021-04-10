#ifndef _MPU9250_PORTING_H_
#define _MPU9250_PORTING_H_

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define log_i NRF_LOG_INFO
#define log_e NRF_LOG_ERROR

int get_ms(unsigned long *count);
int delay_ms(unsigned long num_ms);

int i2c_write(unsigned char slave_addr, unsigned char reg_addr,
                       unsigned char length, unsigned char * data);
int i2c_read(unsigned char slave_addr, unsigned char reg_addr,
                       unsigned char length, unsigned char * data);
void twi_init (void);


#endif // _MPU9250_PORTING_H_
