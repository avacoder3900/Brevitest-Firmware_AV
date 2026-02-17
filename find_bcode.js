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
  var dbs = ['master', 'research', 'production', 'laboratory', 'development', 'nicholas-research-testing'];

  for (var i = 0; i < dbs.length; i++) {
    var db = dbs[i];
    try {
      var result = await fetchCouch('/' + db + '/_all_docs?include_docs=true');
      var docs = (result.rows || []).map(function(r) { return r.doc; }).filter(function(d) { return d; });

      // Find assay docs or docs with BCODE
      var assays = docs.filter(function(d) {
        return d.schema === 'assay' || d.BCODE || d.bcode || d.type === 'assay';
      });

      if (assays.length > 0) {
        console.log('\n=== ' + db + ': ' + assays.length + ' assay docs ===');
        assays.forEach(function(a) {
          var bcode = a.BCODE || a.bcode || '';
          console.log('  ID: ' + a._id);
          console.log('  Name: ' + (a.name || a.assayName || 'N/A'));
          console.log('  Schema: ' + (a.schema || 'N/A'));
          console.log('  BCODE length: ' + bcode.length);
          if (bcode.length > 0) {
            console.log('  BCODE preview: ' + bcode.substring(0, 120));
          }
          console.log('  Keys: ' + Object.keys(a).filter(function(k) { return k[0] !== '_'; }).join(', '));
          console.log('');
        });
      } else {
        console.log('\n' + db + ': no assay docs found');
      }
    } catch(e) {
      console.log('\n' + db + ': error - ' + e.message);
    }
  }
}
main();
