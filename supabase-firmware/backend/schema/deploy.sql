-- ==========================================================================
-- Brevitest Firmware Schema - Full Deployment Script
-- Run this in Supabase Dashboard > SQL Editor
-- Date: 2026-02-17
--
-- This script creates all tables needed by the Brevitest firmware.
-- Safe to re-run (uses IF NOT EXISTS and OR REPLACE).
--
-- NOTE: The existing empty 'devices' table has a different schema
-- than what the firmware needs, so we drop and recreate it.
-- ==========================================================================

-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- ==========================================================================
-- DEVICES TABLE (drop empty table with wrong schema, recreate)
-- ==========================================================================
DROP TABLE IF EXISTS devices CASCADE;

CREATE TABLE devices (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id VARCHAR(64) UNIQUE NOT NULL,
    api_key VARCHAR(128) NOT NULL,
    firmware_version INTEGER DEFAULT 0,
    data_format_version INTEGER DEFAULT 0,
    last_seen TIMESTAMP WITH TIME ZONE,
    metadata JSONB DEFAULT '{}',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_devices_device_id ON devices(device_id);
CREATE INDEX IF NOT EXISTS idx_devices_api_key ON devices(api_key);

-- ==========================================================================
-- ASSAYS TABLE
-- ==========================================================================
CREATE TABLE IF NOT EXISTS assays (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    assay_id VARCHAR(9) UNIQUE NOT NULL,
    name VARCHAR(255) NOT NULL,
    description TEXT,
    duration INTEGER NOT NULL,
    bcode BYTEA NOT NULL,
    bcode_length INTEGER NOT NULL,
    checksum INTEGER NOT NULL,
    version INTEGER DEFAULT 1,
    is_active BOOLEAN DEFAULT true,
    metadata JSONB DEFAULT '{}',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_assays_assay_id ON assays(assay_id);
CREATE INDEX IF NOT EXISTS idx_assays_is_active ON assays(is_active);

-- ==========================================================================
-- CARTRIDGES TABLE (with all CouchDB migration fields)
-- ==========================================================================
CREATE TABLE IF NOT EXISTS cartridges (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    cartridge_uuid VARCHAR(37) UNIQUE NOT NULL,
    assay_id VARCHAR(9) REFERENCES assays(assay_id),
    status VARCHAR(32) DEFAULT 'unused',
    lot_number VARCHAR(64),
    expiration_date DATE,
    serial_number VARCHAR(64),
    site_id VARCHAR(64),
    program VARCHAR(255),
    experiment TEXT,
    arm VARCHAR(255),
    quantity INTEGER,
    validation_errors JSONB DEFAULT '[]',
    status_updated_at TIMESTAMP WITH TIME ZONE,
    validation_count INTEGER DEFAULT 0,
    last_validated_at TIMESTAMP WITH TIME ZONE,
    last_validated_by UUID REFERENCES devices(id),
    test_result_id UUID,
    metadata JSONB DEFAULT '{}',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_cartridges_uuid ON cartridges(cartridge_uuid);
CREATE INDEX IF NOT EXISTS idx_cartridges_status ON cartridges(status);
CREATE INDEX IF NOT EXISTS idx_cartridges_assay_id ON cartridges(assay_id);
CREATE INDEX IF NOT EXISTS idx_cartridges_serial_number ON cartridges(serial_number);
CREATE INDEX IF NOT EXISTS idx_cartridges_site_id ON cartridges(site_id);
CREATE INDEX IF NOT EXISTS idx_cartridges_program ON cartridges(program);

-- ==========================================================================
-- TEST RESULTS TABLE
-- ==========================================================================
CREATE TABLE IF NOT EXISTS test_results (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    data_format_code CHAR(1) NOT NULL DEFAULT 'J',
    cartridge_uuid VARCHAR(37) NOT NULL,
    assay_id VARCHAR(9) NOT NULL,
    device_id UUID REFERENCES devices(id),
    start_time BIGINT NOT NULL,
    duration INTEGER NOT NULL,
    astep INTEGER NOT NULL,
    atime INTEGER NOT NULL,
    again INTEGER NOT NULL,
    number_of_readings INTEGER NOT NULL,
    baseline_scans INTEGER NOT NULL,
    test_scans INTEGER NOT NULL,
    checksum BIGINT NOT NULL,
    raw_record BYTEA,
    status VARCHAR(32) DEFAULT 'uploaded',
    metadata JSONB DEFAULT '{}',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    processed_at TIMESTAMP WITH TIME ZONE
);

CREATE INDEX IF NOT EXISTS idx_test_results_cartridge ON test_results(cartridge_uuid);
CREATE INDEX IF NOT EXISTS idx_test_results_device ON test_results(device_id);
CREATE INDEX IF NOT EXISTS idx_test_results_start_time ON test_results(start_time);
CREATE INDEX IF NOT EXISTS idx_test_results_status ON test_results(status);

-- ==========================================================================
-- SPECTROPHOTOMETER READINGS TABLE
-- ==========================================================================
CREATE TABLE IF NOT EXISTS spectro_readings (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    test_result_id UUID REFERENCES test_results(id) ON DELETE CASCADE,
    reading_number INTEGER NOT NULL,
    channel CHAR(1) NOT NULL,
    position INTEGER NOT NULL,
    temperature INTEGER NOT NULL,
    laser_output INTEGER NOT NULL,
    timestamp_ms BIGINT NOT NULL,
    f1 INTEGER NOT NULL,
    f2 INTEGER NOT NULL,
    f3 INTEGER NOT NULL,
    f4 INTEGER NOT NULL,
    f5 INTEGER NOT NULL,
    f6 INTEGER NOT NULL,
    f7 INTEGER NOT NULL,
    f8 INTEGER NOT NULL,
    clear_channel INTEGER NOT NULL,
    nir_channel INTEGER NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_spectro_readings_test ON spectro_readings(test_result_id);
CREATE INDEX IF NOT EXISTS idx_spectro_readings_channel ON spectro_readings(channel);

-- ==========================================================================
-- DEVICE EVENTS TABLE
-- ==========================================================================
CREATE TABLE IF NOT EXISTS device_events (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id UUID REFERENCES devices(id),
    event_type VARCHAR(64) NOT NULL,
    event_data JSONB DEFAULT '{}',
    cartridge_uuid VARCHAR(37),
    success BOOLEAN DEFAULT true,
    error_message TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_device_events_device ON device_events(device_id);
CREATE INDEX IF NOT EXISTS idx_device_events_type ON device_events(event_type);
CREATE INDEX IF NOT EXISTS idx_device_events_created ON device_events(created_at);

-- ==========================================================================
-- ROW LEVEL SECURITY
-- ==========================================================================
ALTER TABLE devices ENABLE ROW LEVEL SECURITY;
ALTER TABLE assays ENABLE ROW LEVEL SECURITY;
ALTER TABLE cartridges ENABLE ROW LEVEL SECURITY;
ALTER TABLE test_results ENABLE ROW LEVEL SECURITY;
ALTER TABLE spectro_readings ENABLE ROW LEVEL SECURITY;
ALTER TABLE device_events ENABLE ROW LEVEL SECURITY;

-- Service role full access policies
DO $$ BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_policies WHERE policyname = 'Service role full access devices') THEN
        CREATE POLICY "Service role full access devices" ON devices FOR ALL USING (auth.role() = 'service_role');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_policies WHERE policyname = 'Service role full access assays') THEN
        CREATE POLICY "Service role full access assays" ON assays FOR ALL USING (auth.role() = 'service_role');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_policies WHERE policyname = 'Service role full access cartridges') THEN
        CREATE POLICY "Service role full access cartridges" ON cartridges FOR ALL USING (auth.role() = 'service_role');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_policies WHERE policyname = 'Service role full access test_results') THEN
        CREATE POLICY "Service role full access test_results" ON test_results FOR ALL USING (auth.role() = 'service_role');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_policies WHERE policyname = 'Service role full access spectro_readings') THEN
        CREATE POLICY "Service role full access spectro_readings" ON spectro_readings FOR ALL USING (auth.role() = 'service_role');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_policies WHERE policyname = 'Service role full access device_events') THEN
        CREATE POLICY "Service role full access device_events" ON device_events FOR ALL USING (auth.role() = 'service_role');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_policies WHERE policyname = 'Devices can read active assays') THEN
        CREATE POLICY "Devices can read active assays" ON assays FOR SELECT USING (is_active = true);
    END IF;
