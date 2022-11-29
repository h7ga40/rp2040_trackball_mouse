/* 
 * MIT License
 *
 * Copyright (c) 2019 Hiroyuki Okada
 *
 * https://github.com/okhiroyuki/ADNS5050
 */
/*
 * Modify to C language from C++/Arduino
 */
#ifndef adns5050_h
#define adns5050_h

typedef struct _adns5050_t {
	int		_sdio;
	int		_sclk;
	int		_ncs;
} adns5050_t;

/* Register Map for the ADNS5050 Optical Mouse Sensor */
#define ADNS5050_PRODUCT_ID         0x00  //R			0x12
#define ADNS5050_PRODUCTID2         0x3e  //R			0x26
#define ADNS5050_REVISION_ID        0x01  //R			0x01
#define ADNS5050_DELTA_X_REG        0x03  //R			any
#define ADNS5050_DELTA_Y_REG        0x04  //R			any
#define ADNS5050_SQUAL_REG          0x05  //R			any
#define ADNS5050_MAXIMUM_PIXEL_REG  0x08  //R			any
#define ADNS5050_MINIMUM_PIXEL_REG  0x0a  //R			any
#define ADNS5050_PIXEL_SUM_REG      0x09  //R			any
#define ADNS5050_PIXEL_DATA_REG     0x0b  //R			any
#define ADNS5050_SHUTTER_UPPER_REG  0x06  //R			any
#define ADNS5050_SHUTTER_LOWER_REG  0x07  //R			any
#define ADNS5050_RESET				0x3a  //W			N/A
#define ADNS5050_MOUSE_CONTROL      0x0D  //R/W		0x00
#define ADNS5050_CPI500v			0x00
#define ADNS5050_CPI1000v			0x01

#define	NUM_PIXELS			361

#ifdef __cplusplus
extern "C"{
#endif

void adns5050_init(adns5050_t *adns5050, int sdio, int sclk, int ncs);
void adns5050_begin(adns5050_t *adns5050);
void adns5050_sync(adns5050_t *adns5050);
int adns5050_read(adns5050_t *adns5050, unsigned char address);
void adns5050_write(adns5050_t *adns5050, unsigned char address, unsigned char data);

#ifdef __cplusplus
}
#endif

#endif
