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
#include <stdint.h>
#include "adns5050.h"
#include "pico/stdlib.h"

#define LOW false
#define HIGH true

static int ConvertToSignedNumber(uint8_t twoscomp);

//Constructor sets the pins used for the mock 'i2c' communication
void adns5050_init(adns5050_t *adns5050, int sdio, int sclk, int ncs)
{
	adns5050->_sdio = sdio;
	adns5050->_sclk = sclk;
	adns5050->_ncs = ncs;
}

//Configures the communication pins for their initial state
void adns5050_begin(adns5050_t *adns5050)
{
  gpio_init(adns5050->_sdio);
	gpio_set_dir(adns5050->_sdio, GPIO_OUT);
  gpio_init(adns5050->_sclk);
	gpio_set_dir(adns5050->_sclk, GPIO_OUT);
  gpio_init(adns5050->_ncs);
	gpio_set_dir(adns5050->_ncs, GPIO_OUT);
}

//Essentially resets communication to the ADNS5050 module
void adns5050_sync(adns5050_t *adns5050)
{
	gpio_put(adns5050->_ncs, LOW);
	sleep_us(1);
	gpio_put(adns5050->_ncs, HIGH);
}

//Reads a register from the ADNS5050 sensor. Returns the result to the calling function.
//Example: value = mouse.read(CONFIGURATION_REG);
int adns5050_read(adns5050_t *adns5050, unsigned char addr)
{
  uint8_t temp;
  int n;

  //select the chip
  gpio_put(adns5050->_ncs, LOW);	//nADNSCS = 0;
  temp = addr;

  //start clock low
  gpio_put(adns5050->_sclk, LOW); //SCK = 0;

	//set data line for GPIO_OUT
  gpio_set_dir(adns5050->_sdio, GPIO_OUT); //DATA_OUT;
  //read 8bit data
  for (n=0; n<8; n++) {
    //sleep_us(2);
    gpio_put(adns5050->_sclk, LOW);//SCK = 0;
    //sleep_us(2);
    gpio_set_dir(adns5050->_sdio, GPIO_OUT); //DATA_OUT;
    if (temp & 0x80) {
      gpio_put(adns5050->_sdio, HIGH);//SDOUT = 1;
    }
    else {
      gpio_put(adns5050->_sdio, LOW);//SDOUT = 0;
    }
    sleep_us(2);
    temp = (temp << 1);
    gpio_put(adns5050->_sclk, HIGH); //SCK = 1;
  }

  // This is a read, switch to GPIO_IN
  temp = 0;
  gpio_set_dir(adns5050->_sdio, GPIO_IN); //DATA_IN;
  //read 8bit data
	for (n=0; n<8; n++) {		// read back the data
    sleep_us(1);
    gpio_put(adns5050->_sclk, LOW);
    sleep_us(1);
    if(gpio_get(adns5050->_sdio)) {
      temp |= 0x1;
    }
    if( n != 7) temp = (temp << 1); // shift left
    gpio_put(adns5050->_sclk, HIGH);
  }
  sleep_us(20);
  gpio_put(adns5050->_ncs, HIGH);// de-select the chip
  return ConvertToSignedNumber(temp);
}

int ConvertToSignedNumber(uint8_t twoscomp)
{
  int value;

	if (twoscomp & (1 << 7)) {
    value = -128 + (twoscomp & 0b01111111 );
  }
  else{
    value = twoscomp;
  }
	return value;
}

//Writes a value to a register on the ADNS2620.
//Example: mouse.write(CONFIGURATION_REG, 0x01);
void adns5050_write(adns5050_t *adns5050, unsigned char addr, unsigned char data)
{
  char temp;
  int n;

  //select the chip
  //nADNSCS = 0;
  gpio_put(adns5050->_ncs, LOW);

  temp = addr;
  //クロックを開始
  gpio_put(adns5050->_sclk, LOW);//SCK = 0;					// start clock low
  //SDIOピンを出力にセット
  gpio_set_dir(adns5050->_sdio, GPIO_OUT);//DATA_OUT; // set data line for GPIO_OUT
  //8ビットコマンドの送信
  for (n=0; n<8; n++) {
    gpio_put(adns5050->_sclk, LOW);//SCK = 0;
    gpio_set_dir(adns5050->_sdio, GPIO_OUT);
    sleep_us(1);
    if (temp & 0x80)  //0x80 = 0101 0000
      gpio_put(adns5050->_sdio, HIGH);//SDOUT = 1;
    else
      gpio_put(adns5050->_sdio, LOW);//SDOUT = 0;
    temp = (temp << 1);
    gpio_put(adns5050->_sdio, HIGH);//SCK = 1;
    sleep_us(1);//sleep_us(1);			// short clock pulse
  }
	temp = data;
  for (n=0; n<8; n++) {
    sleep_us(1);
    gpio_put(adns5050->_sclk, LOW);//SCK = 0;
    sleep_us(1);
    if (temp & 0x80)
      gpio_put(adns5050->_sdio, HIGH);//SDOUT = 1;
    else
      gpio_put(adns5050->_sdio, LOW);//SDOUT = 0;
    temp = (temp << 1);
    gpio_put(adns5050->_sclk, HIGH);//SCK = 1;
    sleep_us(1);			// short clock pulse
  }
  sleep_us(20);
  gpio_put(adns5050->_ncs, HIGH);//nADNSCS = 1; // de-select the chip
}
