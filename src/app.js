/**
 * Welcome to Pebble.js!
 *
 * This is where you write your app.
 */

var UI = require('ui');
var Vector2 = require('vector2');

var mainCode = (function () {

  var main = new UI.Window();

  var arrow = new UI.Image({
    position: new Vector2(0, 0),
    size: new Vector2(144,168),
    image: 'images/arrowEast.png'
  });

var textfield = new UI.Text({
 position: new Vector2(0, 50),
 size: new Vector2(144, 168),
 font: 'gothic-18-bold',
 text: 'Gothic 18 Bold'
});
  
  
  main.add(arrow);
main.add(textfield);
  
  main.show();

    function constructUrl(lat, lng, angle) {
        return 'http://chrisbdev.com/api/location/GetAngleToNearestPoint?lat=' + lat + '&lng=' + lng + '&facingAngle=' + angle;
    }

    function httpGet(theUrl, callback) {
        var xmlHttp = new XMLHttpRequest();
        xmlHttp.onreadystatechange = function () {
            if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
                callback(xmlHttp);
            }
        };
        xmlHttp.open("GET", theUrl, true);
        xmlHttp.send(null);
    }

    function getMyLocation(callback) {
        var options = {
            enableHighAccuracy: true,
            timeout: 5000,
            maximumAge: 0
        };

        function success(pos) {
            var crd = pos.coords;
            callback(crd);
        }

        function error(err) {
            console.warn('ERROR(' + err.code + '): ' + err.message);
        }

        navigator.geolocation.getCurrentPosition(success, error, options);
    }

    function getNearestBikePointInfo(lat, lng, currentAngleFromNorth, callback) {
        httpGet(
            constructUrl(lat, lng, currentAngleFromNorth),
            function(data) {
                callback(JSON.parse(data.response));
            }
            );
    }

    function rotateUIArrow(deg) {
      arrow.image('images/arrowNorth.png')
        //uiArrowEl.style.transform = "rotate(" + deg + "deg)";
        //uiArrowEl.style.transformOrigin = "0";
        //main.title(deg);
    }

    function updateLocationName(name) {
        //uiNameEl.innerHTML = name;
      var maxLength = 15;
      if (name.length > maxLength) {
        name = name.substring(0, maxLength) + "...";
      }
      textfield.text(name);
    }

    function updateDirectionInfo() {
        getMyLocation(function (location) {
            getNearestBikePointInfo(location.latitude, location.longitude, 30,
                function (response) {
                    rotateUIArrow(response.Angle);
                    updateLocationName(response.Name);
                });
        });
    }
    
    function init() {
        main.on('click', function () {
            updateLocationName('Loading...');
            updateDirectionInfo(); 
        });
    }

    init();

})();