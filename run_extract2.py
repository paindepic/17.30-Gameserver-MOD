#!/usr/bin/env python3
import subprocess
import sys

# Run the extraction script
result = subprocess.run(
    ['python3', '/home/engine/project/extract_ags.py'],
    capture_output=True,
    text=True
)

print("STDOUT:")
print(result.stdout)
print("\nSTDERR:")
print(result.stderr)
print(f"\nExit code: {result.returncode}")
