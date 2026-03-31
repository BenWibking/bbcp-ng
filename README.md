# bbcp

`bbcp` is a secure, high-performance parallel file copy utility. This repository contains the classic Stanford `bbcp` source tree, with the main program in [`src/bbcp.C`](/Users/benwibking/bbcp/src/bbcp.C) and supporting transport, filesystem, and protocol code alongside it in `src/`.

The binary supports local and remote copy specifications of the form `[user@][host:]file` and is aimed at fast bulk transfer over the network.

## Building

Build from `src/`, not the repository root:

```sh
cd src
make
```

The makefile auto-detects the platform via `../MakeSname` and writes the executable to `bin/<OSVER>/bbcp`.

Useful build variants:

```sh
cd src
make clean
make Darwin OPT='-g'
```

## Quick Check

After building, run:

```sh
./bin/_darwin_/bbcp -h
```

That prints the built-in help and confirms the binary is runnable.

## Usage

General syntax:

```sh
bbcp [Options] [Inspec] Outspec
```

File specifications use:

```sh
[user@][host:]file
```

Common options include:

- `-s snum` to set the number of network streams.
- `-w wsz` to set the desired transfer window size.
- `-c [lvl]` to enable compression.
- `-e` to error-check transferred data with SHA-256.
- `-r` to copy directories recursively.
- `-p` to preserve source mode, ownership, and dates.
- `-v` or `-V` for verbose output.
- `-#` to print the version.
- `-$` to print the license.

For the full option list, run `bbcp -h`.

## Repository Layout

- [`src/`](/Users/benwibking/bbcp/src) contains the program sources and headers.
- [`src/Makefile`](/Users/benwibking/bbcp/src/Makefile) contains the platform-specific build logic.
- [`bin/`](/Users/benwibking/bbcp/bin) contains built binaries by platform.
- [`obj/`](/Users/benwibking/bbcp/obj) contains object files by platform.
- [`utils/bbcp.spec`](/Users/benwibking/bbcp/utils/bbcp.spec) contains packaging metadata.

## License

The source headers describe `bbcp` as LGPL-licensed free software. This repository includes [`src/COPYING.LESSER`](/Users/benwibking/bbcp/src/COPYING.LESSER) with the license text referenced by the code.
