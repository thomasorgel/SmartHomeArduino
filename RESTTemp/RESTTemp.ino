/**
 
 **/
#include <OneWire.h>            // OneWire-Bibliothek einbinden
#include <DallasTemperature.h>  // DS18B20-Bibliothek einbinden

#define DS18B20_PIN 2   // Pin für DS18B20 definieren 

OneWire oneWire(DS18B20_PIN);          // OneWire Referenz setzen
DallasTemperature sensors(&oneWire);   // DS18B20 initialisieren

#include <SPI.h>
#include <Ethernet.h>
#include <uHTTP.h>

const PROGMEM char JSON_DIGITAL[] = "{\"id\": %u, \"pin\": %u, \"value\": %u, \"pwm\": %u}";
const PROGMEM char JSON_ANALOG[] = "{\"id\": %u, \"pin\": %u, \"value\": %u}";
const PROGMEM char JSON_TEMP[] = "{\"id\": %u, \"pin\": %u, \"value\": %f}";

struct output_t{
    uint8_t pin;
    uint8_t value;
    uint8_t pwm;
} output[8];

byte macaddr[6] = {  0xD8, 0x5D, 0x4C, 0xA4, 0x0F, 0xEF};

byte ip4addr[4] = {192, 168, 178, 34};

uHTTP *server = new uHTTP(80);
EthernetClient response;

void render(uint16_t code = 200, const char *body = 0);
void render(uint16_t code = 200, const __FlashStringHelper *body = 0);

void setup(){
  Serial.begin(9600);

    Ethernet.begin(macaddr, ip4addr);
    sensors.begin();  // DS18B20 starten
    Serial.print("Sensoren: ");

    Serial.println(sensors.getDeviceCount());

    Serial.print(F("Starting uHTTP at "));
    Serial.print(Ethernet.localIP());
    Serial.println(":80");

    server->begin();

    output[0] = {2, 0, 0};
    output[1] = {3, 0, 1};
    output[2] = {4, 0, 0};
    output[3] = {5, 0, 1};
    output[4] = {6, 0, 1};
    output[5] = {7, 0, 0};
    output[6] = {8, 0, 0};
    output[7] = {9, 0, 1};

    for(uint8_t i = 0; i < 8; i++) pinMode(output[i].pin, OUTPUT);
    for(uint8_t i = 0; i < 6; i++) pinMode(i, INPUT);

    Serial.println(output[7].pwm);

    //printRoutes();
}

void loop(){
  if((response = server->available())){
        if(server->uri(F("/"))){
            if(server->method(uHTTP_METHOD_GET))
                send_headers(302);
            else if(server->method(uHTTP_METHOD_OPTIONS)) 
                send_headers(200);
            else 
                send_headers(404);
        }else{
            if(server->method(uHTTP_METHOD_GET) || server->method(uHTTP_METHOD_PUT)){
                if(server->uri(1, F("digital"))){
                    // GET or PUT digital pin value
                    digitalProcess();
                }else if(server->uri(1, F("analog"))){
                    // GET or PUT analog pin value
                    analogProcess();
                }else if(server->uri(1, F("temp"))){
                    // GET or PUT analog pin value
                    tempProcess();
                }
            }else{
                send_headers(404);  
            }
        }

        response.stop();
    }
}

void digitalProcess(){
    char buffer[64] = {0};
    uint8_t id = atoi(server->uri(2));

    if(id){
        if(server->method(uHTTP_METHOD_PUT)){
            uint8_t value = atoi(server->data(F("value")));

            if(output[id - 1].pwm){
                output[id - 1].value = value;
                analogWrite(output[id - 1].pin, output[id - 1].value);
            }else{
                if(strcmp_P(server->data(F("value")), PSTR("HIGH") ) == 0)
                    output[id - 1].value = 1;
                else if(strcmp_P(server->data(F("value")), PSTR("LOW") ) == 0)
                    output[id - 1].value = 0;
                else if(value == 0 || value == 1)
                    output[id - 1].value = value;
                digitalWrite(output[id - 1].pin, output[id - 1].value);
            }
        }

        sprintf_P(buffer, JSON_DIGITAL, id, output[id - 1].pin, output[id - 1].value, output[id - 1].pwm);
        render(200, buffer);
    }else{
        uint8_t size = sizeof(output) / sizeof(output_t);
        send_headers(200);
        response.print(F("["));
        for(uint8_t i = 0; i < size; i++){
            sprintf_P(buffer, JSON_DIGITAL, i + 1, output[i].pin, output[i].value, output[i].pwm);
            response.print(buffer);
            if(i < size - 1) response.print(F(", "));
        }
        response.println(F("]"));
    }
}

