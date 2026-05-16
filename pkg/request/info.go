package request

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/netip"
)

func GetDeviceInfo(IP *netip.Addr) (*DeviceInfo, error) {
	resp, err := http.Get("http://" + IP.String() + "/info")
	if err != nil {
		return nil, fmt.Errorf("failed requesting device info: %v", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("device info request failed: %v", err)
	}

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return nil, fmt.Errorf("failed reading request answer: %v", err)
	}

	var devInfo DeviceInfo
	err = json.Unmarshal(body, &devInfo)
	if err != nil {
		return nil, fmt.Errorf("failed unmarshalling device info: %v", err)
	}

	return &devInfo, nil
}

type DeviceInfo struct {
	// Common fields
	ID         string `json:"id"`
	Name       string `json:"name"`
	Fw         string `json:"fw"`
	Hw         string `json:"hw"`
	Auth       bool   `json:"auth"`
	AuthDomain bool   `json:"auth_domain"`
}
