# Luciole-v1.0 ??

⚠️ **Supprimez le dossier images/ pour éviter les bugs durant l'upload du programme.**

Luciole est un contrôleur de LED RGB.

La **version 1.0** inclut les fonctionnalités suivantes:
- [x] Portail captif de connexion
- [x] switch ON/OFF
- [x] Roue de couleurs
- [x] Une option de sauvegarde de couleur
- [x] Un mode *'SmartBlue light'*
- [x] La possibilité de paramètrer le mode *'SmartBlue light'

------

- [ ] Un mode *'Sun wake'*
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
l'ESP.
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
Une fonction surveille également le nombre d'enregistrements ne dépasse pas 5. Dans le cas contraire, l'ESP supprime la première valeur entrée et rajoute la 6e en 5e position.

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
