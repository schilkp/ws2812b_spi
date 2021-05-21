import sys
import os
from os.path import basename, splitext
import subprocess


def main(test_suites):
    tests_crashed = []
    tests_passed = []
    tests_failed = []
    tests_ignored = []

    # Print message and exit if there where no tests passed:
    if len(test_suites) == 0:
        print("No test suites specified.")
        sys.exit(0)

    # Run every test
    for test_suite in test_suites:
        test_suite_name = splitext(basename(test_suite))[0]
        print("Running test suite %s..." % test_suite_name)
        test_output = subprocess.run(test_suite, capture_output=True)

        suite_output = test_output.stdout.decode()

        if '-----------------------' not in suite_output:
            # Test suite crashed
            print("Test suite %s crashed, test results not reported!" % test_suite_name)
            tests_crashed.append(test_suite_name)
        else:
            # Test suite completed

            # Save output to .test file:
            with open(splitext(test_suite)[0] + '.test', 'w') as report:
                report.write(suite_output)

            # Separate and sort tests:
            for line in suite_output.split('\n'):
                if line == '' or line == '-----------------------':
                    # Report finished
                    break
                elif ':PASS' in line:
                    tests_passed.append(line)
                elif ':FAIL' in line:
                    tests_failed.append(line)
                elif ':IGNORE' in line:
                    tests_ignored.append(line)
                else:
                    print("Error parsing test output '%s', ignoring line. Did the test crash?" % line)

    # Print summary:
    crashes = len(tests_crashed)
    passes = len(tests_passed)
    fails = len(tests_failed)
    ignores = len(tests_ignored)
    tests = passes + fails + ignores

    print()
    print()
    print("============= Summary =============")
    if crashes != 0:
        print("Warning! %i test suite(s) crashed. Not all tests were performed!" % crashes)
    print("Ran %i tests." % tests)
    print("Failed: %i  Passed: %i  Ignore: %i" % (fails, passes, ignores))
    print("Success rate (without ignored tests): %2.2f%%" % (float(fails) / (tests - ignores) * 100))
    print()
    print("============ Breakdown ============")
    if crashes != 0:
        print("Crashed test suites (%i):" % crashes)
        for suite in tests_crashed:
            print(suite)
        print()

    print("Failed (%i): " % fails)
    for test in tests_failed:
        print(test)
    print()

    print("Ignored (%i): " % ignores)
    for test in tests_ignored:
        print(test)
    print()

    print("Passed (%i): " % passes)
    for test in tests_passed:
        print(test)

    print("===================================")
    if crashes == 0 and fails == 0:
        print("All good! :)")
        print()
        print()
        sys.exit(0)
    else:
        print("There is some work left to do...")
        print()
        print()
        sys.exit(1)

if __name__ == '__main__':
    args = sys.argv.copy()
    path = args.pop(0)
    main(args)
