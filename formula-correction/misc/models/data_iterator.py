#!/usr/bin/env python3
########################################################################
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
########################################################################

import argparse
from pathlib import Path


def get_input_data(filepath):
    with open(filepath, "r") as f:
        for line in f.readlines():
            a = line.strip().split(' ')
            # first item is the number of occurrences
            yield a[0], a[1:]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("filepath", type=Path)
    args = parser.parse_args()

    for n, tokens in get_input_data(args.filepath):
        print(n, tokens)


if __name__ == "__main__":
    main()

