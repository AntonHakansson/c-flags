set -o errexit
set -o nounset
set -o pipefail
set -o xtrace

afl-cc fuzz.c -o fuzz -fsanitize=undefined
afl-fuzz -D -i ./in -o ./out -- ./fuzz
