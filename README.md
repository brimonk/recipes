# Fitness

This is my fitness tracking app.

## Building

Building is basically as simple as

```sh
make
```

## Running

```sh
./recipe <dbname>
```

## Reasoning

If you're looking at this repo, you're probably thinking, "Why did you write this in C? That doesn't
even make any sense. What about the security?" and to those things I say: the code for my dirt
simple apps shouldn't take up more than a few megabytes on disk, I should be able to run all of them
on like, a computer from 2002 with how simple they _should_ be.

Honestly, just watch some of Handmade Hero, and you'll probably get why.

## In Progress

* New User Form

## To Do (Features)

* User Login
* User Session Cookie
* User Logout
* Login Form
* Recipe Edit
* Recipe Delete
* Pictures Support
* Ratings / Comments
* Forgot Password
* Email Verification for Login Stuff
* Profile Pictures?

## To Do (Cleanup)

* CSS wrapper classes for tailwind
* Tailwind in-repo
* Title Bar / Collapsable Menu

## To Do (Performance)

* Each request should be processed in < 10ms
* Each page navigation should be processed in < 10ms (with network considerations)

## To Do (UX)

* Drag and drop list items around
* Compress files from `send_file`
    * Pre-compress files?

