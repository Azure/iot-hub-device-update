# A/B Update Strategy Example

## Instruction

- [A/B Update Strategy Example](#ab-update-strategy-example)
  - [Instruction](#instruction)
    - [Prepare Demo Folder and Payloads](#prepare-demo-folder-and-payloads)
    - [Generate Device Update Import Manifest File](#generate-device-update-import-manifest-file)
      - [Options #1](#options-1)

### Prepare Demo Folder and Payloads

This example requires an update file in the 'payloads' folder.

Follow steps below to prepare the payloads:

1. Create a work folder

   ```sh
   work_folder=/tmp/ab-demo
   mkdir -p $work_folder
   cp -fr ./ $work_folder
   pushd $work_folder
   ```

2. Copy your update file (.swu) to `$work_folder/payloads` folder. For example, `/tmp/ab-demo/payload/my-update.swu`
3. **Optional:** copy [AduUpdate.psm1](../../../../../../../tools/AduCmdlets/AduUpdate.psm1) into the work folder

### Generate Device Update Import Manifest File

#### Options #1

Run provided PowerShell script: [create-yocto-ab-rootfs-update-import-manifest-1.0.ps](create-yocto-ab-rootfs-update-import-manifest-1.0.ps1)

- Import ADU Update PowerShell Module

```ps
Import-Module ./AduUpdate.psm1
```

- Generate an import manifest

```ps
./create-yocto-ab-rootfs-update-import-manifest-1.0.ps -SwuFileName 'my-update.swu'
```

The output import manifest and payloads will be copied into `$work_folder/EDS-ADUClient.yocto-update.1.0\`
