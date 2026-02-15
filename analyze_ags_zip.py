import zipfile
import os

zip_path = "/home/engine/project/AGS-17.30-main.zip"
output_file = "/home/engine/project/ags_structure.txt"

try:
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        with open(output_file, 'w') as f:
            f.write("AGS-17.30-main.zip Contents:\n")
            f.write("=" * 60 + "\n\n")
            for name in sorted(zip_ref.namelist()):
                f.write(name + "\n")
            
            f.write("\n\nDetailed Structure:\n")
            f.write("=" * 60 + "\n\n")
            
            # Build tree structure
            tree = {}
            for name in zip_ref.namelist():
                parts = name.split('/')
                current = tree
                for part in parts:
                    if part not in current:
                        current[part] = {}
                    current = current[part]
            
            def print_tree(d, indent=0):
                for key, value in sorted(d.items()):
                    f.write('  ' * indent + (key if key else '(empty)') + '\n')
                    if value:
                        print_tree(value, indent + 1)
            
            print_tree(tree)
    
    print(f"Successfully wrote structure to {output_file}")
except Exception as e:
    with open(output_file, 'w') as f:
        f.write(f"Error: {e}\n")
    print(f"Error: {e}")
