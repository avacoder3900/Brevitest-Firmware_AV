/**
 * @file migrate_couchdb_to_supabase.js
 * @brief Migrates all cartridge documents from CouchDB to Supabase PostgreSQL
 *
 * Usage: node migrate_couchdb_to_supabase.js
 *
 * Migrates from 4 CouchDB databases:
 *   - research, nicholas-research-testing, production, laboratory
 *
 * Status mapping:
 *   completed → used
 *   cancelled → cancelled
 *   linked → validated
 *   underway → validated
 *   scrapped → invalid
 *   cooled/packaged/received/refrigerated/released/sleeved → unused
 */

const https = require('https');
const http = require('http');
require('dotenv').config();

// ============================================================================
// CONFIGURATION
// ============================================================================

const COUCHDB_HOST = (process.env.COUCHDB_URL || '').replace('https://', '').replace(/:\d+$/, '') || 'localhost';
const COUCHDB_PORT = parseInt((process.env.COUCHDB_URL || '').match(/:(\d+)$/)?.[1] || '6984');
const COUCHDB_AUTH = Buffer.from((process.env.COUCHDB_USER || '') + ':' + (process.env.COUCHDB_PASSWORD || '')).toString('base64');

const SUPABASE_URL = (process.env.SUPABASE_URL || '').replace('https://', '');
const SUPABASE_KEY = process.env.SUPABASE_SERVICE_ROLE_KEY || '';

const DATABASES = ['research', 'nicholas-research-testing', 'production', 'laboratory'];

const BATCH_SIZE = 100; // Insert this many at a time

// ============================================================================
// STATUS MAPPING
// ============================================================================

const STATUS_MAP = {
  'completed': 'used',
  'cancelled': 'cancelled',
  'linked': 'validated',
  'underway': 'validated',
  'scrapped': 'invalid',
  'cooled': 'unused',
  'packaged': 'unused',
  'received': 'unused',
  'refrigerated': 'unused',
  'released': 'unused',
  'sleeved': 'unused',
  'unused': 'unused',
  'validated': 'validated',
  'expired': 'expired',
  'invalid': 'invalid',
  'used': 'used'
};

function mapStatus(status) {
  return STATUS_MAP[status] || status;
}

// ============================================================================
// COUCHDB FETCH
// ============================================================================

function fetchCouchDB(path) {
  return new Promise(function(resolve, reject) {
    var options = {
      hostname: COUCHDB_HOST,
      port: COUCHDB_PORT,
      path: path,
      method: 'GET',
      headers: {
        'Authorization': 'Basic ' + COUCHDB_AUTH
      },
      rejectUnauthorized: false
    };
    var req = https.request(options, function(res) {
      var data = '';
      res.on('data', function(c) { data += c; });
      res.on('end', function() {
        try { resolve(JSON.parse(data)); }
        catch(e) { reject(new Error('JSON parse error: ' + data.substring(0, 200))); }
      });
    });
    req.on('error', reject);
    req.end();
  });
}

// ============================================================================
// SUPABASE INSERT
// ============================================================================

function supabasePost(table, data) {
  return new Promise(function(resolve, reject) {
    var body = JSON.stringify(data);
    var options = {
      hostname: SUPABASE_URL,
      port: 443,
      path: '/rest/v1/' + table,
      method: 'POST',
      headers: {
        'apikey': SUPABASE_KEY,
        'Authorization': 'Bearer ' + SUPABASE_KEY,
        'Content-Type': 'application/json',
        'Prefer': 'resolution=merge-duplicates,return=minimal',
        'Content-Length': Buffer.byteLength(body)
      }
    };
    var req = https.request(options, function(res) {
      var respData = '';
      res.on('data', function(c) { respData += c; });
      res.on('end', function() {
        if (res.statusCode >= 200 && res.statusCode < 300) {
          resolve({ status: res.statusCode, data: respData });
        } else {
          reject(new Error('Supabase ' + res.statusCode + ': ' + respData));
        }
      });
    });
    req.on('error', reject);
    req.write(body);
    req.end();
  });
}

