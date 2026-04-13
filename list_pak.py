#!/usr/bin/env python3
"""List files in the embedded PAK"""
import struct

exe_path = "output\\Zlither.exe"
file_size = __import__('os').path.getsize(exe_path)

with open(exe_path, 'rb') as f:
    # Get PAK info
    f.seek(file_size - 8)
    pak_size = struct.unpack('<I', f.read(4))[0]
    
    pak_offset = file_size - pak_size - 8
    f.seek(pak_offset + 5)  # Skip VLPAK magic
    file_count = struct.unpack('<I', f.read(4))[0]
    
    print(f"PAK contains {file_count} files:\n")
    
    # List files
    ptr = pak_offset + 9  # After magic + count
    for i in range(file_count):
        f.seek(ptr)
        namelen = struct.unpack('<I', f.read(4))[0]
        name = f.read(namelen).decode('utf-8')
        datalen = struct.unpack('<I', f.read(4))[0]
        
        print(f"  {name:<40} {datalen:>10,} bytes")
        
        ptr += 4 + namelen + 4 + datalen
