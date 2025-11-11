packer {
  required_plugins {
    docker = {
      version = ">= 0.0.7"
      source  = "github.com/hashicorp/docker"
    }
  }
}

# Variables for customization
variable "git_repo_url" {
  type    = string
  default = "https://github.com/azure/iot-hub-device-update.git"
  description = "Git repository URL to clone"
}

variable "git_branch" {
  type    = string
  default = "user/nox-msft/vnext-delta"
  description = "Git branch to checkout"
}

variable "container_tag" {
  type    = string
  default = "latest"
  description = "Tag for the resulting Docker image"
}

# Debian 12 sources
source "docker" "debian12_arm64" {
  image    = "arm64v8/debian:12"
  commit   = true
  platform = "linux/arm64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

source "docker" "debian12_amd64" {
  image    = "amd64/debian:12"
  commit   = true
  platform = "linux/amd64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

# Debian 11 sources
source "docker" "debian11_arm64" {
  image    = "arm64v8/debian:11"
  commit   = true
  platform = "linux/arm64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

source "docker" "debian11_amd64" {
  image    = "amd64/debian:11"
  commit   = true
  platform = "linux/amd64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

# Ubuntu 22.04 sources
source "docker" "ubuntu2204_arm64" {
  image    = "arm64v8/ubuntu:22.04"
  commit   = true
  platform = "linux/arm64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

source "docker" "ubuntu2204_amd64" {
  image    = "amd64/ubuntu:22.04"
  commit   = true
  platform = "linux/amd64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

# Ubuntu 20.04 sources
source "docker" "ubuntu2004_arm64" {
  image    = "arm64v8/ubuntu:20.04"
  commit   = true
  platform = "linux/arm64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

source "docker" "ubuntu2004_amd64" {
  image    = "amd64/ubuntu:20.04"
  commit   = true
  platform = "linux/amd64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

# Build configuration
build {
  name = "adu-delta-agent"

  sources = [
    "source.docker.debian12_arm64",
    "source.docker.debian12_amd64",
    "source.docker.debian11_arm64",
    "source.docker.debian11_amd64",
    "source.docker.ubuntu2204_arm64",
    "source.docker.ubuntu2204_amd64",
    "source.docker.ubuntu2004_arm64",
    "source.docker.ubuntu2004_amd64"
  ]

  # Install system prerequisites
  provisioner "shell" {
    inline = [
      "apt-get update",
      "apt-get install -y tzdata",
      "ln -fs /usr/share/zoneinfo/UTC /etc/localtime",
      "dpkg-reconfigure --frontend noninteractive tzdata",
      "apt-get install -y apt-utils git ca-certificates"
    ]
  }

  # Clone the repository
  provisioner "shell" {
    inline = [
      "git clone --branch ${var.git_branch} ${var.git_repo_url} /iot-hub-device-update",
      "cd /iot-hub-device-update",
      "git log -1 --oneline"
    ]
  }

  # Install dependencies
  provisioner "shell" {
    inline = [
      "cd /iot-hub-device-update",
      "./scripts/install-deps.sh --install-aduc-deps --install-do --install-cmake --install-shellcheck --install-delta"
    ]
  }

  # Build the project with delta handler support
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

  # Post-processor to tag the image
  post-processor "docker-tag" {
    repository = "adu-delta-agent"
    tags       = ["${var.container_tag}", "${source.name}"]
  }
}
