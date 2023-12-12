# External LLVM

All files in the `llvm` subfolder are 1:1 copied from LLVM 14.0.6 and are subject to the LLVM license.
You can find a copy of the LLVM license [here](./LLVM-LICENSE.txt).

Note that we needed to copy these files, as LLVM removed them in the transition from version 14 to 15.
To avoid LLVM from blocking PhASAR releases, we provide these files ourselves as a *temporary solution*.

We, as the PhASAR development core team, do not aim for maintaining the here provided LLVM code and will not add any modifications to it (bugfixes, enhancements, etc.).
Rather, we will add a custom replacement eventually.
