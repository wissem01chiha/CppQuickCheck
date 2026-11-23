Changelog
=========

## CHANGES (23-11-2026)

- fix cppcheck warning [cwe-398](https://www.cvefind.com/en/cwe/cwe-398.html) by using std::copy_if algorithm instead of a raw loop. in [Arbitrary.h](https://github.com/wissem01chiha/CppQuickCheck/blob/a8dbb7cce15ff42eb57166dbc566d49d7ad165e9/include/cppqc/Arbitrary.h#L339)
- add .pre-commit-config.yaml file for pre-commit support 
 
#### CHANGES (13-11-2026)

- add coverage generation support
-  add gitHub actions for coverage reporting to [coveralls](https://coveralls.io/)
-  add [bazel](https://bazel.build/) build rules configuration support
-  add CMake configuration files
-  add single header file for distribution and ease of intergation 
-  added a CHANGELOG.md and VERSION file