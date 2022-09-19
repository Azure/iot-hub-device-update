#!/usr/bin/env python3

#
# Device Update for IoT Hub
# Copyright (c) Microsoft Corporation.
#
# Create a sample update with mix of inline and reference install steps.
# The generated sample update contains fake files and cannot be actually installed onto a device.
#
# Example:
# CreateSampleComplexUpdate.py --path './test' --update-version '1.2'
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

    # Create child update 1

    print('Preparing child update 1 ("Microphone") ...')

    child_update1_file = open(
        os.path.join(output_path, 'child-update1.txt'),
        'w')
    with child_update1_file:
        child_update1_file.write('This is the child update 1 payload file.')

    child_update1_update_id = adu_update.update_id(
        provider='Contoso',
        name='Microphone',
        version=update_version)

    compatibility_info = adu_update.compatibility_info(
        {
            'manufacturer': 'Contoso',
            'model': 'Microphone'
        })

    installation_step = adu_update.inline_installation_step(
        handler='microsoft/swupdate:1',
        files=[child_update1_file.name])

    child_update1 = adu_update.import_manifest(
        update_id=child_update1_update_id,
        compatibility_infos=[compatibility_info],
        installation_steps=[installation_step],
        is_deployable=False,
        description='This is the Microphone import manifest.')

    # Create child update 2

    print('Preparing child update 2 ("Speaker") ...')

    child_update2_file = open(
        os.path.join(output_path, 'child-update2.txt'),
        'w')
    with child_update2_file:
        child_update2_file.write('This is the child update 2 payload file.')

    child_update2_update_id = adu_update.update_id(
        provider='Contoso',
        name='Speaker',
        version=update_version)

    compatibility_info = adu_update.compatibility_info({
        'manufacturer': 'Contoso',
        'model': 'Speaker'
    })

    installation_step = adu_update.inline_installation_step(
        handler='microsoft/swupdate:1',
        files=[child_update2_file.name])

    child_update2 = adu_update.import_manifest(
        update_id=child_update2_update_id,
        compatibility_infos=[compatibility_info],
        installation_steps=[installation_step],
        is_deployable=False,
        description='This is the Speaker import manifest.')

    # Create the parent update of the child update

    print('Preparing parent update ("SmartDevice") ...')

    parent_file = open(
        os.path.join(output_path, 'parent.txt'),
        'w')
    with parent_file:
        parent_file.write('This is the parent update payload file.')

    parent_update_id = adu_update.update_id(
        provider='Contoso',
        name='SmartDevice',
        version=update_version)

    compatibility_info = adu_update.compatibility_info(
        {
            'manufacturer': 'Contoso',
            'model': 'SmartDevice'
        })

    installation_steps = []
    installation_steps.append(adu_update.inline_installation_step(
        handler='microsoft/script:1',
        files=[parent_file.name],
        handler_properties={'arguments': '--pre'},
        description='Pre-install script'
    ))
    installation_steps.append(adu_update.reference_installation_step(
        update_id=child_update1_update_id,
        description='Microphone Firmware'
    ))
    installation_steps.append(adu_update.reference_installation_step(
        update_id=child_update2_update_id,
        description='Speaker Firmware'
    ))
    installation_steps.append(adu_update.inline_installation_step(
        handler='microsoft/script:1',
        files=[parent_file.name],
        handler_properties={'arguments': '--post'},
        description='Post-install script'
    ))

    parent_update = adu_update.import_manifest(
        update_id=parent_update_id,
        compatibility_infos=[compatibility_info],
        installation_steps=installation_steps,
        description='This is the SmartDevice import manifest.')

    # Write manifest files

    print('Saving manifest files ...')

    child_update1_manifest = os.path.join(
        output_path, f'{child_update1_update_id}.importmanifest.json')
    with open(child_update1_manifest, 'w') as f:
        f.write(str(child_update1))

    child_update2_manifest = os.path.join(
        output_path, f'{child_update2_update_id}.importmanifest.json')
    with open(child_update2_manifest, 'w') as f:
        f.write(str(child_update2))

    parent_update_manifest = os.path.join(
        output_path, f'{parent_update_id}.importmanifest.json')
    with open(parent_update_manifest, 'w') as f:
        f.write(str(parent_update))

    print(f'Import manifest JSON files saved to {output_path}')


if __name__ == '__main__':
    main()
