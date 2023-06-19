packer {
    required_plugins {
        docker = {
            version = ">= 0.0.7"
            source  = "github.com/hashicorp/docker"
        }
    }
}

variable "docker_tag" {
    type    = string
    default = "latest"
}

variable "diffgen_tool_zip_filepath" {
    type    = string
    default = "/opt/adu/linux64-diffgen-tool.zip"
}

source "docker" "ubuntu-focal" {
    image  = "ubuntu:focal"
    commit = true
    pull = true
    changes = [
        "ENTRYPOINT [\"/tmp/generate-delta.sh\"]"
    ]
}

build {
    name = "deviceupdate-deltagen"
    sources = [
        "source.docker.ubuntu-focal",
    ]

    provisioner "file" {
        source = "scripts/generate-delta.sh"
        destination = "/tmp/generate-delta.sh"
    }

    provisioner "file" {
        source = "scripts/install-prereqs.sh"
        destination = "/tmp/install-prereqs.sh"
    }

    provisioner "file" {
        source = var.diffgen_tool_zip_filepath
        destination = "/tmp/linux64-diffgen-tool.zip"
    }

    provisioner "shell" {
        scripts = [
            "scripts/configure-apt-repo.sh",
            "scripts/install-prereqs.sh",
            "scripts/install-diffgen-tool.sh",
        ]
    }

    post-processor "docker-tag" {
        repository = "dudeltagen"
        tags = ["ubuntu-focal", var.docker_tag]
        only = ["docker.ubuntu-focal"]
    }
}

