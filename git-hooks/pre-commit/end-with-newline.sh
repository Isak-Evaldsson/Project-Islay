# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2024 Isak Evaldsson
#
#!/bin/bash
#
# Ensures that all commited files end with a new line
#
for FILE in $(git diff --cached --name-only)
do
    if [[ $(tail -c1 $FILE) != '' ]]; then
        echo "'$FILE' does not end with a newline"
        exit 1
    fi
done
