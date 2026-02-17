/**
 * @file compile_bcode_to_supabase.js
 * @brief Compiles CouchDB assay BCODE (JSON) to firmware pipe-delimited format
 *        and uploads compiled assays to Supabase.
 *
 * Usage: node compile_bcode_to_supabase.js
 *
 * Source: CouchDB 'development' database (assay documents with schema='assay')
 * Target: Supabase 'assays' table
 *
 * BCODE Firmware Format:
 *   Instructions delimited by '|' (ITEM_DELIM)
 *   Opcode and arguments separated by ':' (ATTR_DELIM)
 *   Multiple arguments separated by ',' (ARG_DELIM)
 *   Example: "0:|2:2750,10000|1:5000|99:"
 *
 * CouchDB BCODE JSON Format:
 *   Array of instruction objects: { command: "...", params: {...} }
 *   Stored on assay.BCODE field as an array (all 242 assays use this format)
 *
 * Command Names Found in CouchDB -> Firmware Opcodes:
 *   "Start Test"                          -> 0  (no params)
 *   "Delay"                               -> 1  (delay_ms)
 *   "Move Microns"                        -> 2  (microns, step_delay_us)
 *   "Oscillate Stage"                     -> 3  (microns, step_delay_us, cycles)
 *   "Set Baseline and Read Sensors"       -> 11 (number_of_readings)
 *   "Set Baseline Time"                   -> 11 (number_of_readings)
 *   "Read Baseline"                       -> 11 (samples, defaults to 3)
 *   "Read Test"                           -> 14 (samples, defaults to 3)
 *   "Read Sensors With Parameters"        -> 15 (1, led_power, param, 0)
 *   "Read Sensors With Baseline"          -> 11 (number_of_readings)
 *   "Read Sensors Multiple Times With Pause" -> 20+14+1+21 (repeat block)
 *   "Repeat"                              -> 21 (repeat end marker)
 *   "Finish Test"                         -> 99 (no params)
 */

const https = require('https');

// ============================================================================
// CONFIGURATION
// ============================================================================

require('dotenv').config();

const COUCHDB_HOST = (process.env.COUCHDB_URL || '').replace('https://', '').replace(/:\d+$/, '') || 'localhost';
const COUCHDB_PORT = parseInt((process.env.COUCHDB_URL || '').match(/:(\d+)$/)?.[1] || '6984');
const COUCHDB_AUTH = Buffer.from((process.env.COUCHDB_USER || '') + ':' + (process.env.COUCHDB_PASSWORD || '')).toString('base64');

const SUPABASE_URL = (process.env.SUPABASE_URL || '').replace('https://', '');
const SUPABASE_KEY = process.env.SUPABASE_SERVICE_ROLE_KEY || '';

const BATCH_SIZE = 50;

// ============================================================================
// CRC32 CALCULATION (polynomial 0xEDB88320)
// ============================================================================

var CRC32_TABLE = null;

function buildCRC32Table() {
  CRC32_TABLE = new Uint32Array(256);
  for (var i = 0; i < 256; i++) {
    var c = i;
    for (var j = 0; j < 8; j++) {
      c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
    }
    CRC32_TABLE[i] = c;
  }
}

function crc32(str) {
  if (!CRC32_TABLE) buildCRC32Table();
  var crc = 0xFFFFFFFF;
  for (var i = 0; i < str.length; i++) {
    crc = CRC32_TABLE[(crc ^ str.charCodeAt(i)) & 0xFF] ^ (crc >>> 8);
  }
  return (crc ^ 0xFFFFFFFF) >>> 0;
}

/**
 * Convert unsigned 32-bit CRC to signed 32-bit integer for PostgreSQL INTEGER.
 * PostgreSQL INTEGER is signed (-2147483648 to 2147483647).
 * CRC32 is unsigned (0 to 4294967295).
 * This preserves all 32 bits - just reinterprets the sign bit.
 */
function toSignedInt32(uint32) {
  if (uint32 > 0x7FFFFFFF) {
    return uint32 - 0x100000000;
  }
  return uint32;
}

