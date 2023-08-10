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
      script = "./scripts/install-deps.sh"
  }

}
