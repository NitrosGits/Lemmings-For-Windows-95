#!/bin/zsh
# score.sh PROJECT MODULE : rebuild module units, report per-function 100% counts
cd $1; T="/Users/daniel/Library/Application Support/decompnet/toolchains"
python3 tools/configure.py lemmings --msvc "$T/msvc" --wibo "$T/wibo/wibo" >/dev/null 2>&1
rm -f build/lemmings/src/auto/$2/*.obj(N)
timeout 300 ninja -k 0 -j 4 $(ls src/auto/$2/*.cpp | sed 's#src/#build/lemmings/src/#; s#.cpp$#.obj#') 2>&1 | grep -E "error|FAILED" | cut -c1-160 | head -${3:-8}
python3 - <<P
import json,os
j=json.load(open('objdiff.json'))
j['units']=[u for u in j['units'] if os.path.exists(u['base_path'])]
json.dump(j,open('objdiff.json','w'),indent=1)
P
"$T/objdiff-cli/objdiff-cli" report generate -o /tmp/sc_$2.json >/dev/null 2>&1
python3 - <<P
import json
r=json.load(open('/tmp/sc_$2.json'))
for u in r['units']:
    if u['name'].startswith('$2/'):
        fs=u['functions']; print(u['name'], sum(1 for f in fs if f.get('fuzzy_match_percent',0)>=99.99),'/',len(fs))
P
