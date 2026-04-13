#!/usr/bin/env python3
"""
Zlither PAK Packer - Embeds app/res into a single PAK file
Usage: python3 pak_packer.py <exe_path>
"""

import os
import sys
import struct
from pathlib import Path

PAK_MAGIC = b'VLPAK'
PAK_VERSION = 1

def pack_resources(output_path):
    """Pack all files from app/res into a PAK file"""
    
    res_dir = Path('app/res')
    if not res_dir.exists():
        print(f"ERROR: {res_dir} not found")
        return False
    
    # Collect all files
    files = []
    for root, dirs, filenames in os.walk(res_dir):
        for filename in filenames:
            filepath = Path(root) / filename
            # Relative path from app/res
            relpath = filepath.relative_to(res_dir)
            files.append((relpath, filepath))
    
    if not files:
        print("ERROR: No files found in app/res")
        return False
    
    print(f"Packing {len(files)} files...")
    
    # Write PAK file with simple format:
    # [Magic: "VLPAK"][File count: uint32][Files: name_len+name+size+data]
    with open(output_path, 'wb') as pak:
        pak.write(PAK_MAGIC)
        pak.write(struct.pack('<I', len(files)))
        
        # Write file table with data inline
        for relpath, filepath in files:
            pathstr = str(relpath).replace('\\', '/')
            pathbytes = pathstr.encode('utf-8')
            
            with open(filepath, 'rb') as f:
                filedata = f.read()
            
            # Format: [name_len:4][name][data_len:4][data]
            pak.write(struct.pack('<I', len(pathbytes)))
            pak.write(pathbytes)
            pak.write(struct.pack('<I', len(filedata)))
            pak.write(filedata)
    
    print(f"Created {output_path}")
    return True

def append_pak_to_exe(exe_path, pak_path, output_exe):
    """Append PAK file to exe with a size marker"""
    
    if not os.path.exists(exe_path):
        print(f"ERROR: {exe_path} not found")
        return False
    
    if not os.path.exists(pak_path):
        print(f"ERROR: {pak_path} not found")
        return False
    
    print(f"Appending PAK to {output_exe}...")
    
    with open(exe_path, 'rb') as f:
        exe_data = f.read()
    
    with open(pak_path, 'rb') as f:
        pak_data = f.read()
    
    with open(output_exe, 'wb') as f:
        f.write(exe_data)
        f.write(pak_data)
        # Write PAK size at end so loader can find it
        f.write(struct.pack('<I', len(pak_data)))
        # Write marker
        f.write(b'PAKX')
    
    print(f"Created {output_exe} with embedded resources ({len(pak_data)} bytes)")
    return True

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: pak_packer.py <exe_path>")
        sys.exit(1)
    
    exe_path = sys.argv[1]
    temp_pak = 'temp_resources.pak'
    
    # Pack resources
    if not pack_resources(temp_pak):
        sys.exit(1)
    
    # Append to exe
    output_exe = exe_path.replace('.exe', '_embedded.exe')
    if not append_pak_to_exe(exe_path, temp_pak, output_exe):
        sys.exit(1)
    
    # Cleanup
    os.remove(temp_pak)
    print("Done!")

