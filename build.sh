#!/bin/sh
set -eu
exec sh "$(dirname "$0")/build_no_install.sh" "$@"
