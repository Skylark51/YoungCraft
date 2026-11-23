# Public corpus seed

Unmodified COD CIFs 1000041–1000050 are retained here so the existing regression
does not depend on ten separate COD downloads for every new pull request.
Embedded authorship, publication and revision records are preserved.

Original URLs and required SHA256 values are recorded in
`../november-corpus-sources.json`. `tools/fetch_november_corpus.py` verifies the
seed bytes with the same manifest as downloaded data. Calculation outputs still
come from the pinned cclib-data commit in that manifest.
