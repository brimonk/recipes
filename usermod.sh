#!/usr/bin/env bash

# NOTE (brian) This script exists to create, read, update, and delete users from the database.

DATABASE="$1"
COMMAND="$2"

if [[ ! -f $DATABASE ]]; then
	echo "DATABASE FILE ${DATABASE} DOESN'T EXIST!"
	exit 1
fi

function sqlite3_exec {
	sqlite3 $DATABASE ".load ./sqlite3_uuid.so sqlite3_uuid_init" ".load ./sqlite3_passwd.so sqlite3_passwd_init" "$1"
}

if [ "$COMMAND" = "insert" ]; then
	echo "insert"

	USERNAME=""
	PASSWORD=""

	while [[ -z $USERNAME ]] || [[ -z $PASSWORD ]]; do
		echo -n "Username: "
		read USERNAME
		echo -n "Password: "
		read -s PASSWORD

		echo ""

		# check that both are non-empty strings
		if [[ -z $USERNAME ]]; then
			echo "'Username' must be a non-empty string"
		fi

		if [[ -z $PASSWORD ]]; then
			echo "'Password' must be a non-empty string"
		fi
	done

	echo $USERNAME $PASSWORD

	# HASH PASSWORD HERE

	sqlite3_exec "insert into users (username, password) values ('$USERNAME', '$PASSWORD');"

elif [ "$COMMAND" = "update" ]; then
	echo "update"

	USERNAME=""
	PASSWORD=""

	while [[ -z $USERNAME ]] || [[ -z $PASSWORD ]]; do
		echo -n "Username: "
		read USERNAME
		echo -n "Password: "
		read -s PASSWORD

		echo ""

		# check that both are non-empty strings
		if [[ -z $USERNAME ]]; then
			echo "'Username' must be a non-empty string"
		fi

		if [[ -z $PASSWORD ]]; then
			echo "'Password' must be a non-empty string"
		fi
	done

	# HASH PASSWORD HERE

	sqlite3_exec "update users set password = \'$PASSWORD\' where username = \'$USERNAME\'";

elif [ "$COMMAND" = "select" ]; then
	echo "select"

	sqlite3_exec "select create_ts, username from users order by create_ts;"

elif [ "$COMMAND" = "delete" ]; then
	echo "delete"

	USERNAME=""

	echo -n "Enter Username to Delete: "
	read USERNAME
	echo ""

	echo -n "Are you sure you want to delete user '$USERNAME'? (Y/N) "
	read CONFIRM

	if [[ "$CONFIRM" -eq "Y" ]] || [[ "$CONFIRM" -eq "y" ]]; then
		sqlite3_exec "delete from users where username = \'$USERNAME\';"
	fi
else
	echo "UNKNOWN COMMAND ${COMMAND}"
	exit 1
fi
