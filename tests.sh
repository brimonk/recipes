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

DATADIR=$(mktemp -d -t ci-XXXXXXXXXX)
DBNAME="test.db"

TEMPLATE='{"cook_time":"COOK TIME Z","ingredients":[],"name":"NAME Z","note":"Z","prep_time":"PREP TIME Z","servings":"Z","steps":[],"tags":[]}'

./recipe $DBNAME &
PID=$!

pushd $DATADIR

# generate: generates input for idx $1
function generate
{
	echo ${TEMPLATE//Z/$i} > "recipe.$1.json"

	for ((j=0;j<$Y;j++)); do
		jq -c '.ingredients += [ "VALUE" ]' "recipe.$1.json" | sponge "recipe.$1.json"
		jq -c '.steps       += [ "VALUE" ]' "recipe.$1.json" | sponge "recipe.$1.json"
		jq -c '.tags        += [ "VALUE" ]' "recipe.$1.json" | sponge "recipe.$1.json"
	done
}

# post: posts the recipe for index $1
function post
{
	curl -s http://localhost:2000/api/v1/recipe -d @recipe.$1.json > /dev/null
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
	curl -X PUT http://localhost:2000/api/v1/recipe/$1 -d @recipe.$1.json > /dev/null
}

# delete: deletes the recipe for index $i
function delete
{
	curl -X DELETE http://localhost:2000/api/v1/recipe/$1
}

for ((i=1;i<=$X;i++)); do
	generate $i
done

for ((i=1;i<=$X;i++)); do
	post $i
done

# GET all of the recipes, and compare
for ((i=1;i<=$X;i++)); do
	GENFILE="recipe.$i.json"
	VERFILE="recipe.verify.$i.json"

	get $i > $VERFILE ; remove_id $VERFILE

	CMP=$(diff $GENFILE $VERFILE | wc -c)

	if [ $CMP -gt 0 ]; then
		echo "Recipe $i does not match what we initially sent!, $CMP bytes different"
		cat "$DATADIR/$GENFILE"
		cat "$DATADIR/$VERFILE"
		exit 1
	fi
done

# update the "pre-made values" (make the json blobs lower-case)
for ((i=1;i<$X;i++)); do
	cat "recipe.$i.json" | tr 'A-Z' 'a-z' | sponge "recipe.$i.json"
	jq -c --arg id $i '. + {id: $id}' "recipe.$i.json" | sponge "recipe.$i.json"
	cat "recipe.$i.json"
done

# PUT all of the recipes
for ((i=1;i<=$X;i++)); do
	put $i
done

# TODO (Brian): still need to compare

# DELETE all of the recipes
for ((i=1;i<=$X;i++)); do
	delete $i
done

popd

kill $PID

rm -rf DATADIR
rm $DBNAME

