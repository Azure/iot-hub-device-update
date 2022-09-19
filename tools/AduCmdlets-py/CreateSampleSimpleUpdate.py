#!/usr/bin/env python3

#
# Device Update for IoT Hub
# Copyright (c) Microsoft Corporation.
#
# Create a sample simple update.
# The generated sample update contains fake files and cannot be actually installed onto a device.
#
# Example:
# CreateSampleSimpleUpdate.py --path './test' --update-version '1.2'
#

import argparse
import os
import sys
import tempfile
import scripts.aduupdate as adu_update


def main():
    parser = argparse.ArgumentParser(
        description='Create a sample update with mix of inline and reference install steps')
    parser.add_argument(
        '--path',
        required=True,
        help='Directory to create the import manifest JSON files in')
    parser.add_argument(
        '--update-version',
        required=True,
        help='Version of update to create and import - major.minor[.build[.revision]]')
    args = parser.parse_args()

    output_path = args.path
    update_version = args.update_version

    os.makedirs(output_path, exist_ok=True)

    # Use arbitrary files as update payload files in this sample.
    # Production code would reference real update files.

    file_name = 'readme.txt'

    file = open(
        os.path.join(output_path, file_name),
        'w')
    with file:
        file.write('This is the payload file.')

    update_id = adu_update.update_id(
        provider='Contoso',
        name='Toaster',
        version=update_version)

    compatibility_info = adu_update.compatibility_info(
        {
            'manufacturer': 'Contoso',
            'model': 'Toaster'
        })

    # Note that files is passed as a list.
    installation_step = adu_update.inline_installation_step(
        handler='microsoft/swupdate:1',
        handler_properties={'installedCriteria': '1.0'},
        files=[file.name])

    # Note that compatibility_infos and installation_steps are passed as lists.
    update = adu_update.import_manifest(
        update_id=update_id,
        compatibility_infos=[compatibility_info],
        installation_steps=[installation_step])

    with open(os.path.join(output_path, 'contoso.toaster.1.0.importmanifest.json'), 'w') as f:
        f.write(str(update))


if __name__ == '__main__':
    main()
