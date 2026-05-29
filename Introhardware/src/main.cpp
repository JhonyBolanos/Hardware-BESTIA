#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <PubSubClient.h>
#include <time.h>

//MQT

// Configuración del servidor MQTT
const char* mqttServer = "broker.emqx.io"; //ESTE ES NUESTRO SERVER, DESDE EL CUAL VAMOS A ENVIAR MENSAJES
const int mqttPort = 1883; //ESTE ES EL DE LA WEB 
const char* clientName = "josvidavid";
const char* inboundtopic = "telematica/inbound";
const char* outboundtopic = "telematica/outbound";

// Objeto WiFiClient
WiFiClient wifiClient; //SOLICITA LA DIRECCION IP
PubSubClient mqttClient(wifiClient); // es capaz de publicar y subscribirse
bool firstMqttConection = false;

//EN ESTE BLOQUE (CALLBACK) EL MQTT RECIBE LOS MENSAJES
// SE PONE AQUI PORQUE MÁS ABAJO SE VA A UTILIZAR.

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Recibido: " + message);
  if(message == "sayhello"){
    mqttClient.publish(outboundtopic, "hello from ESP32, las bestias", 1);
  }else if (message == "uptime") {
  long time = millis();
  String response = "Tiempo encendido de las bestias: " + String(time);
  mqttClient.publish(outboundtopic, response.c_str());
  }else if(message == "mac"){
  String mac = WiFi.macAddress();
  String responsemac = "direccion mac de las bestias: " + String(mac);
  mqttClient.publish(outboundtopic, responsemac.c_str());
  }else if(message == "ip"){
  String ip = WiFi.localIP().toString();
  String responseip = "direccion ip de las bestias: " + String(ip);
  mqttClient.publish(outboundtopic, responseip.c_str());

  }else if (message == "rssi"){
  String responserssi = "RSSI de las bestias: " + String(WiFi.RSSI()) + " dBm";
  mqttClient.publish(outboundtopic, responserssi.c_str());

  }
  else if(message.startsWith("ADMIN_OPEN:")){
    String salon = message.substring(11);

    Serial.println("================================");
    Serial.println("APERTURA MANUAL DESDE ADMIN");
    Serial.println("Sala: " + salon);
    Serial.println("Acceso valido");
    Serial.println("================================");
}

}

//CON ESTE SE INTENTA CONECTAR (SE INTENTA HASTA QUE LO LOGRE)
void keepAlive(){
  if (!mqttClient.connected()) {
    Serial.println("Reconectando");
    // Intenta conectarse al servidor MQTT
    while (!mqttClient.connected()) {
      Serial.println("Intentando conectar al servidor MQTT...");
      if (mqttClient.connect(clientName)) {
        Serial.println("Conectado al servidor MQTT!");
      } else {
        Serial.print("Error al conectar: ");
        Serial.println(mqttClient.state());
        delay(5000);
      }
    }
    mqttClient.subscribe(inboundtopic);
    Serial.println("Suscrito correctamente ");
  }
}


void ConectToBroker(){

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
  keepAlive(); //verifica que este conectado con el codigo anterior, si no lo intenta hasta que se conecte
  firstMqttConection = true;

}


//const char* ssid = "iPhone";
//const char* password = "luciana2013";
//const char* ssid = "PUBLICA";
//const char* password = "";
const char* ssid = "LABREDES";
const char* password = "F0rmul4-1";
//const char* ssid = "JhonySins";
//const char* password = "Jhony2007";
//const char* ssid = "HONOR X7c";
//const char* password = "1109666231";
//const char* ssid = "EL MAGO";
//const char* password = "evolucion10";
//const char* ssid = "DATOS";
//const char* password = "12345678";


//Capa de aplicación
String BASE_URL = "http://192.168.130.32:8000/";


//Tomar un grupo usando Nyquist
void takeTest(){

}

void POSTRequest(String url, String data){
  HTTPClient http;
  http.begin(url.c_str()); //TCP handshake
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(data); // Http request
  Serial.println(httpResponseCode);
  if(httpResponseCode == 200){
    String responseBody = http.getString();
    Serial.println(responseBody);
  }else{
      Serial.println("Error on HTTP request");
  }
}

String takeFullSample(){
  // 2 segundos
  // El fenomeno tiene hasta 25Hz
  // Muestreo a 50Hz, es decir, 50 muestras cada segundo
  JSONVar readings; // []
  for(int i = 0 ; i < 100 ; i ++){
    long tic = millis();
    JSONVar reading; // {}
    int value = random(0, 1024); //ADC 10-bits
    time_t now;
    time(&now);
    int timestamp = (long) now;
    String deviceName = "HX711";
    String units = "ADC";
    reading["value"] = value; // {"value": 234}
    reading["timestamp"] = timestamp; // {"timestamp": 1001, "value": 234}
    reading["deviceName"] = deviceName; // {"deviceName": "HX711", "timestamp": 1001, "value": 234}
    reading["units"] = units; // {"units": "ADC", "deviceName": "HX711", "timestamp": 1001, "value": 234}
    readings[i] = reading; // [ {...} ]
    long toc = millis() - tic;
    delay(20 - toc); // 1000/50 -> 20
  } 
  return JSON.stringify(readings);
}

String takeSingleSample(String clave){
  JSONVar sample;

  time_t now;
  time(&now);

  String expectedKey = "1234"; // simulada
  String espacio = "Salon101";
  String esp32_serial = "ESP-A1";

  bool accesoValido = (clave == expectedKey);

  sample["clave"] = clave;
  sample["acceso"] = accesoValido ? "valido" : "invalido";
  sample["timestamp"] = long(now);
  sample["espacio"] = espacio;
  sample["esp32_serial"] = esp32_serial;

  return JSON.stringify(sample);
}

void sendSingleSample(String clave){
  String json = takeSingleSample(clave);
  Serial.println(json);
  String url = BASE_URL + "readings"; //readings
  POSTRequest(url, json);
}

void sendFullSample(){
  String json = takeFullSample();
  Serial.println(json);
  String url = BASE_URL + "readings/batch"; //readings
  POSTRequest(url, json); 
}

void GETRequest(){
    HTTPClient http;
    http.begin(BASE_URL.c_str()); //TCP handshake
    int httpResponseCode = http.GET(); // Http request
    Serial.println(httpResponseCode);
    if(httpResponseCode == 200){
      String responseBody = http.getString();
      Serial.println(responseBody);
    }else{
      Serial.println("Error on HTTP request");
    }
}

void connectToWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); //Intento para conectarse al WiFi
  Serial.print("Connecting to WiFi ..");

  //While
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  Serial.println("Connected!!");
  Serial.println(WiFi.localIP());
}

void initTime() {
  configTime(-5 * 3600, 0, "pool.ntp.org"); // Colombia UTC-5

  Serial.print("Sincronizando tiempo");

  time_t now = time(nullptr);
  while (now < 100000) {  // espera hasta que tenga tiempo válido
    Serial.print(".");
    delay(500);
    now = time(nullptr);
  }

  Serial.println("\nTiempo sincronizado");
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  initTime();
  ConectToBroker();
}

void loop() {
  if (firstMqttConection){  //si ya se conectó revisamos el buffer de el mqtt (se hace constantemente por eso es loop)
  mqttClient.loop();
  keepAlive();
  }
}

void serialEvent() {
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    data.trim();

    if(data == "test"){
      GETRequest();
    }else{
      // Aqui se toma la "muestra" del teclado real
      Serial.println("Clave digitada: " + data);
      sendSingleSample(data);
    }
  }
}