packer {
  required_plugins {
    docker = {
      version = ">= 0.0.7"
      source  = "github.com/hashicorp/docker"
    }
  }
}

variable "git_branch" {
  type    = string
  default = "user/nox-msft/vnext-delta-xcompile"
  description = "Git branch to checkout"
}

source "docker" "debian12_amd64_delta_test" {
  image    = "amd64/debian:12"
  commit   = true
  platform = "linux/amd64"
  changes = [
    "ENV DEBIAN_FRONTEND=noninteractive",
    "WORKDIR /test"
  ]
}

build {
  name = "delta-test"
  
  sources = ["source.docker.debian12_amd64_delta_test"]

  # Install system prerequisites
  provisioner "shell" {
    inline = [
      "apt-get update",
      "apt-get install -y tzdata ca-certificates git lsb-release",
      "ln -fs /usr/share/zoneinfo/UTC /etc/localtime",
      "dpkg-reconfigure --frontend noninteractive tzdata"
    ]
  }

  # Clone repository
  provisioner "shell" {
    inline = [
      "git clone --branch ${var.git_branch} https://github.com/azure/iot-hub-device-update.git /test/iot-hub-device-update",
      "cd /test/iot-hub-device-update",
      "git log -1 --oneline"
    ]
  }

  # Install ONLY delta dependencies and build delta library
  provisioner "shell" {
    inline = [
      "cd /test/iot-hub-device-update",
      "echo '=== Testing Delta Library Installation ==='",
      "./scripts/install-deps.sh --install-delta",
      "echo '=== Delta Installation Complete ==='",
      "echo 'Checking installed files:'",
      "ls -lh /usr/local/lib/libazure_iot_delta.a 2>/dev/null || echo 'Library not found'",
      "ls -lh /usr/include/adudiffapi.h 2>/dev/null || echo 'Header not found'"
    ]
  }

  post-processor "docker-tag" {
    repository = "adu-delta-test"
    tags       = ["debian12-amd64"]
  }
}
