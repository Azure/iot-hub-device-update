#!/usr/bin/env python3

#
# Device Update for IoT Hub
# Copyright (c) Microsoft Corporation.
#
# Python helpers for creating import manifest for Device Update for IoT Hub.
#

import base64
import datetime
import hashlib
import json
import os
import re


class update_id():
    """! Forward reference for update_id class. """
    pass


class compatibility_info():
    """! Forward reference for compatibility_info class. """
    pass


class installation_step():
    """! Forward reference for installation_step class. """
    pass


def get_filehashes(file_path: str) -> dict:
    """! Gets file hashes for a file.

    @param file_path Full path of file.

    @return dict containing hashes. Currently only a single (sha256) hash is returned.
    """
    if not os.path.isfile(file_path) and not os.path.exists('file_path'):
        raise FileNotFoundError('File {file_path} not found')

    with open(os.path.abspath(file_path), 'rb') as f:
        bytes = f.read()
        digest = hashlib.sha256(bytes).digest()

    return {'sha256': base64.b64encode(digest).decode('ascii')}


def get_file_metadata(file_path: str) -> dict:
    """! Retrieves file metadata for a file.

    @param file_path Full path of file

    @return dict containing file metadata.
    """
    return {
        'filename': os.path.basename(file_path),
        'sizeInBytes': os.path.getsize(file_path),
        'hashes': get_filehashes(file_path)
    }


class import_manifest:
    """! Class that represents an update import manifest """

    def __init__(self, update_id: update_id, compatibility_infos: list[compatibility_info], installation_steps: list[installation_step], is_deployable: bool = True, description: str = None) -> None:
        if description is not None and len(description) > 512:
            raise ValueError(f'Description {description} is not valid')

        if len(installation_steps) < 1 or len(installation_steps) > 10:
            raise ValueError(
                f'Invalid number of installation steps (must be 1-10)')

        file_metadatas = []

        for step in installation_steps:
            if type(step) is inline_installation_step:
                if len(step.files) < 1 or len(step.files) > 10:
                    raise ValueError(
                        'Invalid number of files specified for step (must be 1-10)')

                for file in step.files:
                    # Note: step.files are full pathnames, not just filenames
                    meta = get_file_metadata(file_path=file)

                    basename = os.path.basename(file)

                    # In case multiple steps are sharing a payload file.
                    if not any(fm['filename'] == basename for fm in file_metadatas):
                        file_metadatas.append(meta)

        if len(file_metadatas) > 10:
            raise ValueError('Invalid number of payload files (must be <=10)')

        self.import_manifest = {
            'updateId': update_id.__dict__,
            'isDeployable': is_deployable,
            'compatibility': compatibility_infos,
            'createdDateTime': datetime.datetime.utcnow().isoformat(),
            'manifestVersion': '4.0'
        }

        self.import_manifest['instructions'] = {
            "steps": [step.as_dict() for step in installation_steps]
        }

        if description is not None:
            self.import_manifest['description'] = description

        if len(file_metadatas) > 0:
            self.import_manifest['files'] = file_metadatas

    def __str__(self) -> str:
        """! Returns object stringified as JSON """
        return json.dumps(self.import_manifest, indent=4)


class update_id:
    """!
    Class that represents an update id.
    An update id is a represented by { provider, name, version }, all strings.
    """

    def __init__(self, provider: str, name: str, version: str) -> None:
        # Provider

        pattern = re.compile(r'[a-zA-Z0-9.-]{1,64}')
        if not re.fullmatch(pattern, provider):
            raise ValueError(f'provider {provider} is not valid')
        self.provider = provider

        # Name

        pattern = re.compile(r'[a-zA-Z0-9.-]{1,64}')
        if not re.fullmatch(pattern, name):
            raise ValueError(f'name {name} is not valid')
        self.name = name

        # Version (major.minor[.build[.revision]])

        pattern = re.compile(r'\d{1,5}\.\d{1,5}(?:\.\d{1,5}(?:\.\d{1,5})?)?')
        if not re.fullmatch(pattern, version):
            raise ValueError(f'version {version} is not valid')
        self.version = version

    def __str__(self) -> str:
        """! Returns object stringified in format provider.name.version. """
        return f'{self.provider}.{self.name}.{self.version}'


class compatibility_info(dict):
    """! Class that represents an update compatibility. """

    def __init__(self, *arg, **kw):
        super(compatibility_info, self).__init__(*arg, **kw)

        if len(self) > 5:
            raise ValueError(
                'Invalid number of compatibility info properties (must be 1-5)')


class installation_step:
    """! Base class for installation step. Should not be instantiated. """


class inline_installation_step(installation_step):
    """! Class that represents an inline installation step. """

    def __init__(self, handler: str, files: list, handler_properties: dict = None, description: str = None) -> None:
        self.handler = handler
        self.files = files
        self.handler_properties = handler_properties
        self.description = description

    def as_dict(self):
        """! Return object as dict. """
        json = {
            'type': 'inline',
            'handler': self.handler
        }

        # Inline step requires only payload file name, convert full path to filename.
        json['files'] = [os.path.basename(file) for file in self.files]

        if self.handler_properties is not None:
            json['handlerProperties'] = self.handler_properties

        if self.description is not None:
            json['description'] = self.description

        return json

    def __str__(self):
        """ !Return object as str. """
        return f'[{self.handler}; {self.files}; {self.handler_properties}; {self.description}]'


class reference_installation_step(installation_step):
    """! Class that represents a reference installation step. """

    def __init__(self, update_id: update_id, description: str = None):
        if description is not None and len(description) > 512:
            raise ValueError(f'description {description} is not valid')

        self.update_id = update_id
        self.description = description

    def as_dict(self):
        """! Return object as dict. """
        json = {
            'type': 'reference',
            'updateId': self.update_id.__dict__
        }

        if self.description is not None:
            json['description'] = self.description

        return json

    def __str__(self):
        """! Return object as str. """
        return f'[{self.update_id}; {self.description}]'
