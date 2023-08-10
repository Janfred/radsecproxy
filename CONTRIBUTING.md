# Contributing to radsecproxy

We are happy that you want to help.
Please read this document before starting your contribution to radsecproxy.

## Code Style

We use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to autoformat the code.
Please use this code style in all your contributions.

There is a script available to reformat all your code in `tools/clang-format.hook` that can be run either in the shell standalone or can be included as pre-commit hook in git.
Running the script automatically adds all reformatted files to git.

If you want to keep the script from doing that run it with `--no-git-add`. The script only touches files, that are already marked for commit.

To use the script as a git pre-commit hook, symlink the script to the git hook directory:
```bash
ln -s ../../tools/clang-format.hook .git/hooks/pre-commit
```

The symlink ensures that updates in the code style and the script are automatically included.
If you are cautious about automatically running code from untrusted sources, you can also simply copy the script to the hook directory and rename it to `pre-commit`

## Git workflow

If you want to contribute to radsecproxy, please base your contributions on the current `master` and rebase your changes regularly to the new `master`, after you submitted your pull request.

## OpenSSL Library Backwards compatibility

To ensure a broad compatibility with many different systems, the code should be written with different OpenSSL Library versions in mind.

The oldest and newest supported OpenSSL library version should be the version currently shipped in the oldest and newest maintained OS versions of Debian and Ubuntu.

As of 2023-08-10:
| OS Version | OpenSSL version |
|------------|-----------------|
| Debian 10 (Buster) | 1.1.1n |
| Debian 11 (Bullseye) | 1.1.1n |
| Debian 12 (Bookworm) | **3.0.9** |
| Ubuntu 20.04 LTS (Focal Fossa) | **1.1.1f** |
| Ubuntu 22.04 LTS (Jammy Jellyfish) | 3.0.2 |
| Ubuntu 22.10 (Kinetic Kudu) | 3.0.5 |
| Ubuntu 23.04 (Lunar Lobster) | 3.0.8 |

If there were changes in the OpenSSL API, please include pre-processor clauses depending on the OpenSSL Version number

```c
#if OPENSSL_VERSION_NUMBER < 0x101010efL
  // code for OpenSSL before 1.1.1n
#else
  // code for OpenSSL after (including) 1.1.1n
#endif
```

## Ignore code style commits in git blame

Since the code style was introduced between Radsecproxy 1.10 and 1.11, there were significant changes in the code that were solely code styles.
These changes were made in one commit. This commit is now shown as change commit in commands like `git blame`.

All commits that are only code style commits are included in the file `.git-blame-ignore-revs`.

Github will automatically exclude these commits from the history view.
To hide these commits from the local history in `git blame` too, you can simply run the following command:
```
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

