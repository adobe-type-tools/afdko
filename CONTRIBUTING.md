# Contributing to AFDKO

First, **thank you** for your interest in contributing! We welcome contributions to all aspects of the project, including Python and C code, tests, and documentation (including questions).

Following are some guidelines and information about contributing, mostly so you know what to expect at various stages in the process.

## Project Goals
 - Maintain a _high-quality_, useful set of tools for working with OpenType fonts
 - Keep the toolset current with changes to related standards (Unicode, OpenType, etc.)
 - Promote the use of the AFDKO for creating, editing and testing OpenType fonts

## How to contribute

### Code of Conduct
Please review our [Code of Conduct](./CODE_OF_CONDUCT.md)
statement which explains standards and responsibilities for all contributors and
project maintainers.

### Suggested areas
If you'd like to contribute but are not sure where or how, here are some suggestions to get started:
 - Fix [active LGTM alerts](https://lgtm.com/projects/g/adobe-type-tools/afdko/alerts/?mode=list). Many of these are simple fixes and working through them is a great way to get familiar with the codebase.
 - Address [compiler warnings](https://github.com/adobe-type-tools/afdko/issues/633) for all target platforms.
 - Work on [open Issues marked with the "help wanted" label](https://github.com/adobe-type-tools/afdko/issues?q=is%3Aissue+label%3A%22help+wanted%22+is%3Aopen).
 - Improve [test coverage](https://codecov.io/gh/adobe-type-tools/afdko/branch/develop), particularly modules that have very low or no coverage currently.

### What if I just have a question?
First, **please check to see if [someone has already raised a similar question or issue](https://github.com/adobe-type-tools/afdko/issues?utf8=%E2%9C%93&q=is%3Aissue)**. If you don't see anything related to your question, open a new [Issue](https://github.com/adobe-type-tools/afdko/issues/new) and ask there.

You can also ask questions on the [AFDKO room in Gitter](https://gitter.im/adobe-type-tools/afdko) which is monitored by the AFDKO maintainers.

### Code Style
Please see the [Code Style wiki
page](https://github.com/adobe-type-tools/afdko/wiki/Code-Style) for guidelines.
Note that automated processes will run `cpplint` on C/C++ code and `flake8` on
Python code.

### Testing, test code, and test data
Contributions that add new functionality or substantially change existing
functionality _must_ include test code and data that demonstrates that the
contributed code functions as intended. But fonts and their interactions with
systems and applications can be complicated, so simple tests aren't always
sufficient. We encourage you to test your code on real fonts with tools,
utilities, and applications outside of the AFDKO to ensure that your
contribution produces font files that function correctly in real-world scenarios.

### Branches and Pull Requests
All contributions are handled through the GitHub Pull Request process. You may do this directly via a branch of AFDKO's `develop` branch or via a fork of AFDKO.

In either case, submitting a Pull Request triggers a number of automated actions (more detail on that below) and alerts the maintainers that there is a contribution to review. The maintainers will examine the submission and results of automated testing prior to accepting the contribution.

If you are contributing code, you should run the test suite locally and ensure that all tests pass. If you are contributing _a lot of code_ or introducing major changes, it is *essential* that you also add test cases to the test suite to ensure that your code is getting tested (review the contents of the [`tests`](./tests/) folder to get an idea of how to write tests). 100% coverage is not required, but you should add tests that demonstrate that new features/major changes are actually working as intended (and don't break existing tests). If you are contributing Python code, please pass it through [`flake8`](http://flake8.pycqa.org/en/latest/). If you are contributing C/C++ code, check with [`cpplint`](https://github.com/cpplint/cpplint).

### Automation
There are two main triggers for running automated tests for AFDKO:
 1. When any commit to _any_ branch is pushed to the main AFDKO repository
 2. When a Pull Request is submitted
 
Branch commit testing basically runs the test suite on various platforms (Mac OS X, Windows, and Linux) as well as static analysis (`flake8` for Python and `cpplint` for C/C++ code). You can check test progress by clicking on the "Details" link next to each automated check listed in the Checks section.

Pull Request testing is a bit more rigorous, and includes more detailed code analysis by [LGTM](https://lgtm.com/projects/g/adobe-type-tools/afdko/alerts/?mode=list) for security and other code problems beyond static analysis. If new issues are introduced, the maintainers will probably ask you to address them before proceeding with your contribution.

A [handy chart in the wiki](https://github.com/adobe-type-tools/afdko/wiki/CI-Matrix) details what the various automated systems do.

### Skipping (some) automation
It is possible to skip some of the automated tests by using the **[skip ci]** directive in the subject (first) line of a commit or Pull Request. It is appropriate to use this directive if you are contributing _non-code_ changes, like documentation. Please do _not_ use the directive if you have any code changes (including test code).
