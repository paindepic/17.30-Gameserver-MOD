#!/usr/bin/env python3
import zipfile
import os

# Extract AGS-17.30-main.zip
print("Extracting AGS-17.30-main.zip...")
with zipfile.ZipFile('/home/engine/project/AGS-17.30-main.zip', 'r') as zip_ref:
    zip_ref.extractall('/home/engine/project/')

print("Done!")
