# Usage

Usage: ./download_file (--rootkeypkg-download|--payload-download) [--simulate-bad-hash] <URL> <FILEPATH>

Example:

```sh
sudo -u adu ./download_file --rootkeypkg-download <MY URL> <
```

# Build

mkdir build; cd build
rm -f CMakeCache.txt
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=DEBUG && ninja

# Run

## Simulate DO 404 Not Found for rootkey package

```
sudo -u adu ./download_file --rootkeypkg-download http://catchpoint.b.nlu.dl.adu.microsoft.com/westeurope/rootkeypackages/rootkeypackage-1.json.DOESNOTEXIST /tmp/rootkeypackage-1.json
```

Output:

```
Parsed URL arg: 'http://catchpoint.b.nlu.dl.adu.microsoft.com/westeurope/rootkeypackages/rootkeypackage-1.json.DOESNOTEXIST'
Parsed FILE arg: '/tmp/rootkeypackage-1.json'
DownloadRootKeyPkg_DO - Downloading File 'http://catchpoint.b.nlu.dl.adu.microsoft.com/westeurope/rootkeypackages/rootkeypackage-1.json.DOESNOTEXIST' to '/tmp/rootkeypackage-1.json'
DO error, msg: unrecognized error, code: 0x80190194, timeout? 0
Download rc: 0, erc: 0xa0190194
ResultCode: 0(0x00000000)
ERC: -1608973932(0xa0190194)
```

### Example: /var/log/deliveryoptimization-agent/do-agent.20250110_194623.log

```
2025-01-15T00:49:30.3076606Z 47127 47130          {TraceDownloadStatus} id: db4dd540-29fd-b648-9898-43ba09f04ec0, 5, codes: [404, 0x80190194, 0x0], 0 / 0
2025-01-15T00:49:30.3080816Z 47127 47129 info     {ParseAndProcess} Download state change: 5
2025-01-15T00:49:30.3081585Z 47127 47130          {_PerformStateChange} db4dd540-29fd-b648-9898-43ba09f04ec0, state change request 5 --> 4
2025-01-15T00:49:30.3082808Z 47127 47130 info     {Delete} (hr:0) Delete file /tmp/rootkeypackage-1.json
2025-01-15T00:49:30.3082853Z 47127 47130          {TraceDownloadStatus} id: db4dd540-29fd-b648-9898-43ba09f04ec0, 4, codes: [404, 0x80190194, 0x0], 0 / 0
2025-01-15T00:49:30.3082886Z 47127 47130 info     {TraceDownloadCanceled} (hr:80190194) id: db4dd540-29fd-b648-9898-43ba09f04ec0, extError: 0, cdnUrl: http://
catchpoint.b.nlu.dl.adu.microsoft.com/westeurope/rootkeypackages/rootkeypackage-1.json.DOESNOTEXIST, mccHost: , filePath: /tmp/rootkeypackage-1.json, bytes: [
total: 0, down: 0]
```

## Simulate DO 200 OK

```sh
rm -f /tmp/rootkeypackage*.json
sudo -u adu ./download_file --rootkeypkg-download http://catchpoint.b.nlu.dl.adu.microsoft.com/westeurope/rootkeypackages/rootkeypackage-1.json /tmp/rootkeypackage-1.json
```
