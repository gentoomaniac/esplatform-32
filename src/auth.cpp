#include "auth.h"

#include <Arduino.h>
#include <esp_http_server.h>
#include <stdbool.h>
#include <string.h>

#include "deviceconfig.h"
#include "mbedtls/sha256.h"

// 5 minutes = 300,000ms
const uint32_t MAX_NONCE_AGE = 300000;

typedef struct {
    char username[32];
    char realm[64];
    char nonce[128];
    char uri[128];
    char cnonce[128];
    char nc[9];  // 8-digit hex
    char qop[16];
    char response[65];
    char algorithm[16];
} digest_auth_t;

/**
 * Internal helper to find a key and copy its value into a destination buffer.
 */
static void extract_field(const char* header, const char* key, char* dest, size_t dest_len) {
    const char* pos = header;
    size_t key_len = strlen(key);

    while ((pos = strstr(pos, key)) != NULL) {
        // Check if preceded by start of string, space, or comma
        bool start_match = (pos == header || *(pos - 1) == ' ' || *(pos - 1) == ',');
        // Check if followed by '='
        bool end_match = (*(pos + key_len) == '=');

        if (start_match && end_match) break;
        pos++;  // Keep searching
    }

    if (!pos) return;
    pos = strchr(pos, '=');
    if (!pos) return;
    pos++;

    if (*pos == '"') {
        pos++;
        const char* end = strchr(pos, '"');
        if (end) {
            size_t len = end - pos;
            if (len >= dest_len) len = dest_len - 1;
            strncpy(dest, pos, len);
            dest[len] = '\0';
        }
    } else {
        size_t len = strcspn(pos, ", ");
        if (len >= dest_len) len = dest_len - 1;
        strncpy(dest, pos, len);
        dest[len] = '\0';
    }
}

/**
 * Parses the raw Authorization header into the self-contained struct.
 */
void parse_digest_header(const char* raw_header, digest_auth_t* out_auth) {
    if (!raw_header || !out_auth) return;

    // Zero out the struct first
    memset(out_auth, 0, sizeof(digest_auth_t));

    extract_field(raw_header, "username", out_auth->username, sizeof(out_auth->username));
    extract_field(raw_header, "realm", out_auth->realm, sizeof(out_auth->realm));
    extract_field(raw_header, "nonce", out_auth->nonce, sizeof(out_auth->nonce));
    extract_field(raw_header, "uri", out_auth->uri, sizeof(out_auth->uri));
    extract_field(raw_header, "cnonce", out_auth->cnonce, sizeof(out_auth->cnonce));
    extract_field(raw_header, "nc", out_auth->nc, sizeof(out_auth->nc));
    extract_field(raw_header, "qop", out_auth->qop, sizeof(out_auth->qop));
    extract_field(raw_header, "response", out_auth->response, sizeof(out_auth->response));
    extract_field(raw_header, "algorithm", out_auth->algorithm, sizeof(out_auth->algorithm));
}

