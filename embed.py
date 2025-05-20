import sys, base64, gzip

with open(sys.argv[2], 'rb') as wasm:
    code = base64.b64encode(gzip.compress(wasm.read(), mtime=0)).decode()

with open(sys.argv[1]) as tmpl:
    for line in tmpl:
        if line := line.strip():
            print(line.replace('N9WASM', code, 1))
