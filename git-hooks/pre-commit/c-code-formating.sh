#!/bin/bash -xv
# 
# Ensures that all staged c/c++ files will be formatted before commit.
# If file is not properly formated it will exit with an error msg.
#
for FILE in $(git diff --cached --name-only)
do
    if [[ "$FILE" =~ \.(c|h|cpp|hpp)$ ]]; then
        
        # If diff detectes a difference exit
        diff <(clang-format $FILE) $FILE > /dev/null
        if [ $? -ne 0 ]; then
            echo "$FILE incorrectly formatted (run 'clang-format -i $FILE')"
            exit 1
        fi

        # Run our custom formater after clang format
        ./git-hooks/pre-commit/c-format-checker.py $FILE
        if [ $? -ne 0 ]; then
            echo "$FILE incorrectly formatted (fix errors above)"
            exit 1
        fi
    fi
done
