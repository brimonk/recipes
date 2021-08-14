# Recipes

This is my recipe website.

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

## WIP

### In Progress

* User Login

### To Do (Features)

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
* HTML Templating (tables and things get easier)
* Correct UTF-8/16 Support

### To Do (Cleanup)

* CSS wrapper classes for tailwind
* Tailwind in-repo
* Nav Bar / Collapsable Menu
* Should we be sending a 503 every time a function errors?
* Can we statically allocate the form data? You get 1024 form k/v pairs, and that's it?
* Good Error Messages

### To Do (Performance)

* Each request should be processed in < 10ms
* Each page navigation should be processed in < 10ms (with network considerations)

### To Do (Security)

* Validate our inputs against SQL injection
* Validate behavior against popular attacks

### To Do (UX)

* Drag and drop list items around
* Compress files from `send_file`
    * Pre-compress files?

### To Do (Future)

* Put the Web stuff into a single header file library for future use

