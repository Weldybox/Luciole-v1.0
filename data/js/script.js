
var on;

var red;
var green;
var blue;

var rangeDefine;
var pickColor = 0;

//Fonction qui detecte qu'on l'on active ou non l'éclairage intelligent.
//var previousOn = false; //Précédent état de on
 var previousColor;


/*---------------------------------------------------------
Fonction d'initialisation, lorsque la connexion est réinitialisé
---------------------------------------------------------*/
function init(){
  if(getCookie("onOFf") == "on"){
    document.getElementById("onOFf").checked = true;
    on = true;
  }else if(getCookie("onOFf") == "off"){
    document.getElementById("onOFf").checked = false;
    on = false;
  }

  if(getCookie("smartlight") == "off"){
    document.getElementById("smartLightSwitch").checked = false;

  }else if(getCookie("smartlight") == "on"){
    document.getElementById("smartLightSwitch").checked = true;
    document.getElementById("onOFf").checked = false;
    on = false;
    //checkSLCheckProcess();
  }

  Papa.parse("saveA.csv", {
    header: false,
    download: true,
    delimiter: ";",
    dynamicTyping: true,
    complete: function(results) {
      document.getElementById("sous-menu3").style.background = "rgb(" + results.data[0][0] + ")";
    }
  });
}



/*---------------------------------------------------------
Fonction qui décode les cookie
---------------------------------------------------------*/
function getCookie(cname) {
  var name = cname + "=";
  var decodedCookie = decodeURIComponent(document.cookie);
  var ca = decodedCookie.split(';');
  for(var i = 0; i <ca.length; i++) {
    var c = ca[i];
    while (c.charAt(0) == ' ') {
      c = c.substring(1);
    }
    if (c.indexOf(name) == 0) {
      return c.substring(name.length, c.length);
    }
  }
  return "";
}

/*---------------------------------------------------------
Différents objets et fonction relatifs au Websocket
---------------------------------------------------------*/
var connection = new WebSocket('ws://' + location.hostname + ':81/',['arduino']);
connection.onmessage = function(event){
  console.log(event.data);
  colorPicker.color.rgbString = event.data;
};
connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
};
connection.onclose = function () {
  console.log('WebSocket connection closed');
};
  
function chooseColorRange(i){
  console.log("fonction de changement de background color");
  pickColor = i;
}


window.onload=function(){
  var bouton = document.getElementById('btnMenu');
  var nav = document.getElementById('nav');
  bouton.onclick = function(e){
      if(nav.style.display=="block"){
          nav.style.display="none";
      }else{
          nav.style.display="block";
      }
  };
};


/*---------------------------------------------------------
Fonction montre les couleurs sauvegardé par l'utilisateur
---------------------------------------------------------*/
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

/*---------------------------------------------------------
Fonction qui permet d'utiliser le contenue d'un fichier CSV.
Par défaut il utilise le fichier saveS.csv
---------------------------------------------------------*/
var position = [];
function chooseSave(filename){
  Papa.parse(filename, {
    header: false,
    download: true,
    delimiter: ";",
    dynamicTyping: true,
    complete: function(results) {
      if(filename == "save.csv"){
      displaySave(results); 
      }else{
        console.log(results);

        var count = 0;
        results.data[0].forEach(element => {
          var backgroundcolor = "rgb(" + element + ")";
          document.getElementById("sous-menu"+((count+1).toString(10))).style.background = backgroundcolor;
          count++;
        });
      }
    }
  });
}

/*---------------------------------------------------------
Fonction qui selection dans le cercle de couleur la couleur
choisie par l'utilisateur dans les sauvegardes.
---------------------------------------------------------*/
function setSaveColor(id){
  Papa.parse('save.csv', {
    header: false,
    download: true,
    delimiter: ";",
    dynamicTyping: true,
    complete: function(results) {
      if(results.data[0][0] == null){
        var number = 1;
      }else{var number =0;}
      var tableau = results.data[number];
      colorPicker.color.rgbString = "rgb("+ tableau[id-1] +")";
    }
  });
}

/*---------------------------------------------------------
Fonctino qui envois 0 lumière
---------------------------------------------------------*/
function sendOff(){
  connection.send("R0");
  connection.send("G0");
  connection.send("B0");
}

/*---------------------------------------------------------
Fonction fonction qui envois à l'ESP les couleurs à sauvegarder
---------------------------------------------------------*/
function saving(){
  connection.send("sR"+red);
  connection.send("sG"+green);
  connection.send("sB"+blue);

}

/*---------------------------------------------------------
Fonction qui rafraichie les couleurs toute les 0.5 secondes
---------------------------------------------------------*/
setInterval(function() {
  if(on){
    connection.send("R"+red);
    connection.send("G"+green);
    connection.send("B"+blue);

  }
}, 500);

