const https = require('https');
require('dotenv').config();
const KEY = process.env.SUPABASE_SERVICE_ROLE_KEY || '';

function supabasePost(table, data) {
  return new Promise(function(resolve, reject) {
    var body = JSON.stringify(data);
    var options = {
      hostname: (process.env.SUPABASE_URL || '').replace('https://', ''),
      port: 443,
      path: '/rest/v1/' + table,
      method: 'POST',
      headers: {
        'apikey': KEY,
        'Authorization': 'Bearer ' + KEY,
        'Content-Type': 'application/json',
        'Prefer': 'resolution=merge-duplicates,return=representation',
        'Content-Length': Buffer.byteLength(body)
      }
    };
    var req = https.request(options, function(res) {
      var respData = '';
      res.on('data', function(c) { respData += c; });
      res.on('end', function() { resolve({ status: res.statusCode, data: respData }); });
    });
    req.on('error', reject);
    req.write(body);
    req.end();
  });
}

async function main() {
  // Register the device
  var device = {
    device_id: '0a10aced202194944a071af0',
    api_key: process.env.SUPABASE_ANON_KEY || '',
    firmware_version: 200,
    data_format_version: 40,
    metadata: {
      name: 'BT-M01-0000-0228',
      platform: 'M-SoM',
      device_os: '6.3.4'
    }
  };

  console.log('Registering device: ' + device.device_id);
  var result = await supabasePost('devices', device);
  console.log('Status: ' + result.status);
  console.log('Response: ' + result.data);

  if (result.status >= 200 && result.status < 300) {
    console.log('\nDevice registered successfully!');
  } else {
    console.log('\nFailed to register device');
  }
}
main();
