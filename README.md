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

## Updating

### Update Mithril

Visit [the actual website](https://mithril.js.org/) to ensure this is the real link, but something
like the following should update mithril.js, in-repo.

```sh
curl "https://unpkg.com/mithril/mithril.js" > html/mithril.js
```

## WIP

### In Progress

### To Do (Features)

* Recipe Edit
* Recipe Delete
* Pictures Support
* Ratings / Comments
* Profile Pictures?

### To Do (Account)

* Email Verification for Login Stuff
* Forgot Password

### To Do (Cleanup)

* CSS wrapper classes for tailwind
* Tailwind / Style Sheet In-Repo
* Nav Bar / Collapsable Menu
* Generally better way to handle errors
* Can we statically allocate the form data? You get 1024 form k/v pairs, and that's it?
* Good Error Messages
* Set Expirations for Static Files

### To Do (Performance)

* Replace the HTTP Parser (scalar)
* Replace the HTTP Parser (SSE2)

### To Do (Security)

* Validate our inputs against SQL injection
* Validate behavior against popular attacks
* Validate memory frees, etc. with Valgrind

#### Production Ready

* Ensure our session cookie has `Secure`
* Ensure our session cookie has `HttpOnly`
* Ensure our session cookie has `SameSite=Strict`

### To Do (UX)

* Drag and drop list items around
* Compress files from `send_file`
    * Pre-compress files?

### To Do (Future)

* Put the Web stuff into a single header file library for future use

