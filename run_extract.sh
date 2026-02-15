#!/bin/bash
python3 /home/engine/project/extract_ags.py > /tmp/ags_extract.log 2>&1
echo "Exit code: $?"
cat /tmp/ags_extract.log
