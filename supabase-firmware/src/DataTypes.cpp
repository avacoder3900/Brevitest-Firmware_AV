/**
 * @file DataTypes.cpp
 * @brief Implementation of data structure serialization utilities
 * @details Provides serialization, deserialization, checksum calculation,
 *          and Base64 encoding/decoding functions for firmware data structures.
 *
 * Platform: Particle B-Series SoM (NRF52840) on Acuity GEN2 Main Board R5
 *
 * User Stories Implemented:
 *   - ALPHA-001: Single firmware version source of truth
 *   - DATA-006: Serialization utilities implementation
 *
 * @version 2.0
 * @date 2026-02-11
 */

#include "DataTypes.h"
#include <string.h>

//==============================================================================
// CRC32 LOOKUP TABLE
//==============================================================================

/**
 * @brief Pre-computed CRC32 lookup table (IEEE 802.3 polynomial)
 * @details Uses polynomial 0xEDB88320 (bit-reversed 0x04C11DB7)
 */
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD706B3,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

//==============================================================================
// BASE64 ENCODING TABLE
//==============================================================================

/** @brief Base64 encoding alphabet */
static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/** @brief Base64 decoding lookup table (255 = invalid) */
static const uint8_t base64_decode_table[256] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
     52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255, 255, 255, 255,
    255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
    255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
     41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

//==============================================================================
// CRC32 IMPLEMENTATION
//==============================================================================

uint32_t calculateCRC32(const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) {
        return 0;
    }

    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }

    return crc ^ 0xFFFFFFFF;
}

uint32_t calculateTestRecordChecksum(const BrevitestTestRecord* record) {
    if (record == nullptr) {
        return 0;
    }

    // Calculate checksum over all data except the checksum field itself
    // Checksum field is at offset 64-67
    const uint8_t* data = reinterpret_cast<const uint8_t*>(record);

    uint32_t crc = 0xFFFFFFFF;

    // Hash bytes before checksum field (offset 0-63)
    for (size_t i = 0; i < 64; i++) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }

    // Skip checksum field (offset 64-67), continue with reading array (offset 68+)
    for (size_t i = 68; i < sizeof(BrevitestTestRecord); i++) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }

    return crc ^ 0xFFFFFFFF;
}

bool verifyTestRecordChecksum(const BrevitestTestRecord* record) {
    if (record == nullptr) {
        return false;
    }

    uint32_t calculated = calculateTestRecordChecksum(record);
    return calculated == record->checksum;
}

uint32_t calculateAssayChecksum(const BrevitestAssay* assay) {
    if (assay == nullptr || assay->BCODE_length == 0) {
        return 0;
    }

    // Calculate checksum over BCODE data only
    return calculateCRC32(reinterpret_cast<const uint8_t*>(assay->BCODE),
                         assay->BCODE_length);
}

bool verifyAssayChecksum(const BrevitestAssay* assay, uint32_t expectedChecksum) {
    if (assay == nullptr) {
        return false;
    }

    uint32_t calculated = calculateAssayChecksum(assay);
    return calculated == expectedChecksum;
}

//==============================================================================
// SERIALIZATION IMPLEMENTATION
//==============================================================================

size_t serializeTestRecord(const BrevitestTestRecord* record, uint8_t* buffer, size_t bufferSize) {
    if (record == nullptr || buffer == nullptr) {
        return 0;
    }

    const size_t recordSize = sizeof(BrevitestTestRecord);

    if (bufferSize < recordSize) {
        return 0;
    }

    // Direct memory copy since structure is packed
    memcpy(buffer, record, recordSize);

    return recordSize;
}

bool deserializeTestRecord(const uint8_t* buffer, size_t bufferSize, BrevitestTestRecord* record) {
    if (buffer == nullptr || record == nullptr) {
        return false;
    }

    const size_t recordSize = sizeof(BrevitestTestRecord);

    if (bufferSize < recordSize) {
        return false;
    }

    // Direct memory copy since structure is packed
    memcpy(record, buffer, recordSize);

    return true;
}

