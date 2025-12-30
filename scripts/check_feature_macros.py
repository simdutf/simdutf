#!/usr/bin/env python3
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

def extract_directive(lines: List[str], line_number: int) -> Tuple[str, str]:
    """Extract directive type and condition (if any) from a preprocessor line."""
    line = lines[line_number].strip()
    # Sometimes, the condition may span multiple lines due to backslashes
    # Handle multi-line conditions for #if directives
    # We recursively gather lines if they end with a backslash
    def build_full_condition(comment: str, line_number: int) -> str:
        if comment.endswith('\\') and line_number + 1 < len(lines):
            line_number += 1
            return comment[:-1].strip() + ' ' + build_full_condition(lines[line_number].strip(), line_number)
        else:
            return comment.strip()
    if line.startswith('#if '):
        return 'if', build_full_condition(line[4:].strip(), line_number)
    elif line.startswith('#ifdef '):
        return 'ifdef', build_full_condition(line[7:].strip(), line_number)
    elif line.startswith('#ifndef '):
        return 'ifndef', build_full_condition(line[8:].strip(), line_number)
    elif line.startswith('#endif'):
        match = re.match(r'#endif\s*(?://\s*(.*))?$', line)
        ## In some instances, the comment may be on the next line(s)
        ## We use the following loop to gather such comments
        ## The heuristic is that the next line starts with '//' and contains 'SIMDUTF_FEATURE'
        ## Note that if we don't check for SIMDUTF_FEATURE, we may pick up unrelated comments.
        comment = match.group(1).strip() if match and match.group(1) else ''
        while line_number + 1 < len(lines):
            next_line = lines[line_number + 1].strip()
            if next_line.startswith('//') and 'SIMDUTF_FEATURE' in next_line:
                comment_part = next_line[2:].strip()
                comment = (comment + ' ' + comment_part).strip() if comment else comment_part
                line_number += 1
            else:
                break
        return 'endif', comment
    return '', ''

def is_feature_if_directive(condition: str) -> bool:
    """Check if the #if condition contains a macro starting with SIMDUTF_FEATURE."""
    return bool(re.search(r'\bSIMDUTF_FEATURE\w*\b', condition))

def check_file(filename: str) -> List[str]:
    """Check #if directives with SIMDUTF_FEATURE macros for matching #endif comments."""
    errors = []
    stack = []  # Stack of (directive, condition, is_feature, line_number)
    statistics = {'if': 0, 'ifdef': 0, 'ifndef': 0, 'endif': 0}

    try:
        with open(filename, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        for line_number, line in enumerate(lines):
            if not is_preprocessor_directive(line):
                continue

            directive, condition = extract_directive(lines, line_number)
            if not directive:
                continue
            statistics[directive] += 1

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

        return errors, statistics

    except FileNotFoundError:
        return [f"{filename}: File not found"], statistics
    except UnicodeDecodeError:
        return [f"{filename}: Unable to decode file (invalid encoding)"], statistics

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
    statistics = {'if': 0, 'ifdef': 0, 'ifndef': 0, 'endif': 0}
    for filename in files:
        errors, file_stats = check_file(filename)
        all_errors.extend(errors)
        for key in statistics:
            statistics[key] += file_stats.get(key, 0)

    if all_errors:
        print("Errors found:")
        for error in all_errors:
            print(error)
        sys.exit(1)  # Exit with error code if errors are found
    else:
        for key, count in statistics.items():
            print(f"Total #{key} directives: {count}")
        print("All #if...#endif pairs for SIMDUTF feature macros are correctly matched with valid comments.")
        sys.exit(0)  # Exit with success code

if __name__ == "__main__":
    main()