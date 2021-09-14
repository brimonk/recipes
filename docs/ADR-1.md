# No Dependencies on External Package Managers

## Context

Most software is kind of a pain in the butt to get hold of. Normally, there's a very expensive, very
repetitive task (for the computer) of acquiring libraries from the internet. It's the case that as
of writing, the CI job for this repository must first install two libraries to operate correctly.

This is kind of a crime.

And, the problem _can_ balloon into an unmaintainable mess if a programmer isn't careful about what
to pull in.

## Decision

To avoid all of that, this project is going to restrict itself to the minimal set of system software
to ensure clean and function code in all points of the pipeline, and to ensure that we know what
occurs at all points in the code.

## Status

Accepted, but more work must be done to accommodate this change. See #39 and #40 for more details.

## Consequences

This does mean more work on our behalf for the following areas of work:

* UI Unit Testing
* UI E2E Testing
* Backend Unit Testing

But we have some issues for MVP that will require these things to be done.