// ============================================================================
// BCODE COMPILER: CouchDB JSON instructions -> firmware pipe-delimited string
// ============================================================================

/**
 * Compile a single CouchDB BCODE instruction to firmware format.
 * Returns array of firmware instruction strings (some commands expand to multiple).
 */
function compileInstruction(entry, index) {
  var commandName = entry.command || entry.cmd || entry.name;
  var params = entry.params || entry.parameters || entry.args || {};
  var warnings = [];

  if (!commandName) {
    return { instructions: [], warnings: ['Instruction ' + index + ' has no command name'] };
  }

  switch (commandName) {
    // ----- Test lifecycle -----
    case 'Start Test':
      // Opcode 0: no params
      return { instructions: ['0:'], warnings: warnings };

    case 'Finish Test':
    case 'End Test':
      // Opcode 99: no params
      return { instructions: ['99:'], warnings: warnings };

    // ----- Timing -----
    case 'Delay':
      // Opcode 1: milliseconds
      // CouchDB uses 'delay_ms' as param name
      var delayMs = params.delay_ms || params.milliseconds || params.delayMs || params.ms || 0;
      return { instructions: ['1:' + Math.round(Number(delayMs))], warnings: warnings };

    // ----- Stage control -----
    case 'Move Microns':
      // Opcode 2: microns, step_delay_us
      var microns = params.microns || 0;
      var stepDelay = params.step_delay_us || params.stepDelayUs || params.step_delay || 300;
      return { instructions: ['2:' + Math.round(Number(microns)) + ',' + Math.round(Number(stepDelay))], warnings: warnings };

    case 'Oscillate Stage':
    case 'Oscillate':
      // Opcode 3: microns, step_delay_us, cycles
      var oMicrons = params.microns || 0;
      var oStepDelay = params.step_delay_us || params.stepDelayUs || params.step_delay || 350;
      var oCycles = params.cycles || params.count || 10;
      return { instructions: ['3:' + Math.round(Number(oMicrons)) + ',' + Math.round(Number(oStepDelay)) + ',' + Math.round(Number(oCycles))], warnings: warnings };

    // ----- Spectrophotometer configuration -----
    case 'Set Sensor Params':
      // Opcode 10: gain, step, integration
      var gain = params.gain || 7;
      var step = params.step || params.astep || 999;
      var integration = params.integration || params.atime || 49;
      return { instructions: ['10:' + gain + ',' + step + ',' + integration], warnings: warnings };

    // ----- Spectrophotometer readings -----
    case 'Baseline Scans':
    case 'Set Baseline and Read Sensors':
    case 'Set Baseline Time':
    case 'Read Sensors With Baseline':
    case 'Read Baseline':
      // Opcode 11: num_scans
      var bScans = params.num_scans || params.number_of_readings || params.numScans || params.scans || params.samples || 3;
      return { instructions: ['11:' + Math.round(Number(bScans))], warnings: warnings };

    case 'Test Scans':
    case 'Read Test':
      // Opcode 14: num_scans
      var tScans = params.num_scans || params.number_of_readings || params.numScans || params.scans || params.samples || 3;
      return { instructions: ['14:' + Math.round(Number(tScans))], warnings: warnings };

    case 'Sensor Reading':
      // Opcode 15: channel, gain, step, integration
      var srChan = params.channel || 0;
      var srGain = params.gain || 7;
      var srStep = params.step || params.astep || 999;
      var srInt = params.integration || params.atime || 49;
      return { instructions: ['15:' + srChan + ',' + srGain + ',' + srStep + ',' + srInt], warnings: warnings };

    case 'Read Sensors With Parameters':
      // Maps to opcode 15 (Sensor Reading)
      // CouchDB params: { param: 182, led_power: 235 }
      // 'param' maps to astep, 'led_power' maps to gain-like config
      // Use channel=1 (A), gain from led_power, step from param, integration=0
      var rspParam = params.param || 0;
      var rspLedPower = params.led_power || 0;
      return { instructions: ['15:1,' + Math.round(Number(rspLedPower)) + ',' + Math.round(Number(rspParam)) + ',0'], warnings: warnings };

    case 'Continuous Scans':
      // Opcode 16: baseline, start_pos, distance, step_delay
      var csBaseline = params.baseline || 0;
      var csStartPos = params.start_pos || params.startPos || params.start_position || 0;
      var csDistance = params.distance || 0;
      var csStepDelay = params.step_delay || params.stepDelay || params.step_delay_us || 300;
      return { instructions: ['16:' + csBaseline + ',' + csStartPos + ',' + csDistance + ',' + csStepDelay], warnings: warnings };

    // ----- Control flow -----
    case 'Repeat Begin':
      // Opcode 20: iterations
      var iterations = params.iterations || params.count || 1;
      return { instructions: ['20:' + Math.round(Number(iterations))], warnings: warnings };

    case 'Repeat End':
      // Opcode 21: no params
      return { instructions: ['21:'], warnings: warnings };

    case 'Repeat':
      // Nested repeat: { command: "Repeat", count: N, code: [...] }
      // This is handled in compileBCODE, not here. If we get here, it's an error.
      warnings.push('"Repeat" without nested code at instruction ' + index + ' - treating as Repeat End');
      return { instructions: ['21:'], warnings: warnings };

    case 'Read Sensors Multiple Times With Pause':
      // This compound command expands to a repeat block:
      //   20:<number_of_readings> | 14:1 | 1:<pause_ms> | 21:
      // Repeat N times: take 1 test scan, pause
      var rmCount = params.number_of_readings || params.count || 1;
      var rmPause = params.pause_ms || params.pauseMs || params.pause || 1000;
      return {
        instructions: [
          '20:' + Math.round(Number(rmCount)),
          '14:1',
          '1:' + Math.round(Number(rmPause)),
          '21:'
        ],
        warnings: warnings
      };

    default:
      warnings.push('Unknown command "' + commandName + '" at instruction ' + index + ' (skipped)');
      return { instructions: [], warnings: warnings };
  }
}

