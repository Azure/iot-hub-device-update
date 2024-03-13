
# rootkeypackage test app

## Setup and run python http server

- Ensure deliveryoptimization-agent is installed and running
- Ensure `adu` user is in `do` group and `do` user is in `do` and `adu` groups
- Run following steps:

```sh
sudo rm -rf /tmp/deviceupdate/rootkey_download_test_app
mkdir /tmp/htdocs
cp src/utils/rootkeypackage_utils/tests/scripts/rootkeypkg/rootkeyfiles/rootkey.json /tmp/htdocs/
pushd /tmp/htdocs
python3 -m http.server 8083 &
popd
```

## Run as adu user

```sh
sudo -u adu ./out/bin/rootkeypackage_test_app
```
