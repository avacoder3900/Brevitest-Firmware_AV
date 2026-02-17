const https = require('https');
require('dotenv').config();
var KEY = process.env.SUPABASE_SERVICE_ROLE_KEY || '';

function supaGet(path) {
  return new Promise(function(resolve, reject) {
    var options = {
      hostname: (process.env.SUPABASE_URL || '').replace('https://', ''),
      port: 443,
      path: '/rest/v1/' + path,
      method: 'GET',
      headers: { 'apikey': KEY, 'Authorization': 'Bearer ' + KEY }
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
  // Count total assays
  var all = await supaGet('assays?select=assay_id');
  console.log('Total assays in Supabase: ' + all.length);

  // Count those with BCODE
  var withBcode = await supaGet('assays?select=assay_id,bcode_length&bcode_length=gt.0');
  console.log('Assays with compiled BCODE: ' + withBcode.length);

  // Show the target assay A9356EB6 (Cortisol Test from user example)
  var target = await supaGet('assays?assay_id=eq.A9356EB6&select=assay_id,name,bcode_length,checksum,duration,is_active');
  console.log('\nTarget assay A9356EB6:');
  console.log(JSON.stringify(target, null, 2));

  // Show a few samples
  var samples = await supaGet('assays?select=assay_id,name,bcode_length,checksum,duration&bcode_length=gt.0&limit=5');
  console.log('\nSample assays with BCODE:');
  samples.forEach(function(r) {
    console.log('  ' + r.assay_id + ' | ' + r.name + ' | len=' + r.bcode_length + ' | dur=' + r.duration + 's | crc=' + r.checksum);
  });
}
main().catch(function(e) { console.error(e); });
