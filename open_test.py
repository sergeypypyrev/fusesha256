#!/usr/bin/env python3

import threading
import sys

def open_thread(file):
    for _ in range(3):
        with open(file):
            pass

def main(file):
    for _ in range(10):
        t = threading.Thread(target=open_thread, args=(file,))
        t.start()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"""
Usage:

python3 {sys.argv[0]} FILE
""",file=sys.stderr)
        sys.exit(1)
    main(sys.argv[1])
