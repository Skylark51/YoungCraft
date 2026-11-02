"""Extract 14-MTP from the supplied 51-atom metal/ligand/O2 XYZ file.

Align its O2 frame to the existing 12-MTP/O2 reference; keep all ligand
internal coordinates. Bond perception is checked against explicit-H valences.
"""
from pathlib import Path
import argparse, json, hashlib
from collections import Counter
import numpy as np

p=argparse.ArgumentParser();p.add_argument('source',type=Path);args=p.parse_args()
root=Path(__file__).resolve().parents[1]
lines=args.source.read_text().splitlines(); n=int(lines[0]); rows=[line.split() for line in lines[2:2+n]]
assert n==51 and rows[0][0]=='Mn'
symbols=[r[0] for r in rows]; xyz=np.array([[float(v) for v in r[1:4]] for r in rows])
ox=[i for i,s in enumerate(symbols) if s=='O']; assert len(ox)==2
indices=[i for i in range(1,n) if i not in ox]
assert Counter(symbols[i] for i in indices)=={'C':16,'H':28,'N':4}
ref=json.loads((root/'lab_fragments/adducts/peroxo-eta2.cjson').read_text(encoding='utf-8'))
refxyz=np.array(ref['atoms']['coords']['3d']).reshape(-1,3)
def frame(o):
    x=o.mean(axis=0);x/=np.linalg.norm(x)
    y=o[1]-o[0];y-=x*np.dot(x,y);y/=np.linalg.norm(y)
    return np.array([x,y,np.cross(x,y)]).T
rotation=frame(refxyz[1:])@frame(xyz[ox]-xyz[0]).T
positions=(xyz-xyz[0])@rotation.T
mapping={old:new for new,old in enumerate([0]+indices)}
radii={'C':.76,'H':.31,'N':.71}; edges=[]
for a,i in enumerate(indices):
    for j in indices[a+1:]:
        if .4<np.linalg.norm(xyz[i]-xyz[j])<1.2*(radii[symbols[i]]+radii[symbols[j]]):edges.append((i,j))
adj={i:set() for i in indices}
for i,j in edges:adj[i].add(j);adj[j].add(i)
cycles=set()
def visit(path):
    if len(path)==6:
        if path[0] in adj[path[-1]] and Counter(symbols[i] for i in path)=={'C':5,'N':1}:
            cycles.add(tuple(sorted(path)))
        return
    for j in adj[path[-1]]:
        if j not in path and symbols[j]!='H':visit(path+[j])
for i in indices:
    if symbols[i]!='H':visit([i])
assert len(cycles)==1,cycles
ring=set(next(iter(cycles))); donors=[i for i in indices if symbols[i]=='N']
assert len(donors)==4 and all(np.linalg.norm(xyz[i]-xyz[0])<2.7 for i in donors)
connections=[];orders=[];valence=Counter()
for i in donors:connections += [0,mapping[i]];orders.append(1)
for i,j in edges:
    order=1.5 if i in ring and j in ring else 1
    connections += [mapping[i],mapping[j]];orders.append(order);valence[i]+=order;valence[j]+=order
for i in indices:
    expected=1 if symbols[i]=='H' else 4 if symbols[i]=='C' else 3
    assert valence[i]==expected,(i+1,symbols[i],valence[i])
data={'chemicalJson':1,'atoms':{'elements':{'number':[0]+[{'C':6,'H':1,'N':7}[symbols[i]] for i in indices]},
    'coords':{'3d':positions[[0]+indices].flatten().tolist()},'formalCharges':[0]*(len(indices)+1)},
    'bonds':{'connections':{'index':connections},'order':orders},
    'properties':{'name':'14-MTP','category':'lab-ligand','totalCharge':0,
        'source':{'file':args.source.name,'sha256':hashlib.sha256(args.source.read_bytes()).hexdigest(),
            'kind':'supplied side-on scan starting structure; rigidly aligned to 12-MTP reference O2 frame',
            'excludedAtoms':[i+1 for i in ox],'originalDonors':[i+1 for i in donors],
            'bondPerception':'distance with explicit-H valence checks; one C5N aromatic ring'},
        'originalAtomNumbers':[i+1 for i in [0]+indices]}}
(root/'lab_fragments/ligands/14-mtp.cjson').write_text(json.dumps(data,indent=2),encoding='utf-8')
print('Verified 14-MTP: C16H28N4, four N donors, one pyridine ring; rigid internal geometry preserved.')
