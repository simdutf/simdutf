#!/usr/bin/env python3
"""
Script to automate adding a new function to the simdutf library.
This script reads a file containing the function signatures and documentation,
then adds the functions to the appropriate files in the codebase.

Usage:
    python add_function.py <signature_file>

Where <signature_file> is a file containing the function signatures, wrapped in a feature macro block, e.g.:
    #if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
    /**
     * Documentation for first function.
     */
    simdutf_warn_unused size_t utf8_length_from_utf16le(
        const char16_t *buf, size_t len) noexcept;

    /**
     * Documentation for second function.
     */
    simdutf_warn_unused size_t utf8_length_from_utf16be(
        const char16_t *buf, size_t len) noexcept;
    #endif
"""

import sys
import os
import re
import glob

def read_signature_file(file_path):
    """Read and parse the signature file."""
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Extract the feature macro (e.g., SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16)
    feature_match = re.search(r'#if\s+(.+)', content)
    feature_macro = feature_match.group(1).strip() if feature_match else None
    
    # Extract the content between #if and #endif
    if_match = re.search(r'#if\s+.+?(#endif)', content, re.DOTALL)
    if not if_match:
        raise ValueError("Could not find #if/#endif block.")
    
    block_content = if_match.group(0)
    
    # Extract all function signatures (documentation + signature)
    sig_matches = re.findall(r'(/\*\*.*?\*/)\s*(.*?);', block_content, re.DOTALL)
    if not sig_matches:
        raise ValueError("Could not parse function signatures from file.")
    
    functions = []
    for doc, sig in sig_matches:
        doc = doc.strip()
        signature = sig.strip() + ';'
        # Extract function name
        name_match = re.search(r'\s+(\w+)\s*\(', signature)
        func_name = name_match.group(1) if name_match else None
        functions.append((doc, signature, func_name))
    
    return feature_macro, functions

def add_to_implementation_h(repo_root, feature_macro, functions):
    """Add to include/simdutf/implementation.h"""
    file_path = os.path.join(repo_root, 'include/simdutf/implementation.h')
    
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Check for existing #if block for standalone functions (after namespace)
    ns_start = content.find('namespace simdutf {')
    if ns_start == -1:
        raise ValueError("Could not find namespace in implementation.h")
    
    existing_if = content.find(f'#if {feature_macro}', ns_start)
    if existing_if != -1:
        # Find the corresponding #endif
        endif_pos = content.find('#endif', existing_if)
        if endif_pos != -1:
            insert_pos = endif_pos
        else:
            insert_pos = len(content)
    else:
        insert_pos = ns_start + len('namespace simdutf {')
    
    # Insert standalone functions
    standalone = ""
    for doc, signature, func_name in functions:
        standalone_sig = signature.replace(' const ', ' ')
        standalone += f"""{doc}

{standalone_sig}


"""
    content = content[:insert_pos] + standalone + content[insert_pos:]
    
    # Now for virtual functions in class
    class_start = content.find('class implementation {')
    public_start = content.find('public:', class_start)
    if public_start == -1:
        raise ValueError("Could not find public section in implementation class")
    
    existing_if_class = content.find(f'#if {feature_macro}', public_start)
    if existing_if_class != -1:
        endif_pos_class = content.find('#endif', existing_if_class)
        if endif_pos_class != -1:
            insert_pos_class = endif_pos_class
        else:
            insert_pos_class = len(content)
    else:
        insert_pos_class = public_start + len('public:')
    
    # Insert virtual functions
    virtual = ""
    for doc, signature, func_name in functions:
        # Normalize signature for virtual
        clean_sig = re.sub(r'\s*;?\s*$', '', signature).strip()
        virtual += f"""{doc}
  virtual {clean_sig} = 0;


"""
    content = content[:insert_pos_class] + virtual + content[insert_pos_class:]
    
    with open(file_path, 'w') as f:
        f.write(content)

