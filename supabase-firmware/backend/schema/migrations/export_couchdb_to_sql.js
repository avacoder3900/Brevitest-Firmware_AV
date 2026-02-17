/**
 * @file export_couchdb_to_sql.js
 * @brief Export CouchDB cartridge documents to PostgreSQL INSERT statements
 *
 * Usage:
 *   node export_couchdb_to_sql.js <couchdb-url> [output-file]
 *
 * Examples:
 *   node export_couchdb_to_sql.js http://localhost:5984/brevitest
 *   node export_couchdb_to_sql.js http://admin:pass@localhost:5984/brevitest output.sql
 *   node export_couchdb_to_sql.js http://localhost:5984/brevitest > 002_migrate_couchdb_cartridges.sql
 *
 * This script:
 *   1. Fetches all documents from the specified CouchDB database
 *   2. Filters for documents with schema === "cartridge"
 *   3. Maps CouchDB fields to PostgreSQL columns
 *   4. Maps status values ("completed" -> "used")
 *   5. Outputs SQL INSERT statements with ON CONFLICT upsert
 *   6. Also generates stub assay INSERTs for FK constraint satisfaction
 */

const http = require("http");
const https = require("https");
const fs = require("fs");
const url = require("url");

// ============================================================================
// STATUS MAPPING
// ============================================================================

const STATUS_MAP = {
  completed: "used",
  unused: "unused",
  validated: "validated",
  expired: "expired",
  invalid: "invalid",
};

function mapStatus(couchStatus) {
  return STATUS_MAP[couchStatus] || couchStatus;
}

// ============================================================================
// SQL ESCAPING
// ============================================================================

