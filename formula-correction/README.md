
Formula Correction Model
========================

## Training data preparation

### Download training documents

TBD

### Process training documents to generate formula XML files

Once downloaded the documents in the directory named `docs`, run the following
command:

```bash
python -m orcus.tools.file_processor \
    -p misc/extract-formulas.py \
    --skip-files misc/skip-files.txt \
    ./docs
```
in order to process all the documents and extract their formula data.  The extracted
formula data are written directly to the directory tree as XML files named
`[original doc name].orcus-pf.skip.formulas.xml`.  Note that this process may
take a long time to finish depending on how many training documents you have
downloaded.

Once that finishes, run:

```bash
python misc/extract-formulas.py -o formulas ./docs
```
in order to collect all the formula XML files into directory named `formulas`.

### Parse formula XML files

TBD
