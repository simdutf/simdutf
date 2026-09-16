#!/usr/bin/env python3
"""
Script to automate adding a new function to the simdutf library.
This script reads a file containing the function signatures and documentation,
then adds the functions to the appropriate files in the codebase.

Usage:
    python add_function.py [--no-c-api] <signature_file>

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

The C API (include/simdutf_c.h, src/simdutf_c.cpp) is updated as well, unless
--no-c-api is given or the signature uses types that have no C counterpart.
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
    
    existing_if = content.find(f'#if {feature_macro}\n', ns_start)
    if existing_if != -1:
        # Find the corresponding #endif
        endif_pos = content.find(f'#endif // {feature_macro}\n', existing_if)
        if endif_pos != -1:
            insert_pos = endif_pos
        else:
            insert_pos = len(content)
    else:
        insert_pos = ns_start + len('namespace simdutf {')
    
    # Insert standalone functions
    standalone = ""
    for doc, signature, func_name in functions:
        standalone_sig = signature.replace(' const noexcept', ' noexcept')
        standalone += f"""{doc}

{standalone_sig}


"""
    content = content[:insert_pos] + standalone + content[insert_pos:]
    
    # Now for virtual functions in class
    class_start = content.find('class implementation {')
    public_start = content.find('public:', class_start)
    if public_start == -1:
        raise ValueError("Could not find public section in implementation class")
    
    existing_if_class = content.find(f'#if {feature_macro}\n', public_start)
    if existing_if_class != -1:
        endif_pos_class = content.find(f'#endif // {feature_macro}\n', existing_if_class)
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
        existing_if = content.find(f'#if {feature_macro}\n', detect_start)
        if existing_if != -1:
            endif_pos = content.find(f'#endif // {feature_macro}\n', existing_if)
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
        existing_if = content.find(f'#if {feature_macro}\n', unsupported_start)
        if existing_if != -1:
            endif_pos = content.find(f'#endif // {feature_macro}\n', existing_if)
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
    existing_if_end = content.rfind(f'#if {feature_macro}\n')
    if existing_if_end != -1:
        endif_pos = content.find(f'#endif // {feature_macro}\n', existing_if_end)
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
        impl += f"""{re.sub(r';$', ' {', modified_signature)}
  // TODO: implement
}}


