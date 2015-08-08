function getMyLocation(callback) {
    var options = {
        enableHighAccuracy: true,
        timeout: 5000,
        maximumAge: 0
    };

    function success(pos) {
        callback(pos.coords);
    }

    function error(err) {
      console.warn('ERROR(' + err.code + '): ' + err.message);
    }

    navigator.geolocation.getCurrentPosition(success, error, options);
}
function constructUrl(lat, lng) {
    return 'http://chrisbdev.com/api/location/GetAngleToNearestPoint?lat=' + lat + '&lng=' + lng + '&facingAngle=0';
}

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function getNearestBikePointInfo(lat, lng, callback) {
    var url = constructUrl(lat, lng);
    console.log('gettin data from ' + url);
      xhrRequest(
          url,
          'GET',
          function(data) {
              console.log(data);
              callback(JSON.parse(data));
          }
      );
}



function getLocationDataForUserAndBike() {
   getMyLocation(function(pos) {
     
        getNearestBikePointInfo(pos.latitude, pos.longitude, function(bikeLocationData) {
          
          console.log('Got users pos, getting bike loc'); 
          
          var angleFound = bikeLocationData.Angle;
          angleFound = angleFound < 0 ? angleFound + 360 : angleFound;
          
          var dictionary = {
            'KEY_BIKENAME': bikeLocationData.Name,
            'KEY_ANGLE': angleFound
          };
          
          console.log('About to send data back with angle ' + parseInt(angleFound,  10));
          
          // Send to Pebble
          Pebble.sendAppMessage(dictionary,
            function(e) {
              console.log('Info sent to Pebble successfully!');
            },
            function(e) {
              console.log('Error sending weather info to Pebble!');
            }
          );
          
      });
    });
}

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    getLocationDataForUserAndBike();
  }                     
);
