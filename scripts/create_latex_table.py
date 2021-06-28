#!/usr/bin/env python3
import sys
import re
import argparse

# Construct an argument parser
all_args = argparse.ArgumentParser()

# Add arguments to the parser
all_args.add_argument("-f", "--file", required=True,
   help="file name")
args = vars(all_args.parse_args())

filename = args['file']
with open(filename) as f:
    content = f.readlines()
table = []
currentrow = {}

datasets = set()
codecs = set()

for line in content:
        
    if line.startswith("convert"):
        codec = re.search(r"\+(\w+)",line).group(1)
        rfile = re.search(r"/(\w+)[\.-]",line).group(1)
        currentrow["codec"] = codec
        currentrow["dataset"] = rfile
        datasets.add(rfile)
        codecs.add(codec)

    m = re.search(r"\s([\.0-9]+) Gc/s",line)
    if m:
        v = float(m.group(1))
        currentrow["result"] = '{:#.2g}'.format(v)
        table.append(currentrow)
        currentrow = {}

favorite_kernels = ["icu", "llvm", "hoehrmann",  "cppcon2018", "u8u16", "utf8sse4", "utf8lut", "haswell", "arm64"]
kernels = []
s = "  "
for k in favorite_kernels:
    if k in codecs:
        kernels.append(k)
        s += " & " + k
s += " \\\\"
print(s)
def get(d, k):
    for x in table:
        if(x['codec'] == k) and (x['dataset'] == d):
            return x["result"]
datasets=sorted(datasets)
for dataset in datasets:
    s = dataset
    for k in kernels:
      s +=  " & " + get(dataset, k)
    s += " \\\\"
    print(s)
