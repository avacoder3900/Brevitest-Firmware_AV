const https = require('https');
require('dotenv').config();

function fetchCouch(path) {
  return new Promise(function(resolve, reject) {
    var options = {
      hostname: (process.env.COUCHDB_URL || '').replace('https://', '').replace(/:\d+$/, ''),
      port: parseInt((process.env.COUCHDB_URL || '').match(/:(\d+)$/)?.[1] || '6984'),
      path: path,
      method: 'GET',
      headers: { 'Authorization': 'Basic ' + Buffer.from((process.env.COUCHDB_USER || '') + ':' + (process.env.COUCHDB_PASSWORD || '')).toString('base64') },
      rejectUnauthorized: false
    };
    var req = https.request(options, function(res) {
      var data = '';
      res.on('data', function(c) { data += c; });
      res.on('end', function() { resolve(JSON.parse(data)); });
    });
    req.on('error', reject);
    req.end();
  });
}

async function main() {
  // Get a few assay docs from development db
  var result = await fetchCouch('/development/_all_docs?include_docs=true&limit=500');
  var docs = (result.rows || []).map(function(r) { return r.doc; }).filter(function(d) { return d && d.schema === 'assay'; });

  console.log('Total assays in development: ' + docs.length);

  // Inspect first 3 assays' BCODE field
  for (var i = 0; i < Math.min(3, docs.length); i++) {
    var doc = docs[i];
    console.log('\n=== ' + doc._id + ': ' + doc.name + ' ===');
    console.log('Type of BCODE: ' + typeof doc.BCODE);
    console.log('Duration: ' + doc.duration);

    if (typeof doc.BCODE === 'object' && doc.BCODE !== null) {
      if (Array.isArray(doc.BCODE)) {
        console.log('BCODE is array, length: ' + doc.BCODE.length);
        console.log('First 3 entries: ' + JSON.stringify(doc.BCODE.slice(0, 3)));
      } else {
        console.log('BCODE keys: ' + Object.keys(doc.BCODE).join(', '));
        console.log('BCODE sample: ' + JSON.stringify(doc.BCODE).substring(0, 300));
      }
    } else if (typeof doc.BCODE === 'string') {
      console.log('BCODE string length: ' + doc.BCODE.length);
      console.log('BCODE preview: ' + doc.BCODE.substring(0, 200));
    }
  }

  // Also check nicholas-research-testing for the assay matching our example (A9356EB6)
  console.log('\n\n=== Looking for assay A9356EB6 ===');
  var dbs = ['development', 'nicholas-research-testing', 'research'];
  for (var d = 0; d < dbs.length; d++) {
    try {
      var r = await fetchCouch('/' + dbs[d] + '/A9356EB6');
      if (r && !r.error) {
        console.log('\nFound in ' + dbs[d] + ':');
        console.log('Name: ' + r.name);
        console.log('Schema: ' + r.schema);
        console.log('Type of BCODE: ' + typeof r.BCODE);
        if (r.BCODE) {
          var bcodeStr = typeof r.BCODE === 'string' ? r.BCODE : JSON.stringify(r.BCODE);
          console.log('BCODE length: ' + bcodeStr.length);
          console.log('BCODE preview: ' + bcodeStr.substring(0, 300));
        }
        console.log('Duration: ' + r.duration);
      }
    } catch(e) {}
  }
}
main();
