#!/usr/bin/env python3
"""Check if PAK is properly embedded in exe"""
import os
import struct

exe_path = "output\\Zlither.exe"

if not os.path.exists(exe_path):
    print(f"ERROR: {exe_path} not found")
    exit(1)

file_size = os.path.getsize(exe_path)
print(f"EXE size: {file_size:,} bytes ({file_size / 1024 / 1024:.1f} MB)")

# Read last 4 bytes - should be "PAKX"
with open(exe_path, 'rb') as f:
    f.seek(file_size - 4)
    marker = f.read(4)
    print(f"Last 4 bytes (marker): {marker}")
    
    if marker != b'PAKX':
        print("ERROR: No PAKX marker found - PAK not embedded!")
        exit(1)
    
    # Read PAK size (4 bytes before PAKX)
    f.seek(file_size - 8)
    pak_size_bytes = f.read(4)
    pak_size = struct.unpack('<I', pak_size_bytes)[0]
    print(f"PAK size: {pak_size:,} bytes ({pak_size / 1024 / 1024:.1f} MB)")
    
    # Verify we can read the PAK magic
    pak_offset = file_size - pak_size - 8
    f.seek(pak_offset)
    pak_magic = f.read(5)
    print(f"PAK magic: {pak_magic}")
    
    if pak_magic != b'VLPAK':
        print("ERROR: Invalid PAK magic!")
        exit(1)
    
    # Read file count
    file_count_bytes = f.read(4)
    file_count = struct.unpack('<I', file_count_bytes)[0]
    print(f"Files in PAK: {file_count}")
    
    if file_count == 0:
        print("ERROR: PAK has no files!")
        exit(1)
    
    print("\nSUCCESS: PAK properly embedded with files!")
