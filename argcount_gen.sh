#!/bin/bash
name="${1:-_v2_argcount}"
size="${2:-512}"

macro0="#define ${name}_0(_1"
macro1="#define $name(...) ${name}_0(__VA_ARGS__, $size"

i=1
while (( i < size )); do
	macro1="$macro1, $((size - i))"
	let i++
	macro0="$macro0, _$i"
done

macro0="$macro0, x, ...) (x)"
macro1="$macro1)"

printf '%s\n' "$macro0" "$macro1"