// ============================================================================
// TRANSFORM COUCHDB DOC TO SUPABASE ROW
// ============================================================================

function transformCartridge(doc, sourceDb) {
  var row = {
    cartridge_uuid: (doc._id || '').trim(),
    assay_id: doc.assayId || null,
    status: mapStatus(doc.status || 'unused'),
    lot_number: doc.folderId || null,
    expiration_date: doc.expirationDate ? doc.expirationDate.split('T')[0] : null,
    serial_number: doc.serialNumber || null,
    site_id: doc.siteId || doc.department || sourceDb,
    program: doc.program || null,
    experiment: doc.experiment || null,
    arm: doc.arm || null,
    quantity: (doc.quantity != null) ? doc.quantity : null,
    validation_errors: doc.validationErrors || [],
    status_updated_at: doc.statusUpdatedOn || null,
    metadata: {
      source_db: sourceDb,
      couch_rev: doc._rev,
      original_status: doc.status,
      assay_name: doc.assayName || null
    }
  };

  // Skip docs with empty or invalid cartridge_uuid
  if (!row.cartridge_uuid || row.cartridge_uuid.length < 10) {
    return null;
  }

  return row;
}

function transformAssay(assayId, assayName) {
  return {
    assay_id: assayId,
    name: assayName || 'Migrated from CouchDB',
    description: 'Auto-migrated - BCODE needs to be populated',
    duration: 0,
    bcode: '\\x00',
    bcode_length: 0,
    checksum: 0,
    is_active: true
  };
}

// ============================================================================
// BATCH INSERT WITH RETRY
// ============================================================================

async function insertBatch(table, rows, retries) {
  retries = retries || 3;
  for (var attempt = 1; attempt <= retries; attempt++) {
    try {
      await supabasePost(table, rows);
      return true;
    } catch(e) {
      if (attempt === retries) {
        console.error('  FAILED after ' + retries + ' attempts: ' + e.message);
        return false;
      }
      console.error('  Retry ' + attempt + '/' + retries + ': ' + e.message);
      await new Promise(function(r) { setTimeout(r, 2000 * attempt); });
    }
  }
  return false;
}

// ============================================================================
// MAIN MIGRATION
// ============================================================================

