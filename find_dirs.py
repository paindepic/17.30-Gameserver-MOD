#!/usr/bin/env python3
import os

project_dir = "/home/engine/project"
for item in os.listdir(project_dir):
    full_path = os.path.join(project_dir, item)
    if os.path.isdir(full_path):
        print(f"DIR: {item}")
    elif os.path.isfile(full_path):
        print(f"FILE: {item}")
