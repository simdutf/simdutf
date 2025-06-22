import subprocess
import re
import os
import argparse
import sys
from typing import List, Tuple

def get_git_files(limit_to_src_include: bool) -> List[str]:
    """Retrieve list of .cpp and .h files checked into the Git repository."""
    try:
        patterns = ['src/*.cpp', 'src/*.h', 'include/*.cpp', 'include/*.h'] if limit_to_src_include else ['*.cpp', '*.h']
        result = subprocess.run(
            ['git', 'ls-files'] + patterns,
            capture_output=True,
            text=True,
            check=True
        )
        files = result.stdout.splitlines()
        if limit_to_src_include:
            files = [f for f in files if f.startswith(('src/', 'include/'))]
        return files
    except subprocess.CalledProcessError as e:
        print(f"Error running git ls-files: {e}")
        return []

def is_preprocessor_directive(line: str) -> bool:
    """Check if the line contains a preprocessor directive (not in a comment)."""
    line = re.sub(r'//.*$', '', line)
    return bool(re.match(r'^\s*#', line))

def extract_directive(line: str) -> Tuple[str, str]:
    """Extract directive type and condition (if any) from a preprocessor line."""
    line = line.strip()
    if line.startswith('#if '):
        return 'if', line[4:].strip()
    elif line.startswith('#ifdef '):
        return 'ifdef', line[7:].strip()
    elif line.startswith('#ifndef '):
        return 'ifndef', line[8:].strip()
    elif line.startswith('#endif'):
        match = re.match(r'#endif\s*(?://\s*(.*))?$', line)
        comment = match.group(1).strip() if match and match.group(1) else ''
        return 'endif', comment
    return '', ''

def is_feature_if_directive(condition: str) -> bool:
    """Check if the #if condition contains a macro starting with SIMDUTF_FEATURE."""
    return bool(re.search(r'\bSIMDUTF_FEATURE\w*\b', condition))

def check_file(filename: str) -> List[str]:
    """Check #if directives with SIMDUTF_FEATURE macros for matching #endif comments."""
    errors = []
    stack = []  # Stack of (directive, condition, is_feature, line_number)
    line_number = 0

    try:
        with open(filename, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        for line in lines:
            line_number += 1
            if not is_preprocessor_directive(line):
                continue

            directive, condition = extract_directive(line)
            if not directive:
                continue

            if directive in ('if', 'ifdef', 'ifndef'):
                is_feature = directive == 'if' and is_feature_if_directive(condition)
                stack.append((directive, condition, is_feature, line_number))
            elif directive == 'endif' and stack:
                orig_directive, orig_condition, is_feature, orig_line = stack.pop()
                if is_feature:
                    if not condition:
                        errors.append(
                            f"\033[31mERROR: {filename}:{line_number}: #endif missing comment (expected '{orig_condition}')\033[0m"
                        )
                    elif not re.match(r'SIMDUTF_FEATURE\w*', condition):
                        errors.append(
                            f"\033[31mERROR: {filename}:{line_number}: #endif comment '{condition}' does not start with SIMDUTF_FEATURE "
                            f"(expected '{orig_condition}')\033[0m"
                        )
                    elif condition != orig_condition:
                        errors.append(
                            f"\033[31mERROR: {filename}:{line_number}: #endif comment '{condition}' does not match "
                            f"#if condition '{orig_condition}' at line {orig_line}\033[0m"
                        )

        # Check for unclosed feature-related #if directives
        for directive, condition, is_feature, line in stack:
            if is_feature:
                red_warning = f"\033[31mWARNING: {filename}:{line}: Unclosed #if '{condition}' at end of file\033[0m"
                errors.append(red_warning)

        return errors

    except FileNotFoundError:
        return [f"{filename}: File not found"]
    except UnicodeDecodeError:
        return [f"{filename}: Unable to decode file (invalid encoding)"]

def main():
    """Main function to check .cpp and .h files for SIMDUTF feature #if...#endif pairs."""
    parser = argparse.ArgumentParser(description="Check #if...#endif pairs for SIMDUTF feature macros in .cpp and .h files.")
    parser.add_argument('--limit', action='store_true', help="Limit checks to files in src/ and include/ directories")
    args = parser.parse_args()

    files = get_git_files(args.limit)
    if not files:
        print("No .cpp or .h files found in the Git repository" + (" in src/ or include/" if args.limit else "") + ".")
        sys.exit(1)  # Exit with error code if no files are found

    all_errors = []
    for filename in files:
        errors = check_file(filename)
        all_errors.extend(errors)

    if all_errors:
        print("Errors found:")
        for error in all_errors:
            print(error)
        sys.exit(1)  # Exit with error code if errors are found
    else:
        print("All #if...#endif pairs for SIMDUTF feature macros are correctly matched with valid comments.")
        sys.exit(0)  # Exit with success code