"""
    
    existing_if = content.rfind(f'#if {feature_macro}\n')
    if existing_if != -1:
        endif_pos = content.find(f'#endif // {feature_macro}\n', existing_if)
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
            if 'override' not in modified_signature:
                modified_signature = modified_signature.replace('noexcept', 'noexcept override')
            decl += f"{modified_signature}\n"
        
        # Wrap with #if
        existing_if_h = h_content.rfind(f'#if {feature_macro}\n')
        if existing_if_h != -1:
            endif_pos_h = h_content.find(f'#endif // {feature_macro}\n', existing_if_h)
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

# ---------------------------------------------------------------------------
# C API generation (include/simdutf_c.h and src/simdutf_c.cpp)
# ---------------------------------------------------------------------------

# simdutf structs mirrored by the C API, with the converter that turns the C++
# struct into the C one in src/simdutf_c.cpp.
C_RESULT_TYPES = {
    'result': ('simdutf_result', 'to_c_result'),
    'full_result': ('simdutf_full_result', 'to_c_full_result'),
}

# simdutf enums mirrored value for value by the C API: they only need a cast.
C_ENUM_TYPES = {
    'encoding_type': 'simdutf_encoding_type',
    'base64_options': 'simdutf_base64_options',
    'last_chunk_handling_options': 'simdutf_last_chunk_handling_options',
}

# Types that spell the same thing in C and in C++.
C_PLAIN_TYPES = {
    'void', 'bool', 'char', 'char16_t', 'char32_t', 'signed', 'unsigned',
    'short', 'int', 'long', 'float', 'double', 'size_t', 'ptrdiff_t',
    'int8_t', 'int16_t', 'int32_t', 'int64_t',
    'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t',
}

# Qualifiers that have no place in a C declaration.
CXX_QUALIFIERS = ('simdutf_warn_unused', 'simdutf_really_inline',
                  'simdutf_constexpr', 'constexpr', 'inline', 'static')

# We insert the wrappers at the end of the extern "C" block of each file.
C_HEADER_ANCHOR = '#ifdef __cplusplus\n} /* extern "C" */'
C_SOURCE_ANCHOR = '} // extern "C"'

def split_declaration(text):
    """Split 'const char16_t *buf' into ('const char16_t *', 'buf')."""
    match = re.match(r'^(.*?)(\w+)$', text.strip(), re.DOTALL)
    if not match:
        return None, None
    type_str = re.sub(r'\s*\*', ' *', match.group(1).strip())
    return type_str.strip(), match.group(2)

def base_type_name(type_str):
    """Return the type name of a declaration, without cv-qualifiers or '*'."""
    tokens = type_str.replace('*', ' ').replace('simdutf::', ' ').split()
    tokens = [t for t in tokens if t not in ('const', 'volatile')]
    return tokens[-1] if tokens else ''

def check_c_type(type_str, base, allow_struct):
    """Raise ValueError if the type cannot cross the C boundary as is."""
    if '<' in type_str or '&' in type_str:
        raise ValueError("'%s' has no C counterpart" % type_str)
    if '::' in type_str.replace('simdutf::', ''):
        raise ValueError("'%s' has no C counterpart" % type_str)
    if base in C_RESULT_TYPES:
        if not allow_struct:
            raise ValueError("'%s' can only be converted on the way out" % base)
    elif base not in C_PLAIN_TYPES and base not in C_ENUM_TYPES:
        raise ValueError("type '%s' is not part of the C API" % base)

def c_type_name(type_str, base):
    """Rewrite a C++ declaration type into its C API spelling."""
    if base in C_ENUM_TYPES:
        c_base = C_ENUM_TYPES[base]
    elif base in C_RESULT_TYPES:
        c_base = C_RESULT_TYPES[base][0]
    else:
        c_base = base
    return re.sub(r'(simdutf::)?\b%s\b' % re.escape(base), c_base, type_str)

def declarator(type_str, name):
    """Join a type and a name, keeping the '*' next to the name."""
    return type_str + name if type_str.endswith('*') else type_str + ' ' + name

def to_c_doc(doc):
    """Turn a doxygen /** ... */ block into a plain C comment."""
    body = re.sub(r'\*+/\s*$', '', re.sub(r'^\s*/\*+', '', doc.strip()))
    lines = [re.sub(r'^\s*\*', '', line).strip() for line in body.splitlines()]
    while lines and not lines[0]:
        lines.pop(0)
    while lines and not lines[-1]:
        lines.pop()
    if not lines:
        return ''
    if len(lines) == 1:
        return '/* %s */' % lines[0]
    body = ''.join('\n' + ('   ' + line).rstrip() for line in lines[1:])
    return '/* ' + lines[0] + body + ' */'

def c_api_wrapper(doc, signature, func_name):
    """Build the C declaration and definition wrapping a simdutf function.

    Raises ValueError when the signature uses something that has no C
    counterpart (references, spans, templates, ...), in which case the
    wrapper has to be written by hand.
    """
    sig = re.sub(r'\bnoexcept\b', ' ', signature.strip().rstrip(';'))
    for qualifier in CXX_QUALIFIERS:
        sig = re.sub(r'\b%s\b' % qualifier, ' ', sig)
    # Drop the trailing const of the member function form.
    sig = re.sub(r'\bconst\s*$', '', sig.strip()).strip()

    match = re.match(r'^(?P<ret>.*?)\b(?P<name>\w+)\s*\((?P<params>.*)\)$',
                     sig, re.DOTALL)
    if not match or match.group('name') != func_name:
        raise ValueError('cannot parse the signature')

    ret_type = re.sub(r'\s*\*', ' *', match.group('ret').strip())
    if not ret_type:
        raise ValueError('cannot parse the return type')
    ret_base = base_type_name(ret_type)
    check_c_type(ret_type, ret_base, allow_struct=True)
    c_ret_type = c_type_name(ret_type, ret_base)

    c_params = []
    args = []
    for param in match.group('params').split(','):
        if not param.strip():
            continue
        type_str, name = split_declaration(param)
        if not type_str or not name:
            raise ValueError("cannot parse the parameter '%s'" % param.strip())
        base = base_type_name(type_str)
        check_c_type(type_str, base, allow_struct=False)
        c_params.append(declarator(c_type_name(type_str, base), name))
        if base in C_ENUM_TYPES:
            args.append('static_cast<simdutf::%s>(%s)' % (base, name))
        else:
            args.append(name)

    prototype = '%s(%s)' % (declarator(c_ret_type, 'simdutf_' + func_name),
                            ', '.join(c_params))
    call = 'simdutf::%s(%s)' % (func_name, ', '.join(args))
    if ret_base == 'void':
        body = '  %s;' % call
    elif ret_base in C_RESULT_TYPES:
        body = '  return %s(%s);' % (C_RESULT_TYPES[ret_base][1], call)
    elif ret_base in C_ENUM_TYPES:
        body = '  return static_cast<%s>(%s);' % (C_ENUM_TYPES[ret_base], call)
    else:
        body = '  return %s;' % call

    c_doc = to_c_doc(doc)
    declaration = (c_doc + '\n' if c_doc else '') + prototype + ';'
    definition = '%s {\n%s\n}' % (prototype, body)
    return declaration, definition

def insert_before_anchor(file_path, anchor, text, use_last):
    """Insert text right before the anchor of the given file."""
    with open(file_path, 'r') as f:
        content = f.read()

    insert_pos = content.rfind(anchor) if use_last else content.find(anchor)
    if insert_pos == -1:
        print(f"Warning: could not find the extern \"C\" block in {file_path}, "
              "skipping it.")
        return False

    content = content[:insert_pos] + text + content[insert_pos:]
    with open(file_path, 'w') as f:
        f.write(content)
    return True

def add_to_c_api(repo_root, functions):
    """Add wrappers to include/simdutf_c.h and src/simdutf_c.cpp.

    The C API is a thin forwarding layer, so the wrappers can be generated
    whenever every type of the signature has a C counterpart. Functions that
    need more than forwarding are reported and left to the caller.
    """
    header = os.path.join(repo_root, 'include/simdutf_c.h')
    source = os.path.join(repo_root, 'src/simdutf_c.cpp')
    with open(header, 'r') as f:
        header_content = f.read()

    wrapped = []
    declarations = []
    definitions = []
    for doc, signature, func_name in functions:
        if f'simdutf_{func_name}(' in header_content:
            print(f"'{func_name}' is already in the C API, skipping it.")
            continue
        try:
            declaration, definition = c_api_wrapper(doc, signature, func_name)
        except ValueError as e:
            print(f"Skipping the C API for '{func_name}': {e}.")
            print("  Add the wrapper by hand to include/simdutf_c.h and "
                  "src/simdutf_c.cpp if it belongs to the C API.")
            continue
        wrapped.append(func_name)
        declarations.append(declaration)
        definitions.append(definition)

    if not wrapped:
        return []

    print(f"Updating {header}...")
    insert_before_anchor(header, C_HEADER_ANCHOR,
                         '\n\n'.join(declarations) + '\n\n', use_last=False)
    print(f"Updating {source}...")
    insert_before_anchor(source, C_SOURCE_ANCHOR,
                         '\n\n'.join(definitions) + '\n\n', use_last=True)
    return wrapped

def main():
    args = list(sys.argv[1:])
    generate_c_api = '--no-c-api' not in args
    args = [a for a in args if a != '--no-c-api']
    if len(args) != 1:
        print("Usage: python add_function.py [--no-c-api] <signature_file>")
        sys.exit(1)
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.abspath(os.path.join(script_dir, '..'))
    
    sig_file = args[0]
    feature_macro, functions = read_signature_file(sig_file)
    
    # Ensure all signatures have const
    updated_functions = []
    for doc, sig, name in functions:
        if 'const noexcept' not in sig:
            sig = sig.replace('noexcept', 'const noexcept')
        updated_functions.append((doc, sig, name))
    functions = updated_functions
    
    if not feature_macro or not functions:
        print("Error: Could not extract feature macro or functions.")
        sys.exit(1)
    
    add_to_implementation_h(repo_root, feature_macro, functions)
    add_to_src_implementation_cpp(repo_root, feature_macro, functions)
    add_to_all_impl_files(repo_root, feature_macro, functions)
    add_declaration_to_all_impl_files(repo_root, feature_macro, functions)
    c_api_names = add_to_c_api(repo_root, functions) if generate_c_api else []
    
    func_names = [name for _, _, name in functions]
    print(f"Functions '{', '.join(func_names)}' added successfully.")
    print("Please remember to implement the functions in the respective implementation.cpp files.")
    print("Also, consider adding tests for the new functions.")
    if c_api_names:
        print(f"C wrappers for '{', '.join(c_api_names)}' were added to include/simdutf_c.h and src/simdutf_c.cpp.")
        print("Review them (documentation, placement) and cover them in tests/straight_c_test.c and tests/nostdlibcxx_c_api_test.c.")
    print("Run formatting tools to ensure code style consistency: ./scripts/clang_format_docker.sh ")
    print("Done.")

if __name__ == '__main__':
    main()