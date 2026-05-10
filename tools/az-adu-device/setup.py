#!/usr/bin/env python
"""Setup for az-adu-device Azure CLI extension."""

from setuptools import setup, find_packages

VERSION = "0.1.0"

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

DEPENDENCIES = [
    "paramiko>=2.0",
]

setup(
    name="az-adu-device",
    version=VERSION,
    description="Azure CLI extension for ADU device-side agent management and diagnostics",
    long_description=long_description,
    long_description_content_type="text/markdown",
    license="MIT",
    author="Microsoft Corporation",
    author_email="adusupport@microsoft.com",
    url="https://github.com/Azure/adu-agent-ignite",
    packages=find_packages(exclude=["tests"]),
    install_requires=DEPENDENCIES,
    package_data={"azext_adu_device": ["azext_metadata.json"]},
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
    ],
    python_requires=">=3.8",
)
