# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2024 Isak Evaldsson
#
#!/bin/bash

# 
# Small script ensuring the development environment to be correctly setup
#

# TOOD - check for dependencies

#Ensure correctly configured hooks folder
git config --local core.hooksPath ".git/hooks"

# Install git hooks
cp -f git-hooks/run-hooks.sh .git/hooks/pre-commit
