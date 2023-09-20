# Azure DPS 2 Client Tests

### Prerequisites

- Root and Intermediate certificate files must be installed on the test device.

```bash
# NOTE | files extension must be '.crt' for update-ca-certificates to pick them up.
sudo cp /path/to/my/certs/*.crt /usr/local/share/ca-certificates

sudo update-ca-certificates
```

- **/tmp/adu/testdata/mqtt-client-functional-test-config/du-config.json** must be updated accordingly

```json
 "connectionSource": {
          "connectionType": "MQTTBroker",
          "connectionData": {
            "mqttBroker" : {
              "hostName" : "my-mqtt-broker.westus2-1.ts.eventgrid.azure.net",
              "tcpPort" : 8883,
              "ustTLS": true,
              "cleanSession" : true,
              "keepAliveInSeconds" : 3600,
              "clientId" : "my-device-red-device02-devbox-wus3",
              "userName" : "my-device-red-device02-devbox-wus3",
              "certFile" : "/home/me/aducerts/my-device-red-device02-devbox-wus3.cer",
              "keyFile" : "/home/me/aducerts/my-device-red-device02-devbox-wus3.key"
            }
          }
        },
```

> NOTE | **caFile** is not required when using Mosquitto Client SDK. **clientId** and **userName** should match the 'CN' in the device certificate.


## Troubleshooting Connection Error

- Ensure that port 8883 is open on the endpoint
```sh
$nmap -p 8883 adutest-devbox-wus3-dps.azure-devices-provisioning.net
Starting Nmap 7.80 ( https://nmap.org ) at 2023-09-11 02:02 PDT
Nmap scan report for adutest-devbox-wus3-dps.azure-devices-provisioning.net (20.150.165.200)
Host is up (0.058s latency).

PORT     STATE SERVICE
8883/tcp open  secure-mqtt

Nmap done: 1 IP address (1 host up) scanned in 1.30 seconds
```
