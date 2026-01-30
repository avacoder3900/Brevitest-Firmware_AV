# Brevitest Cloud Protocol Specification

**Version:** 1.0
**Date:** 2026-01-30
**Status:** Implementation Reference

## Overview

This document specifies the cloud communication protocol between Brevitest diagnostic devices and the Supabase backend. It replaces the legacy Particle Pub/Sub architecture with HTTPS REST API calls to Supabase Edge Functions.

## Base Configuration

| Setting | Value |
|---------|-------|
| Base URL | `https://{project}.supabase.co/functions/v1` |
| Content-Type | `application/json` |
| Authorization | `Bearer {SUPABASE_ANON_KEY}` |
| Default Timeout | 45000 ms |
| Max Retries | 3 |
| Retry Base Delay | 5000 ms |

## Authentication

All requests must include:
```
Authorization: Bearer {SUPABASE_ANON_KEY}
Content-Type: application/json
X-Device-ID: {device_uuid}
X-Firmware-Version: {firmware_version}
```

---

## API Endpoints

### 1. Validate Cartridge

**Endpoint:** `POST /functions/v1/validate-cartridge`

Validates a cartridge UUID and returns the associated assay ID if valid.

#### Request
```json
{
  "requestId": "550e8400-e29b-41d4-a716-446655440000",
  "deviceId": "e00fce68xxxxyyyyzzzz",
  "cartridgeUuid": "12345678-1234-1234-1234-123456789012",
  "timestamp": 1706620800,
  "firmwareVersion": 57,
  "dataFormatVersion": 39
}
```

#### Response (Success)
```json
{
  "requestId": "550e8400-e29b-41d4-a716-446655440000",
  "status": 0,
  "isValid": true,
  "cartridgeUuid": "12345678-1234-1234-1234-123456789012",
  "assayId": "ASSAY001",
  "serverTimestamp": 1706620801
}
```

#### Response (Invalid Cartridge)
```json
{
  "requestId": "550e8400-e29b-41d4-a716-446655440000",
  "status": 201,
  "isValid": false,
  "errorMessage": "Cartridge already used",
  "serverTimestamp": 1706620801
}
```

#### Firmware Behavior
1. Generate unique `requestId` (UUID v4)
2. Set `cloud_operation_pending = true`
3. POST request with 45s timeout
4. On success: Set `cartridge_state = VALIDATED`, store `assayId`
5. On failure: Retry with exponential backoff (max 3 attempts)
6. On max retries: Set `cartridge_state = INVALID`, transition to ERROR_STATE
7. Set `cloud_operation_pending = false` on response

---

### 2. Load Assay

**Endpoint:** `GET /functions/v1/load-assay?id={assayId}`

Downloads assay BCODE instructions for test execution.

#### Request
Query parameter: `id={assayId}` (8 characters)

Headers include standard authentication.

#### Response (Success)
```json
{
  "requestId": "generated-by-server",
  "status": 0,
  "assayId": "ASSAY001",
  "duration": 300000,
  "bcodeLength": 245,
  "bcodeChecksum": 1234567890,
  "bcode": "0:|10:7,999,49|2:7860,300|11:5|1:5000|14:10|2:-7860,300|99:"
}
```

#### Response (Not Found)
```json
{
  "requestId": "generated-by-server",
  "status": 204,
  "errorMessage": "Assay not found: ASSAY001"
}
```

#### Firmware Behavior
1. Check local cache for assay first (`/assay/{assayId}`)
2. If cached with matching checksum, use cached version
3. Otherwise, fetch from server
4. Validate BCODE checksum using CRC32
5. Cache valid assay locally
6. Store in `BrevitestAssay` struct

---

### 3. Upload Test Results

**Endpoint:** `POST /functions/v1/upload-test`

Uploads completed test results to the server.

#### Request
```json
{
  "requestId": "550e8400-e29b-41d4-a716-446655440000",
  "deviceId": "e00fce68xxxxyyyyzzzz",
  "cartridgeUuid": "12345678-1234-1234-1234-123456789012",
  "assayId": "ASSAY001",
  "startTime": 1706620800,
  "duration": 300,
  "numberOfReadings": 150,
  "baselineScans": 5,
  "testScans": 10,
  "checksum": 1234567890,
  "recordSize": 9668,
  "timestamp": 1706621100,
  "firmwareVersion": 57,
  "dataFormatVersion": 39
}
```