function compileBCODE(bcodeObj) {
  // If BCODE is already a string, pass through
  if (typeof bcodeObj === 'string') {
    return { compiled: bcodeObj, wasString: true, warnings: [] };
  }

  // If BCODE is not an object, it's invalid
  if (typeof bcodeObj !== 'object' || bcodeObj === null) {
    return { compiled: null, wasString: false, warnings: ['BCODE is not an object or string'] };
  }

  // Get the code array - could be bcodeObj itself (if array) or bcodeObj.code
  var codeArray = null;
  if (Array.isArray(bcodeObj)) {
    codeArray = bcodeObj;
  } else if (bcodeObj.code && Array.isArray(bcodeObj.code)) {
    codeArray = bcodeObj.code;
  } else {
    return { compiled: null, wasString: false, warnings: ['BCODE has no code array'] };
  }

  var allInstructions = [];
  var allWarnings = [];

  for (var i = 0; i < codeArray.length; i++) {
    var entry = codeArray[i];

    // Handle nested Repeat: { command: "Repeat", count: N, code: [...] }
    if (entry.command === 'Repeat' && entry.code && Array.isArray(entry.code)) {
      var repeatCount = entry.count || entry.iterations || 1;
      allInstructions.push('20:' + Math.round(Number(repeatCount)));
      // Recursively compile the inner code array
      var innerResult = compileBCODE(entry.code);
      if (innerResult.compiled) {
        allInstructions.push(innerResult.compiled);
      }
      for (var iw = 0; iw < innerResult.warnings.length; iw++) {
        allWarnings.push(innerResult.warnings[iw]);
      }
      allInstructions.push('21:');
      continue;
    }

    var result = compileInstruction(entry, i);
    for (var j = 0; j < result.instructions.length; j++) {
      allInstructions.push(result.instructions[j]);
    }
    for (var w = 0; w < result.warnings.length; w++) {
      allWarnings.push(result.warnings[w]);
    }
  }

  if (allInstructions.length === 0) {
    return { compiled: null, wasString: false, warnings: allWarnings.concat(['No valid instructions compiled']) };
  }

  // Join with '|' delimiter
  var compiled = allInstructions.join('|');

  return {
    compiled: compiled,
    wasString: false,
    warnings: allWarnings
  };
}

