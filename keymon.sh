# shellcheck shell=bash
if [ "${0%/*}" = bash ]; then
	_dir=${BASH_SOURCE[0]}
	_dir=${_dir%/*}
	pushd -- "$_dir" >/dev/null
	_dir=$PWD
	popd >/dev/null
else
	_dir=$PWD
fi

#sudo --background --preserve-env=DISPLAY /usr/local/bin/keymon >|"$_dir/output" 2>&1

unset -v _dir
