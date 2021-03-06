# Luciole-v1.0

⚠️ **Supprimez le dossier images/ pour éviter les bugs durant l'upload du programme.**

Luciole est un contrôleur de LED RGB.

Ci-dessous vous trouverez les commandes à utiliser pour chargé le code dans la carte et dans sa mémoire SPIFFS:
```c
platformio run -t upload //upload du code arduino dans la carte
platformio run -t upload //upload des fichiers dans la mémoire SPIFFS

```

La **version 1.0** inclut les fonctionnalités suivantes:
- [x] Portail captif de connexion
- [x] switch ON/OFF
- [x] Roue de couleurs
- [x] Une option de sauvegarde de couleur
- [x] Un mode *'SmartBlue light'*
- [x] La possibilité de paramètrer le mode *'SmartBlue light'
- [x] Un mode *'Sun wake'*

------
- [ ] Plusieurs animation de couleurs

## Portail captif de connexion

Le portail captif permet de connecter Luciole à votre box internet.

Celui-ci se manifeste seulement à la première connexion du module.Le SSID et le mot de passe sont ensuite stockés dans la mémoire de l'ESP.

Cette fonctionnalité est directement issue de la libraire [WiFimanager](https://github.com/tzapu/WiFiManager)
que j'ai modifiée [WiFiManagerByWeldy](https://github.com/Weldybox/WiFiManager-by-Julfi).


```c
#include <WiFiManagerByWeldy.h>

```

## Roue de couleurs

La roue de couleur est générée grâce à l'API [iro.js](https://iro.js.org/).
> Javascript
```js
var colorPicker = new iro.ColorPicker(".colorPicker", {
  width: 350,
  borderWidth: 1,
  borderColor: "#fff",
});
```

Les couleurs sont générées si l'utilisateur bouge le curseur de la roue:
> Javascript
```js
 colorPicker.on(["color:init", "color:change"], function(color){
    red = ((color.rgbString).split(',')[0]).substring(4, 7);
    green = (color.rgbString).split(',')[1];
    var longueur =  ((color.rgbString).split(',')[2]).length;
    blue = (((color.rgbString).split(',')[2]).substring(0,longueur-1));
});
```
Puis elle sont envoyées au microcontrôleur au travers du websocket. Celui-ci va ensuite
s'occuper d'allumer plus ou moins intensément les LEDs concernés:

> Arduino IDE
```c
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
```
<p align="center"><img src="https://github.com/Weldybox/Luciole-v1.0/blob/master/images/roueCouleurs.png" alt="image roue couleur"></img></p>

## switch ON/OFF

Le switch ON/OFF permet comme son nom l'indique d'allumer ou d'éteindre les LEDs. Lorsque celui-ci est actionné la fonction *checkbox.addEventListener* elle même fonction de l'EventListener se lance. Si celui-ci est off on donne la directive d'éteindre les LEDs:
> Javascript
```c
  //Fonction qui detecte lorsque l'on affiche les couleurs sauvegardées.
  checkbox.addEventListener('change', function () {
    if (checkbox.checked) {
      document.cookie = "onOFf=on";
      on = true;
    } else {
      document.cookie = "onOFf=off";
      connection.send("R0");
      connection.send("G0");
      connection.send("B0");
      on = false;
    }
  });
 
  ```
  
  ## Une option de sauvegarde de couleur
  
Sauvegarde dans la mémoire SPIFFS de l'ESP jusqu'à 5 couleurs sélectionnées par l'utilisateur.

Les couleurs sélectionnées et sauvegardées sont transférées à l'ESP avec un label spécial indiquant qu'il faut les stocker.

> Arduino
```c
if(payload[0] =='s'){
  addData(payload[1],payload,"/save.csv",2);
}
```
Par la suite, si l'utilisateur souhaite afficher le menu des couleurs sauvegardées, le fichier save.csv est analysé et les couleurs retournées dans l'ordre de leurs sauvegardes:
> JavaScript
```c
function displaySave(results){
  if(results.data[0][0] == null){
    var number = 1;
  }else{var number =0;}
  console.log(results);
  var count = 0;
  results.data[number].forEach(element => {
    var id= 'carre' + String(count+1);
    console.log(element[count]);

    var backgroundcolor = "rgb(" + element + ")";
    document.getElementById(id).style.background = backgroundcolor;
    count++;
  });
}
```
Une fonction surveille également que le nombre d'enregistrements ne dépasse pas 5. Dans le cas contraire, l'ESP supprime la première valeur entrée et rajoute la 6e en 5e position.

> Arduino
```c
//On regarde si le nombre de couleur sauvegardé n'exède pas 5
  if(index == 3 && checkSpace(1,2,"go2")){
    //Serial.println("true");
    Serial.println("Suppression save1");
    suprSelect("n1","/saveS.csv",5,12);
    //Si c'est le cas on libère de l'espace dans la mémoire

  }else if(index == 2 && checkSpace(1,5,"go1")){
    Serial.println("Suppression save");
    suprSelect("n0","/save.csv",5,1);
  }
```
<p align="center"><img src="https://github.com/Weldybox/Luciole-v1.0/blob/master/images/savedColors.gif" alt="image roue couleur" width="450"></img></p>

## Option smartLight

L'option smart light permet d'adapter la lumière avec le lever et le coucher de soleil

L'application récupère tous les jours l'heure du lever et coucher de soleil grâce à l'API Openweathermap ainsi que l'heure format unix.

> Arduino
```c
dataSmartEcl[0] = (requete("5258129e3a2c4e8144a8c755cfb8e97d","La rochelle","sunrise")+utcOffsetInSeconds);
dataSmartEcl[1] = (requete("5258129e3a2c4e8144a8c755cfb8e97d","La rochelle","sunset")+utcOffsetInSeconds);
dataSmartEcl[2] = (timeClient.getEpochTime());
```

Si l'application détecte qu'on est à une heure du crépuscule alors elle va dériver vers une couleur sélectionnée par l'utilisateur. Cette couleur est sélectionnée en JS et sauvegardée dans la mémoire spiffs de la même manière que la sauvegarde classique.

> JavaScipt
```c
  //Fonction qui detecte quand on veut régler la plage de couleur de l'éclairage intelligent
  Reglage.addEventListener('change', function () {
    if(Reglage.checked){
      if(SaveMenu.checked){
        document.getElementById("menu-toggle").checked = false;
      }
      chooseSave("saveS.csv");
      rangeDefine = true;
    }else{
      rangeDefine = false;
      pickColor = 0;

      var couleurs = ["sTR","sTG","sTB"];

      for(var i=0;i<3;i++){
        console.log(couleurs[i] + ((((((document.getElementById("sous-menu2").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);

        connection.send(couleurs[i] + ((((((document.getElementById("sous-menu1").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);

      }
      for(var i=0;i<3;i++){
      connection.send(couleurs[i] + ((((((document.getElementById("sous-menu2").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);
      }
    }
  });
```
<p align="center"><img src="https://github.com/Weldybox/Luciole-v1.0/blob/master/images/SelectionColorSmartLight.gif" alt="image roue couleur" width="650"></img></p>

Pour éviter les conflits, le mode smart light n'est pas compatible avec le mode libre. Le site est aussi équipé de cookies qui permettent de reprendre la session à l'endroit ou l'utilisateur l'a quittée.

<p align="center"><img src="https://github.com/Weldybox/Luciole-v1.0/blob/master/images/gestionCompatibilitéSmartLight.gif" alt="image roue couleur" width="650"></img></p>

 ## Option alarme
 
 Avec cette option il est possible de se réveiller en lumière
 
L'utilisateur peut sélectionner la couleur d'allumage des LEDs. Cette couleur est stockée dans un fichiers .csv à l'intérieur de la mémoire SPIFFS de l'ESP.

<p align="center"><img src="https://github.com/Weldybox/Luciole-v1.0/blob/master/images/selectWakeColors.gif" alt="image roue couleur" width="650"></img></p>

La seconde page permet de régler l'heure à laquelle il souhaite que les LEds s'allument. Sur la partie supérieure, l'heure actuelle. Sur la partie centrale, l'alarme a réglé. Enfin sur la partie inférieure, le switch qui permet d'activer ou non la fonctionnalité.
<p align="center"><img src="https://github.com/Weldybox/Luciole-v1.0/blob/master/images/DefAlarmeHeure.png" alt="image roue couleur" width="650"></img></p>
L'ESP va ensuite transformer l'heure sélectionnée par l'utilisateur en timestamp (traitable plus facilement par le code arduino).
 
Le code arduino va vérifier tous les X secondes si l'on approche de l'heure sélectionner par l'utilisateur.
> Arduino
```c
  if(wakeHour){
    unsigned long currentAlarmeMillis = millis();
    if(currentAlarmeMillis - previousAlarme >= refreshAlarme){
      Alarme();
      previousAlarme = millis();
    }
}
```
Le temps de rafraichissement est par la même occasion défini selon l'écart entre l'heure alarme et le timestamp actuel.

> Arduino
```c
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
    WakeTime += 86400;
    refreshAlarme = 3600000;
    Serial.println("à dans une heure");

  }else if(timeClient.getEpochTime() > (WakeTime - 300)){
    int time = timeClient.getEpochTime();
    if(refreshAlarme != 1000){
      Serial.println(time);
      Serial.println(WakeTime);
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
```
Si l'on se trouve à moins de 5 minutes de l'heure sélectionnée alors les LEDs s'allumeront petit à petit vers la couleur sélectionnée grâce aux mapping!

