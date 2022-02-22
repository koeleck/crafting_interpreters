#!/usr/bin/env bash

conan install . -if build $@
conan build . -c -if build -bf build
