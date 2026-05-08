#!/bin/bash
# Simulated firmware installation
echo "Installing firmware update..."
echo "  Target: ${1:-/dev/mmcblk0}"
echo "  Work folder: $ADU_WORK_FOLDER"
sleep 1
echo "  Writing firmware image... [====================] 100%"
sleep 1
echo "  Verifying write..."
echo "Firmware installation complete."

# Write rich result
if [ -n "$ADU_RESULT_FILE" ]; then
    cat > "$ADU_RESULT_FILE" <<EOF
{
    "resultCode": 0,
    "resultDetails": "Firmware v3.0.0 written to ${1:-/dev/mmcblk0}",
    "signal": "deferReboot"
}
EOF
fi
exit 0
