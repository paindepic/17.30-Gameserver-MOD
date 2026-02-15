#!/usr/bin/env python3
import subprocess
import sys

# Execute Python script and capture output
result = subprocess.run(
    ['python3', '/home/engine/project/read_zip.py'],
    capture_output=True,
    text=True,
    timeout=30
)

# Write output to a file
with open('/tmp/ags_zip_output.txt', 'w') as f:
    f.write("STDOUT:\n")
    f.write(result.stdout)
    f.write("\n\nSTDERR:\n")
    f.write(result.stderr)
    f.write(f"\n\nReturn Code: {result.returncode}")

print("Done, output saved to /tmp/ags_zip_output.txt")
