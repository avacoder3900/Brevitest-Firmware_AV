-- Brevitest Supabase Database Schema
-- Version: 1.0.0
-- Created: 2026-01-29
--
-- This schema replaces the CouchDB document store with PostgreSQL tables
-- designed for the Brevitest medical testing device firmware.

-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- ============================================================================
-- DEVICES TABLE
-- Stores registered Brevitest devices and their authentication credentials
-- ============================================================================
CREATE TABLE IF NOT EXISTS devices (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id VARCHAR(64) UNIQUE NOT NULL,          -- Particle device ID
    api_key VARCHAR(128) NOT NULL,                   -- Device API key for authentication
    firmware_version INTEGER DEFAULT 0,              -- Current firmware version
    data_format_version INTEGER DEFAULT 0,           -- Data format version
    last_seen TIMESTAMP WITH TIME ZONE,              -- Last communication timestamp
    metadata JSONB DEFAULT '{}',                     -- Additional device metadata
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Index for device lookups
CREATE INDEX IF NOT EXISTS idx_devices_device_id ON devices(device_id);
CREATE INDEX IF NOT EXISTS idx_devices_api_key ON devices(api_key);

-- ============================================================================
-- ASSAYS TABLE
-- Stores test assay definitions including BCODE instructions
-- ============================================================================
CREATE TABLE IF NOT EXISTS assays (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    assay_id VARCHAR(9) UNIQUE NOT NULL,            -- 8-char assay identifier
    name VARCHAR(255) NOT NULL,                      -- Human-readable name
    description TEXT,                                -- Assay description
    duration INTEGER NOT NULL,                       -- Test duration in milliseconds
    bcode BYTEA NOT NULL,                           -- BCODE binary instructions (up to 5000 bytes)
    bcode_length INTEGER NOT NULL,                  -- Length of BCODE
    checksum INTEGER NOT NULL,                      -- CRC32 checksum of BCODE
    version INTEGER DEFAULT 1,                      -- Assay version number
    is_active BOOLEAN DEFAULT true,                 -- Whether assay is available for use
    metadata JSONB DEFAULT '{}',                    -- Additional assay metadata
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Index for assay lookups
CREATE INDEX IF NOT EXISTS idx_assays_assay_id ON assays(assay_id);
CREATE INDEX IF NOT EXISTS idx_assays_is_active ON assays(is_active);

-- ============================================================================
-- CARTRIDGES TABLE
-- Stores cartridge information and status
-- ============================================================================
CREATE TABLE IF NOT EXISTS cartridges (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    cartridge_uuid VARCHAR(37) UNIQUE NOT NULL,     -- 36-char UUID from barcode
    assay_id VARCHAR(9) REFERENCES assays(assay_id), -- Associated assay
    status VARCHAR(32) DEFAULT 'unused',            -- unused, validated, used, expired, invalid
    lot_number VARCHAR(64),                         -- Manufacturing lot
    expiration_date DATE,                           -- Cartridge expiration
    validation_count INTEGER DEFAULT 0,             -- Number of validation attempts
    last_validated_at TIMESTAMP WITH TIME ZONE,     -- Last validation timestamp
    last_validated_by UUID REFERENCES devices(id),  -- Device that last validated
    test_result_id UUID,                            -- Link to test result (set after test)
    metadata JSONB DEFAULT '{}',                    -- Additional cartridge metadata
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Indexes for cartridge lookups
CREATE INDEX IF NOT EXISTS idx_cartridges_uuid ON cartridges(cartridge_uuid);
CREATE INDEX IF NOT EXISTS idx_cartridges_status ON cartridges(status);
CREATE INDEX IF NOT EXISTS idx_cartridges_assay_id ON cartridges(assay_id);

-- ============================================================================
-- TEST RESULTS TABLE
-- Stores complete test results including all spectrophotometer readings
-- ============================================================================
CREATE TABLE IF NOT EXISTS test_results (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),

    -- Header fields (matching BrevitestTestRecord)
    data_format_code CHAR(1) NOT NULL DEFAULT 'J', -- Format identifier
    cartridge_uuid VARCHAR(37) NOT NULL,            -- Cartridge UUID
    assay_id VARCHAR(9) NOT NULL,                   -- Assay identifier
    device_id UUID REFERENCES devices(id),          -- Testing device

    -- Timing information
    start_time BIGINT NOT NULL,                     -- Unix timestamp (seconds)
    duration INTEGER NOT NULL,                       -- Test duration (milliseconds)

    -- Spectrophotometer configuration
    astep INTEGER NOT NULL,                         -- Integration step value
    atime INTEGER NOT NULL,                         -- Integration time
    again INTEGER NOT NULL,                         -- Gain setting

    -- Reading counts
    number_of_readings INTEGER NOT NULL,            -- Total readings collected
    baseline_scans INTEGER NOT NULL,                -- Baseline scan count
    test_scans INTEGER NOT NULL,                    -- Test scan count

    -- Data integrity
    checksum BIGINT NOT NULL,                       -- CRC32 checksum

    -- Raw test record (for compatibility)
    raw_record BYTEA,                               -- Complete 9668-byte record

    -- Status
    status VARCHAR(32) DEFAULT 'uploaded',          -- uploaded, processed, flagged, archived

    -- Metadata
    metadata JSONB DEFAULT '{}',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    processed_at TIMESTAMP WITH TIME ZONE
);

-- Indexes for test result lookups
CREATE INDEX IF NOT EXISTS idx_test_results_cartridge ON test_results(cartridge_uuid);
CREATE INDEX IF NOT EXISTS idx_test_results_device ON test_results(device_id);
CREATE INDEX IF NOT EXISTS idx_test_results_start_time ON test_results(start_time);
CREATE INDEX IF NOT EXISTS idx_test_results_status ON test_results(status);

-- ============================================================================
-- SPECTROPHOTOMETER READINGS TABLE
-- Stores individual spectrophotometer readings (normalized from test records)
-- ============================================================================
CREATE TABLE IF NOT EXISTS spectro_readings (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    test_result_id UUID REFERENCES test_results(id) ON DELETE CASCADE,

    -- Reading metadata
    reading_number INTEGER NOT NULL,                -- Sequence number (0-299)
    channel CHAR(1) NOT NULL,                       -- 'A', 'B', or 'C'
    position INTEGER NOT NULL,                      -- Stage position (microns)
    temperature INTEGER NOT NULL,                   -- Temperature (x10, e.g., 450 = 45.0C)
    laser_output INTEGER NOT NULL,                  -- Laser power value
    timestamp_ms BIGINT NOT NULL,                   -- Milliseconds from test start

    -- AS7341 wavelength channels
    f1 INTEGER NOT NULL,                            -- 415nm (violet)
    f2 INTEGER NOT NULL,                            -- 445nm (violet-blue)
    f3 INTEGER NOT NULL,                            -- 480nm (blue)
    f4 INTEGER NOT NULL,                            -- 515nm (cyan-green)
    f5 INTEGER NOT NULL,                            -- 555nm (green)
    f6 INTEGER NOT NULL,                            -- 590nm (yellow-orange)
    f7 INTEGER NOT NULL,                            -- 630nm (orange-red)
    f8 INTEGER NOT NULL,                            -- 680nm (red)
    clear_channel INTEGER NOT NULL,                 -- Clear/ambient
    nir_channel INTEGER NOT NULL,                   -- Near-infrared

    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Indexes for readings lookups
CREATE INDEX IF NOT EXISTS idx_spectro_readings_test ON spectro_readings(test_result_id);
CREATE INDEX IF NOT EXISTS idx_spectro_readings_channel ON spectro_readings(channel);

-- ============================================================================
-- DEVICE EVENTS TABLE
-- Audit log for device events and state transitions
-- ============================================================================
CREATE TABLE IF NOT EXISTS device_events (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id UUID REFERENCES devices(id),
    event_type VARCHAR(64) NOT NULL,                -- validate, load_assay, upload, reset, error
    event_data JSONB DEFAULT '{}',                  -- Event-specific data
    cartridge_uuid VARCHAR(37),                     -- Associated cartridge (if any)
    success BOOLEAN DEFAULT true,
    error_message TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Index for event lookups
CREATE INDEX IF NOT EXISTS idx_device_events_device ON device_events(device_id);
CREATE INDEX IF NOT EXISTS idx_device_events_type ON device_events(event_type);
CREATE INDEX IF NOT EXISTS idx_device_events_created ON device_events(created_at);

-- ============================================================================
-- ROW LEVEL SECURITY POLICIES
-- ============================================================================

-- Enable RLS on all tables
ALTER TABLE devices ENABLE ROW LEVEL SECURITY;
ALTER TABLE assays ENABLE ROW LEVEL SECURITY;
ALTER TABLE cartridges ENABLE ROW LEVEL SECURITY;
ALTER TABLE test_results ENABLE ROW LEVEL SECURITY;
ALTER TABLE spectro_readings ENABLE ROW LEVEL SECURITY;
ALTER TABLE device_events ENABLE ROW LEVEL SECURITY;

-- Device can only access its own data (requires authenticated device context)
-- These policies will be refined based on actual authentication method

-- Service role has full access (for Edge Functions)
CREATE POLICY "Service role full access devices" ON devices
    FOR ALL USING (auth.role() = 'service_role');

CREATE POLICY "Service role full access assays" ON assays
    FOR ALL USING (auth.role() = 'service_role');

CREATE POLICY "Service role full access cartridges" ON cartridges
    FOR ALL USING (auth.role() = 'service_role');

CREATE POLICY "Service role full access test_results" ON test_results
    FOR ALL USING (auth.role() = 'service_role');

CREATE POLICY "Service role full access spectro_readings" ON spectro_readings
    FOR ALL USING (auth.role() = 'service_role');

CREATE POLICY "Service role full access device_events" ON device_events
    FOR ALL USING (auth.role() = 'service_role');

-- Public read access to assays (devices need to download)
CREATE POLICY "Devices can read active assays" ON assays
    FOR SELECT USING (is_active = true);

-- ============================================================================
-- FUNCTIONS
-- ============================================================================

-- Function to update updated_at timestamp
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Triggers for updated_at
CREATE TRIGGER update_devices_updated_at
    BEFORE UPDATE ON devices
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_assays_updated_at
    BEFORE UPDATE ON assays
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_cartridges_updated_at
    BEFORE UPDATE ON cartridges
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

-- Function to validate cartridge and return assay info
CREATE OR REPLACE FUNCTION validate_cartridge(
    p_cartridge_uuid VARCHAR(37),
    p_device_id UUID
)
RETURNS TABLE (
    success BOOLEAN,
    assay_id VARCHAR(9),
    cartridge_id UUID,
    error_message TEXT
) AS $$
DECLARE
    v_cartridge cartridges%ROWTYPE;
    v_assay assays%ROWTYPE;
BEGIN
    -- Find cartridge
    SELECT * INTO v_cartridge FROM cartridges WHERE cartridge_uuid = p_cartridge_uuid;

    IF NOT FOUND THEN
        RETURN QUERY SELECT false, NULL::VARCHAR(9), NULL::UUID, 'Cartridge not found'::TEXT;
        RETURN;
    END IF;

    -- Check status
    IF v_cartridge.status = 'used' THEN
        RETURN QUERY SELECT false, NULL::VARCHAR(9), NULL::UUID, 'Cartridge already used'::TEXT;
        RETURN;
    END IF;

    IF v_cartridge.status = 'expired' THEN
        RETURN QUERY SELECT false, NULL::VARCHAR(9), NULL::UUID, 'Cartridge expired'::TEXT;
        RETURN;
    END IF;

    -- Check expiration date
    IF v_cartridge.expiration_date IS NOT NULL AND v_cartridge.expiration_date < CURRENT_DATE THEN
        UPDATE cartridges SET status = 'expired' WHERE id = v_cartridge.id;
        RETURN QUERY SELECT false, NULL::VARCHAR(9), NULL::UUID, 'Cartridge expired'::TEXT;
        RETURN;
    END IF;

    -- Verify assay exists and is active
    SELECT * INTO v_assay FROM assays WHERE assay_id = v_cartridge.assay_id AND is_active = true;

    IF NOT FOUND THEN
        RETURN QUERY SELECT false, NULL::VARCHAR(9), NULL::UUID, 'Associated assay not found or inactive'::TEXT;
        RETURN;
    END IF;

    -- Update cartridge status
    UPDATE cartridges SET
        status = 'validated',
        validation_count = validation_count + 1,
        last_validated_at = NOW(),
        last_validated_by = p_device_id
    WHERE id = v_cartridge.id;

    -- Return success
    RETURN QUERY SELECT true, v_cartridge.assay_id, v_cartridge.id, NULL::TEXT;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

-- Function to record test upload
CREATE OR REPLACE FUNCTION upload_test_result(
    p_cartridge_uuid VARCHAR(37),
    p_assay_id VARCHAR(9),
    p_device_id UUID,
    p_start_time BIGINT,
    p_duration INTEGER,
    p_astep INTEGER,
    p_atime INTEGER,
    p_again INTEGER,
    p_number_of_readings INTEGER,
    p_baseline_scans INTEGER,
    p_test_scans INTEGER,
    p_checksum BIGINT,
    p_raw_record BYTEA
)
RETURNS UUID AS $$
DECLARE
    v_result_id UUID;
BEGIN
    -- Insert test result
    INSERT INTO test_results (
        cartridge_uuid, assay_id, device_id,
        start_time, duration,
        astep, atime, again,
        number_of_readings, baseline_scans, test_scans,
        checksum, raw_record, status
    ) VALUES (
        p_cartridge_uuid, p_assay_id, p_device_id,
        p_start_time, p_duration,
        p_astep, p_atime, p_again,
        p_number_of_readings, p_baseline_scans, p_test_scans,
        p_checksum, p_raw_record, 'uploaded'
    ) RETURNING id INTO v_result_id;

    -- Update cartridge status
    UPDATE cartridges SET
        status = 'used',
        test_result_id = v_result_id
    WHERE cartridge_uuid = p_cartridge_uuid;

    -- Log event
    INSERT INTO device_events (device_id, event_type, cartridge_uuid, event_data)
    VALUES (p_device_id, 'upload', p_cartridge_uuid, jsonb_build_object(
        'test_result_id', v_result_id,
        'readings', p_number_of_readings
    ));

    RETURN v_result_id;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

-- ============================================================================
-- COMMENTS
-- ============================================================================
COMMENT ON TABLE devices IS 'Registered Brevitest devices and authentication';
COMMENT ON TABLE assays IS 'Test assay definitions with BCODE instructions';
COMMENT ON TABLE cartridges IS 'Test cartridges with status tracking';
COMMENT ON TABLE test_results IS 'Completed test results with header data';
COMMENT ON TABLE spectro_readings IS 'Individual spectrophotometer readings (normalized)';
COMMENT ON TABLE device_events IS 'Audit log for device operations';
