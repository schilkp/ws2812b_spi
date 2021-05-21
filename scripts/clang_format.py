import argparse
import os
import sys
from os import listdir
from os.path import isfile, join

# Folders in which to search for files non-recursively
source_folders = ['test']

# Folders in which to search for files recursively
source_folder_roots = ['src']

# File extensions to format
extensions = ['.c', '.h']


def main(arguments):
    # Determine what to do based on arguments:
    parser = argparse.ArgumentParser(description="clang-format automation.")
    parser.add_argument('-v', '--verify', required=False, action="store_true",
                        help="Verify that everything is formatted correctly.")
    parser.add_argument('-f', '--format', required=False,
                        action="store_true", help="Format everything (default).")

    parsed_args = parser.parse_args(arguments)

    # Find all files that should be formatted:
    source_files = []

    # Recurse the folders that should be searched recursively:
    for source_folder in source_folder_roots:
        for root, _, files in os.walk(source_folder):
            for f in files:
                for extension in extensions:
                    if f.endswith(extension):
                        source_files.append(join(root, f))
                        break

    # List the files in the source folders:
    for source_folder in source_folders:
        for f in listdir(source_folder):
            if isfile(join(source_folder, f)):
                for extension in extensions:
                    if f.endswith(extension):
                        source_files.append(join(source_folder, f))
                        break

    # Remove duplicates:
    source_files = list(dict.fromkeys(source_files))

    if parsed_args.verify:
        # Verify all files are ok
        count = 0
        for f in source_files:
            ret = os.system("clang-format -i -style=file --dry-run -Werror --ferror-limit=1 " + f)
            if ret != 0:
                count += 1

        if count != 0:
            print("Formatting errrors found. Run 'make format' to fix")
            sys.exit(1)
        else:
            print("Formatting OK...")

    else:
        # Format all files
        for f in source_files:
            os.system("clang-format -i -style=file " + f)

    return 0


if __name__ == '__main__':
    args = sys.argv.copy()
    path = args.pop(0)
    main(args)
