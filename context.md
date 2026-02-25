# Context
Improve unit test coverage to 85% or greater in adu_types, please scan other directories to get a sense of how unit tests should be written such as `utils`, `logging`, `adu_workflow`, `diagnostics_component`, `adu_types`, or `communication_abstraction`. At the end of creating the unit tests, generate a new report following the template below. If unit tests can't be written please provide an explanation as to why and how it can be made unit testable in the future. I want two .mds to be created. One for the Current Coverage Summary of the directories requested and another for the overall Code Coverage Report.


## Code Coverage

Before creating the two files below please run the code coverage tool, the unit test MUST pass the ./scripts/build.sh script execution. If there are errors in that script, please reconcile and fix the tests before continuing to generate the reports.

## Run code coverage tool
At the end of the unit test creation, execute the below command. This generates a cobertura report in the out/coverage directory.
```bash
# Build and run tests with coverage, this generates the cobertura report
./scripts/build.sh --coverage
```

After that runs successfully, please then generate the files by analyzing the cobertura.xml file.


file 1: diectory-specific-report.md
---------------------------
Parse the cobertura.xml file and generate the directory specific code coverage. Directory Specific coverage should be stored in diectory-specific-report.md. It should only be relevant to the directories that code coverage was added to in this request.
## Current Coverage Summary

## Files Not Unit Testable (0% coverage) - Detailed Explanations

### Summary: Why These Files Cannot Be Unit Tested

### Recommended Patterns for Future Testability
---------------------------



file 2: code-coverage.md
---------------------------
Parse the covertura.xml file and generate a total unit test coverage of the whole project. It MUST follow the template requested from code-coverage-report.txt and stored in a file named code-coverage.md
---------------------------

