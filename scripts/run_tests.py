import sys
from os.path import basename, splitext
import subprocess


class Color:
    OK = '\033[92m'
    WARN = '\033[93m'
    ERR = '\033[91m'
    INFO = '\033[96m'
    END = '\33[0m'


def main(test_suites):
    tests_crashed = []
    tests_passed = []
    tests_failed = []
    tests_ignored = []

    # Empty line to clean up makefile output a bit.
    print()

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

        suite_fail_count = 0

        if '-----------------------' not in suite_output:
            # Test suite crashed
            print(Color.ERR, end='')
            print("Test suite %s crashed, test results not reported!" %
                  test_suite_name)
            print(Color.END, end='')
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
                    suite_fail_count += 1
                elif ':IGNORE' in line:
                    tests_ignored.append(line)
                elif ':INFO' in line:
                    print(Color.INFO, end='')
                    print(line)
                    print(Color.END, end='')
                else:
                    print(Color.WARN, end='')
                    print("Error parsing test output '%s', ignoring line. Did the test crash?"
                          % line)
                    print(Color.END, end='')

        # Unity test suites return the number of failed tests.
        # Make sure that matches what we parsed above:
        failed_tests_unity_report = test_output.returncode

        if suite_fail_count != failed_tests_unity_report:
            print(Color.WARN, end='')
            print("Unity report %i failed tests, but %i where tracked by the test output." % (
                failed_tests_unity_report, suite_fail_count))
            print("Verify test suite output manually!")
            print(Color.END, end='')

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
        print(Color.ERR, end='')
        print("Warning! %i test suite(s) crashed. Not all tests were performed!" % crashes)
        print(Color.END, end='')

    print("Ran %i tests." % tests)

    print(Color.ERR, end='')
    print("Failed: %i" % fails)
    print(Color.END + Color.OK, end='')
    print("Passed: %i" % passes)
    print(Color.END + Color.WARN, end='')
    print("Ignore: %i" % ignores)
    print(Color.END, end='')

    if (tests - ignores) != 0:
        print("Success rate (without ignored tests): %2.2f%%" %
              (float(passes) / (tests - ignores) * 100))

    print()
    print("============ Breakdown ============")
    if crashes != 0:
        print("Crashed test suites (%i):" % crashes)
        print(Color.ERR, end='')
        for suite in tests_crashed:
            print(suite)
        print(Color.END, end='')
        print()

    print("Failed (%i): " % fails)
    for test in tests_failed:
        print(Color.ERR, end='')
        print(test)
        print(Color.END, end='')
    print()

    print("Ignored (%i): " % ignores)
    for test in tests_ignored:
        print(Color.WARN, end='')
        print(test)
        print(Color.END, end='')
    print()

    print("Passed (%i): " % passes)
    if passes <= 50:
        for test in tests_passed:
            print(test)
    else:
        print('...')

    print("===================================")
    if crashes == 0 and fails == 0:
        print(Color.OK, end='')
        print("All good! :)")
        print(Color.END)
        print()
        print()
        sys.exit(0)
    else:
        print(Color.WARN, end='')
        print("There is some work left to do...")
        print(Color.END, end='')
        print()
        print()
        sys.exit(1)


if __name__ == '__main__':
    args = sys.argv.copy()
    path = args.pop(0)
    main(args)
