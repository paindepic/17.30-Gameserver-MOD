import zipfile

zip_path = "/home/engine/project/AGS-17.30-main.zip"

# Read the zip file contents
with zipfile.ZipFile(zip_path, 'r') as zip_ref:
    print("Contents of AGS-17.30-main.zip:")
    print("=" * 60)
    for name in zip_ref.namelist():
        print(name)
    
    # Save to file
    with open("/tmp/ags_zip_contents.txt", "w") as f:
        for name in zip_ref.namelist():
            f.write(name + "\n")

print("\nSaved to /tmp/ags_zip_contents.txt")
