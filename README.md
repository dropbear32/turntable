*This repository is archived and will not be updated.*

## turntable
turntable is a client-to-server audio streaming program for Linux, macOS, and Windows, written in C99.

## Dependencies
turntable only has dependencies for your platform's audio:

| Platform | Supported backend |
|----------|-------------------|
| Linux    | ALSA              |
| macOS    | Core Audio        |
| Windows  | WinMM / MME       |

For ALSA, you must install the `libasound2-dev` package. On macOS, no packages are needed. For Windows, see [Compiling / Windows](#windows).

## Compiling

You will need a C compiler. `clang` is recommended, but `gcc` works as well. For a compiler other than `clang`, be sure to modify the `CC` variable in the Makefile and double-check that its atomics intrinsics work the same way.

On macOS, you will need to run `xcode-select --install` or have Xcode.app installed.

0. Install the libraries for the filetypes you wish to have support for.
1. Clone the repository.
2. `cd` to the repository on your computer.
3. Run `make`.

### Windows

As I am not versed in Windows development, I used MinGW to compile this program.

0. Install MinGW and then `binutils` (to provide `ar`) and `make`.
1. Perform steps 0 through 2 from the above section.
2. Have a working C compiler for your architecture accessible from MinGW<sup>1</sup>.
3. Run `make -f Makefile.NT`.

<sup><sup>1</sup>This is beyond the scope of this document, and anything other than `clang` or `gcc` will almost certainly require modification of the Makefile (read: MSVC/`cl`).</sup>

## License
This code is licensed under the BSD 3-Clause License. A copy of this license is included in the repository.
