#!/bin/bash

function safe_cd {
    cd "$@" || exit
}