END $$;

-- ==========================================================================
-- FUNCTIONS & TRIGGERS
-- ==========================================================================

-- updated_at trigger function
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- status_updated_at trigger function
CREATE OR REPLACE FUNCTION update_cartridge_status_timestamp()
RETURNS TRIGGER AS $$
BEGIN
    IF OLD.status IS DISTINCT FROM NEW.status THEN
        NEW.status_updated_at = NOW();
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Triggers (drop first to avoid duplicates)
DROP TRIGGER IF EXISTS update_devices_updated_at ON devices;
CREATE TRIGGER update_devices_updated_at BEFORE UPDATE ON devices
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

DROP TRIGGER IF EXISTS update_assays_updated_at ON assays;
CREATE TRIGGER update_assays_updated_at BEFORE UPDATE ON assays
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

DROP TRIGGER IF EXISTS update_cartridges_updated_at ON cartridges;
CREATE TRIGGER update_cartridges_updated_at BEFORE UPDATE ON cartridges
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

DROP TRIGGER IF EXISTS update_cartridges_status_timestamp ON cartridges;
CREATE TRIGGER update_cartridges_status_timestamp BEFORE UPDATE ON cartridges
    FOR EACH ROW EXECUTE FUNCTION update_cartridge_status_timestamp();

-- ==========================================================================
-- STORED PROCEDURES
-- ==========================================================================

