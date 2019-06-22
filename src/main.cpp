#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WiFiManagerByWeldy.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "NTPClientByJulfi.h"
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
/*--------------------------------------------------------------------------------
Variables qui peremttent de manipuler des constantes de temps
--------------------------------------------------------------------------------*/
#define MSECOND  1000
#define MMINUTE  60*MSECOND
#define MHOUR    60*MMINUTE
#define MDAY 24*MHOUR

/*--------------------------------------------------------------------------------
Variable qui stock les numéros de broche de chaque MOSFET.
--------------------------------------------------------------------------------*/
#define REDPIN 13
#define GREENPIN 12
#define BLUEPIN 14

int dataSmartEcl[3]; //Tableau qui va contenir les 3 temps unix nécessaire au traitement.
String couleurComp[2] = {"255,72,0","255,255,255"};


unsigned long utcOffsetInSeconds = 7200; 

/*--------------------------------------------------------------------------------
Variables qui définissent le mode de fonctionement et le taux de rafraichissement
--------------------------------------------------------------------------------*/
unsigned long refresh = 10000;

String mode = "Desative"; //Le mode de fonctionement smart des LEDs, de base désactivé

uint8_t go1 = 0; //Variable qui se devient vrai lorsque l'on doit supprimer une valeure en mémoire de l'ESP.
uint8_t go2 = 0;   

unsigned long previousLoopMillis = 0; //Variable qui permet d'actualiser la couleur des LEDs toute les X secondes en mode Smart Eclairage
unsigned long previousAlarme = 0;

bool wakeHour = false;
int WakeTime;
int refreshAlarme;
int fromhigh;

uint8_t AlarmeRed = 0;
uint8_t AlarmeGreen = 0;
uint8_t AlarmeBlue = 0;

/*--------------------------------------------------------------------------------
Définition des objets nécessaire dans la suite du programme.
--------------------------------------------------------------------------------*/
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org",utcOffsetInSeconds);  

HTTPClient http;


