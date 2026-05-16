package request

import (
	"bytes"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/netip"
	"net/url"
	"regexp"
	"time"

	"github.com/rs/zerolog/log"
)

type RPCMethod string

const (
	MethodShellyGetConfig   RPCMethod = "Shelly.GetConfig"
	MethodSysSetConfig      RPCMethod = "Sys.SetConfig"
	MethodWifiSetConfig     RPCMethod = "Wifi.SetConfig"
	MethodEthSetConfig      RPCMethod = "Eth.SetConfig"
	MethodCloudConfig       RPCMethod = "Cloud.SetConfig"
	MethodMQTTConfig        RPCMethod = "MQTT.SetConfig"
	MethodWSConfig          RPCMethod = "WS.SetConfig"
	MethodModbusConfig      RPCMethod = "Modbus.SetConfig"
	MethodBLEConfig         RPCMethod = "BLE.SetConfig"
	MethodBTHomeConfig      RPCMethod = "BTHome.SetConfig"
	MethodEMConfig          RPCMethod = "EM.SetConfig"
	MethodEMDataConfig      RPCMethod = "EMData.SetConfig"
	MethodTemperatureConfig RPCMethod = "Temperature.SetConfig"
)

type Auth struct {
	Username string
	Password string
}

// RPCRequest for the JSON payload
type RPCRequest struct {
	JSONRPC string    `json:"jsonrpc"`
	ID      int       `json:"id"`
	Method  RPCMethod `json:"method"`
	Params  any       `json:"params,omitempty"`
	Auth    *RPCAuth  `json:"auth,omitempty"`
}
type RPCAuth struct {
	Realm     string `json:"realm"`
	Username  string `json:"username"`
	Nonce     string `json:"nonce"`
	CNonce    string `json:"cnonce"`
	Nc        string `json:"nc,omitempty"`
	Response  string `json:"response"`
	Algorithm string `json:"algorithm"`
}

func SendRequest(ip netip.Addr, rpcMethod RPCMethod, params any, auth *Auth) (json.RawMessage, error) {
	url, err := url.Parse(fmt.Sprintf("http://%s/rpc", ip))
	if err != nil {
		return nil, err
	}
	client := &http.Client{Timeout: 5 * time.Second}

	resp, body, err := doRequest(client, url, rpcMethod, params, nil)
	if err != nil {
		return nil, err
	}

	// If challenged with 401, perform Digest Auth
	if resp.StatusCode == 401 && auth != nil {
		authHeader := resp.Header.Get("WWW-Authenticate")
		log.Debug().Str("header", authHeader).Msg("")

		realm := findMatch(`realm="([^"]+)"`, authHeader)
		nonce := findMatch(`nonce="([^"]+)"`, authHeader) // Keep as string (uptime:signature)
		qop := findMatch(`qop="([^"]+)"`, authHeader)

		if realm == "" || nonce == "" {
			return nil, fmt.Errorf("failed to parse WWW-Authenticate header")
		}

		cnonce := fmt.Sprintf("%x", time.Now().UnixNano())
		nc := "00000001"

		// 1. HA1 = SHA256(user:realm:password)
		ha1Raw := sha256.Sum256([]byte(fmt.Sprintf("%s:%s:%s", auth.Username, realm, auth.Password)))
		ha1 := hex.EncodeToString(ha1Raw[:])

		// 2. HA2 = SHA256(method:uri)
		// Matched to ESP32: snprintf(buffer, sizeof(buffer), "%s:%s", method, auth->uri);
		ha2Raw := sha256.Sum256([]byte("POST:/rpc"))
		ha2 := hex.EncodeToString(ha2Raw[:])

		// 3. Response = SHA256(HA1:nonce:nc:cnonce:qop:HA2)
		// Matched to ESP32: snprintf(buffer, ..., "%s:%s:%s:%s:%s:%s", ha1, nonce, nc, cnonce, qop, ha2);
		sigStr := fmt.Sprintf("%s:%s:%s:%s:%s:%s", ha1, nonce, nc, cnonce, qop, ha2)
		sigRaw := sha256.Sum256([]byte(sigStr))

		rpcAuth := &RPCAuth{
			Realm:     realm,
			Username:  auth.Username,
			Nonce:     nonce,
			CNonce:    cnonce,
			Nc:        nc,
			Response:  hex.EncodeToString(sigRaw[:]),
			Algorithm: "SHA-256",
		}

		resp, body, err = doRequest(client, url, rpcMethod, params, rpcAuth)
		if err != nil {
			return nil, fmt.Errorf("failed sending authed request: %v", err)
		}
		if resp.StatusCode != http.StatusOK {
			return nil, fmt.Errorf("non OK response after auth: %d ", resp.StatusCode)
		}
	}

	var finalResp struct {
		Result json.RawMessage `json:"result"`
		Error  any             `json:"error"`
	}
	if err := json.Unmarshal(body, &finalResp); err != nil {
		return nil, fmt.Errorf("failed to unmarshal: %v | Body: %s", err, string(body))
	}

	if finalResp.Error != nil {
		return nil, fmt.Errorf("api error: %v", finalResp.Error)
	}

	return finalResp.Result, nil
}

func doRequest(client *http.Client, url *url.URL, method RPCMethod, params any, auth *RPCAuth) (*http.Response, []byte, error) {
	// 1. Create the clean JSON-RPC payload (No Auth inside)
	payload := RPCRequest{
		JSONRPC: "2.0",
		ID:      1,
		Method:  method,
		Params:  params,
	}

	b, err := json.Marshal(payload)
	if err != nil {
		return nil, nil, err
	}

	req, err := http.NewRequest(http.MethodPost, url.String(), bytes.NewBuffer(b))
	if err != nil {
		return nil, nil, err
	}

	req.Header.Set("Content-Type", "application/json")

	if auth != nil {
		authStr := fmt.Sprintf(
			"Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", cnonce=\"%s\", nc=%s, qop=%s, response=\"%s\", algorithm=\"%s\"",
			auth.Username, auth.Realm, auth.Nonce, "/rpc", auth.CNonce, auth.Nc, "auth", auth.Response, auth.Algorithm,
		)
		req.Header.Set("Authorization", authStr)
	}

	resp, err := client.Do(req)
	if err != nil {
		return nil, nil, err
	}
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	return resp, body, err
}

func findMatch(pattern, text string) string {
	re := regexp.MustCompile(pattern)
	match := re.FindStringSubmatch(text)
	if len(match) > 1 {
		return match[1]
	}
	return ""
}
