#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <SPI.h>
#include <bitBangedSPI.h>
#include <MAX7219_Dot_Matrix.h>


// Numero de modulos de display
const byte chips = 6;
unsigned long lastMoved = 0;

// Tempo entre movimentos quando a mensagem rola no display
unsigned long MOVE_INTERVAL = 30;
int  messageOffset;

// Tipo de mensagem no display, fixa ou scroll.
const byte FIXED = 0;
const byte SCROLL = 1;
int effect = FIXED;

// Chips / LOAD
MAX7219_Dot_Matrix display(chips, 2);  

const int MESSAGE_MAX_CHARS = 200;
char message[MESSAGE_MAX_CHARS] = "";

const char* host = "yt-subscriber-count.appspot.com";

// Arquivo que tem a configuração de sua rede wifi
#include "wifi_config.h"


// Envia mensagens para o display a partir da variavel message
void updateDisplay() {
  switch (effect) {
    case FIXED: {
        display.sendString(message);
        break;
      }
    case SCROLL: {
        display.sendSmooth(message, messageOffset);
        break;
      }
  }
  // next time show one pixel onwards
  if (messageOffset++ >= (int) (strlen (message) * 8)) {
    messageOffset = - chips * 8;
  }
  effect = FIXED;
}


// Ajusta a mensagem a ser exibida no display, colocando-a na variavel message
void setMessage(String msg, bool smooth) {
  msg.toCharArray(message, MESSAGE_MAX_CHARS);
  updateDisplay();
}


// Conecta ao WiFi usando as confiracoes do arquivo adicionado acima
void connectToWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  display.sendString(" WiFi ");
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); // obrigatorio
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  setMessage("WiFiOK", FIXED);
}




// Busca o numero de inscritos a partir do backend no App Engine
int getSubsCount() {
  WiFiClientSecure client;

  Serial.print("connecting to ");
  Serial.println(host);
  while (!client.connect(host, 443)) {
    Serial.println("connection failed");
    return 0;
  }

  // You can get the website certificate here: https://www.grc.com/fingerprints.htm
  const char* fingerprint = "3A 56 3F F5 57 7F 64 AA 83 DD 04 3A AD 52 BD 29 6B 9B 9E 8E";

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
    // when using google servers... it is not always an issue
    //return;
  }

  // We now create a URI for the request
  String url = "/?moduleId=MEL100k&f=yt";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.0\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");


  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  // if there are incoming bytes available
  // from the server, read them and print them:
  int i = 0;
  while (client.available()) {
    char c = client.read();
    if (i < MESSAGE_MAX_CHARS - 1) {
      message[i++] = c;
      message[i] = '\0';
    }
    Serial.write(c);
  }

  client.stop();
  Serial.write(message);

  Serial.println();
  Serial.println("closing connection");

  return 1;  
}




// Roda ao ligar, inicializando modulos e configuracoes
void setup() {
  display.begin ();
  Serial.begin(115200);
  delay(100);
  display.setIntensity(2);
  connectToWifi();
}




// Roda sequencialmente apos o setup
void loop() {
  byte serverOk;
    
  if(!getSubsCount()){
    serverOk = 0;
    if (WiFi.status() != WL_CONNECTED) {
      connectToWifi();
    }
    else{
      setMessage("No Net", FIXED);
    }
  }
  else{
    serverOk = 1;
  }

  //for (int i = 0; i < 500; i++) {
  delay(1);
  // update display if time is up
  if (millis () - lastMoved >= MOVE_INTERVAL) {
    updateDisplay();
    lastMoved = millis ();
  }
  //}

  if(serverOk){
    // delay between requests when wifi and internet is ok
    delay(60000);        
  }
  else{
    delay(2000);        
  } 
} // end of loop
