#include "mpu9250_porting.h"
#include "transfer_handler.h"

//#include "nrf_drv_twi.h"
//#include "nrf_delay.h"

//#if TWI0_ENABLED
//#define TWI_INSTANCE_ID     0
//#endif

///* TWI instance. */
//static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);
//static volatile bool m_xfer_done = false;
static uint8_t sendbuf[20];

//void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
//{
//    switch (p_event->type)
//    {
//        case NRF_DRV_TWI_EVT_DONE:
//            m_xfer_done = true;
//            break;
//        default:
//            break;
//    }
//}

//void twi_init (void)
//{
//    ret_code_t err_code;

//    const nrf_drv_twi_config_t twi_config = {
//       .scl                = 8,
//       .sda                = 7,
//       .frequency          = NRF_DRV_TWI_FREQ_100K,
//       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
//       .clear_bus_init     = false
//    };

//    err_code = nrf_drv_twi_init(&m_twi, &twi_config, twi_handler, NULL);
//    APP_ERROR_CHECK(err_code);

//    nrf_drv_twi_enable(&m_twi);
//}

int i2c_write(unsigned char slave_addr, unsigned char reg_addr,
                       unsigned char length, unsigned char * data)
{
		
		sendbuf[0] = reg_addr;
		memcpy(sendbuf + 1, data, length);
//	
//		m_xfer_done = false;
//    ret_code_t err_code = nrf_drv_twi_tx(&m_twi, slave_addr, sendbuf, length + 1, false);
//    APP_ERROR_CHECK(err_code);
//    while (m_xfer_done == false);
	
		iic_send(slave_addr, sendbuf, length + 1, false);
	
		return 0;
}

int i2c_read(unsigned char slave_addr, unsigned char reg_addr,
                       unsigned char length, unsigned char * data)
{

	sendbuf[0] = reg_addr;
//	m_xfer_done = false;
//  ret_code_t err_code = nrf_drv_twi_tx(&m_twi, slave_addr, sendbuf, 1, true);
//  APP_ERROR_CHECK(err_code);
//  while (m_xfer_done == false);
//	
//	m_xfer_done = false;
//  err_code = nrf_drv_twi_rx(&m_twi, slave_addr, data, length);
//  APP_ERROR_CHECK(err_code);
//  while (m_xfer_done == false);
	
	iic_send(slave_addr, sendbuf, 1, true);
	iic_read(slave_addr, data, length);
	
	return 0;
}

int get_ms(unsigned long *count)
{
	*count = 0;
	return 0;
}

int delay_ms(unsigned long num_ms)
{
	delay(num_ms);
	return 0;
}