/*--------------------------------------------------------------------------------
Fonction de programmation OTA
--------------------------------------------------------------------------------*/
void confOTA() {
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("ESP8266_LEDstrip");

  ArduinoOTA.onStart([]() {
    Serial.println("/!\\ Maj OTA");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\n/!\\ MaJ terminee");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progression: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

String split(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}


void Alarme(){
  if((timeClient.getEpochTime() < (WakeTime - 300)) && refreshAlarme != 60000){
    refreshAlarme = 60000;
    Serial.println(timeClient.getEpochTime());
    Serial.println("<");
    Serial.println(WakeTime);
    Serial.println(refreshAlarme);
    analogWrite(REDPIN, 0);
    analogWrite(GREENPIN, 0);
    analogWrite(BLUEPIN, 0);

  }else if(timeClient.getEpochTime() >= WakeTime){
    wakeHour = false;
  }else if(timeClient.getEpochTime() > (WakeTime - 300)){
    int time = timeClient.getEpochTime();
    if(refreshAlarme != 1000){
      refreshAlarme = 1000;
      Serial.println("refresh toute les secondes");
      fromhigh = WakeTime - time;
    }
    
    int red = map((WakeTime - time), fromhigh, 0, 0, AlarmeRed);
    int green = map((WakeTime - time), fromhigh, 0, 0, AlarmeGreen);
    int blue = map((WakeTime - time), fromhigh, 0, 0, AlarmeBlue);

    Serial.println(red);
    Serial.println(green);
    Serial.println(blue);

    analogWrite(REDPIN, red);
    analogWrite(GREENPIN, green);
    analogWrite(BLUEPIN, blue);

  }
}

/*--------------------------------------------------------------------------------
Fonction qui retourne si l'on est plus proche du levé ou coucher de soleuil.
--------------------------------------------------------------------------------*/
String sunPosition(int dataSmartEcl[], uint8_t longueur){
  
  //Compare la différence entre le timestamp et les valeurs de sunrise et sunset
  //Si la différence de seconde entre l'heure actuelle et l'heure de levé de soleil est supérieur à la différence pour le coucher de soleil
  if(abs(dataSmartEcl[2] - dataSmartEcl[0]) > abs(dataSmartEcl[2] - dataSmartEcl[1])){
    return "soir"; //On considère qu'on est dans la période "soir"
  }else{
    return "matin"; //Sinon on considère qu'on est dans la période matin
  }
}

/*--------------------------------------------------------------------------------
Fonction qui va retourner l'écart en seconde entre le jour et la nuit
--------------------------------------------------------------------------------*/
uint16_t checkTime(int dataSmartEcl[], uint8_t longueur){
  uint8_t index;
  //Selon si l'on est le matin ou le soir on définir l'index de comparaison 'sunset/sunrise'
  if(sunPosition(dataSmartEcl, 3) == "matin"){index = 0;}
  else{index=1;}

  //On vérifie que l'heure du jour est au alentour de l'heure de sunset ou sunrise
  int ecart = dataSmartEcl[2] - dataSmartEcl[index]; //L'écart c'est la différence entre l'heur actuelle et l'heure de coucher/levé de soleil
  if(ecart < 0 && ecart > -3600){ //Si c'ette écart se trouve dans la tranche de transition des LEDs

    int result = map(ecart, -3600, 0, 0, 3600); //On retrourne un résultat positif
    return result;
  }else if(ecart < -3600){return 0;} //Si l'écart est inférieur à la valeur de tranche basse on retourne 0
  else if(ecart > 0){return 3600;} //Sinon si l'écart est supérieur à la valeure de tranche haut on retourne 3600
  else{return 0;} //Sinon on retourne 0

}

/*--------------------------------------------------------------------------------
Fonction qui retourne les valeur de rouge vert et bleu en fonction de l'écart
entre le jour et la nuit
--------------------------------------------------------------------------------*/
void displayColors(int dataSmartEcl[],uint8_t longueur, int cooldown, String mode){
  /*--------------------------------------------------------------------------------
  Selon l'a position du soleil nous cherchons à aller vers le blanc ou le orange/rouge
  --------------------------------------------------------------------------------*/
  Serial.println(sunPosition(dataSmartEcl, 3));
  if((sunPosition(dataSmartEcl, 3) == "soir") && (mode = "SmartLight")){ //S'il on se trouve dans la tranche soir
    Serial.println("soir");

    //On map les valeurs de vert et bleu de 255 (couleur blanche) au valeurs attendue en sortie.
    uint8_t rouge = map(checkTime(dataSmartEcl, 3),0,cooldown,(split(couleurComp[1],',',0)).toInt(),(split(couleurComp[0],',',0)).toInt());
    uint8_t vert = map(checkTime(dataSmartEcl, 3),0,cooldown,(split(couleurComp[1],',',1)).toInt(),(split(couleurComp[0],',',1)).toInt());
    uint8_t bleu = map(checkTime(dataSmartEcl, 3),0,cooldown,(split(couleurComp[1],',',2)).toInt(),(split(couleurComp[0],',',2)).toInt());
    //Puis on set le bandeau de LED tel quel.
    analogWrite(REDPIN, rouge);
    analogWrite(GREENPIN, vert);
    analogWrite(BLUEPIN, bleu);
    
    //On envoie également cette valeure sur le websocket pour avoir un retour du côté client.
    String rgb = ("rgb("+String(rouge, DEC) +","+String(vert,DEC)+","+String(bleu,DEC)+")");
    Serial.println(rgb);
    webSocket.sendTXT(0,rgb);

  }else{
    Serial.println("journee"); //Dans le cas contraire on est dans la seconde partie de tranche

    //On map les valeus de vert et bleu allant des valeurs de couleurs chaudes à 255 (couleurs blanche)
    uint8_t rouge = map(checkTime(dataSmartEcl, 3),0,cooldown,(split(couleurComp[0],',',0)).toInt(),(split(couleurComp[1],',',0)).toInt());
    uint8_t vert = map(checkTime(dataSmartEcl, 3),0,cooldown,(split(couleurComp[0],',',1)).toInt(),(split(couleurComp[1],',',1)).toInt());
    uint8_t bleu = map(checkTime(dataSmartEcl, 3),0,cooldown,(split(couleurComp[0],',',2)).toInt(),(split(couleurComp[1],',',2)).toInt());

    //Puis on set le bandeau de LED te quel.
    analogWrite(REDPIN, rouge);
    analogWrite(GREENPIN, vert);
    analogWrite(BLUEPIN, bleu);

    //ON envoie également cette valeure sur le websocket pour avoir un retour du côté client.
    String rgb = ("rgb("+String(rouge, DEC) +","+String(vert,DEC)+","+String(bleu,DEC)+")");
    Serial.println(rgb);
    webSocket.sendTXT(0,rgb);
  }

}

/*--------------------------------------------------------------------------------
Fonction qui retourne l'heure de sunset ou de sunrise (en unix timestamp)
--------------------------------------------------------------------------------*/
int requete(String Apikey, String ville,String type){
  String requeteHTTP =("http://api.openweathermap.org/data/2.5/weather?q=" + ville + "&APPID="
  + Apikey); 
  int timestamp;

  http.begin(requeteHTTP);  //Specify request destination
  int httpCode = http.GET();

  if (httpCode > 0) { //Check the returning code
 
  String payload = http.getString();   //Get the request response payload
  //Serial.println(payload);      //Print the response payload

  DynamicJsonDocument doc(765);
  DeserializationError error = deserializeJson(doc, payload);
  if(type == "sunrise"){
    timestamp = doc["sys"]["sunrise"];
  }else if(type == "sunset"){
    timestamp = doc["sys"]["sunset"];
  }
  

  }else{
    timestamp = 0;
  }
  http.end();   //Close connection
  return timestamp;
}

/*--------------------------------------------------------------------------------
Fonction qui suprrime la première ligne que plus de 4 couleurs ont été enregistré
--------------------------------------------------------------------------------*/
void suprdata(String data[55], int tabIndex[5], String nomFichier, int limiteSave,int beginSave){
  File ftemp = SPIFFS.open(nomFichier, "w"); //On ouvre en mode écriture le fichier save.csv
  
  for(uint8_t x=beginSave;x<limiteSave;x++){ //Pour chaques couleurs sauvegardé
    char msg[60];
    int index = tabIndex[x]; //On récupère la position de chaque élément
    String ligne = data[index]; //Puis on récupère la couleur voulue
    sprintf(msg, "%s;",ligne.c_str()); //On concatène le point virgule avec le code couleur

    if (nomFichier == "/saveS.csv"){
      Serial.println(msg);
      couleurComp[x-beginSave] = msg;

    }
    ftemp.print(msg);

    
  }
  ftemp.close();
}


/*--------------------------------------------------------------------------------
Test s'il faut suprrimer la première couleur entrée.
--------------------------------------------------------------------------------*/
bool checkSpace(uint8_t count, uint8_t longueur, String go){
  if(go == "go1"){
    Serial.println("longeur = 5");
    go1 += count;
    if(go1 >= longueur){
      return 1;
    }else{
      return 0;
    }
  }else{
    go2 += count;
    if(go2 >= longueur){
      return 1;
    }else{
      return 0;
    }
  }
}

/*--------------------------------------------------------------------------------
Supression de des couleurs enregistrer en trop.
--------------------------------------------------------------------------------*/
void suprSelect(String nom, String nomFichier, uint8_t nombre, uint8_t Nsuppr){
  File file = SPIFFS.open(nomFichier, "r");
 
  //Tableau à sauvegarder dans le fichier temporaire
  String save[55] = {};
  int index[5] = {};
  int longueur=1;


  for(uint8_t i=0;i<nombre;i++){ //On répète la boucle 5 fois pour chaque couleur
    String part = file.readStringUntil(';'); //On lie la ligne jusqu'au changement de couleur
    Serial.println(part);
    if(longueur > Nsuppr){ //On ne prend pas en compte la première
      save[longueur] = part; //On ajoute au tableau save la couleur
      index[i] = longueur; //On ajoute au tableau index l'index de la couleur
    }

    longueur += part.length();
  }
  if(nom == "n0"){
    go1 --;

    file.close(); 
    suprdata(save, index, "/save.csv",5,1);
  }else if(nom == "n1"){
    go2 -=2;
    //Serial.println(go2);

    file.close(); 
    suprdata(save, index, "/saveS.csv",4,2);
  }
}

/*--------------------------------------------------------------------------------
Fonction qui traite les requêtes websocket arrivant depuis le serveur web.
--------------------------------------------------------------------------------*/
void addData(uint16_t couleur, uint8_t * couleurSave, String nomFichier, uint8_t index){
  uint16_t save = (uint16_t) strtol((const char *) &couleurSave[index], NULL, 10);
  //Si on souhaite sauvegarder les donnés pour l'alarme alors on écrase la donnée précédente.
  //Sinon on l'ajoute simplement.
  File f = SPIFFS.open(nomFichier, "a+");
  if (nomFichier == "/saveA.csv" && couleur == 'R'){
    File f = SPIFFS.open(nomFichier, "w");
  }

  if (!f) {
    Serial.println("erreur ouverture fichier!");
  }else{
    if(couleur == 'R'){
        char buffred[16];
        sprintf(buffred,"%d,",save);
        Serial.println(buffred);
        f.print(buffred);
        if(nomFichier == "/saveA.csv"){ AlarmeRed = save;}

    }else if(couleur == 'G'){
        char buffgreen[16];
        sprintf(buffgreen,"%d,",save);
        f.print(buffgreen);
        Serial.println(buffgreen);
        if(nomFichier == "/saveA.csv"){AlarmeGreen = save;}
        
        
    }else if(couleur == 'B'){
        char buffblue[16];
        sprintf(buffblue,"%d;",save);
        Serial.println(buffblue);
        f.print(buffblue);
        if(nomFichier == "/saveA.csv"){ AlarmeBlue = save;}

        //On regarde si le nombre de couleur sauvegardé n'exède pas 5
       //Serial.println(index);
       if(nomFichier != "/saveA.csv"){
          if(index == 3 && checkSpace(1,2,"go2")){
            //Serial.println("true");
            Serial.println("Suppression save1");
            suprSelect("n1","/saveS.csv",5,12);
            //Si c'est le cas on libère de l'espace dans la mémoire

          }else if(index == 2 && checkSpace(1,5,"go1")){
            Serial.println("Suppression save");
            suprSelect("n0","/save.csv",5,1);

          }
       }
    }
    f.close();
  }
}

/*--------------------------------------------------------------------------------
Permet de définir si le réveil est pour le jour même ou le jour suivant.
--------------------------------------------------------------------------------*/
int WakeUPday(int heureWake, int MinWake){
  Serial.println(heureWake);
  Serial.println(timeClient.getHours());
  if(heureWake < timeClient.getHours()){
    return timeClient.getRealDay()+1;
  }else if(heureWake == timeClient.getHours() && MinWake < timeClient.getMinutes()){
    return timeClient.getRealDay()+1;
  }else{
    return timeClient.getRealDay();
    Serial.println(timeClient.getRealDay());
  }
}

/*--------------------------------------------------------------------------------
Fonction qui traite les requêtes websocket arrivant depuis le serveur web.
--------------------------------------------------------------------------------*/
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  if(type == WStype_TEXT){
    uint16_t couleur = (uint16_t) strtol((const char *) &payload[1], NULL, 10);
    if(payload[0] == 'A'){
      if(payload[1] == 'O'){
        wakeHour = true;
      }else if(payload[1] == 'F'){
        wakeHour = false;
      }else{
        int heure = 0;
        int minute = 0;

        if(couleur < 1000){
          String str = String(couleur, DEC);
          heure = (str.substring(0,1)).toInt();
          minute = (str.substring(1,3)).toInt();
        }else{
          String str = String(couleur, DEC);
          Serial.println((str.substring(0,1)).toInt());
          Serial.println((str.substring(1,3)).toInt());
          heure = (str.substring(0,2)).toInt();
          minute = (str.substring(2,4)).toInt();
        }

        struct tm info;
        char buffer[80];

        info.tm_min = minute;
        info.tm_hour = heure;
        info.tm_year =timeClient.getYear() - 1900;
        info.tm_mon = timeClient.getMonth()-1;
        info.tm_mday = WakeUPday(heure, minute);
        info.tm_sec = 1;
        info.tm_isdst = -1;

        WakeTime = mktime(&info);

        Serial.println(mktime(&info));
      }
    }
    if(payload[0] == 'R'){
      analogWrite(REDPIN, couleur);

      Serial.print("rouge = ");
      Serial.println(couleur);
    }
    if(payload[0] == 'G'){
      analogWrite(GREENPIN, couleur);

      Serial.print("vert = ");
      Serial.println(couleur);
    }
    if(payload[0] == 'B'){
      analogWrite(BLUEPIN, couleur);

      Serial.print("bleu = ");
      Serial.println(couleur);
    }
    if(payload[0] =='s'){
      if(payload[1] == 'T'){
        addData(payload[2],payload,"/saveS.csv",3);
      }else if(payload[1] == 'A'){
        Serial.println("ok");
        
        addData(payload[2],payload,"/saveA.csv",3);
      }else{
        addData(payload[1],payload,"/save.csv",2);
      }
    }
    if(payload[0] == '#'){
      if(payload[1] == '1'){
        dataSmartEcl[0] = (requete("5258129e3a2c4e8144a8c755cfb8e97d","La rochelle","sunrise")+utcOffsetInSeconds);
        dataSmartEcl[1] = (requete("5258129e3a2c4e8144a8c755cfb8e97d","La rochelle","sunset")+utcOffsetInSeconds);
        dataSmartEcl[2] = (timeClient.getEpochTime());

        if (checkTime(dataSmartEcl, 3)==0){
          refresh = 10*MMINUTE;
          mode="Active";
          Serial.println("mode = active");
          displayColors(dataSmartEcl,3,3600,"SmartLight");
        }else if(checkTime(dataSmartEcl, 3)>=0){
          refresh = 5*MMINUTE;
          mode="Process";
          Serial.println("mode = process");
          displayColors(dataSmartEcl,3,3600,"SmartLight");
        }
      
      }else if(payload[1] == '2'){
        mode="Desative";
        Serial.println("mode = desactive");
      }
    }
  }

}

void setup() {

  Serial.begin(115200);

  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);

  

    //WiFiManager
  WiFiManager wifiManager;
  wifiManager.autoConnect("Webportail");
  Serial.println("connected...yeey :)");

  if (!MDNS.begin("LEDstrip")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  confOTA();

   if(!SPIFFS.begin()) {
  Serial.println("Erreur initialisation SPIFFS");
  }



  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/js", SPIFFS, "/js");
  server.serveStatic("/style", SPIFFS, "/style");
  server.serveStatic("/save.csv", SPIFFS, "/save.csv");
  server.serveStatic("/saveA.csv", SPIFFS, "/saveA.csv");
  server.serveStatic("/saveS.csv", SPIFFS, "/saveS.csv");
  server.serveStatic("/iconWeldy.ico", SPIFFS, "/iconWeldy.ico");
  server.serveStatic("/alarme.html", SPIFFS, "/alarme.html");

  
  timeClient.begin();
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent); //Fonction de callback si l'on reçois un message du Webserveur
}

        
void loop() {

  /*--------------------------------------------------------------------------------
  Test iteratifs pour l'éclairage intelligent
  --------------------------------------------------------------------------------*/
  unsigned long currentLoopMillis = millis();
  if(currentLoopMillis - previousLoopMillis >= refresh){
    
    if(((timeClient.getFormattedTime()).substring(0,2)).toInt() < 1){
      if(((timeClient.getFormattedTime()).substring(3,6)).toInt() < 30){
        dataSmartEcl[0] = (requete("5258129e3a2c4e8144a8c755cfb8e97d","La rochelle","sunrise")+utcOffsetInSeconds);
        dataSmartEcl[1] = (requete("5258129e3a2c4e8144a8c755cfb8e97d","La rochelle","sunset")+utcOffsetInSeconds);
      }
    }
    dataSmartEcl[2] = (timeClient.getEpochTime());
    if(mode=="Active" or mode=="Process"){
      displayColors(dataSmartEcl,3,3600,"SmartLight");
    }
    previousLoopMillis = millis();
  }

  if(wakeHour){
    unsigned long currentAlarmeMillis = millis();
    if(currentAlarmeMillis - previousAlarme >= refreshAlarme){
      Alarme();
      previousAlarme = millis();
    }
  }
  /*--------------------------------------------------------------------------------
  Les watchdogs!
  --------------------------------------------------------------------------------*/
  timeClient.update();
  ArduinoOTA.handle(); //Appel de la fonction qui gère OTA
  webSocket.loop(); //Appel de la fonction qui gère la communication avec le websocket
  server.handleClient(); //Appel de la fonctino qui gère la communication avec le serveur web
}
