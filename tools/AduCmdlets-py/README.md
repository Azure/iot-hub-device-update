# Device Update for IoT Hub - Scripts

This directory contains sample Python scripts for creating an import manifest for Device Update for IoT Hub.

## Creating import manifest

### Simple example

To create a simple update, refer to the Python sample [CreateSampleSimpleUpdate.py](CreateSampleSimpleUpdate.py) for usage. Modify the script as necessary, and run it as follows:

```shell
python ./CreateSampleSimpleUpdate.py --path './output' --update-version '1.1'
```

### Complex example

To create a more complex update that references one or more child updates, refer to the Python sample [CreateSampleComplexUpdate.py](CreateSampleComplexUpdate.py) for usage. Modify the script as necessary, and run it as follows:

```shell
python ./CreateSampleComplexUpdate.py --path './output' --update-version '1.1'
```

## References

- [Import Manifest JSON Schema version 4.0](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/import-schema)
- [Import concepts](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/import-concepts)