//==============================================================================
// BASE64 IMPLEMENTATION
//==============================================================================

size_t base64Encode(const uint8_t* data, size_t dataLength, char* output, size_t outputSize) {
    if (data == nullptr || output == nullptr || dataLength == 0) {
        return 0;
    }

    // Calculate required output size (including null terminator)
    size_t encodedLength = ((dataLength + 2) / 3) * 4;

    if (outputSize < encodedLength + 1) {
        return 0;
    }

    size_t i = 0;
    size_t j = 0;

    while (i < dataLength) {
        uint32_t octet_a = i < dataLength ? data[i++] : 0;
        uint32_t octet_b = i < dataLength ? data[i++] : 0;
        uint32_t octet_c = i < dataLength ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        output[j++] = base64_chars[(triple >> 18) & 0x3F];
        output[j++] = base64_chars[(triple >> 12) & 0x3F];
        output[j++] = base64_chars[(triple >> 6) & 0x3F];
        output[j++] = base64_chars[triple & 0x3F];
    }

    // Add padding
    size_t mod = dataLength % 3;
    if (mod == 1) {
        output[j - 1] = '=';
        output[j - 2] = '=';
    } else if (mod == 2) {
        output[j - 1] = '=';
    }

    output[j] = '\0';

    return j;
}

size_t base64Decode(const char* input, size_t inputLength, uint8_t* output, size_t outputSize) {
    if (input == nullptr || output == nullptr || inputLength == 0) {
        return 0;
    }

    // Input length must be multiple of 4
    if (inputLength % 4 != 0) {
        return 0;
    }

    // Calculate output size
    size_t decodedLength = (inputLength / 4) * 3;

    // Account for padding
    if (inputLength >= 1 && input[inputLength - 1] == '=') {
        decodedLength--;
    }
    if (inputLength >= 2 && input[inputLength - 2] == '=') {
        decodedLength--;
    }

    if (outputSize < decodedLength) {
        return 0;
    }

    size_t i = 0;
    size_t j = 0;

    while (i < inputLength) {
        uint8_t a = base64_decode_table[(uint8_t)input[i++]];
        uint8_t b = base64_decode_table[(uint8_t)input[i++]];
        uint8_t c = base64_decode_table[(uint8_t)input[i++]];
        uint8_t d = base64_decode_table[(uint8_t)input[i++]];

        // Check for invalid characters
        if (a == 255 || b == 255) {
            return 0;
        }

        uint32_t triple = (a << 18) | (b << 12);

        if (c != 255) {
            triple |= (c << 6);
        }
        if (d != 255) {
            triple |= d;
        }

        if (j < decodedLength) {
            output[j++] = (triple >> 16) & 0xFF;
        }
        if (j < decodedLength) {
            output[j++] = (triple >> 8) & 0xFF;
        }
        if (j < decodedLength) {
            output[j++] = triple & 0xFF;
        }
    }

    return j;
}

//==============================================================================
// INITIALIZATION UTILITIES
//==============================================================================

void initSpectrophotometerReading(BrevitestSpectrophotometerReading* reading) {
    if (reading == nullptr) {
        return;
    }

    reading->number = 0;
    reading->channel = 'A';
    reading->position = 0;
    reading->temperature = 0;
    reading->laser_output = 0;
    reading->msec = 0;
    reading->f1 = 0;
    reading->f2 = 0;
    reading->f3 = 0;
    reading->f4 = 0;
    reading->f5 = 0;
    reading->f6 = 0;
    reading->f7 = 0;
    reading->f8 = 0;
    reading->clear = 0;
    reading->nir = 0;
}

