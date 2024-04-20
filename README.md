# FAT32 File Recovery Tool

## Objectives

This lab is designed to provide practical experience with the FAT32 file system, offering insights into the internals of how file systems manage data. The objectives are:

1. **Understanding FAT32 Internals:** Learn the structure and operation of the FAT32 file system.
2. **Raw Disk Access and Recovery:** Develop skills in accessing and recovering files from a raw disk image.
3. **File System Concepts:** Gain a clearer understanding of fundamental file system concepts.

## Overview

In this lab, I worked directly with data stored on a FAT32 file system, bypassing operating system file system support. I developed a tool that recovers a deleted file specified by the user. For the sake of simplicity, this tool will only recover files from the root directory, avoiding the need to navigate through subdirectories.

### Functionality

- **Recover Deleted Files:** Implement functionality to recover a deleted file located in the root directory of a FAT32 disk image.
- **Command-Line Interface:** Provide a simple CLI to specify the disk image and the name of the file to recover.
