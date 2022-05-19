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
int temperature, humidity, pressure;

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
#include "./tcpsetup.c"

#define BMP280_I2C_ADDR       0x76
#include "./bmp280.h"
#include "./dht.c"
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
struct ClientData client[16] = {0};;
int rate=5, num, numremotes = 0, regnum;
char temp[25];
char * token[20];
int hostreq(char *rx_buffer, char *tx_buffer)
{
   //printf("rx_buffer --> %s\n", rx_buffer);
   sprintf(tx_buffer, "\n");
   int toknum = 0;
   token[0] = strtok(rx_buffer, ",");
   while((token[toknum] != NULL)) {
      //printf("%d %s\n", toknum, token[toknum]);
      ++toknum;
      token[toknum] = strtok(NULL, ",");
   }
   if(toknum !=  3) return(0);
   num = atoi(token[0]); 
   strcpy(client[num].name, token[1]);
   printf("name	 %s  numremotews %d\n", client[0].name, numremotes);
   //rate = atoi(token[2]);
   sprintf(tx_buffer, "%d, %s,%d,%d,%d,", numremotes, client[0].name, client[0].humid, client[0].temp, pressure);
   for(int a=1; a<= numremotes; a++) {
      sprintf(temp, " %s,%d,%d,", client[a].name, client[a].humid, client[a].temp);
      strcat(tx_buffer, temp);
   }
   strcat(tx_buffer, "\n");
   tx_buffer[strlen(tx_buffer)] = '\0';
   //printf("tx_buffer %s\n", tx_buffer);
   return(0);
}

int clientreq(char *rx_buffer, char *tx_buffer)
{
   int tempanum, tempip;
   //printf("rx_buffer --> %s\n", rx_buffer);
   sprintf(tx_buffer, "\n");
   int toknum = 0;
   token[0] = strtok(rx_buffer, ",");
   while((token[toknum] != NULL)) {
      //printf("%d %s\n", toknum, token[toknum]);
      ++toknum;
      token[toknum] = strtok(NULL, ",");
   }
   if(toknum !=  5) return(0);

   tempanum = atoi(token[0]); 
   //tempanum = atoi(token[0]+7); 
   sscanf(token[1], "192.168.0.%d", &tempip);
   //printf("tempanum= %d tempip= %d\n", tempanum, tempip);
   if(tempanum != 0) { num = tempanum; if(tempanum > numremotes) numremotes = tempanum; }
      else { int match = 0;
             for(int a = 0; a<= numremotes; a++) {
                if(tempip == client[a].ip) { num = a; match = 1; }
             }
	     if(match != 1) { ++numremotes; num = numremotes; strcpy(client[num].name, "registered"); }
 	   }
   client[num].ip = tempip;
   if(strlen(client[num].name) < 5) strcpy(client[num].name, "unreg");
   client[num].humid = atoi(token[3]);
   client[num].temp = atoi(token[4]);
   client[num].active = 100;

   for (int a = 0; a <= numremotes; a++) if(client[a].active > 0) --client[a].active;
   printf("state of clients struct\n");
   for(int a = 0; a<= numremotes; a++)
      printf("-->  client[%d]  %3d  %10s   %3d   %3d    %d\n", 
            a, client[a].ip, client[a].name, client[a].humid, client[a].temp, client[a].active);
   sprintf(tx_buffer, "%d,%s,%d\n", num, client[num].name, rate);
   tx_buffer[strlen(tx_buffer)] = '\0';
   //printf("tx_buffer %s\n\n", tx_buffer);
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
    DHT11_init(GPIO_NUM_16);
    xTaskCreatePinnedToCore (tcp_server_task, "tcp_server", 8096, NULL, 6, NULL, 0);
    while(1) {
       pressure =  bmp280_read();
       aht10_read();
       DHT11_read();
       //
       //printf("%d\n", cnt);
       //printf("pressure= %d.%02d\n ", pressure/100, pressure%100);
       //printf("hum = %02d.%d   temp = %02d.%d\n",
       //     humidity/10, humidity%10, temperature/10, temperature%10);
       client[0].humid = humidity;
       client[0].temp = temperature;
       client[0].active = 17;
       client[0].ip = 106;
       strcpy(client[0].name, "host");


       sprintf(disp_str,"|4  Pressure||||4 %4d.%02dmb", pressure/100, pressure%100);
       ssd1306_text(disp_str);

       cnt++;
       vTaskDelay(rate*1000/portTICK_RATE_MS);
    }

}
