#!/usr/bin/env python3
import os
import sys
import subprocess
import argparse
import difflib

# Color helper functions
def color(text, color_code):
    if sys.stdout.isatty():
        return f"\033[{color_code}m{text}\033[0m"
    return text

def green(text): return color(text, "32")
def red(text): return color(text, "31")
def yellow(text): return color(text, "33")
def cyan(text): return color(text, "36")
def bold(text): return color(text, "1")

def run_command(cmd, cwd=None):
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=10
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "Timeout expired"
    except Exception as e:
        return -1, "", str(e)

def build_project():
    print(cyan("Building the project..."))
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    build_dir = os.path.join(project_root, "build")
    
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
        print("Running cmake...")
        code, stdout, stderr = run_command(["cmake", ".."], cwd=build_dir)
        if code != 0:
            print(red("CMake configuration failed:"))
            print(stderr)
            sys.exit(1)
            
    code, stdout, stderr = run_command(["make"], cwd=build_dir)
    if code != 0:
        print(red("Build failed:"))
        print(stderr)
        sys.exit(1)
    
    executable = os.path.join(build_dir, "nir")
    if not os.path.exists(executable):
        print(red(f"Executable not found at {executable}"))
        sys.exit(1)
        
    print(green("Build successful!"))
    return executable

def main():
    parser = argparse.ArgumentParser(description="Run regression tests for nir-lang")
    parser.add_argument("-u", "--update", action="store_true", help="Update expected output files with actual output")
    parser.add_argument("tests", nargs="*", help="Specific test files to run. If empty, all tests under tests/cases/ are run.")
    args = parser.parse_args()

    tests_dir = os.path.dirname(os.path.abspath(__file__))
    cases_dir = os.path.join(tests_dir, "cases")
    
    if not os.path.exists(cases_dir):
        os.makedirs(cases_dir)

    executable = build_project()
    
    # Resolve which test cases to run
    if args.tests:
        test_files = []
        for t in args.tests:
            path = os.path.abspath(t)
            if os.path.exists(path):
                test_files.append(path)
            else:
                print(red(f"Test file not found: {t}"))
                sys.exit(1)
    else:
        test_files = [
            os.path.join(cases_dir, f)
            for f in sorted(os.listdir(cases_dir))
            if f.endswith(".nir")
        ]
        
    if not test_files:
        print(yellow("No test cases found in tests/cases/"))
        print("To add test cases, place .nir files in tests/cases/")
        sys.exit(0)
        
    passed_count = 0
    failed_count = 0
    
    print(bold(f"\nRunning {len(test_files)} tests...\n"))
    
    for test_file in test_files:
        test_name = os.path.basename(test_file)
        expected_out_file = os.path.splitext(test_file)[0] + ".expected"
        expected_exit_file = os.path.splitext(test_file)[0] + ".exit"
        
        # Run compiler on the test file
        # We pass the relative path to avoid absolute path differences in AST output/error messages if any
        rel_test_file = os.path.relpath(test_file, os.path.dirname(executable))
        exit_code, stdout, stderr = run_command([executable, rel_test_file], cwd=os.path.dirname(executable))
        
        # Clean/normalize paths in the output to make tests machine-independent
        actual_output = (stdout + stderr)
        # Replace the relative path prefix in stdout/stderr to make it predictable
        actual_output = actual_output.replace(rel_test_file, f"examples/{test_name}")
        actual_output = actual_output.replace(test_file, f"examples/{test_name}")
        
        if args.update:
            # Write actual output to expected file
            with open(expected_out_file, "w", encoding="utf-8") as f:
                f.write(actual_output)
            if exit_code != 0:
                with open(expected_exit_file, "w", encoding="utf-8") as f:
                    f.write(str(exit_code))
            elif os.path.exists(expected_exit_file):
                os.remove(expected_exit_file)
            print(f"{test_name}: {yellow('UPDATED')}")
            passed_count += 1
            continue
            
        # Read expected output
        expected_output = ""
        if os.path.exists(expected_out_file):
            with open(expected_out_file, "r", encoding="utf-8") as f:
                expected_output = f.read()
        else:
            # If no expected file exists and not updating, fail the test
            print(f"{test_name}: {red('FAILED')} (Missing expected output file)")
            print(f"  Run with {cyan('--update')} or {cyan('-u')} to generate expected outputs.")
            failed_count += 1
            continue
            
        # Read expected exit code
        expected_exit_code = 0
        if os.path.exists(expected_exit_file):
            try:
                with open(expected_exit_file, "r", encoding="utf-8") as f:
                    expected_exit_code = int(f.read().strip())
            except ValueError:
                expected_exit_code = 0
                
        # Compare actual and expected
        output_diff = list(difflib.unified_diff(
            expected_output.splitlines(keepends=True),
            actual_output.splitlines(keepends=True),
            fromfile=f"expected ({test_name})",
            tofile=f"actual ({test_name})"
        ))
        
        if exit_code != expected_exit_code:
            print(f"{test_name}: {red('FAILED')}")
            print(f"  Exit code mismatch: expected {expected_exit_code}, got {exit_code}")
            failed_count += 1
        elif output_diff:
            print(f"{test_name}: {red('FAILED')}")
            print("".join(output_diff))
            failed_count += 1
        else:
            print(f"{test_name}: {green('PASSED')}")
            passed_count += 1
            
    print(bold("\n" + "=" * 40))
    print(bold("Test Summary:"))
    print(f"  Passed: {green(passed_count)}")
    print(f"  Failed: {red(failed_count) if failed_count > 0 else '0'}")
    print(bold("=" * 40))
    
    if failed_count > 0:
        sys.exit(1)
    else:
        sys.exit(0)

if __name__ == "__main__":
    main()
