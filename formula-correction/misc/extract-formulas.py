#!/usr/bin/env python3
########################################################################
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
########################################################################

import os
import os.path
import sys
import enum

import orcus
from orcus.tools.file_processor import config


FORMULAS_FILENAME_XML = f"{config.prefix_skip}formulas.xml"


def to_string(obj):
    if isinstance(obj, orcus.FormulaTokenType):
        n = len("FormulaTokenType.")
        s = str(obj)
        return s[n:].lower()

    return str(obj)


def escape_str(s):
    buf = []
    for c in s:
        if c == '"':
            buf.append("&quot;")
        elif c == '&':
            buf.append("&amp;")
        elif c == '<':
            buf.append("&lt;")
        elif c == '>':
            buf.append("&gt;")
        elif c == "'":
            buf.append("&apos;")
        else:
            buf.append(c)
    return "".join(buf)


def process_document(filepath, doc):

    def write_tokens(iter, f):
        for token in iter:
            f.write(f'<token s="{escape_str(str(token))}" type="{to_string(token.type)}"/>')

    def write_global_named_exps(iter, f):
        for name, exp in iter:
            f.write(f'<named-expression name="{escape_str(name)}" origin="{escape_str(exp.origin)}" formula="{escape_str(exp.formula)}" scope="global">')
            write_tokens(exp.get_formula_tokens(), f)
            f.write("</named-expression>")

    def write_sheet_named_exps(iter, f, sheet_name):
        for name, exp in iter:
            f.write(f'<named-expression name="{escape_str(name)}" origin="{escape_str(exp.origin)}" formula="{escape_str(exp.formula)}" scope="sheet" sheet="{escape_str(sheet_name)}">')
            write_tokens(exp.get_formula_tokens(), f)
            f.write("</named-expression>")

    outpath = f"{filepath}{FORMULAS_FILENAME_XML}"
    with open(outpath, "w") as f:
        output_buffer = list()
        f.write(f'<doc filepath="{escape_str(filepath)}"><sheets count="{len(doc.sheets)}">')
        for sheet in doc.sheets:
            f.write(f'<sheet name="{escape_str(sheet.name)}"/>')
        f.write("</sheets>")
        f.write("<named-expressions>")

        write_global_named_exps(doc.get_named_expressions(), f)
        for sheet in doc.sheets:
            write_sheet_named_exps(sheet.get_named_expressions(), f, sheet.name)

        f.write("</named-expressions>")

        f.write("<formulas>")

        for sheet in doc.sheets:
            output_buffer.append(f"* sheet: {sheet.name}")
            for row_pos, row in enumerate(sheet.get_rows()):
                for col_pos, cell in enumerate(row):
                    if cell.type == orcus.CellType.FORMULA:
                        f.write(f'<formula sheet="{escape_str(sheet.name)}" row="{row_pos}" column="{col_pos}" formula="{escape_str(cell.formula)}" valid="true">')
                        write_tokens(cell.get_formula_tokens(), f)
                        f.write("</formula>")
                    elif cell.type == orcus.CellType.FORMULA_WITH_ERROR:
                        tokens = [str(t) for t in cell.get_formula_tokens()]
                        f.write(f'<formula sheet="{escape_str(sheet.name)}" row="{row_pos}" column="{col_pos}" formula="{escape_str(tokens[1])}" error="{escape_str(tokens[2])}" valid="false"/>')
                    else:
                        continue

        f.write("</formulas>")
        f.write("</doc>")

        return output_buffer


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", "--output", type=str, required=True, help="Output directory to write all the formula data to.")
    parser.add_argument("rootdir", help="Root directory from which to traverse for the formula files.")
    args = parser.parse_args()

    os.makedirs(args.output, exist_ok=True)

    i = 0
    for root, dir, files in os.walk(args.rootdir):
        for filename in files:
            if not filename.endswith(FORMULAS_FILENAME_XML):
                continue

            inpath = os.path.join(root, filename)
            outpath = os.path.join(args.output, f"{i+1:04}.xml")
            os.link(inpath, outpath)
            i += 1


if __name__ == "__main__":
    main()
