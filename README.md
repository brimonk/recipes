# Fitness

This is my fitness tracking app.

## Building

Building is basically as simple as

```sh
make
```

## Running

```sh
./fitness <dbname>
```

## Features

This application has list pages and CRUD pages for the following tables:

* meal
* weight
* exercise
* bloodpressure
* sleep

## Reasoning

If you're looking at this repo, you're probably thinking, "Why did you write this in C? That doesn't
even make any sense. What about the security?" and to those things I say: the code for my dirt
simple apps shouldn't take up more than a few megabytes on disk, I should be able to run all of them
on like, a $5 RPI.

## To Do

* BearSSL - In Repo
* User Support
* Password hashing
* JWT Support (probably means a JSON library too)