void analogProcess(){
    char buffer[64] = {0};
    uint8_t id = atoi(server->uri(2));

    if(id){
        sprintf_P(buffer, JSON_ANALOG, id, id - 1, analogRead(id - 1));
        render(200, buffer);
    }else{
        send_headers(200);
        response.print(F("["));        

        for(uint8_t i = 0; i < 6; i++){
            sprintf_P(buffer, JSON_ANALOG, i + 1, i, analogRead(i));
            response.print(buffer);
            if(i < 5) response.print(F(", "));
        }
        response.println(F("]"));
    }   
}

void tempProcess(){
    char buffer[64] = {0};
    uint8_t id = atoi(server->uri(2));

    if(id){
        if (false){// mit int INDEX
            sensors.requestTemperatures(); // Temperatursensor(en) auslesen
            Serial.println("temp:");
            Serial.println(sensors.getTempCByIndex(id));
            send_headers(200);
            response.print(F("{\"id\": \""));
            response.print(id);
            response.print(F("\", \"value\":"));
            
            response.print(sensors.getTempCByIndex(id));
            response.print(F("}"));
        } else {// mit address
            String addressString = server->uri(2);
            Serial.println("addressString:");
            Serial.println(addressString);
            sensors.requestTemperatures(); // Temperatursensor(en) auslesen
            for(uint8_t i = 0; i < sensors.getDeviceCount(); i++){
              DeviceAddress address;
              if (sensors.getAddress(address, i)){
                String actAddress(address[0]);
                actAddress += address[1];
                actAddress += address[2];
                actAddress += address[3];
                actAddress += address[4];
                actAddress += address[5];
                actAddress += address[6];
                actAddress += address[7];
                if (addressString == actAddress) {
                  send_headers(200);
                  response.print(F("{\"id\": \""));
                  response.print(id);
                  response.print(F("\", \"value\":"));
                  response.print(sensors.getTempCByIndex(i));
                  response.print(F(", \"address\":\""));
                  response.print(addressString);
                  response.print(F("\"}"));
                  
                }
              }
            }
   //         Serial.println(sensors.getTempC(id));
        }
    }else{
        sensors.requestTemperatures(); // Temperatursensor(en) auslesen
        send_headers(200);
        response.print(F("["));        

        for(uint8_t i = 0; i < sensors.getDeviceCount(); i++){
            response.print(F("{\"id\": \""));
            response.print(id);
            response.print(F("\", \"value\":"));
            response.print(sensors.getTempCByIndex(i));
            response.print(F(", \"address\":\""));
            DeviceAddress address;
            if (sensors.getAddress(address, i)){
                response.print(address[0]);
                response.print(address[1]);
                response.print(address[2]);
                response.print(address[3]);
                response.print(address[4]);
                response.print(address[5]);
                response.print(address[6]);
                response.print(address[7]);
            }else 
                response.print(F("ERROR"));
            response.print(F("\"}"));
            if(i < sensors.getDeviceCount()-1) response.print(F(", "));
        }
        response.println(F("]"));
    }   
}

void send_headers(uint16_t code){
    header_t head = server->head();

    response.print(F("HTTP/1.1"));
    response.print(F(" "));
    switch(code){
        case 200:
            response.println(F("200 OK"));
            response.println(F("Content-Type: application/json"));
            if(strlen(head.orig)){
                response.print(F("Access-Control-Allow-Origin: "));
                response.println(head.orig);
                response.println(F("Access-Control-Allow-Methods: GET,PUT"));
                response.println(F("Access-Control-Allow-Headers: Authorization, Content-Type"));
                response.println(F("Access-Control-Allow-Credentials: true"));
                response.println(F("Access-Control-Max-Age: 1000"));
            }
            break;
        case 302:
            response.println(F("302 Found"));
            response.println(F("Location: https://github.com/nomadnt/"));
            break;
        case 401:
            response.println(F("401 Unauthorized"));
            response.println(F("WWW-Authenticate: Basic realm=\"uHTTP\""));
            break;
        case 404:
            response.println(F("404 Not Found"));
            break;
    }
    response.println(F("Connection: close"));
    response.println();
}

void send_body(const char *body){
    response.println(body);
}

void send_body(const __FlashStringHelper *body){
    response.println(body);
}

void render(uint16_t code, const char *body){
    send_headers(code);
    if(body) send_body(body);
}

void render(uint16_t code, const __FlashStringHelper *body){
    send_headers(code);
    if(body) send_body(body);
}

