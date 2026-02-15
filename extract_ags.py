import zipfile
import os
import sys

# Extract AGS-17.30-main.zip
zip_path = "/home/engine/project/AGS-17.30-main.zip"
extract_to = "/home/engine/project"

if os.path.exists(zip_path):
    print(f"Extracting {zip_path} to {extract_to}...")
    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(extract_to)
            print("Extraction successful!")
            
            # List extracted files
            print("\nExtracted contents:")
            for root, dirs, files in os.walk(extract_to, topdown=True):
                level = root.replace(extract_to, '').count(os.sep)
                indent = ' ' * 2 * level
                print(f'{indent}{os.path.basename(root)}/')
                subindent = ' ' * 2 * (level + 1)
                for file in files[:10]:  # Limit to first 10 files per dir
                    print(f'{subindent}{file}')
                if len(files) > 10:
                    print(f'{subindent}... and {len(files) - 10} more files')
                if level > 2:  # Limit depth
                    dirs[:] = []
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
else:
    print(f"Zip file not found: {zip_path}")
    sys.exit(1)
