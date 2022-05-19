int aht10_read()
{
       unsigned char ath10[16];
       float hum, tempt;

       ath10[0] = 0x33;
       ath10[1] = 0x00;
       i2c_write_block(0x38, 0xac, ath10, 2);
       vTaskDelay(20);
       i2c_read(0x38, 0x00, ath10, 7);

       hum = 100 * ((float)(ath10[1]<<12) + (ath10[2]<<4) + ((ath10[3] & 0xf0)>>4))/(1<<20);
       tempt = -50 + 200 * ((float)((ath10[3]%16)<<16) + (ath10[4]<<8) + ath10[5])/(1<<20);

       printf(" hum %5.2f%%   temp %5.2fC   %5.2fF\n", hum, tempt, 32+1.8*tempt);
       return(0);
}