def add_to_src_implementation_cpp(repo_root, feature_macro, functions):
    """Add to src/implementation.cpp"""
    file_path = os.path.join(repo_root, 'src/implementation.cpp')
    
    with open(file_path, 'r') as f:
        content = f.read()
    
    # For detect class
    detect_start = content.find('class detect_best_supported_implementation_on_first_use')
    if detect_start != -1:
        existing_if = content.find(f'#if {feature_macro}', detect_start)
        if existing_if != -1:
            endif_pos = content.find('#endif', existing_if)
            insert_pos = endif_pos if endif_pos != -1 else len(content)
        else:
            brace_pos = content.find('{', detect_start)
            insert_pos = brace_pos + 1 if brace_pos != -1 else len(content)
    else:
        insert_pos = len(content)
    
    detect_impl = ""
    for doc, signature, func_name in functions:
        params = [p.strip().split()[-1].lstrip('*') for p in signature.split('(')[1].split(')')[0].split(',') if p.strip()]
        param_list = ', '.join(params)
        detect_impl += f"""{re.sub(r';$', ' {', signature.replace(' const noexcept', ' const noexcept final override'))}
  return set_best()->{func_name}({param_list});
}}


"""
    content = content[:insert_pos] + detect_impl + content[insert_pos:]
    
    # For unsupported class
    unsupported_start = content.find('class unsupported_implementation final : public implementation {')
    if unsupported_start != -1:
        existing_if = content.find(f'#if {feature_macro}', unsupported_start)
        if existing_if != -1:
            endif_pos = content.find('#endif', existing_if)
            insert_pos_unsup = endif_pos if endif_pos != -1 else len(content)
        else:
            insert_pos_unsup = unsupported_start + len('class unsupported_implementation final : public implementation {')
    else:
        insert_pos_unsup = len(content)
    
    unsupported_impl = ""
    for doc, signature, func_name in functions:
        return_stmt = "return nullptr;" if "const char" in signature.split('(')[0] or "char16_t" in signature.split('(')[0] else "return 0;"
        unsupported_impl += f"""{re.sub(r';$', ' {', signature.replace(' const noexcept', ' const noexcept final override'))}
  {return_stmt}  // Not supported
}}


"""
    content = content[:insert_pos_unsup] + unsupported_impl + content[insert_pos_unsup:]
    
    # For standalone functions at end
    existing_if_end = content.rfind(f'#if {feature_macro}')
    if existing_if_end != -1:
        endif_pos = content.find('#endif', existing_if_end)
        insert_pos_end = endif_pos if endif_pos != -1 else len(content)
    else:
        insert_pos_end = len(content)
    
    standalone = ""
    for doc, signature, func_name in functions:
        params = [p.strip().split()[-1].lstrip('*') for p in signature.split('(')[1].split(')')[0].split(',') if p.strip()]
        param_list = ', '.join(params)
        # Remove const for standalone functions
        standalone_sig = signature.replace(' const noexcept', ' noexcept')
        standalone += f"""{re.sub(r';$', ' {', standalone_sig)}
  return get_default_implementation()->{func_name}({param_list});
}}


"""
    content = content[:insert_pos_end] + standalone + content[insert_pos_end:]
    
    with open(file_path, 'w') as f:
        f.write(content)

def add_to_impl_files(file_path, functions, feature_macro):
    with open(file_path, 'r') as f:
        content = f.read()
    
    impl = ""
    for doc, signature, func_name in functions:
        modified_signature = re.sub(re.escape(func_name), f'implementation::{func_name}', signature)
        if ' const ' not in modified_signature:
            modified_signature = modified_signature.replace(' noexcept', ' const noexcept')
        modified_signature = modified_signature.replace(' const noexcept', ' const noexcept final override')
        impl += f"""{re.sub(r';$', ' {', modified_signature)}
  // TODO: implement
}}


"""
    
    existing_if = content.rfind(f'#if {feature_macro}')
    if existing_if != -1:
        endif_pos = content.find('#endif', existing_if)
        insert_pos = endif_pos if endif_pos != -1 else len(content)
        content = content[:insert_pos] + impl + content[insert_pos:]
    else:
        impl = f"#if {feature_macro}\n{impl}#endif // {feature_macro}\n"
        content += impl
    
    with open(file_path, 'w') as f:
        f.write(content)

