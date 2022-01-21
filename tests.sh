#!/usr/bin/env bash
#
# Brian Chrzanowski
# 2022-01-20 18:49:52
#
# Recipe Website Performance Test
#
#  X -> Recipes
#  Y -> Children
#
# - Create X Recipe JSON blobs
#
# - timer starts
#
# - POST all X Recipes
# - GET X (verify)
# - Update X Recipes (and all Y children)
# - PUT X
# - GET X (verify)
# - DELETE X
#
# Recipe Format
# {
#	name: string,
#	prep_time: string,
#	cook_time: string,
#	servings: string,
#	note: string,
#	ingredients: string[]
#	steps: string[]
#	tags: string[]
# }

X=10
Y=10

[ $# -gt 0 ] && X=$1 && shift
[ $# -gt 0 ] && Y=$1 && shift

DATADIR=$(mktemp -d -t ci-XXXXXXXXXX)
DBNAME="test.db"

TEMPLATE='{"cook_time":"COOK TIME Z","ingredients":[],"name":"NAME Z","note":"Z","prep_time":"PREP TIME Z","servings":"Z","steps":[],"tags":[]}'

./recipe $DATADIR/$DBNAME > /dev/null &
PID=$!

pushd $DATADIR

function expected
{
	echo "recipe.expected.$1.json"
}

function actual
{
	echo "recipe.actual.$1.json"
}

# generate: generates input for idx $1
function generate
{
	FNAME="$(expected $1)"
	echo ${TEMPLATE//Z/$1} > $FNAME

	for ((j=0;j<$Y;j++)); do
		jq -c '.ingredients += ["VALUE"] | .steps += ["VALUE"] | .tags += ["VALUE"]' \
			$FNAME | sponge $FNAME
	done
}

# post: posts the recipe for index $1
function post
{
	curl -s http://localhost:2000/api/v1/recipe -d @$1 > /dev/null
}

# get: gets the recipe for index $1
function get
{
	curl -s http://localhost:2000/api/v1/recipe/$1
}

# remove_id
function remove_id
{
	jq -c 'del(.id)' "$1" | sponge "$1"
}

# put: puts the recipe for index $i
function put
{
	curl -s -X PUT http://localhost:2000/api/v1/recipe/$1 -d @$2 > /dev/null
}

# delete: deletes the recipe for index $i
function delete
{
	curl -s -X DELETE http://localhost:2000/api/v1/recipe/$1
}

# verify: compares two files, returns true if there's an error
function verify
{
	# $1 - ID
	# $2 - Expected
	# $3 - Actual

	CMP=$(diff <(jq -S -f $2 .) <(jq -S -f $3 .))

	if [[ ! -z $CMP ]]; then
		echo "Recipe $1 does not match what has been sent! $CMP bytes don't match!"
		cat $2
		cat $3
		echo ""
	fi
}

for ((i=1;i<=$X;i++)); do
	generate $i
done

for ((i=1;i<=$X;i++)); do
	post $(expected $i)
done

# GET all of the recipes, and compare
for ((i=1;i<=$X;i++)); do
	get $i > $(actual $i); remove_id $(actual $i)
	verify $i $(expected $i) $(actual $i)
done

# update the "pre-made values" (make the json blobs lower-case)
for ((i=1;i<=$X;i++)); do
	# make everything lowercase, and add in the expected 'id'
	cat $(expected $i) | tr 'A-Z' 'a-z' | sponge $(expected $i)
	jq -S -c --argjson id $i '. + {id: $id}' $(expected $i) | sponge $(expected $i)
done

# PUT all of the recipes
for ((i=1;i<=$X;i++)); do
	put $i $(expected $i)
done

# GET all of the recipes, and compare (again)
for ((i=1;i<=$X;i++)); do
	get $i > $(actual $i)
	verify $i $(expected $i) $(actual $i)
done

# DELETE all of the recipes
for ((i=1;i<=$X;i++)); do
	delete $i
done

popd

kill $PID

rm -rf DATADIR

