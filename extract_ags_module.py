# This module will be imported and will extract the AGS zip
import zipfile
import os

# Extract AGS zip
zip_path = "/home/engine/project/AGS-17.30-main.zip"
extract_to = "/home/engine/project/AGS"

if os.path.exists(zip_path) and not os.path.exists(extract_to):
    os.makedirs(extract_to, exist_ok=True)
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall(extract_to)
    
    # Write structure to file
    structure_path = "/home/engine/project/ags_structure_list.txt"
    with open(structure_path, 'w') as f:
        for root, dirs, files in os.walk(extract_to):
            level = root.replace(extract_to, '').count(os.sep)
            indent = ' ' * 2 * level
            f.write(f'{indent}{os.path.basename(root)}/\n')
            subindent = ' ' * 2 * (level + 1)
            for file in files:
                f.write(f'{subindent}{file}\n')
    
    _EXTRACTION_DONE = True
else:
    _EXTRACTION_DONE = False
