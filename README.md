# esplatform-32

IoT base project for ESP32 microcontrollers

THIS PROJECT IS HIGHLY EXPERIMENTAL!

The code is unpolished and potentially broken in parts, there is no guarantee tomorrows version will work with todays config etc.

Be aware of this if you want to use it or wait for a potential first release version.

## features

* `/metrics` endpoint for prometheus
* `/rpc` JSONRPC api with authentication
* `/info` to get some basic device info

## examples

### system info
```
$ curl http://10.1.3.162/info | jq
{
  "id": "2805A5353814",
  "name": "ESP32-2805A5353814",
  "fw": "v0.0.0-alpha",
  "auth": "true",
  "auth_domain": "2805A5353814",
  "device": {
    "model": "esp32",
    "arch": "Xtensa",
    "chip_revision": 3,
    "cores": 2,
    "cpu_freq_mhz": 240,
    "mac": "2805A5353814",
    "memory_kb": 0,
    "flash_mb": 4,
    "flash_mode": "DIO",
    "flash_speed_mhz": 40,
    "wifi_gen": "Wi-Fi 4 (n)",
    "wifi_band": "2.4GHz",
    "bt_version": "4.2",
    "bt_classic": true,
    "zigbee_capable": false,
    "matter_capable": true,
    "adc": 12,
    "usb_otg": false,
    "touch_gpio": true
  }
}
```

### metrics

```
$ curl http://10.1.3.162/metrics
# HELP esp32_temperature_c Current temperature
# TYPE esp32_temperature_c gauge
esp32_temperature_c{id="2805A5353814",name="ESP32-2805A5353814"} 50.56
# HELP esp32_clockspeed CPU clockspeed
# TYPE esp32_clockspeed gauge
esp32_clockspeed{id="2805A5353814",name="ESP32-2805A5353814"} 240
# HELP esp32_memory_bytes Memory usage
# TYPE esp32_memory_bytes gauge
esp32_memory_bytes{id="2805A5353814",name="ESP32-2805A5353814",type="total"} 330784
esp32_memory_bytes{id="2805A5353814",name="ESP32-2805A5353814",type="free"} 234984
esp32_memory_byte# HELP esp32_fs_bytes Filesystem stats
# TYPE esp32_fs_bytes gauge
esp32_fs_bytes{id="2805A5353814",name="ESP32-2805A5353814",type="total"} 1441792
esp32_fs_bytes{id="2805A5353814",name="ESP32-2805A5353814",type="used"} 8192
# HELP esp32_wifi_rssi_dbm WiFi signal strength
# TYPE esp32_wifi_rssi_dbm gauge
esp32_wifi_rssi_dbm{id="2805A5353814",name="ESP32-2805A5353814"} -64
# HELP esp32_uptime_seconds System uptime
# TYPE esp32_uptime_seconds counter
esp32_uptime_seconds{id="2805A5353814",name="ESP32-2805A5353814"} 524
```

### RPC api

#### Config.Get

```
$go run cmd/tool/main.go -u admin -p 2805A5353814 -a 10.1.3.162 -r Config.Get|jq
{
  "sys": {
    "id": "2805A5353814",
    "led": false,
    "auth": {
      "enabled": true,
      "username": "admin"
    },
    "device": {
      "name": "ESP32-2805A5353814",
      "mac": "2805A5353814",
      "fwVersion": "v0.0.0-alpha"
    }
  },
  "wifi": {
    "ap": {
      "enabled": true,
      "ssid": "ESP32-2805A5353814"
    },
    "sta": {
      "enabled": true,
      "ssid": "Metalmania-iot"
    }
  }
}
```

#### Config.Set

```
$ go run cmd/tool/main.go -u admin -p 2805A5353814 -a 10.1.3.162 -r Config.Set -d '{"config":{"sys":{"led": true}}}'
{}
```

#### Reboot

```
$ go run cmd/tool/main.go -u admin -p 2805A5353814 -a 10.1.3.162 -r System.Reboot
```

#### "Factory" reset

Delete the local config and reboot the device.

```
$ go run cmd/tool/main.go -u admin -p 2805A5353814 -a 10.1.3.162 -r System.Reset
```
