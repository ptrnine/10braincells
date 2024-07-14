#!/bin/bash

_run() {
    local CC="$1"
    local target=$($CC -dumpmachine)
    local ARCH=

    case "$target" in
        arm*)                           ARCH=arm ;;
        aarch64*)                       ARCH=aarch64 ;;
        i?86-nt32*)                     ARCH=nt32 ;;
        i?86*)                          ARCH=i386 ;;
        x86_64-x32*|x32*|x86_64*x32)    ARCH=x32 ;;
        x86_64-nt64*)                   ARCH=nt64 ;;
        x86_64*)                        ARCH=x86_64 ;;
        m68k*)                          ARCH=m68k ;;
        mips64*|mipsisa64*)             ARCH=mips64 ;;
        mips*)                          ARCH=mips ;;
        microblaze*)                    ARCH=microblaze ;;
        or1k*)                          ARCH=or1k ;;
        powerpc64*|ppc64*)              ARCH=powerpc64 ;;
        powerpc*|ppc*)                  ARCH=powerpc ;;
        riscv64*)                       ARCH=riscv64 ;;
        sh[1-9bel-]*|sh|superh*)        ARCH=sh ;;
        s390x*)                         ARCH=s390x ;;
        *) ARCH=unknown ;;
    esac

    printf '%s' "$ARCH"
}

_run "$1"