Binary payload follows as `multipart/form-data` or base64-encoded in a separate field:
- Field name: `testRecord`
- Content: 9668 bytes (BrevitestTestRecord binary)

#### Response (Success)
```json
{
  "requestId": "550e8400-e29b-41d4-a716-446655440000",
  "status": 0,
  "cartridgeUuid": "12345678-1234-1234-1234-123456789012",
  "testResultId": "660e8400-e29b-41d4-a716-446655440001",
  "acknowledged": true,
  "serverTimestamp": 1706621101
}
```

#### Firmware Behavior
1. Calculate CRC32 checksum of test record
2. Set `cloud_operation_pending = true`
3. POST with 30s timeout
4. On success: Set `test_state = UPLOADED`
5. On failure: Cache to `/cache/{cartridgeId}` for retry
6. Set `cloud_operation_pending = false` on response

---

### 4. Reset Cartridge

**Endpoint:** `POST /functions/v1/reset-cartridge`

Requests a cartridge reset on the server (for re-testing).

#### Request
```json
{
  "requestId": "550e8400-e29b-41d4-a716-446655440000",
  "deviceId": "e00fce68xxxxyyyyzzzz",
  "cartridgeUuid": "12345678-1234-1234-1234-123456789012",
  "timestamp": 1706620800,
  "firmwareVersion": 57,
  "dataFormatVersion": 39
}
```

#### Response (Success)
```json
{
  "requestId": "550e8400-e29b-41d4-a716-446655440000",
  "status": 0,
  "cartridgeUuid": "12345678-1234-1234-1234-123456789012",
  "wasReset": true,
  "serverTimestamp": 1706620801
}
```

#### Firmware Behavior
1. Set `cloud_operation_pending = true`
2. POST with 30s timeout
3. On success: Transition to IDLE
4. On failure: Set error state

---

## Error Codes

### Network Errors (1-99)
| Code | Name | Description |
|------|------|-------------|
| 1 | ERR_NO_CONNECTION | No network connection |
| 2 | ERR_TIMEOUT | Request timed out |
| 3 | ERR_DNS_FAILED | DNS resolution failed |
| 4 | ERR_TLS_FAILED | TLS/SSL handshake failed |
| 5 | ERR_HTTP_ERROR | HTTP protocol error |

### Authentication Errors (100-199)
| Code | Name | Description |
|------|------|-------------|
| 100 | ERR_UNAUTHORIZED | Invalid or missing API key |
| 101 | ERR_FORBIDDEN | Device not authorized |
| 102 | ERR_DEVICE_NOT_FOUND | Device ID not registered |

### Validation Errors (200-299)
| Code | Name | Description |
|------|------|-------------|
| 200 | ERR_CARTRIDGE_NOT_FOUND | Cartridge UUID not in database |
| 201 | ERR_CARTRIDGE_USED | Cartridge already used |
| 202 | ERR_CARTRIDGE_EXPIRED | Cartridge past expiration |
| 203 | ERR_CARTRIDGE_INVALID | Cartridge validation failed |
| 204 | ERR_ASSAY_NOT_FOUND | Associated assay not found |
| 205 | ERR_ASSAY_INACTIVE | Assay is not active |

### Upload Errors (300-399)
| Code | Name | Description |
|------|------|-------------|
| 300 | ERR_UPLOAD_FAILED | Test upload failed |
| 301 | ERR_CHECKSUM_MISMATCH | Data checksum mismatch |
| 302 | ERR_DUPLICATE_UPLOAD | Test already uploaded |
| 303 | ERR_INVALID_FORMAT | Invalid data format |

### Server Errors (400-499)
| Code | Name | Description |
|------|------|-------------|
| 400 | ERR_SERVER_ERROR | Internal server error |
| 401 | ERR_DATABASE_ERROR | Database operation failed |
| 402 | ERR_SERVICE_UNAVAILABLE | Service temporarily unavailable |

### Client Errors (500-599)
| Code | Name | Description |
|------|------|-------------|
| 500 | ERR_INVALID_REQUEST | Malformed request |
| 501 | ERR_MISSING_FIELD | Required field missing |
| 502 | ERR_PARSE_ERROR | JSON parse error |
| 503 | ERR_CACHE_FULL | Local cache is full |

