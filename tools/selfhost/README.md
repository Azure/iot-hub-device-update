# selfhost scripts

This dir contains scripts for use in bootstrapping devices (real or virtual) to run the agent and including additional functionality/convenience for self-hosting scenarios.

## bootstrap.sh

- Installs python3 and any PyPi pip modules needed
- It does not validate arguments but passes through to bootstrap.py
- Calls bootstrap.py with arguments passed.

## bootstrap.py

Runs in 2 modes

- Bundle creation mode (bundler role)
- Bootstrap device mode (self-hoster role)

### Bundle Creation Mode

- `./bootstrap.sh --bundle-name selfhost_1 --config /path/to/bootstrap_selfhost_1.json --out-dir /path/to/out_dir --work-dir /path/to/work-dir`

### Bootstrap Device Mode

- `./bootstrap.sh --install [--verbose]`
- or `./bootstrap.sh -i [-v]`
