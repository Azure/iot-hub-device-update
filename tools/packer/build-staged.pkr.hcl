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

variable "stage" {
  type    = string
  default = "all"
  description = "Build stage: deps, build, test, or all"
}

# ============================================================================
# STAGE 1: Dependencies Image (Reusable base)
# ============================================================================

source "docker" "debian12_amd64_deps" {
  image    = "amd64/debian:12"
  commit   = true
  platform = "linux/amd64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

source "docker" "debian12_arm64_deps" {
  image    = "arm64v8/debian:12"
  commit   = true
  platform = "linux/arm64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /iot-hub-device-update"
  ]
}

build {
  name = "adu-deps"

  sources = [
    "source.docker.debian12_amd64_deps",
    "source.docker.debian12_arm64_deps"
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

  # Install dependencies (this is the slow part - we cache this!)
  provisioner "shell" {
    inline = [
      "cd /iot-hub-device-update",
      "./scripts/install-deps.sh --install-aduc-deps --install-do --install-cmake --install-shellcheck --install-delta"
    ]
  }

  # Tag the dependencies image for reuse
  post-processor "docker-tag" {
    repository = "adu-delta-agent-deps"
    tags       = ["${var.container_tag}", "${source.name}"]
  }
}

# ============================================================================
# STAGE 2: Build Image (Uses deps image as base)
# ============================================================================

source "docker" "debian12_amd64_build" {
  image    = "adu-delta-agent-deps:debian12_amd64_deps"
  commit   = true
  pull     = false  # Don't pull, use local image
  changes = [
    "WORKDIR /iot-hub-device-update"
  ]
}

source "docker" "debian12_arm64_build" {
  image    = "adu-delta-agent-deps:debian12_arm64_deps"
  commit   = true
  pull     = false
  changes = [
    "WORKDIR /iot-hub-device-update"
  ]
}

build {
  name = "adu-build"

  sources = [
    "source.docker.debian12_amd64_build",
    "source.docker.debian12_arm64_build"
  ]

  # Update code (in case it changed)
  provisioner "shell" {
    inline = [
      "cd /iot-hub-device-update",
      "git fetch origin",
      "git checkout ${var.git_branch}",
      "git pull"
    ]
  }

  # Build the project
  provisioner "shell" {
    inline = [
      "cd /iot-hub-device-update",
      "./scripts/build.sh --clean --build-unit-tests --build-packages --type MinSizeRel --delta-handler"
    ]
  }

  # Tag the build image
  post-processor "docker-tag" {
    repository = "adu-delta-agent-build"
    tags       = ["${var.container_tag}", "${source.name}"]
  }
}

# ============================================================================
# STAGE 3: Test & Package (Uses build image as base)
# ============================================================================

source "docker" "debian12_amd64_final" {
  image    = "adu-delta-agent-build:debian12_amd64_build"
  commit   = true
  pull     = false
  changes = [
    "WORKDIR /iot-hub-device-update"
  ]
}

source "docker" "debian12_arm64_final" {
  image    = "adu-delta-agent-build:debian12_arm64_build"
  commit   = true
  pull     = false
  changes = [
    "WORKDIR /iot-hub-device-update"
  ]
}

build {
  name = "adu-final"

  sources = [
    "source.docker.debian12_amd64_final",
    "source.docker.debian12_arm64_final"
  ]

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

  # Final tagged image
  post-processor "docker-tag" {
    repository = "adu-delta-agent"
    tags       = ["${var.container_tag}", "${source.name}"]
  }
}
