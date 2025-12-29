#!/bin/bash

# Usage: ./valgrind_check.sh ./bin -arg

valgrind --version > /dev/null || ( echo "valgrind not installed" && exit 1 )

valgrind --error-exitcode=1 \
         --quiet \
         --malloc-fill=0x2a \
         --track-origins=yes \
         --leak-check=full \
         --gen-suppressions=all \
         --suppressions=./scripts/valgrind.supp \
         "$@"

exit $?
