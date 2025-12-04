packer {
  required_plugins {
    docker = {
      version = ">= 0.0.7"
      source  = "github.com/hashicorp/docker"
    }
  }
}

# Variables for customization
variable "source_path" {
  type    = string
  default = "../.."
  description = "Path to local source code (relative to packer directory)"
}

variable "container_tag" {
  type    = string
  default = "local"
  description = "Tag for the resulting Docker image"
}

# ============================================================================
# LOCAL SOURCE BUILD - Uses your local workspace files
# ============================================================================

source "docker" "debian12_amd64_local" {
  image    = "amd64/debian:12"
  commit   = true
  platform = "linux/amd64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

source "docker" "debian12_arm64_local" {
  image    = "arm64v8/debian:12"
  commit   = true
  platform = "linux/arm64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

source "docker" "debian11_amd64_local" {
  image    = "amd64/debian:11"
  commit   = true
  platform = "linux/amd64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

source "docker" "ubuntu2204_amd64_local" {
  image    = "amd64/ubuntu:22.04"
  commit   = true
  platform = "linux/amd64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

build {
  name = "adu-local"

  sources = [
    "source.docker.debian12_amd64_local",
    "source.docker.debian12_arm64_local",
    "source.docker.debian11_amd64_local",
    "source.docker.ubuntu2204_amd64_local"
  ]

  # Install system prerequisites
  provisioner "shell" {
    inline = [
      "apt-get update",
      "apt-get install -y tzdata",
      "ln -fs /usr/share/zoneinfo/UTC /etc/localtime",
      "dpkg-reconfigure --frontend noninteractive tzdata",
      "apt-get install -y apt-utils ca-certificates rsync"
    ]
  }

  # Copy local source code into container
  provisioner "file" {
    source      = "${var.source_path}/"
    destination = "/iot-hub-device-update/"
  }

  # Install dependencies
  provisioner "shell" {
    inline = [
      "cd /iot-hub-device-update",
      "ls -la scripts/install-deps.sh",
      "./scripts/install-deps.sh --install-aduc-deps --install-do --install-cmake --install-shellcheck --install-delta"
    ]
  }

  # Build the project
  provisioner "shell" {
    inline = [
      "cd /iot-hub-device-update",
      "./scripts/build.sh --clean --build-unit-tests --build-packages --type MinSizeRel --delta-handler"
    ]
  }

  # Run unit tests
  provisioner "shell" {
    inline = [
      "cd /iot-hub-device-update/out",
      "ctest --output-on-failure"
    ]
  }

  # Output build information
  provisioner "shell" {
    inline = [
      "echo '=== Build Complete ==='",
      "cd /iot-hub-device-update/out",
      "ls -lh *.deb 2>/dev/null || echo 'No .deb packages found'",
      "uname -m",
      "lsb_release -a 2>/dev/null || cat /etc/os-release"
    ]
  }

  # Tag the image
  post-processor "docker-tag" {
    repository = "adu-delta-agent-local"
    tags       = ["${var.container_tag}", "${source.name}"]
  }
}