---

## Retry Behavior

### Exponential Backoff Algorithm
```
retry_delay = RETRY_BASE_DELAY * retry_count
```

Where:
- `RETRY_BASE_DELAY` = 5000 ms
- `retry_count` = 1, 2, 3 (max)

### Retry Schedule
| Attempt | Delay |
|---------|-------|
| 1 | 5 seconds |
| 2 | 10 seconds |
| 3 | 15 seconds |

After 3 failed attempts:
- Set `cartridge_state = INVALID`
- Transition to `ERROR_STATE`
- Clear retry tracking

### Retry Conditions
- Network timeout
- HTTP 5xx errors
- ERR_SERVICE_UNAVAILABLE

### No Retry Conditions
- HTTP 4xx errors (client error)
- ERR_CARTRIDGE_USED
- ERR_CARTRIDGE_EXPIRED
- ERR_UNAUTHORIZED

---

## Binary Data Format

### BrevitestTestRecord (9668 bytes)

```
Offset  Size  Field
------  ----  -----
0       1     data_format_code ('J')
1       37    cartridge_id (36 chars + null)
38      9     assay_id (8 chars + null)
47      1     reserved
48      4     start_time (Unix timestamp)
52      2     duration (seconds)
54      2     astep (default: 999)
56      1     atime (default: 49)
57      1     again (default: 7)
58      2     number_of_readings
60      2     baseline_scans
62      2     test_scans
64      4     checksum (CRC32)
68      9600  readings[300] (300 x 32 bytes)
```

### BrevitestSpectrophotometerReading (32 bytes)

```
Offset  Size  Field
------  ----  -----
0       1     number (reading sequence)
1       1     channel ('A', 'B', 'C')
2       2     position (microsteps)
4       2     temperature (10x Celsius)
6       2     laser_output
8       4     msec (timestamp)
12      2     f1 (415nm)
14      2     f2 (445nm)
16      2     f3 (480nm)
18      2     f4 (515nm)
20      2     f5 (555nm)
22      2     f6 (590nm)
24      2     f7 (630nm)
26      2     f8 (680nm)
28      2     clear
30      2     nir
```

---

## Offline Caching Strategy

### Cache Directories
| Directory | Purpose | Max Files |
|-----------|---------|-----------|
| `/cache/` | Failed test uploads | 50 |
| `/assay/` | Downloaded assays | 50 |
| `/validation/` | Magnetometer data | 50 |

### Cache File Naming
- Test cache: `/cache/{cartridge_uuid}`
- Assay cache: `/assay/{assay_id}`
- Validation: `/validation/magnet_{timestamp}.txt`

### FIFO Eviction
When cache reaches max files:
1. Find oldest file by modification time
2. Delete oldest file
3. Write new file

### Retry on Reconnection
On network reconnection:
1. Check `/cache/` for pending uploads
2. Attempt upload of each cached test
3. Delete cache file on successful upload
4. Keep file on failure (will retry later)

---

## Request ID Correlation

Every request includes a unique `requestId` (UUID v4 format):
- Generated by device before sending
- Echoed back in response
- Used for:
  - Matching responses to requests
  - Debugging and logging
  - Duplicate detection on server

### UUID v4 Format
```
xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
```
Where `y` is one of: 8, 9, a, b

---

## Connection Monitoring

### State Tracking
```cpp
bool last_cloud_connected = false;
```

### Reconnection Handling
1. Detect connection restoration
2. Re-register Particle subscriptions (legacy compatibility)
3. Process cached uploads
4. Resume normal operation

### Connection Check Functions
- `isConnected()` - Current connection status
- `processConnection()` - Update connection state
- `updateConnectionState()` - Handle state changes

---

## Implementation Files

| File | Purpose |
|------|---------|
| `CloudProtocol.h` | Request/response structures |
| `SupabaseClient.h` | HTTP client interface |
| `SupabaseClient.cpp` | HTTP client implementation |
| `StorageManager.h` | Cache management interface |
| `StorageManager.cpp` | Cache management implementation |
| `DataTypes.h` | Binary data structures |

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-30 | Initial specification |
