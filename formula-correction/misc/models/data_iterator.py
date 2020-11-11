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
from torchtext.data import Example, Field, Dataset


class _FormulaExample(Example):

    def __init__(self, count, formula_tokens):
        self.count = count  # number of occurrences
        self.formula_tokens = formula_tokens


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

    examples = list()
    for n, tokens in get_input_data(args.filepath):
        fex = _FormulaExample(n, tokens)
        examples.append(fex)

    formula_tokens_field = Field(sequential=True, use_vocab=True, eos_token="<eos>")
    fields = [("formula_tokens", formula_tokens_field),]
    ds = Dataset(examples, fields)
    print(f"dataset contains {len(ds)} examples.")

    # build vocabulary
    formula_tokens_field.build_vocab(ds, min_freq=1)

    print(formula_tokens_field.vocab.itos)  # index to string
    print(formula_tokens_field.vocab.stoi)  # string to index

    # Sample the first 10 examples
    for i, ex in enumerate(ds):
        print(ex.formula_tokens)
        if i == 10:
            break


if __name__ == "__main__":
    main()