-- Validate cartridge
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
    SELECT * INTO v_cartridge FROM cartridges WHERE cartridge_uuid = p_cartridge_uuid;
    IF NOT FOUND THEN
        RETURN QUERY SELECT false, NULL::VARCHAR(9), NULL::UUID, 'Cartridge not found'::TEXT;
        RETURN;
    END IF;
    IF v_cartridge.status = 'used' THEN
        RETURN QUERY SELECT false, NULL::VARCHAR(9), NULL::UUID, 'Cartridge already used'::TEXT;
        RETURN;
    END IF;
    IF v_cartridge.status = 'expired' THEN
        RETURN QUERY SELECT false, NULL::VARCHAR(9), NULL::UUID, 'Cartridge expired'::TEXT;
        RETURN;
    END IF;
    IF v_cartridge.expiration_date IS NOT NULL AND v_cartridge.expiration_date < CURRENT_DATE THEN
        UPDATE cartridges SET status = 'expired' WHERE id = v_cartridge.id;
        RETURN QUERY SELECT false, NULL::VARCHAR(9), NULL::UUID, 'Cartridge expired'::TEXT;
        RETURN;
    END IF;
    SELECT * INTO v_assay FROM assays WHERE assays.assay_id = v_cartridge.assay_id AND is_active = true;
    IF NOT FOUND THEN
        RETURN QUERY SELECT false, NULL::VARCHAR(9), NULL::UUID, 'Associated assay not found or inactive'::TEXT;
        RETURN;
    END IF;
    UPDATE cartridges SET
        status = 'validated',
        validation_count = cartridges.validation_count + 1,
        last_validated_at = NOW(),
        last_validated_by = p_device_id
    WHERE id = v_cartridge.id;
    RETURN QUERY SELECT true, v_cartridge.assay_id, v_cartridge.id, NULL::TEXT;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

-- Upload test result
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

    UPDATE cartridges SET
        status = 'used',
        test_result_id = v_result_id
    WHERE cartridge_uuid = p_cartridge_uuid;

    INSERT INTO device_events (device_id, event_type, cartridge_uuid, event_data)
    VALUES (p_device_id, 'upload', p_cartridge_uuid, jsonb_build_object(
        'test_result_id', v_result_id,
        'readings', p_number_of_readings
    ));

    RETURN v_result_id;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

-- ==========================================================================
-- TABLE COMMENTS
-- ==========================================================================
COMMENT ON TABLE devices IS 'Registered Brevitest devices and authentication';
COMMENT ON TABLE assays IS 'Test assay definitions with BCODE instructions';
COMMENT ON TABLE cartridges IS 'Test cartridges with status tracking';
COMMENT ON TABLE test_results IS 'Completed test results with header data';
COMMENT ON TABLE spectro_readings IS 'Individual spectrophotometer readings (normalized)';
COMMENT ON TABLE device_events IS 'Audit log for device operations';

-- ==========================================================================
-- DONE! Verify with:
-- ==========================================================================
SELECT 'devices' AS table_name, COUNT(*) AS row_count FROM devices
UNION ALL SELECT 'assays', COUNT(*) FROM assays
UNION ALL SELECT 'cartridges', COUNT(*) FROM cartridges
UNION ALL SELECT 'test_results', COUNT(*) FROM test_results
UNION ALL SELECT 'spectro_readings', COUNT(*) FROM spectro_readings
UNION ALL SELECT 'device_events', COUNT(*) FROM device_events;
