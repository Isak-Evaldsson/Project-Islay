#!/bin/python3
# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2024 Isak Evaldsson
#

#
# A complement to clang-format, checking additional c style rules
#

import sys

# If header file verify the include guards
CHECK_INCLUDE_GUARDS = True

# Check if comments follow the rule of '//' within functions/structs and '/*...*/' outside
CHECK_COMMENTS = True

def check_include_guard_name(name):
    lowercase = any(c.islower() for c in name)
    ends_with_h = name.endswith("_H") or name.endswith("_HPP")

    if lowercase or not ends_with_h:
        return False
    
    return True

def check_include_guard(file_name, lines):
    if len(lines) < 3:
        print("%s: header-file missing include guard" % file_name)
        return False

    # Header might begin with a large documentation comment
    start_idx = 0
    if "/*" in lines[0]:
        while "*/" not in lines[start_idx]:
            start_idx += 1
        start_idx += 1

    first = lines[start_idx].split(' ')
    second = lines[start_idx+1].split(' ')
    last = lines[-1].split(' ')

    if len(first) != 2 or first[0] != '#ifndef':
        print("%s: first line missing #ifndef" % file_name)
        return False
    
    name = first[1]
    if not check_include_guard_name(name):
        print("%s: bad include guard name '%s'" % (file_name, name))
        return False

    if len(second) != 2 or second[0] != '#define' or second[1] != name:
        print("%s: second lines should be '#define %s'" % (file_name, name))
        return False

    if len(last) != 4 or last[0] != "#endif" or last[1] != "/*" or last[2] != name or last[3] != "*/":
        print("%s: should end with '#endif /* %s */'" % (file_name, name))
        return False

    return True

def process_file(file_name):
    curly_bracket_counter = 0;
    in_block_comment = False;
    correct = True

    block_comment_line_count = 0;

    file = open(file_name, 'r')
    lines = [line.rstrip() for line in file]

    if file_name.endswith('.h') or file_name.endswith('.hpp'):
        if CHECK_INCLUDE_GUARDS:
            check_include_guard(file_name, lines)

    for index, line in enumerate(lines):
        prev = ''

        for c in line:
            if c == '{':
                curly_bracket_counter += 1
            elif c == '}':
                curly_bracket_counter -= 1

            elif CHECK_COMMENTS and prev == '*' and c == '/':
                in_block_comment = False

                # /*...*/-comments within function/struct are only allowed when multi-line
                if curly_bracket_counter > 0 and block_comment_line_count < 1:
                    correct = False
                    print("%s: Invalid /*...*/ style comment at line %d" % (file_name, index + 1))

            elif CHECK_COMMENTS and prev == '/':
        
                if c == '*':
                    in_block_comment = True

                # //-comments outside function/struct
                if not in_block_comment and curly_bracket_counter == 0 and c == '/':
                    correct = False
                    print("%s: Invalid // style comment at line %d" % (file_name, index + 1))

            prev = c

        if in_block_comment:
            block_comment_line_count += 1
        else:
            block_comment_line_count = 0

    file.close()
    return correct


def main(argv):
    correct = True

    if len(argv) < 2:
        print("Usage: %s c-files..." % argv[0])
        sys.exit(2)

    for file in argv[1:]:
        if not process_file(file):
            correct = False
    
        
    if not correct:
        sys.exit(1)

if '__main__':
    main(sys.argv)
