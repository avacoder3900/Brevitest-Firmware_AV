-- ==========================================================================
-- CouchDB to Supabase Migration - Sample / Template
-- Generated: 2026-02-17
--
-- This file demonstrates the mapping from CouchDB cartridge documents
-- to PostgreSQL rows. Replace/extend the VALUES with your actual data,
-- or use export_couchdb_to_sql.js to generate this file automatically.
-- ==========================================================================

-- Prerequisites:
--   1. Run 001_add_cartridge_research_fields.sql first
--   2. This script creates stub assay rows if they don't exist

-- Status mapping applied:
--   CouchDB "completed" -> PostgreSQL "used"
--   CouchDB "unused"    -> PostgreSQL "unused"
--   CouchDB "validated"  -> PostgreSQL "validated"
--   CouchDB "expired"    -> PostgreSQL "expired"
--   CouchDB "invalid"    -> PostgreSQL "invalid"

-- ==========================================================================
-- STEP 1: Ensure assays exist (stub rows for FK constraint)
-- NOTE: Update BCODE, duration, and checksum with real values after migration
-- ==========================================================================

INSERT INTO assays (assay_id, name, description, duration, bcode, bcode_length, checksum)
VALUES (
    'A9356EB6',
    'EXP 231 - Cortisol Test - Control BCode, code c',
    'Migrated from CouchDB - BCODE needs to be populated',
    0,
    '\x00',
    0,
    0
)
ON CONFLICT (assay_id) DO NOTHING;

-- ==========================================================================
-- STEP 2: Insert cartridges
-- ==========================================================================

-- Field mapping reference:
--   CouchDB _id           -> cartridge_uuid
--   CouchDB assayId       -> assay_id
--   CouchDB status        -> status (with value mapping)
--   CouchDB folderId      -> lot_number
--   CouchDB expirationDate -> expiration_date (date only)
--   CouchDB serialNumber  -> serial_number
--   CouchDB siteId        -> site_id
--   CouchDB program       -> program
--   CouchDB experiment    -> experiment
--   CouchDB arm           -> arm
--   CouchDB quantity      -> quantity
--   CouchDB validationErrors -> validation_errors (JSONB)
--   CouchDB statusUpdatedOn -> status_updated_at

INSERT INTO cartridges (
    cartridge_uuid,
    assay_id,
    status,
    lot_number,
    expiration_date,
    serial_number,
    site_id,
    program,
    experiment,
    arm,
    quantity,
    validation_errors,
    status_updated_at
) VALUES
(
    '00592c52-cc9c-44e4-af03-cf6fb430590a',
    'A9356EB6',
    'used',                                     -- CouchDB "completed" -> "used"
    '354484393616',                             -- CouchDB folderId
    '2030-02-16',                               -- CouchDB expirationDate (date only)
    'A9356EB6-35448439361-056',                 -- CouchDB serialNumber
    'research',                                 -- CouchDB siteId
    'Fluorescence Platform',                    -- CouchDB program
    '176 - 12_05_25 - Exploration - Running new optimized mixing times in black cartridge and testing putting the tracer in the diluent to mix sample and tracer at the same time',
    'Exp 236: SPAAC chemistry',                 -- CouchDB arm
    30,                                         -- CouchDB quantity
    '[]'::jsonb,                                -- CouchDB validationErrors
    '2026-02-16T23:41:17.634Z'                  -- CouchDB statusUpdatedOn
)
-- Add more cartridge rows here, separated by commas:
-- , (
--     'next-cartridge-uuid-here',
--     'ASSAYID2',
--     'unused',
--     ...
-- )
ON CONFLICT (cartridge_uuid) DO UPDATE SET
    assay_id = EXCLUDED.assay_id,
    status = EXCLUDED.status,
    lot_number = EXCLUDED.lot_number,
    expiration_date = EXCLUDED.expiration_date,
    serial_number = EXCLUDED.serial_number,
    site_id = EXCLUDED.site_id,
    program = EXCLUDED.program,
    experiment = EXCLUDED.experiment,
    arm = EXCLUDED.arm,
    quantity = EXCLUDED.quantity,
    validation_errors = EXCLUDED.validation_errors,
    status_updated_at = EXCLUDED.status_updated_at;

-- ==========================================================================
-- STEP 3: Verification queries
-- ==========================================================================

-- Count migrated cartridges
SELECT COUNT(*) AS total_cartridges FROM cartridges WHERE serial_number IS NOT NULL;

-- Verify no "completed" status leaked through
SELECT COUNT(*) AS unmapped_completed FROM cartridges WHERE status = 'completed';

-- Status distribution
SELECT status, COUNT(*) FROM cartridges GROUP BY status ORDER BY status;

-- FK integrity check (should return 0 rows)
SELECT c.cartridge_uuid, c.assay_id
FROM cartridges c
LEFT JOIN assays a ON c.assay_id = a.assay_id
WHERE a.assay_id IS NULL AND c.assay_id IS NOT NULL;

-- Spot-check the sample record
SELECT
    cartridge_uuid,
    serial_number,
    assay_id,
    status,
    site_id,
    program,
    arm,
    quantity,
    lot_number,
    expiration_date,
    status_updated_at
FROM cartridges
WHERE cartridge_uuid = '00592c52-cc9c-44e4-af03-cf6fb430590a';