def add_declarations_to_impl_h(file_path, functions, feature_macro):
    print(f"Adding declarations to {file_path}...")
    # Now add declarations to the corresponding .h file
    arch = os.path.basename(os.path.dirname(file_path))
    h_file = file_path
    with open(h_file, 'r') as f:
        h_content = f.read()
        
    
    insert_pos = h_content.rfind('};')
    if insert_pos == -1:
        print(f"}}; in {h_file}")
    if insert_pos != -1:
        print(f"Inserting declarations at position {insert_pos}...")
        decl = ""
        for doc, signature, func_name in functions:
            modified_signature = signature
            if ' const ' not in modified_signature:
                modified_signature = modified_signature.replace(' noexcept', ' const noexcept')
            decl += f"{modified_signature};\n"
        
        # Wrap with #if
        existing_if_h = h_content.rfind(f'#if {feature_macro}')
        if existing_if_h != -1:
            endif_pos_h = h_content.find('#endif', existing_if_h)
            insert_pos_h = endif_pos_h if endif_pos_h != -1 else insert_pos
            h_content = h_content[:insert_pos_h] + decl + h_content[insert_pos_h:]
        else:
            decl = f"#if {feature_macro}\n{decl}#endif // {feature_macro}\n"
            h_content = h_content[:insert_pos] + decl + h_content[insert_pos:]
        print(f"Updating {h_file}...")
        with open(h_file, 'w') as f:
            f.write(h_content)

def add_to_all_impl_files(repo_root, feature_macro, functions):
    """Add to all src/**/implementation.cpp files"""
    impl_files = glob.glob(os.path.join(repo_root, 'src', '**', 'implementation.cpp'), recursive=True)
    # Exclude src/implementation.cpp as it's handled separately
    impl_files = [f for f in impl_files if os.path.dirname(f) != os.path.join(repo_root, 'src')]
    print(f"Found {len(impl_files)} implementation.cpp files to update.")
    
    for file_path in impl_files:
        print(f"Updating {file_path}...")
        add_to_impl_files(file_path, functions, feature_macro)

def add_declaration_to_all_impl_files(repo_root, feature_macro, functions):
    """Add declarations to all src/simdutf/**/implementation.h files"""
    impl_files = glob.glob(os.path.join(repo_root, 'src/simdutf', '**', 'implementation.h'), recursive=True)
    # Exclude src/implementation.cpp as it's handled separately
    impl_files = [f for f in impl_files if os.path.dirname(f) != os.path.join(repo_root, 'src')]
    print(f"Found {len(impl_files)} implementation.h files to update.")
    
    for file_path in impl_files:
        print(f"Updating declarations for {file_path}...")
        add_declarations_to_impl_h(file_path, functions, feature_macro)

def main():
    if len(sys.argv) != 2:
        print("Usage: python add_function.py <signature_file>")
        sys.exit(1)
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.abspath(os.path.join(script_dir, '..'))
    
    sig_file = sys.argv[1]
    feature_macro, functions = read_signature_file(sig_file)
    
    # Ensure all signatures have const
    updated_functions = []
    for doc, sig, name in functions:
        if ' const ' not in sig:
            sig = sig.replace(' noexcept', ' const noexcept')
        updated_functions.append((doc, sig, name))
    functions = updated_functions
    
    if not feature_macro or not functions:
        print("Error: Could not extract feature macro or functions.")
        sys.exit(1)
    
    #add_to_implementation_h(repo_root, feature_macro, functions)
    #add_to_src_implementation_cpp(repo_root, feature_macro, functions)
    #add_to_all_impl_files(repo_root, feature_macro, functions)
    add_declaration_to_all_impl_files(repo_root, feature_macro, functions)
    
    func_names = [name for _, _, name in functions]
    print(f"Functions '{', '.join(func_names)}' added successfully.")
    print("Please remember to implement the functions in the respective implementation.cpp files.")
    print("Also, consider adding tests for the new functions.")
    print("Run formatting tools to ensure code style consistency: ./scripts/clang_format_docker.sh ")
    print("Done.")

if __name__ == '__main__':
    main()