#!/bin/bash
cd /home/engine/project
ls -la > /tmp/project_structure.txt
echo "---" >> /tmp/project_structure.txt
find . -type d -maxdepth 2 >> /tmp/project_structure.txt
echo "---" >> /tmp/project_structure.txt
file *.zip >> /tmp/project_structure.txt 2>&1
