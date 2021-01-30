#include <WiFi.h>
#include "ESPino32CAM.h"
#include "ESPino32CAM_QRCode.h"
#include "esp_camera.h"
ESPino32CAM cam;
ESPino32QRCode qr;

#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define BUTTON_QR 0

const char* ssid     = "Tenda_7E4F40";
const char* password = "0677265098";
const char* host = "localhost";

void setup()
{
  //pinMode(4,OUTPUT);
  //digitalWrite(4,HIGH);
  
    Serial.begin(115200);
    Serial.println("\r\nESPino32CAM");
    if (cam.init() != ESP_OK)
    {
      Serial.println(F("Fail"));
      while (1);
    } 

    camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  


  

 

  sensor_t * s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);//flip it back
    s->set_brightness(s, 1);//up the blightness just a bit
    s->set_saturation(s, -2);//lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);  //FRAMESIZE_VGA  FRAMESIZE_CIF
  s->set_whitebal(s, true);

    
  qr.init(&cam);

    // Connect to WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}


void loop()
{
    unsigned long pv_time  = millis();
    camera_fb_t *fb = cam.capture();
   
    if (!fb)
    {
      Serial.println("Camera capture failed");
      return;
    }
    
    Serial.write(fb->buf, fb->len);
    dl_matrix3du_t *image_rgb;
    if(cam.jpg2rgb(fb,&image_rgb))
    {
       cam.clearMemory(fb);
       if(!digitalRead(BUTTON_QR))
       {
         cam.printDebug("\r\nQR Read:");
         qrResoult barcode = qr.recognition(image_rgb);
         if(barcode.status)
         {
            cam.printDebug("");
            cam.printDebug("Data type: "+ qr.dataType(barcode.dataType));
            cam.printfDebug("Length: %d",barcode.length);
            cam.printDebug("Payload: "+barcode.payload);
         }
         else
            cam.printDebug("FAIL");
         

         
       /*  qrResoult res = qr.recognition(image_rgb);
         if(res.status)
         {
            cam.printDebug("");
            cam.printfDebug("Version: %d", res.version);
            cam.printfDebug("ECC level: %c",res.eccLevel);
            cam.printfDebug("Mask: %d", res.mask);
            cam.printDebug("Data type: "+ qr.dataType(res.dataType));
            cam.printfDebug("Length: %d",res.length);
            cam.printDebug("Payload: "+res.payload);
         }
         else
            cam.printDebug("FAIL"); */

            
       }  
    }
    cam.clearMemory(image_rgb);
      
     
    Serial.print("connecting to ");
    Serial.println(host);
  
    WiFiClient client;
    const int httpPort = 8889;
    if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
  }

 
    // send request to the server
 client.print(String("GET http://localhost/count/get_barcode.php?") + 
                          ("&date=") + 
                          ("&barcode=") + 
                          " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 1000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
        
    }

    Serial.println();
    Serial.println("closing connection");
}
