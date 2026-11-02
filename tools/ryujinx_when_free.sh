#!/bin/bash
# kameo-nx: tools/run_ryujinx.ps1, asked again every 20 s while it refuses because two Ryujinx are running across the ports or
# free commit memory is short (the user's rules), for up to 30 minutes.
#   bash tools/ryujinx_when_free.sh <seconds> <name> <toml>
cd "$(dirname "$0")/.." || exit 1
code=2
for attempt in $(seq 1 90); do
  powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_ryujinx.ps1 -Seconds "$1" -Name "$2" -Toml "$3" > "out/$2.txt" 2>&1
  code=$?
  if [ $code -ne 2 ] && [ $code -ne 3 ]; then
    break
  fi
  sleep 20
done
echo "run_ryujinx exit $code after $attempt attempts"
