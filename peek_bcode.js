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
  // Fetch a few assays to see all command name variants
  var ids = [
    { db: 'nicholas-research-testing', id: 'A9356EB6' },
    { db: 'nicholas-research-testing', id: 'A9DA2513' },
    { db: 'development', id: 'A4D8B4CB' },
    { db: 'development', id: 'A9F2DAAB' }
  ];

  // Also collect ALL unique command names across development db
  var result = await fetchCouch('/development/_all_docs?include_docs=true&limit=500');
  var assays = (result.rows || []).map(function(r) { return r.doc; }).filter(function(d) { return d && d.schema === 'assay' && d.BCODE; });

  var commandSet = {};
  var paramKeySet = {};
  assays.forEach(function(a) {
    var bcode = a.BCODE;
    if (typeof bcode === 'object' && bcode.code && Array.isArray(bcode.code)) {
      bcode.code.forEach(function(instr) {
        commandSet[instr.command] = (commandSet[instr.command] || 0) + 1;
        if (instr.params) {
          Object.keys(instr.params).forEach(function(k) {
            if (k !== 'comment') {
              var key = instr.command + '.' + k;
              paramKeySet[key] = (paramKeySet[key] || 0) + 1;
            }
          });
        }
      });
    } else if (typeof bcode === 'string') {
      commandSet['[RAW_STRING]'] = (commandSet['[RAW_STRING]'] || 0) + 1;
    }
  });

  console.log('=== ALL COMMAND NAMES (development db) ===');
  Object.keys(commandSet).sort().forEach(function(cmd) {
    console.log('  ' + cmd + ': ' + commandSet[cmd] + ' occurrences');
  });

  console.log('\n=== ALL PARAM KEYS ===');
  Object.keys(paramKeySet).sort().forEach(function(k) {
    console.log('  ' + k + ': ' + paramKeySet[k]);
  });

  // Also check nicholas-research-testing
  var result2 = await fetchCouch('/nicholas-research-testing/_all_docs?include_docs=true&limit=500');
  var assays2 = (result2.rows || []).map(function(r) { return r.doc; }).filter(function(d) { return d && d.schema === 'assay' && d.BCODE; });

  var cmdSet2 = {};
  assays2.forEach(function(a) {
    var bcode = a.BCODE;
    if (typeof bcode === 'object' && bcode.code && Array.isArray(bcode.code)) {
      bcode.code.forEach(function(instr) {
        cmdSet2[instr.command] = (cmdSet2[instr.command] || 0) + 1;
      });
    } else if (typeof bcode === 'string') {
      cmdSet2['[RAW_STRING]'] = (cmdSet2['[RAW_STRING]'] || 0) + 1;
    }
  });

  console.log('\n=== ALL COMMAND NAMES (nicholas-research-testing) ===');
  Object.keys(cmdSet2).sort().forEach(function(cmd) {
    console.log('  ' + cmd + ': ' + cmdSet2[cmd] + ' occurrences');
  });

  // Show one sample from nicholas-research-testing
  for (var i = 0; i < ids.length; i++) {
    try {
      var doc = await fetchCouch('/' + ids[i].db + '/' + ids[i].id);
      if (doc && !doc.error && doc.BCODE) {
        console.log('\n=== ' + ids[i].db + '/' + ids[i].id + ': ' + doc.name + ' ===');
        var bcodeStr = JSON.stringify(doc.BCODE);
        if (bcodeStr.length > 500) bcodeStr = bcodeStr.substring(0, 500) + '...';
        console.log(bcodeStr);
      }
    } catch(e) {}
  }
}
main().catch(function(e) { console.error(e); });