async function main() {
  console.log('=== CouchDB to Supabase Migration ===');
  console.log('Started: ' + new Date().toISOString());
  console.log('');

  // First verify Supabase is accessible
  try {
    var testResult = await new Promise(function(resolve, reject) {
      var options = {
        hostname: SUPABASE_URL,
        port: 443,
        path: '/rest/v1/cartridges?select=id&limit=1',
        method: 'GET',
        headers: {
          'apikey': SUPABASE_KEY,
          'Authorization': 'Bearer ' + SUPABASE_KEY
        }
      };
      var req = https.request(options, function(res) {
        var data = '';
        res.on('data', function(c) { data += c; });
        res.on('end', function() { resolve({ status: res.statusCode, data: data }); });
      });
      req.on('error', reject);
      req.end();
    });

    if (testResult.status === 200) {
      console.log('Supabase connection: OK (cartridges table exists)');
    } else {
      console.error('Supabase error: ' + testResult.status + ' ' + testResult.data);
      console.error('');
      console.error('Have you run deploy.sql in Supabase SQL Editor?');
      process.exit(1);
    }
  } catch(e) {
    console.error('Cannot connect to Supabase: ' + e.message);
    process.exit(1);
  }

  // Collect all cartridges from all databases
  var allCartridges = [];
  var assayMap = new Map();

  for (var i = 0; i < DATABASES.length; i++) {
    var db = DATABASES[i];
    console.log('\nFetching from CouchDB: ' + db + '...');

    var result = await fetchCouchDB('/' + db + '/_all_docs?include_docs=true');
    var docs = (result.rows || []).map(function(r) { return r.doc; }).filter(function(d) { return d; });
    var cartridges = docs.filter(function(d) { return d.schema === 'cartridge'; });

    console.log('  Total docs: ' + docs.length + ', Cartridges: ' + cartridges.length);

    for (var j = 0; j < cartridges.length; j++) {
      var doc = cartridges[j];
      var row = transformCartridge(doc, db);
      if (row) {
        allCartridges.push(row);
        // Track unique assays
        if (doc.assayId && !assayMap.has(doc.assayId)) {
          assayMap.set(doc.assayId, doc.assayName || null);
        }
      }
    }
  }

  console.log('\n=== Migration Summary ===');
  console.log('Total cartridges to migrate: ' + allCartridges.length);
  console.log('Unique assays: ' + assayMap.size);

  // Deduplicate by cartridge_uuid (keep latest by status_updated_at)
  var uuidMap = new Map();
  allCartridges.forEach(function(c) {
    var existing = uuidMap.get(c.cartridge_uuid);
    if (!existing) {
      uuidMap.set(c.cartridge_uuid, c);
    } else {
      // Keep the one with the later status update
      var existingTime = existing.status_updated_at ? new Date(existing.status_updated_at).getTime() : 0;
      var newTime = c.status_updated_at ? new Date(c.status_updated_at).getTime() : 0;
      if (newTime > existingTime) {
        uuidMap.set(c.cartridge_uuid, c);
      }
    }
  });

  var dedupedCartridges = Array.from(uuidMap.values());
  console.log('After deduplication: ' + dedupedCartridges.length);

  // Status distribution
  var statusCounts = {};
  dedupedCartridges.forEach(function(c) {
    statusCounts[c.status] = (statusCounts[c.status] || 0) + 1;
  });
  console.log('\nStatus distribution (mapped):');
  Object.keys(statusCounts).sort().forEach(function(s) {
    console.log('  ' + s + ': ' + statusCounts[s]);
  });

  // Step 1: Insert assay stubs
  console.log('\n--- Step 1: Inserting ' + assayMap.size + ' assay stubs ---');
  var assayRows = [];
  assayMap.forEach(function(name, id) {
    assayRows.push(transformAssay(id, name));
  });

  if (assayRows.length > 0) {
    for (var b = 0; b < assayRows.length; b += BATCH_SIZE) {
      var batch = assayRows.slice(b, b + BATCH_SIZE);
      var ok = await insertBatch('assays', batch);
      if (ok) {
        process.stdout.write('  Assays: ' + Math.min(b + BATCH_SIZE, assayRows.length) + '/' + assayRows.length + '\r');
      }
    }
    console.log('  Assays: ' + assayRows.length + '/' + assayRows.length + ' - DONE');
  }

  // Step 2: Insert cartridges
  console.log('\n--- Step 2: Inserting ' + dedupedCartridges.length + ' cartridges ---');

  var successCount = 0;
  var failCount = 0;

  for (var b = 0; b < dedupedCartridges.length; b += BATCH_SIZE) {
    var batch = dedupedCartridges.slice(b, b + BATCH_SIZE);
    var ok = await insertBatch('cartridges', batch);
    if (ok) {
      successCount += batch.length;
    } else {
      // Try one-by-one for failed batch
      for (var k = 0; k < batch.length; k++) {
        try {
          await supabasePost('cartridges', [batch[k]]);
          successCount++;
        } catch(e) {
          failCount++;
          console.error('  Skip: ' + batch[k].cartridge_uuid + ' - ' + e.message.substring(0, 100));
        }
      }
    }
    process.stdout.write('  Cartridges: ' + (successCount + failCount) + '/' + dedupedCartridges.length + ' (ok: ' + successCount + ', fail: ' + failCount + ')\r');
  }

  console.log('\n\n=== Migration Complete ===');
  console.log('Inserted: ' + successCount);
  console.log('Failed: ' + failCount);
  console.log('Finished: ' + new Date().toISOString());
}

main().catch(function(err) {
  console.error('Fatal error:', err);
  process.exit(1);
});
