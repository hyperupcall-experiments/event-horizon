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

if ! pgrep keymon >/dev/null; then
	pkexec bash -c "DISPLAY=$DISPLAY \"$_dir/out/keymon\""
fi

unset -v _dir