function escapeSQL(value) {
  if (value === null || value === undefined) return "NULL";
  return "'" + String(value).replace(/'/g, "''") + "'";
}

function escapeDate(isoString) {
  if (!isoString) return "NULL";
  // Extract date portion only (YYYY-MM-DD)
  const dateOnly = isoString.split("T")[0];
  return "'" + dateOnly + "'";
}

function escapeTimestamp(isoString) {
  if (!isoString) return "NULL";
  return "'" + isoString + "'";
}

function escapeInt(value) {
  if (value === null || value === undefined) return "NULL";
  const num = parseInt(value, 10);
  return isNaN(num) ? "NULL" : String(num);
}

function escapeJsonb(value) {
  if (value === null || value === undefined) return "'[]'::jsonb";
  return "'" + JSON.stringify(value).replace(/'/g, "''") + "'::jsonb";
}

// ============================================================================
// FETCH COUCHDB DATA
// ============================================================================

function fetchUrl(targetUrl) {
  return new Promise((resolve, reject) => {
    const parsed = new URL(targetUrl);
    const client = parsed.protocol === "https:" ? https : http;

    client
      .get(targetUrl, (res) => {
        let data = "";
        res.on("data", (chunk) => (data += chunk));
        res.on("end", () => {
          if (res.statusCode >= 400) {
            reject(
              new Error(`HTTP ${res.statusCode}: ${data.substring(0, 200)}`)
            );
          } else {
            resolve(JSON.parse(data));
          }
        });
      })
      .on("error", reject);
  });
}

// ============================================================================
// GENERATE SQL
// ============================================================================

function generateCartridgeInsert(doc) {
  const cartridgeUuid = doc._id;
  const assayId = doc.assayId || null;
  const status = mapStatus(doc.status || "unused");
  const lotNumber = doc.folderId || null;
  const expirationDate = doc.expirationDate || null;
  const serialNumber = doc.serialNumber || null;
  const siteId = doc.siteId || null;
  const program = doc.program || null;
  const experiment = doc.experiment || null;
  const arm = doc.arm || null;
  const quantity = doc.quantity != null ? doc.quantity : null;
  const validationErrors = doc.validationErrors || [];
  const statusUpdatedOn = doc.statusUpdatedOn || null;

  return `(
    ${escapeSQL(cartridgeUuid)},
    ${escapeSQL(assayId)},
    ${escapeSQL(status)},
    ${escapeSQL(lotNumber)},
    ${escapeDate(expirationDate)},
    ${escapeSQL(serialNumber)},
    ${escapeSQL(siteId)},
    ${escapeSQL(program)},
    ${escapeSQL(experiment)},
    ${escapeSQL(arm)},
    ${escapeInt(quantity)},
    ${escapeJsonb(validationErrors)},
    ${escapeTimestamp(statusUpdatedOn)}
)`;
}

function generateAssayStub(assayId, assayName) {
  return `INSERT INTO assays (assay_id, name, description, duration, bcode, bcode_length, checksum)
VALUES (${escapeSQL(assayId)}, ${escapeSQL(assayName || "Migrated from CouchDB")}, 'Migrated from CouchDB - BCODE needs to be populated', 0, '\\x00', 0, 0)
ON CONFLICT (assay_id) DO NOTHING;`;
}

// ============================================================================
// MAIN
// ============================================================================

async function main() {
  const args = process.argv.slice(2);

  if (args.length < 1) {
    console.error("Usage: node export_couchdb_to_sql.js <couchdb-url> [output-file]");
    console.error("");
    console.error("Examples:");
    console.error("  node export_couchdb_to_sql.js http://localhost:5984/brevitest");
    console.error("  node export_couchdb_to_sql.js http://admin:pass@localhost:5984/brevitest output.sql");
    process.exit(1);
  }

  const couchUrl = args[0].replace(/\/$/, "");
  const outputFile = args[1] || null;

  console.error(`Fetching documents from: ${couchUrl}`);

  // Fetch all documents with content
  const result = await fetchUrl(
    `${couchUrl}/_all_docs?include_docs=true`
  );

  if (!result.rows) {
    console.error("No rows found in CouchDB response");
    process.exit(1);
  }

  console.error(`Total documents: ${result.rows.length}`);

  // Filter for cartridge documents
  const cartridges = result.rows
    .map((row) => row.doc)
    .filter((doc) => doc && doc.schema === "cartridge");

  console.error(`Cartridge documents: ${cartridges.length}`);

  if (cartridges.length === 0) {
    console.error("No cartridge documents found. Check the database URL and document schema.");
    process.exit(1);
  }

  // Collect unique assays for stub generation
  const assayMap = new Map();
  for (const doc of cartridges) {
    if (doc.assayId && !assayMap.has(doc.assayId)) {
      assayMap.set(doc.assayId, doc.assayName || null);
    }
  }

  // Build SQL output
  const lines = [];

  lines.push("-- ==========================================================================");
  lines.push("-- CouchDB to Supabase Migration");
  lines.push(`-- Generated: ${new Date().toISOString()}`);
  lines.push(`-- Source: ${couchUrl}`);
  lines.push(`-- Total cartridges: ${cartridges.length}`);
  lines.push(`-- Unique assays: ${assayMap.size}`);
  lines.push("-- ==========================================================================");
  lines.push("");
  lines.push("-- Prerequisites:");
  lines.push("--   1. Run 001_add_cartridge_research_fields.sql first");
  lines.push("--   2. This script creates stub assay rows if they don't exist");
  lines.push("");
  lines.push("-- Status mapping applied:");
  lines.push('--   CouchDB "completed" -> PostgreSQL "used"');
  lines.push('--   All other statuses mapped as-is');
  lines.push("");

  // Assay stubs
  lines.push("-- ==========================================================================");
  lines.push("-- STEP 1: Ensure assays exist (stub rows for FK constraint)");
  lines.push("-- NOTE: Update BCODE, duration, and checksum after migration");
  lines.push("-- ==========================================================================");
  lines.push("");

  for (const [assayId, assayName] of assayMap) {
    lines.push(generateAssayStub(assayId, assayName));
  }

  lines.push("");

  // Cartridge inserts
  lines.push("-- ==========================================================================");
  lines.push(`-- STEP 2: Insert ${cartridges.length} cartridges`);
  lines.push("-- ==========================================================================");
  lines.push("");
  lines.push("INSERT INTO cartridges (");
  lines.push("    cartridge_uuid,");
  lines.push("    assay_id,");
  lines.push("    status,");
  lines.push("    lot_number,");
  lines.push("    expiration_date,");
  lines.push("    serial_number,");
  lines.push("    site_id,");
  lines.push("    program,");
  lines.push("    experiment,");
  lines.push("    arm,");
  lines.push("    quantity,");
  lines.push("    validation_errors,");
  lines.push("    status_updated_at");
  lines.push(") VALUES");

  const valueBlocks = cartridges.map((doc) => generateCartridgeInsert(doc));
  lines.push(valueBlocks.join(",\n"));

  lines.push("ON CONFLICT (cartridge_uuid) DO UPDATE SET");
  lines.push("    assay_id = EXCLUDED.assay_id,");
  lines.push("    status = EXCLUDED.status,");
  lines.push("    lot_number = EXCLUDED.lot_number,");
  lines.push("    expiration_date = EXCLUDED.expiration_date,");
  lines.push("    serial_number = EXCLUDED.serial_number,");
  lines.push("    site_id = EXCLUDED.site_id,");
  lines.push("    program = EXCLUDED.program,");
  lines.push("    experiment = EXCLUDED.experiment,");
  lines.push("    arm = EXCLUDED.arm,");
  lines.push("    quantity = EXCLUDED.quantity,");
  lines.push("    validation_errors = EXCLUDED.validation_errors,");
  lines.push("    status_updated_at = EXCLUDED.status_updated_at;");
  lines.push("");

  // Verification queries
  lines.push("-- ==========================================================================");
  lines.push("-- STEP 3: Verification queries");
  lines.push("-- ==========================================================================");
  lines.push("");
  lines.push("-- Count migrated cartridges");
  lines.push("SELECT COUNT(*) AS total_cartridges FROM cartridges WHERE serial_number IS NOT NULL;");
  lines.push("");
  lines.push('-- Verify no "completed" status leaked through');
  lines.push("SELECT COUNT(*) AS unmapped_completed FROM cartridges WHERE status = 'completed';");
  lines.push("");
  lines.push("-- Status distribution");
  lines.push("SELECT status, COUNT(*) FROM cartridges GROUP BY status ORDER BY status;");
  lines.push("");
  lines.push("-- FK integrity check (should return 0 rows)");
  lines.push("SELECT c.cartridge_uuid, c.assay_id");
  lines.push("FROM cartridges c");
  lines.push("LEFT JOIN assays a ON c.assay_id = a.assay_id");
  lines.push("WHERE a.assay_id IS NULL AND c.assay_id IS NOT NULL;");

  const sql = lines.join("\n");

  if (outputFile) {
    fs.writeFileSync(outputFile, sql);
    console.error(`Written to: ${outputFile}`);
  } else {
    console.log(sql);
  }

  // Summary
  console.error("");
  console.error("Migration summary:");
  console.error(`  Cartridges: ${cartridges.length}`);
  console.error(`  Assay stubs: ${assayMap.size}`);

  const statusCounts = {};
  for (const doc of cartridges) {
    const mapped = mapStatus(doc.status || "unused");
    statusCounts[mapped] = (statusCounts[mapped] || 0) + 1;
  }
  console.error("  Status distribution:");
  for (const [status, count] of Object.entries(statusCounts)) {
    console.error(`    ${status}: ${count}`);
  }
}

main().catch((err) => {
  console.error("Error:", err.message);
  process.exit(1);
});
