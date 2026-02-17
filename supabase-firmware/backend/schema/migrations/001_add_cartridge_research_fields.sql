-- Migration: 001_add_cartridge_research_fields
-- Purpose: Add dedicated columns for CouchDB cartridge fields
-- Date: 2026-02-17
--
-- Run this against your existing Supabase database via SQL Editor or psql.
-- All columns are NULLable for backward compatibility with existing rows.

-- ============================================================================
-- ADD NEW COLUMNS
-- ============================================================================

ALTER TABLE cartridges ADD COLUMN IF NOT EXISTS serial_number VARCHAR(64);
ALTER TABLE cartridges ADD COLUMN IF NOT EXISTS site_id VARCHAR(64);
ALTER TABLE cartridges ADD COLUMN IF NOT EXISTS program VARCHAR(255);
ALTER TABLE cartridges ADD COLUMN IF NOT EXISTS experiment TEXT;
ALTER TABLE cartridges ADD COLUMN IF NOT EXISTS arm VARCHAR(255);
ALTER TABLE cartridges ADD COLUMN IF NOT EXISTS quantity INTEGER;
ALTER TABLE cartridges ADD COLUMN IF NOT EXISTS validation_errors JSONB DEFAULT '[]';
ALTER TABLE cartridges ADD COLUMN IF NOT EXISTS status_updated_at TIMESTAMP WITH TIME ZONE;

-- ============================================================================
-- ADD INDEXES
-- ============================================================================

CREATE INDEX IF NOT EXISTS idx_cartridges_serial_number ON cartridges(serial_number);
CREATE INDEX IF NOT EXISTS idx_cartridges_site_id ON cartridges(site_id);
CREATE INDEX IF NOT EXISTS idx_cartridges_program ON cartridges(program);

-- ============================================================================
-- ADD TRIGGER: auto-update status_updated_at when status changes
-- ============================================================================

CREATE OR REPLACE FUNCTION update_cartridge_status_timestamp()
RETURNS TRIGGER AS $$
BEGIN
    IF OLD.status IS DISTINCT FROM NEW.status THEN
        NEW.status_updated_at = NOW();
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS update_cartridges_status_timestamp ON cartridges;
CREATE TRIGGER update_cartridges_status_timestamp
    BEFORE UPDATE ON cartridges
    FOR EACH ROW
    EXECUTE FUNCTION update_cartridge_status_timestamp();

-- ============================================================================
-- COLUMN DOCUMENTATION
-- ============================================================================

COMMENT ON COLUMN cartridges.serial_number IS 'Manufacturing serial number (e.g., A9356EB6-35448439361-056)';
COMMENT ON COLUMN cartridges.site_id IS 'Research site identifier (e.g., research)';
COMMENT ON COLUMN cartridges.program IS 'Research program name (e.g., Fluorescence Platform)';
COMMENT ON COLUMN cartridges.experiment IS 'Experiment description (long text)';
COMMENT ON COLUMN cartridges.arm IS 'Experimental arm (e.g., Exp 236: SPAAC chemistry)';
COMMENT ON COLUMN cartridges.quantity IS 'Lot/batch quantity';
COMMENT ON COLUMN cartridges.validation_errors IS 'Array of validation error objects (JSONB)';
COMMENT ON COLUMN cartridges.status_updated_at IS 'Auto-updated timestamp of last status change';
