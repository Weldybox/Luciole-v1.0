
var connection = new WebSocket('ws://' + location.hostname + ':81/');
connection.onmessage = function(event){
  if(event.data == "off"){
    console.log("off");
  }
};
connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
};
connection.onclose = function () {
  console.log('WebSocket connection closed');
};


/*---------------------------------------------------------
Fonction qui initialise les cookies
---------------------------------------------------------*/
function init(){
  if(getCookie("wakeOn") == "on"){
    document.getElementById("wakeOn").checked = true;
  }else{
    document.getElementById("wakeOn").checked = false;
  }

  document.getElementById("heure").innerHTML = getCookie("heure");
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
objet qui contient les caractièristiques de l'alarme
---------------------------------------------------------*/
var ac = {

  init : function () {


    // ac.init() : start the alarm clock

      ac.connection = connection;
      // Get the current time - hour, min, seconds
      ac.chr = document.getElementById("chr");
      ac.cmin = document.getElementById("cmin");
      ac.csec = document.getElementById("csec");
  
      // The time picker - Hr, Min, Sec
      ac.thr = ac.createSel(23);
      document.getElementById("tpick-h").appendChild(ac.thr);
      ac.thm = ac.createSel(59);
      document.getElementById("tpick-m").appendChild(ac.thm);
      ac.ths = ac.createSel(59);
      document.getElementById("tpick-s").appendChild(ac.ths);
  
      // The time picker - Set, reset
      ac.tset = document.getElementById("tset");
      ac.tset.addEventListener("click", ac.set);
      ac.treset = document.getElementById("treset");
      ac.treset.addEventListener("click", ac.reset);
  
  
      // Start the clock
      ac.alarm = null;
      setInterval(ac.tick, 1000);
    },

  
    createSel : function (max) {
    // createSel() : support function - creates a selector for hr, min, sec
  
      var selector = document.createElement("select");
      for (var i=0; i<=max; i++) {
        var opt = document.createElement("option");
        i = ac.padzero(i);
        opt.value = i;
        opt.innerHTML = i;
        selector.appendChild(opt);
      }
      return selector
    },
  
    padzero : function (num) {
    // ac.padzero() : support function - pads hr, min, sec with 0 if <10
  
      if (num < 10) { num = "0" + num; }
      else { num = num.toString(); }
      return num;
    },
  
    tick : function () {
    // ac.tick() : update the current time
      // Current time
      var now = new Date();
      var hr = ac.padzero(now.getHours());
      var min = ac.padzero(now.getMinutes());
      var sec = ac.padzero(now.getSeconds());
  
      // Update current clock
      ac.chr.innerHTML = hr;
      ac.cmin.innerHTML = min;
      ac.csec.innerHTML = sec;
  
      // Check and sound alarm
      if (ac.alarm != null) {
        now = hr + min + sec;
        if (now == ac.alarm) {
          alert("bip bip");
          
        }
      }
    },
  
    set : function () {
    // ac.set() : set the alarm

      ac.alarm = ac.thr.value + ac.thm.value + ac.ths.value;
      ac.connection.send("A" + ac.thr.value + ac.thm.value);
      document.getElementById("heure").innerHTML = ac.thr.value + ":" + ac.thm.value;
      document.cookie = "heure="+ac.thr.value + ":" + ac.thm.value;
      ac.thr.disabled = true;
      ac.thm.disabled = true;
      ac.ths.disabled = true;
      ac.tset.disabled = true;
      ac.treset.disabled = false;
    },
  
    reset : function () {
    // ac.reset() : reset the alarm
  
      ac.alarm = null;
      ac.thr.disabled = false;
      ac.thm.disabled = false;
      ac.ths.disabled = false;
      ac.tset.disabled = false;
      ac.treset.disabled = true;
    }


  };
  // INIT - RUN ALARM CLOCK
  window.addEventListener("load", ac.init);

  /*---------------------------------------------------------
  Fonction qui écourte si des intéractions sont faites dans
  le document
  ---------------------------------------------------------*/
  document.addEventListener('DOMContentLoaded', function () {
    var wakeup = document.querySelector('input[name=wakeUP]');

    wakeup.addEventListener('change', function () {
      if(wakeup.checked){
        connection.send("AO");
        document.cookie = "wakeOn=on; expires=Thu, 18 Dec 2020 12:00:00 UTC";
      }else{
        connection.send("AF");
        document.cookie = "wakeOn=off; expires=Thu, 18 Dec 2020 12:00:00 UTC";
      }
    });
  });
  
  init();
  /*Fonction d'initialisation*/