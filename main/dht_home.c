#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "driver/i2c.h"
#include "sdkconfig.h"
//globals
//int temperature, humidity;
int pressure;

#define i2c_port                 0
#define i2c_frequency       800000
#define i2c_gpio_sda            22
#define i2c_gpio_scl            21
#include "./interfaces/i2c.c"

#define EXAMPLE_WIFI_SSID "troutstream"
#define EXAMPLE_WIFI_PASS "password"
#define PORT 80
//#include "./interfaces/wifisetup.c"

void trfData();
char outstr[4096];
char rx_buffer[1024];
#include "./functionc/tcpsetup.c"

#define BMP280_I2C_ADDR       0x76
#include "./functionc/bmp280.h"
float tempt, hum;
#include "./functionc/ath10.c"

#define I2C_ADDR_SSD1306       0x3c
#include "./functionc/ssd1306.c"

struct ClientData 
   {  
      int ip;
      int active;
      char name[16];
      int humid;
      int temp;
};
struct ClientData client[256] = {0};
int rate, num, numremotes = 0, regnum;
char temp[128];
char * token[20];

//handles client requests from outside data collection network
int hostreq(char *rx_buffer, char *tx_buffer)
{
   //printf("rx_buffer --> %s\n", rx_buffer);
   sprintf(tx_buffer, "\n");
   int toknum = 0;
   //split up incomming packet 
   token[0] = strtok(rx_buffer, ",");
   while((token[toknum] != NULL)) {
      //printf("%d %s\n", toknum, token[toknum]);
      ++toknum;
      token[toknum] = strtok(NULL, ",");
   }
   if(toknum !=  3) return(0);
   num = atoi(token[0]); 
   strcpy(client[num].name, token[1]);
   rate = atoi(token[2]);
   sprintf(tx_buffer, "%d, %s,%d,%d,%d,", numremotes, client[0].name, client[0].humid, client[0].temp, pressure);
   for(int a=1; a<= 255; a++) {
      if(client[a].ip > 0) {
         sprintf(temp, "%d, %s,%d,%d,", client[a].ip, client[a].name, client[a].humid, client[a].temp);
         strcat(tx_buffer, temp);
      }
   }
   strcat(tx_buffer, "\n");
   tx_buffer[strlen(tx_buffer)] = '\0';
   //printf("tx_buffer %s\n", tx_buffer);
   return(0);
}

//handels data from remote clients with sensor data
int clientreq(char *rx_buffer, char *tx_buffer)
{
   //printf("rx_buffer --> %s\n", rx_buffer);
   sprintf(tx_buffer, "\n");
   int toknum = 0;
   //split up incomming packet
   token[0] = strtok(rx_buffer, ",");
   while((token[toknum] != NULL)) {
      //printf("%d %s\n", toknum, token[toknum]);
      ++toknum;
      token[toknum] = strtok(NULL, ",");
   }
   if(toknum !=  5) return(0);

   int num, tempnum, tempip;
   tempnum = atoi(token[0]);
   sscanf(token[1], "192.168.0.%d", &tempip);
   client[tempip].ip = tempip;
   if(strlen(client[tempip].name) < 4) strcpy(client[tempip].name, token[2]);
   client[tempip].humid = atoi(token[3]);
   client[tempip].temp = atoi(token[4]);
   client[tempip].active = 30;
   printf("Received num=%d  %d %s %d %d\n", tempnum,tempip,client[tempip].name,atoi(token[3]), atoi(token[4]) );

   //decrement activity for each registered channel - unregister offline remotes
   for (int a = 0; a <= 255; a++)  if(client[a].active > 0)  --client[a].active; 

   printf("state of clients struct\n");
   numremotes = 0;
   for(int a = 0; a <= 255; a++){
      if(client[a].active > 0) { ++numremotes;
         printf("-->  client[%03d]  %3d  %16s   %3d   %3d    %d\n", 
            a, client[a].ip, client[a].name, client[a].humid, client[a].temp, client[a].active);
         if(client[a].active < 10) { strcpy(client[a].name, "offline"); client[a].humid=0; client[a].temp= -18; }
      }
   }
   --numremotes;

   //packet back to remote client
   sprintf(tx_buffer, "%d,%s,%d\n", tempip, client[tempip].name, rate);
   printf("tx_buffer %s\n\n", tx_buffer);
   tx_buffer[strlen(tx_buffer)] = '\0';
   return(0);
}

int cnt = 0;
void app_main(void)
{
    nvs_flash_init();
    initialize_wifi();  //in wifisetup.h
    wait_for_ip();      //in wifisetup.h
    i2c_init();         //setup and detect devices on i2c interface
    i2cdetect();
    bmp280_cal();       //air pressure altitude measurements
    ssd1306_init();   //setup oled display
    ssd1306_blank(0x00);
    char disp_str[128] = "4 Hi There";
    ssd1306_text(disp_str);
    strcpy(client[0].name, "host");
    rate = 5;
    //DHT11_init(GPIO_NUM_16);
    xTaskCreatePinnedToCore (tcp_server_task, "tcp_server", 8096, NULL, 6, NULL, 0);
    while(1) {
       pressure =  bmp280_read();
       aht10_read();
       //DHT11_read();
       //
       //printf("%d\n", cnt);
       //printf("pressure= %d.%02d\n ", pressure/100, pressure%100);
       //printf("hum = %02d.%d   temp = %02d.%d\n",
       //     humidity/10, humidity%10, temperature/10, temperature%10);
       client[0].humid = (int)10*hum;
       client[0].temp = (int)10*tempt;
       client[0].active = 30;
       client[0].ip = 106;


       sprintf(disp_str,"1Pressure %7.1fmb|1Hum %4.1f%% Temp %4.1fF|", (float)pressure/100,hum, 32+1.8*tempt);
       for(int a=1; a<= 255; a++) if(client[a].active>0) {
         sprintf(temp,"%9s %4.1f%%%5.1fF|", client[a].name, (float)client[a].humid/10, 32+.18*(float)client[a].temp);
         strcat(disp_str, temp); }
       ssd1306_text(disp_str);

       cnt++;
       vTaskDelay(5000/portTICK_RATE_MS);
    }

}