void initTestRecord(BrevitestTestRecord* record) {
    if (record == nullptr) {
        return;
    }

    record->data_format_code = TEST_DATA_FORMAT_CODE;
    memset(record->cartridge_id, 0, sizeof(record->cartridge_id));
    memset(record->assay_id, 0, sizeof(record->assay_id));
    record->reserved = '\0';
    record->start_time = 0;
    record->duration = 0;
    record->astep = SPECTRO_ASTEP_DEFAULT;
    record->atime = SPECTRO_ATIME_DEFAULT;
    record->again = SPECTRO_AGAIN_DEFAULT;
    record->number_of_readings = 0;
    record->baseline_scans = 0;
    record->test_scans = 0;
    record->checksum = 0;

    // Initialize all readings
    for (int i = 0; i < SPECTRO_MAX_READINGS; i++) {
        initSpectrophotometerReading(&record->reading[i]);
    }
}

void copyTestRecord(BrevitestTestRecord* dest, const BrevitestTestRecord* src) {
    if (dest == nullptr || src == nullptr) {
        return;
    }

    // Direct memory copy since structure is packed and has no pointers
    memcpy(dest, src, sizeof(BrevitestTestRecord));
}

//==============================================================================
// STRING CONVERSION UTILITIES
//==============================================================================

const char* deviceModeToString(DeviceMode mode) {
    switch (mode) {
        case DeviceMode::IDLE:                    return "IDLE";
        case DeviceMode::INITIALIZING:            return "INITIALIZING";
        case DeviceMode::HEATING:                 return "HEATING";
        case DeviceMode::BARCODE_SCANNING:        return "BARCODE_SCANNING";
        case DeviceMode::VALIDATING_CARTRIDGE:    return "VALIDATING_CARTRIDGE";
        case DeviceMode::VALIDATING_MAGNETOMETER: return "VALIDATING_MAGNETOMETER";
        case DeviceMode::RUNNING_TEST:            return "RUNNING_TEST";
        case DeviceMode::UPLOADING_RESULTS:       return "UPLOADING_RESULTS";
        case DeviceMode::RESETTING_CARTRIDGE:     return "RESETTING_CARTRIDGE";
        case DeviceMode::STRESS_TESTING:          return "STRESS_TESTING";
        case DeviceMode::ERROR_STATE:             return "ERROR_STATE";
        default:                                  return "UNKNOWN";
    }
}

const char* testStateToString(TestState state) {
    switch (state) {
        case TestState::NOT_STARTED:        return "NOT_STARTED";
        case TestState::RUNNING:            return "RUNNING";
        case TestState::COMPLETED:          return "COMPLETED";
        case TestState::CANCELLED:          return "CANCELLED";
        case TestState::UPLOAD_PENDING:     return "UPLOAD_PENDING";
        case TestState::UPLOAD_IN_PROGRESS: return "UPLOAD_IN_PROGRESS";
        case TestState::UPLOADED:           return "UPLOADED";
        default:                            return "UNKNOWN";
    }
}

const char* cartridgeStateToString(CartridgeState state) {
    switch (state) {
        case CartridgeState::NOT_INSERTED:  return "NOT_INSERTED";
        case CartridgeState::DETECTED:      return "DETECTED";
        case CartridgeState::BARCODE_READ:  return "BARCODE_READ";
        case CartridgeState::VALIDATED:     return "VALIDATED";
        case CartridgeState::INVALID:       return "INVALID";
        case CartridgeState::TEST_COMPLETE: return "TEST_COMPLETE";
        default:                            return "UNKNOWN";
    }
}

const char* barcodeTypeToString(BarcodeType type) {
    switch (type) {
        case BarcodeType::CARTRIDGE:        return "CARTRIDGE";
        case BarcodeType::MAGNETOMETER:     return "MAGNETOMETER";
        case BarcodeType::OPTICAL:          return "OPTICAL";
        case BarcodeType::STRESS_TEST:      return "STRESS_TEST";
        case BarcodeType::SHIPPING:         return "SHIPPING";
        case BarcodeType::GENERAL_ERROR:    return "GENERAL_ERROR";
        case BarcodeType::VALIDATION_ERROR: return "VALIDATION_ERROR";
        case BarcodeType::OPTICAL_ERROR:    return "OPTICAL_ERROR";
        default:                            return "UNKNOWN";
    }
}

