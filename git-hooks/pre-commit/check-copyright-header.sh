# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2024 Isak Evaldsson
#
#!/bin/bash

# Allow script to be run outside of the git hook
if [ $# -ge 1 ]; then
    if [ $1 = "all" ]; then
        FILES=$(find . \( -path './.git*' -o -path './sysroot*'  -o -path './isodir*' -o -path './.vscode*' -o -path '*build*' \) -prune -o -print);
    else
        echo "unknown option"
        exit 1
    fi
else
    FILES=$(git diff --cached --name-only)
fi

for file in $FILES; do
    if [ -d $file ] ; then
        continue
    fi

    # We only require a copyright header for soruce code files and scripts
    if [[ $(basename $file) =~ .*\.(c|h|S|ld|py|sh) ]]; then
        if ! head -2 $file | grep -q "SPDX-License-Identifier: BSD-3-Clause"; then
            echo "Warning: $file is missing a copyright header"
        fi
    fi
done
