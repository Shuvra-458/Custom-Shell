#!/usr/bin/env bash
set -e
./bin/myshell <<'EOF'
echo hello
cd /
echo $(pwd)
exit
EOF