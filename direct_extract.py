import zipfile
import os

zip_path = "/home/engine/project/AGS-17.30-main.zip"
extract_to = "/home/engine/project/AGS_extracted"

# Create extraction directory
os.makedirs(extract_to, exist_ok=True)

# Extract the zip file
with zipfile.ZipFile(zip_path, 'r') as zip_ref:
    zip_ref.extractall(extract_to)

# List the extracted structure
def print_dir_structure(start_path, prefix=""):
    items = sorted(os.listdir(start_path))
    for i, item in enumerate(items):
        path = os.path.join(start_path, item)
        is_last = i == len(items) - 1
        current_prefix = "└── " if is_last else "├── "
        print(f"{prefix}{current_prefix}{item}")
        if os.path.isdir(path):
            extension = "    " if is_last else "│   "
            print_dir_structure(path, prefix + extension)

print("Extracted structure:")
print_dir_structure(extract_to)

# Save to file
with open("/tmp/ags_structure.txt", "w") as f:
    def write_structure(start_path, prefix=""):
        items = sorted(os.listdir(start_path))
        for i, item in enumerate(items):
            path = os.path.join(start_path, item)
            is_last = i == len(items) - 1
            current_prefix = "└── " if is_last else "├── "
            f.write(f"{prefix}{current_prefix}{item}\n")
            if os.path.isdir(path):
                extension = "    " if is_last else "│   "
                write_structure(path, prefix + extension)
    
    write_structure(extract_to)