const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        // Success
        case ErrorCode::SUCCESS:                    return "SUCCESS";

        // General errors
        case ErrorCode::ERR_UNKNOWN:                return "ERR_UNKNOWN";
        case ErrorCode::ERR_TIMEOUT:                return "ERR_TIMEOUT";
        case ErrorCode::ERR_INVALID_STATE:          return "ERR_INVALID_STATE";
        case ErrorCode::ERR_INVALID_PARAMETER:      return "ERR_INVALID_PARAMETER";
        case ErrorCode::ERR_MEMORY_ALLOCATION:      return "ERR_MEMORY_ALLOCATION";

        // Hardware errors
        case ErrorCode::ERR_MOTOR_FAULT:            return "ERR_MOTOR_FAULT";
        case ErrorCode::ERR_HEATER_FAULT:           return "ERR_HEATER_FAULT";
        case ErrorCode::ERR_THERMISTOR_FAULT:       return "ERR_THERMISTOR_FAULT";
        case ErrorCode::ERR_SPECTRO_FAULT:          return "ERR_SPECTRO_FAULT";
        case ErrorCode::ERR_BARCODE_FAULT:          return "ERR_BARCODE_FAULT";
        case ErrorCode::ERR_LASER_FAULT:            return "ERR_LASER_FAULT";
        case ErrorCode::ERR_I2C_FAULT:              return "ERR_I2C_FAULT";

        // Cartridge errors
        case ErrorCode::ERR_CARTRIDGE_NOT_INSERTED: return "ERR_CARTRIDGE_NOT_INSERTED";
        case ErrorCode::ERR_CARTRIDGE_INVALID:      return "ERR_CARTRIDGE_INVALID";
        case ErrorCode::ERR_CARTRIDGE_EXPIRED:      return "ERR_CARTRIDGE_EXPIRED";
        case ErrorCode::ERR_CARTRIDGE_USED:         return "ERR_CARTRIDGE_USED";
        case ErrorCode::ERR_BARCODE_READ_FAILED:    return "ERR_BARCODE_READ_FAILED";

        // Cloud/Network errors
        case ErrorCode::ERR_CLOUD_TIMEOUT:          return "ERR_CLOUD_TIMEOUT";
        case ErrorCode::ERR_CLOUD_CONNECTION:       return "ERR_CLOUD_CONNECTION";
        case ErrorCode::ERR_CLOUD_VALIDATION:       return "ERR_CLOUD_VALIDATION";
        case ErrorCode::ERR_CLOUD_UPLOAD:           return "ERR_CLOUD_UPLOAD";
        case ErrorCode::ERR_ASSAY_DOWNLOAD:         return "ERR_ASSAY_DOWNLOAD";
        case ErrorCode::ERR_CHECKSUM_MISMATCH:      return "ERR_CHECKSUM_MISMATCH";

        // Test execution errors
        case ErrorCode::ERR_TEST_CANCELLED:         return "ERR_TEST_CANCELLED";
        case ErrorCode::ERR_TEST_FAILED:            return "ERR_TEST_FAILED";
        case ErrorCode::ERR_BCODE_INVALID:          return "ERR_BCODE_INVALID";
        case ErrorCode::ERR_BCODE_TIMEOUT:          return "ERR_BCODE_TIMEOUT";

        // Storage errors
        case ErrorCode::ERR_EEPROM_READ:            return "ERR_EEPROM_READ";
        case ErrorCode::ERR_EEPROM_WRITE:           return "ERR_EEPROM_WRITE";
        case ErrorCode::ERR_FILESYSTEM:             return "ERR_FILESYSTEM";
        case ErrorCode::ERR_FILE_NOT_FOUND:         return "ERR_FILE_NOT_FOUND";

        default:                                    return "UNKNOWN_ERROR";
    }
}
