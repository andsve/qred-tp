import argparse
import subprocess
import sys

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--patch', help='Bump patch number', action='store_true')
    parser.add_argument('--minor', help='Bump minor number', action='store_true')
    parser.add_argument('--major', help='Bump major number', action='store_true')
    parser.add_argument('--publish', help='Publish Release', action='store_true')
    args = parser.parse_args()

    #print(args)
    f = open("VERSION","r")
    version = f.readlines()[0].replace('\"', '')
    f.close()

    (major,minor,patch) = version.split(".")

    major = int(major)
    minor = int(minor)
    patch = int(patch)

    do_write = False

    if args.patch:
        patch += 1
        do_write = True
    if args.minor:
        minor += 1
        do_write = True
    if args.major:
        major += 1
        do_write = True

    if do_write:
        print("")

    version_str = "%d.%d.%d" % (major,minor,patch)
    version = "\"%s\"" % (version_str)

    print("Bumping version to " + version_str)

    f = open("VERSION", "w")
    f.write(version)
    f.close()

    if args.publish:
        def check_result_or_exit(code, if_error):
            if code != 0:
                print(if_error)
                sys.exit(-1)

        retcode = subprocess.call("git tag v%s" % version_str, shell=True)
        check_result_or_exit(retcode, "Unable to create tag")

        retcode = subprocess.call("git push --tags", shell=True)
        check_result_or_exit(retcode, "Unable to push")

        print("Finished publishing to github")
