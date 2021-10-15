import sys
import glob
import shutil
import subprocess

def main():
    try:
        src_dir = sys.argv[1]
        build_dir = sys.argv[2]
        targets = sys.argv[3:]
    except IndexError:
        print("usage: %s <src dir> <build dir> <targets...>" % sys.argv[0])
        exit(-1)

    for target in targets:
        try:
            subprocess.run(
                ["make", "-C", src_dir],
                env={"TARGET" : target},
                check=True
            )
        except subprocess.CalledProcessError:
            print("failed to build: %s" % target)
            exit(-1)

    try:
        for file in glob.glob(r"%s/build/*.bin" % src_dir):
            shutil.copy(file, build_dir)
    except OSError:
        print("failed to copy payloads")
        exit(-1)

if __name__ == '__main__':
    main()
