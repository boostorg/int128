# Contributing to Boost.int128

## We are welcoming contributions

- Bug fixes and test cases for them
- New algorithms (with tests, documentation, complexity notes, and a reference)
- Documentation, examples, performance work
- Concept refinements (handle with care as these are API-visible)

## Getting set up

Clone the [Boost superproject](https://github.com/boostorg/boost):

```bash
git clone --depth 1 https://github.com/boostorg/boost
cd boost
git submodule update --init --depth 1
./bootstrap.sh
./b2 headers
```

Then replace `libs/int128/` with your fork:

```bash
rm -rf libs/int128
git clone https://github.com/<you>/int128 libs/int128
cd libs/int128
git remote add upstream https://github.com/boostorg/int128
```

## Building and testing

- Headers only: `./b2 headers` from boost root
- All tests: `./b2` from `libs/int128/test`
- Single test: `./b2 github_issue_207` from `libs/int128/test`
- Different C++ standard: `./b2 cxxstd=20`
- Different compiler: `./b2 toolset=clang`

## Pull request process

- Fork, branch from `develop`, PR back to `develop`
- One logical change per PR; rebase before requesting review
- Tests required for new features and bug fixes
- **Open (non-draft) PRs are assumed ready for review.** Use GitHub's Draft state while iterating, then mark the PR as *Ready for review* when you want maintainers to look at it.
- A maintainer will review within ~2 weeks (see Maintainers below)
- Squash on merge by default

## Merge criteria

Before a PR is merged, all of the following must hold:

1. CI is green on the full matrix.
2. New behavior has tests.
3. Bug fixes have a regression test.
4. Documentation under [doc/](doc/) is updated if the public API changed.
5. No new compiler warnings on the supported toolchains.
6. The PR is rebased on current `develop` with a clean commit history.
7. The principal maintainer has approved.

## Reporting bugs

1. Search [existing issues](https://github.com/boostorg/int128/issues) first
2. Note compiler, version, OS, Boost version
3. Minimal reproducer on [Compiler Explorer](https://godbolt.org/z/37dPWd5bs)
4. Expected Output versus Actual Output is explained.

## Security

See [SECURITY.md](SECURITY.md).

## License

By contributing, you agree your contribution is licensed under the [Boost Software License 1.0](https://www.boost.org/LICENSE_1_0.txt).
