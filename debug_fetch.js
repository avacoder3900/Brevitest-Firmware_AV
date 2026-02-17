const https = require('https');
require('dotenv').config();
var COUCH_AUTH = Buffer.from((process.env.COUCHDB_USER || '') + ':' + (process.env.COUCHDB_PASSWORD || '')).toString('base64');

function fetchCouch(path) {
  return new Promise(function(resolve, reject) {
    var options = {
      hostname: (process.env.COUCHDB_URL || '').replace('https://', '').replace(/:\d+$/, ''),
      port: parseInt((process.env.COUCHDB_URL || '').match(/:(\d+)$/)?.[1] || '6984'),
      path: path,
      method: 'GET',
      headers: { 'Authorization': 'Basic ' + COUCH_AUTH },
      rejectUnauthorized: false
    };
    var req = https.request(options, function(res) {
      var data = '';
      res.on('data', function(c) { data += c; });
      res.on('end', function() { resolve({ status: res.statusCode, raw: data }); });
    });
    req.on('error', reject);
    req.end();
  });
}

async function main() {
  var r = await fetchCouch('/development/_all_docs?include_docs=true&limit=3');
  console.log('Status: ' + r.status);
  console.log('Response length: ' + r.raw.length);
  console.log('First 500 chars:');
  console.log(r.raw.substring(0, 500));
}
main().catch(function(e) { console.error(e); });
