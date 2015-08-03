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
function getNearestBikePointInfo(lat, lng, facingAngle, callback) {
//           callback({lat: 52, lng: 0});
    var url = constructUrl(lat, lng, facingAngle);
  console.log('gettin data from ' + url);
    httpGet(
        url,
        function(data) {
//           callback({lat: 52, lng: 0});
            callback(JSON.parse(data.response));
        }
    );
}



function getLocationDataForUserAndBike(facingAngle) {
   getMyLocation(function(pos) {
       getNearestBikePointInfo(pos.latitude, pos.longitude, facingAngle, function(bikeLocationData) {
    console.log('Got users pos, getting bike loc'); 
//    getNearestBikePointInfo(51.534098051, -0.07666800, facingAngle, function(bikeLocationData) {
      
      console.log('Got bike data ' + JSON.stringify(bikeLocationData));
//       var dictionary = {
//         'KEY_BIKENAME': "BIKE LOC TEST 1",
//         'KEY_ANGLE': 90
//       };
      var dictionary = {
        'KEY_BIKENAME': bikeLocationData.Name,
        'KEY_ANGLE': bikeLocationData.Angle
      };
      console.log('About to send data back');
      
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
    console.log('AppMessage received!');
    console.log('payload -- ' + JSON.stringify(e.payload));
    console.log('angle val-- ' + e.payload["0"]);
    getLocationDataForUserAndBike(JSON.stringify(e.payload["0"]));
  }                     
);

// Listen for when the watchface is opened
// Pebble.addEventListener('ready', 
//   function(e) {
//     console.log('PebbleKit JS ready!');
//     getLocationDataForUserAndBike(JSON.stringify(e.payload));
//   }
// );