/*---------------------------------------------------------
Fonction qui exécute les procédures quand l'utilisateur
active la smart light
---------------------------------------------------------*/
function checkSLCheckProcess(){ 
  if(document.getElementById("onOFf").checked){ //Si le mode libre est activé
    document.cookie = "previousC =" + colorPicker.color.rgbString;
    //previousColor = colorPicker.color.rgbString;
    document.getElementById("onOFf").checked = false; //On desactive le mode libre
    on = false;
    document.cookie = "previousOn =true"; //Dans le cas ou le bouton on/off est activé on 'set' previousOn à true
  }else{
    document.cookie = "previousOn =false"; //Dans le cas ou le bouton on/off n'était pas activé on 'set' previousOn à false
  }
  connection.send("#1");
}

/*---------------------------------------------------------
Fonction qui exécute les procédures quand l'utilisateur
désactive la smart light
---------------------------------------------------------*/
function checkSLUncheckProcess(){
  if(getCookie("previousOn") == "true"){ //Si l'état précédent était "on"
    colorPicker.color.rgbString = getCookie("previousC");
    document.getElementById("onOFf").checked = true; //Alors on remet l'état actuel du switch à "on"
    
    on = true;
    connection.send("#2");
  }else{

    sendOff();
    connection.send("#2");
  }
  document.cookie = "previousOn =true"; //On remet le cookie previousOn à true

  setInterval.println("off");
}

function saveAlarme(){
  console.log("ok");
  //connection.send("sA");
  var couleurs = ["sAR","sAG","sAB"];

  for(var i=0;i<3;i++){
    console.log(couleurs[i] + ((((((document.getElementById("sous-menu3").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);

    connection.send(couleurs[i] + ((((((document.getElementById("sous-menu3").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);

  }
  
}

//Fonction qui permet de gérer les interactions de l'utilisateur sur la page.
document.addEventListener('DOMContentLoaded', function () {
  var SmartLight = document.querySelector('input[name=SmartLight]');
  var checkbox = document.querySelector('input[type="checkbox"]');
  var Reglage = document.querySelector('input[name=menu-open]');
  var SaveMenu = document.querySelector('input[name=SaveMenu]');


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
        //console.log(couleurs[i] + ((((((document.getElementById("sous-menu2").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);

        connection.send(couleurs[i] + ((((((document.getElementById("sous-menu1").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);

      }
      for(var i=0;i<3;i++){
      connection.send(couleurs[i] + ((((((document.getElementById("sous-menu2").style.background).split("("))[1]).split(")"))[0]).split(","))[i]);
      }
    }
  });

  //Fonction qui detecte lorsque l'on affiche les couleurs sauvegardées.
  checkbox.addEventListener('change', function () {
    if (checkbox.checked) {
      document.cookie = "onOFf=on; expires=Thu, 18 Dec 2020 12:00:00 UTC";
      on = true;
    } else {
      document.cookie = "onOFf=off; expires=Thu, 18 Dec 2020 12:00:00 UTC";
      sendOff();
      on = false;
    }
  });

  /*---------------------------------------------------------
  Quand l'utilisateur interagis avec l'interrupteur du smartlight
  ---------------------------------------------------------*/
  SmartLight.addEventListener('change', function () {
    
    if (SmartLight.checked) {
      document.cookie = "smartlight=on; expires=Thu, 18 Dec 2020 12:00:00 UTC";
      checkSLCheckProcess();
    } else {
      document.cookie = "smartlight=off; expires=Thu, 18 Dec 2020 12:00:00 UTC"; 
      checkSLUncheckProcess();
    }
  });

  //Fonction qui detecte quand on veut afficher ou non le menu des couleurs sauvegardés
  SaveMenu.addEventListener('change', function () {
    if(SaveMenu.checked){
      if(Reglage.checked){
        document.getElementById("menu-open").checked = false;
      }
    }else{
      console.log("off");
    }
  });
});

var colorPicker = new iro.ColorPicker(".colorPicker", {
  width: 350,
  borderWidth: 1,
  color:getCookie("cursor"),// "#ffff",
  borderColor: "#fff",
});

 colorPicker.on(["color:init", "color:change"], function(color){
    red = ((color.rgbString).split(',')[0]).substring(4, 7);
    green = (color.rgbString).split(',')[1];
    var longueur =  ((color.rgbString).split(',')[2]).length;
    blue = (((color.rgbString).split(',')[2]).substring(0,longueur-1));

    document.cookie = "cursor="+"rgb("+red+","+green+","+blue+"); expires=Thu, 18 Dec 2020 12:00:00 UTC";

    if(rangeDefine && pickColor){
      var backgroundcolor = "rgb("+red+","+green+","+blue+")";
      document.getElementById("sous-menu"+pickColor).style.background = backgroundcolor;
    }else if(pickColor == 3){
      var backgroundcolor = "rgb("+red+","+green+","+blue+")";
      document.getElementById("sous-menu"+pickColor).style.background = backgroundcolor;
    }
});


/*Fonction d'initialisation*/
init();
