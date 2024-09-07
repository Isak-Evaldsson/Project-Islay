# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2024 Isak Evaldsson
#
#!/bin/bash
#
# Git hook runner, when used git hook it will run all shell scripts within
# the subfolder of $HOOKS_DIR with the same sames as the hook. E.g. for a pre-commit
# hook it will run all scripts $HOOKS_DIR/pre-coomit/*.sh
#

$HOOKS_DIR=git-hooks

hook=$(basename $0)
for FILE in $HOOKS_DIR/$hook/*.sh
do
    name=$(basename $FILE)
    bash $FILE 
    if [ $? -ne 0 ]; then
        echo "'$name' $hook-hook failed!"
        exit 1
    fi
done
