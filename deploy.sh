#!/usr/bin/env bash
# Brian Chrzanowski
# 2021-03-18 17:55:43
#
# Deploy to recipes.chrzanowski.me
#
# This assumes that `make watch` is running on the server. It just ssh's in, and gets updates from
# github (or the origin).

REMOTE="chrzanowski.me"
DIR="/var/www/recipes.chrzanowski.me"

ssh ${REMOTE} "cd ${DIR} ; git fetch"
ssh ${REMOTE} "cd ${DIR} ; git pull origin master"
ssh ${REMOTE} "cd ${DIR} ; ./backup.sh"

