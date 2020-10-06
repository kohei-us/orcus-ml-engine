
Formula Correction Model
========================

## Training data preparation

### Download training documents

Download all attachments from the [Bugzilla hosted on TDF site](https://bugs.documentfoundation.org).
Since we are only interested in spreadsheet documents, narrow the query by
specifying the product and component to `LibreOffice Calc`.  Running something
like the following:

```bash
python3 -m orcus.tools.bugzilla \
    -o ./bugdocs \
    --limit 200 \
    --cont \
    --worker 8 \
    --cache-dir ./bugzilla-cache \
    --url https://bugs.documentfoundation.org \
    Product=LibreOffice \
    Component=Calc
```
should get the job done.  Feel free to remove the cache directory once the
download is finished.

### Process training documents to generate formula XML files

Once downloaded the documents in the directory named `bugdocs`, run the following
command:

```bash
python3 -m orcus.tools.file_processor \
    -p misc/extract-formulas.py \
    --skip-file misc/skip-files.txt \
    ./bugdocs
```
in order to process all the documents and extract their formula data.  The extracted
formula data are written directly to the directory tree as XML files named
`[original doc name].orcus-pf.skip.formulas.xml`.  Note that this process may
take a long time to finish depending on how many training documents you have
downloaded.

Once that finishes, run:

```bash
python3 misc/extract-formulas.py -o formulas ./bugdocs
```
in order to collect all the formula XML files into directory named `formulas`.

### Parse formula XML files

TBD
