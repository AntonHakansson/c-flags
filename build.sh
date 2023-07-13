set -o errexit
set -o nounset
set -o pipefail

if [[ "${TRACE-0}" == "1" ]]; then
	set -o xtrace
fi

cc example.c -o example -Wall -Wextra -fsanitize=address,undefined -ggdb -O0

set +o errexit
set -o xtrace
./example --bool
./example --str "test"
./example --str
./example --int 49
./example -i -69
./example -i arst # invalid number
./example -i -999999999999999999999999999999 # number out of range
./example --bool --str "tste" --int 59
./example --sthisisnotanrgument # Warn: unknown argument
./example --help
./example -h