// ============================================================================
// STRING TO HEX (for BYTEA column)
// ============================================================================

function stringToHex(str) {
  var hex = '';
  for (var i = 0; i < str.length; i++) {
    var code = str.charCodeAt(i);
    hex += ('0' + code.toString(16)).slice(-2);
  }
  return '\\x' + hex;
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
// SUPABASE POST (upsert)
// ============================================================================

function supabasePost(table, data) {
  return new Promise(function(resolve, reject) {
    var body = JSON.stringify(data);
    var options = {
      hostname: SUPABASE_URL,
      port: 443,
      path: '/rest/v1/' + table + '?on_conflict=assay_id',
      method: 'POST',
      headers: {
        'apikey': SUPABASE_KEY,
        'Authorization': 'Bearer ' + SUPABASE_KEY,
        'Content-Type': 'application/json',
        'Prefer': 'resolution=merge-duplicates,return=representation',
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
// BATCH INSERT WITH RETRY
// ============================================================================

async function insertBatch(table, rows, retries) {
  retries = retries || 3;
  for (var attempt = 1; attempt <= retries; attempt++) {
    try {
      var result = await supabasePost(table, rows);
      return result;
    } catch(e) {
      if (attempt === retries) {
        console.error('  FAILED after ' + retries + ' attempts: ' + e.message);
        return null;
      }
      console.error('  Retry ' + attempt + '/' + retries + ': ' + e.message);
      await new Promise(function(r) { setTimeout(r, 2000 * attempt); });
    }
  }
  return null;
}

// ============================================================================
// MAIN
// ============================================================================

async function main() {
  console.log('=== BCODE Compiler: CouchDB -> Supabase ===');
  console.log('Started: ' + new Date().toISOString());
  console.log('');

  // -----------------------------------------------------------------------
  // Step 1: Fetch all assay documents from CouchDB databases
  // -----------------------------------------------------------------------
  console.log('--- Step 1: Fetching assays from CouchDB ---');
  var databases = ['development', 'nicholas-research-testing', 'production'];
  var assayDocs = [];
  var seenIds = {};

  for (var d = 0; d < databases.length; d++) {
    var dbName = databases[d];
    try {
      console.log('  Fetching from ' + dbName + '...');
      var result = await fetchCouchDB('/' + dbName + '/_all_docs?include_docs=true&startkey=%22A%22&endkey=%22B%22');
      var docs = (result.rows || []).map(function(r) { return r.doc; })
        .filter(function(doc) { return doc && doc.schema === 'assay'; });
      console.log('  Found ' + docs.length + ' assays in ' + dbName);

      // Add new assays (first db wins for duplicates)
      for (var di = 0; di < docs.length; di++) {
        if (!seenIds[docs[di]._id]) {
          seenIds[docs[di]._id] = true;
          assayDocs.push(docs[di]);
        }
      }
    } catch(e) {
      console.error('  Error fetching ' + dbName + ': ' + e.message);
    }
  }

  console.log('Total unique assays: ' + assayDocs.length);
  console.log('');

  // -----------------------------------------------------------------------
  // Step 2: Compile each assay's BCODE
  // -----------------------------------------------------------------------
  console.log('--- Step 2: Compiling BCODE for each assay ---');

  var compiled = [];
  var skipped = [];
  var compileErrors = [];
  var sampleCount = 0;
  var totalWarnings = 0;

  for (var i = 0; i < assayDocs.length; i++) {
    var doc = assayDocs[i];
    var assayId = doc._id || '';
    var assayName = doc.name || 'Unknown';
    var duration = doc.duration || 0;

    // Check for BCODE field
    if (!doc.BCODE && !doc.bcode) {
      skipped.push({ id: assayId, name: assayName, reason: 'No BCODE field' });
      continue;
    }

    var bcodeRaw = doc.BCODE || doc.bcode;
    var compileResult = compileBCODE(bcodeRaw);

    if (!compileResult.compiled) {
      compileErrors.push({
        id: assayId,
        name: assayName,
        error: compileResult.warnings.join('; ')
      });
      continue;
    }

    var bcodeString = compileResult.compiled;
    var checksumUnsigned = crc32(bcodeString);
    var checksumSigned = toSignedInt32(checksumUnsigned);

    if (compileResult.warnings.length > 0) {
      totalWarnings += compileResult.warnings.length;
    }

    // Print sample for first 3 assays
    if (sampleCount < 3) {
      console.log('');
      console.log('  Sample ' + (sampleCount + 1) + ': ' + assayId + ' (' + assayName + ')');
      console.log('  Type: ' + (compileResult.wasString ? 'already string' : 'compiled from JSON'));
      console.log('  Duration: ' + duration + 's');
      console.log('  BCODE length: ' + bcodeString.length);
      console.log('  Checksum: ' + checksumUnsigned + ' (signed: ' + checksumSigned + ')');
      console.log('  BCODE: ' + bcodeString.substring(0, 200) + (bcodeString.length > 200 ? '...' : ''));
      if (compileResult.warnings.length > 0) {
        console.log('  Warnings: ' + compileResult.warnings.join('; '));
      }
      sampleCount++;
    }

    compiled.push({
      assay_id: assayId,
      name: assayName,
      bcode: stringToHex(bcodeString),
      bcode_length: bcodeString.length,
      checksum: checksumSigned,
      duration: duration,
      is_active: true
    });
  }

  console.log('');
  console.log('Compilation results:');
  console.log('  Successfully compiled: ' + compiled.length);
  console.log('  Skipped (no BCODE): ' + skipped.length);
  console.log('  Compile errors: ' + compileErrors.length);
  console.log('  Total warnings: ' + totalWarnings);

  if (skipped.length > 0) {
    console.log('');
    console.log('Skipped assays:');
    for (var s = 0; s < Math.min(10, skipped.length); s++) {
      console.log('  - ' + skipped[s].id + ' (' + skipped[s].name + '): ' + skipped[s].reason);
    }
    if (skipped.length > 10) {
      console.log('  ... and ' + (skipped.length - 10) + ' more');
    }
  }

  if (compileErrors.length > 0) {
    console.log('');
    console.log('Compile errors:');
    for (var e = 0; e < Math.min(10, compileErrors.length); e++) {
      console.log('  - ' + compileErrors[e].id + ' (' + compileErrors[e].name + '): ' + compileErrors[e].error);
    }
    if (compileErrors.length > 10) {
      console.log('  ... and ' + (compileErrors.length - 10) + ' more');
    }
  }

  if (compiled.length === 0) {
    console.log('');
    console.log('No assays to upload. Exiting.');
    return;
  }

  // -----------------------------------------------------------------------
  // Step 3: Upload compiled assays to Supabase
  // -----------------------------------------------------------------------
  console.log('');
  console.log('--- Step 3: Uploading ' + compiled.length + ' compiled assays to Supabase ---');

  var uploadSuccess = 0;
  var uploadFail = 0;

  for (var b = 0; b < compiled.length; b += BATCH_SIZE) {
    var batch = compiled.slice(b, b + BATCH_SIZE);
    var batchResult = await insertBatch('assays', batch);

    if (batchResult) {
      uploadSuccess += batch.length;
    } else {
      // Try one-by-one for failed batch
      for (var k = 0; k < batch.length; k++) {
        try {
          await supabasePost('assays', [batch[k]]);
          uploadSuccess++;
        } catch(err) {
          uploadFail++;
          console.error('  Failed: ' + batch[k].assay_id + ' - ' + err.message.substring(0, 150));
        }
      }
    }

    process.stdout.write('  Uploaded: ' + (uploadSuccess + uploadFail) + '/' + compiled.length +
      ' (ok: ' + uploadSuccess + ', fail: ' + uploadFail + ')\r');
  }

  console.log('');
  console.log('');
  console.log('=== Upload Complete ===');
  console.log('Successfully uploaded: ' + uploadSuccess);
  console.log('Failed: ' + uploadFail);
  console.log('Finished: ' + new Date().toISOString());
}

main().catch(function(err) {
  console.error('Fatal error:', err);
  process.exit(1);
});
