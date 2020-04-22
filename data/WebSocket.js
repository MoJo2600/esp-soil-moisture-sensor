var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);

connection.onopen = function () {
  connection.send('Connect ' + new Date());
};

connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
};

connection.onmessage = function (e) {
  console.log('Server: ', e.data);
  JSONobj = JSON.parse(e.data); 
  $("#MoistureMeter").gaugeMeter({used:JSONobj.moisture});
};

connection.onclose = function () {
  console.log('WebSocket connection closed');
};
