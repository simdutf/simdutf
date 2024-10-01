# Formatting

This directory contains a dockerfile to build an image
suitable for clang format.

These are the instructions for building and publishing the image.

## Get an access token
Follow the instructions on https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens

The end result is a string with the token, which you use in the next step.

## Build the docker image

In this example, the docker image will be uploaded to namespace "pauldreik".

    docker build --build-arg CLANGVERSION=18 --tag ghcr.io/pauldreik/clang-format-18:latest .

## Publish the docker image

In this example, the docker image will be uploaded to namespace "pauldreik" by user "pauldreik".

    export CR_PAT=xxxxx # token from above
    echo $CR_PAT | docker login ghcr.io -u pauldreik --password-stdin
    docker push ghcr.io/pauldreik/clang-format-18:latest

## Setting the rights

Go to the packages page in the github UI and modify the rights from private to public
on the clang-format-18 image.