void bytes_to_hex(const unsigned char* hash, char* output) {
    for (int i = 0; i < 32; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[64] = '\0';
}

// Simple wrapper for mbedTLS SHA256
void calc_sha256(const char* input, char* output_hex) {
    unsigned char hash[32];
    mbedtls_sha256((const unsigned char*)input, strlen(input), hash, 0);  // 0 for SHA-256
    bytes_to_hex(hash, output_hex);
}

/**
 * Validates the Authorization header using a stateless signed nonce.
 * @param auth    The struct containing parsed fields from the Authorization header.
 * @param config  The system configuration (contains credentials and internal secret).
 * @param method  The HTTP method string (e.g., "POST" or "GET").
 * @return true if the nonce is authentic/fresh and the digest matches.
 */
bool is_digest_valid(const digest_auth_t* auth, const Config* config, const char* method) {
    // --- NONCE SIGNATURE & AGE VERIFICATION ---
    char nonce_copy[130];
    strlcpy(nonce_copy, auth->nonce, sizeof(nonce_copy));

    char* colon = strchr(nonce_copy, ':');
    if (colon == nullptr) return false;

    *colon = '\0';
    const char* ts_str = nonce_copy;       // The raw uptime string
    const char* received_sig = colon + 1;  // The signature part

    uint32_t issued_at = (uint32_t)strtoul(ts_str, nullptr, 10);
    uint32_t now = millis();

    // Reject if rebooted or expired
    if (now < issued_at || (now - issued_at) > MAX_NONCE_AGE) {
#ifdef DEBUG
        Serial.println("age check failed");
#endif
        return false;
    }

    // Verify authenticity: signature = SHA256(uptime + secretKey)
    char sign_verify_buf[128];
    char expected_sig[65];
    snprintf(sign_verify_buf, sizeof(sign_verify_buf), "%s:%s", ts_str, config->sys.internal.secretKey);
    calc_sha256(sign_verify_buf, expected_sig);

    if (strcmp(expected_sig, received_sig) != 0) {
#ifdef DEBUG
        Serial.println("incorrect hash in nonce signature");
#endif
        return false;  // Forged or incorrect secretKey
    }

    // --- DIGEST RESPONSE CALCULATION ---
    char ha1[65], ha2[65], expected_response[65];
    char buffer[512];

    // HA1 = SHA256(username:realm:password)
    snprintf(buffer, sizeof(buffer), "%s:%s:%s", config->sys.auth.username, config->sys.id, config->sys.auth.password);
    calc_sha256(buffer, ha1);

    // HA2 = SHA256(method:digestURI)
    // In Header Auth, the URI comes from the 'uri=' field in the Authorization header.
    snprintf(buffer, sizeof(buffer), "%s:%s", method, auth->uri);
    calc_sha256(buffer, ha2);

    // Final Response = SHA256(HA1:nonce:nc:cnonce:qop:HA2)
    // We use the full original nonce string ("uptime:signature") as the client did.
    snprintf(buffer, sizeof(buffer), "%s:%s:%s:%s:%s:%s", ha1, auth->nonce, auth->nc, auth->cnonce, auth->qop, ha2);
    calc_sha256(buffer, expected_response);
#ifdef DEBUG
    Serial.println(String("buffer: ") + String(buffer));
    Serial.println(String("exxpected: ") + String(expected_response));
#endif

    // Constant-time compare is safer, but memcmp for 64 chars is fine for this use case.
    return (memcmp(expected_response, auth->response, 64) == 0);
}

esp_err_t send401(httpd_req_t* req, Config* config) {
    uint32_t uptime = millis();
    char sign_input[128];
    char signature[65];

    // Create the input for the hash: "uptime:secretKey"
    // This secretKey should be your config->sys.internal.secretKey
    snprintf(sign_input, sizeof(sign_input), "%u:%s", uptime, config->sys.internal.secretKey);

    // Generate the signature
    calc_sha256(sign_input, signature);

    // Build the combined nonce: "uptime:signature"
    char combined_nonce[130];
    snprintf(combined_nonce, sizeof(combined_nonce), "%u:%s", uptime, signature);

    // Build the WWW-Authenticate header
    char auth_header[300];
    snprintf(auth_header, sizeof(auth_header), "Digest realm=\"%s\", nonce=\"%s\", algorithm=SHA-256, qop=\"auth\"",
             config->sys.id, combined_nonce);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_set_hdr(req, "WWW-Authenticate", auth_header);

    return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Authentication required");
}

esp_err_t authed(httpd_req_t* req, Config* config, AuthedHandler handler) {
    if (config->sys.auth.enabled) {
        digest_auth_t authHeader;
        size_t hdr_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;

        if (hdr_len > 1) {
            char* hdrValue = (char*)malloc(hdr_len);
            if (httpd_req_get_hdr_value_str(req, "Authorization", hdrValue, hdr_len) == ESP_OK) {
#ifdef DEBUG
                Serial.println(hdrValue);
#endif
                parse_digest_header(hdrValue, &authHeader);
                bool is_valid = is_digest_valid(&authHeader, config, "POST");
#ifdef DEBUG
                Serial.println("is_valid: " + is_valid ? "true" : "false");
#endif
                free(hdrValue);
                if (is_valid) return handler(req, config);
            } else {
                free(hdrValue);
                return send401(req, config);
            }
        }
        return send401(req, config);
    }

    return handler(req, config);
}
