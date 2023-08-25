packer {
  required_plugins {
    docker = {
      version = ">= 0.0.7"
      source  = "github.com/hashicorp/docker"
    }
  }
}

source "docker" "debian_arm64" {
  image    = "arm64v8/debian:11"
  commit   = true
  platform = "linux/arm64"
}

source "docker" "debian_amd64" {
  image    = "amd64/debian:11"
  commit   = true
  platform = "linux/amd64"
}

build {
  name = "duagent"
  sources = [
    "source.docker.debian_arm64",
    "source.docker.debian_amd64"
  ]

  provisioner "shell" {
    inline = [
      "apt-get update && apt-get --yes install apt-utils git",
      "git clone https://github.com/azure/iot-hub-device-update --branch develop",
      "cd iot-hub-device-update && ./scripts/install-deps.sh -a && ./scripts/build.sh -c -u --build-packages"
    ]
  }
}
