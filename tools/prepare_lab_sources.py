"""Extract source-backed ligand models; exclude substrates, solvent and O/N adducts."""
from pathlib import Path
from collections import Counter
import json,hashlib
import numpy as np
from inspect_lab_sources import read_last,graph

root=Path(__file__).resolve().parents[1]
reference=json.loads((root/'lab_fragments/adducts/peroxo-eta2.cjson').read_text(encoding='utf-8'))
oxygen=np.array(reference['atoms']['coords']['3d']).reshape(-1,3)[1:]
axis=oxygen.mean(axis=0);axis/=np.linalg.norm(axis)
side=oxygen[1]-oxygen[0];side-=axis*np.dot(side,axis);side/=np.linalg.norm(side)
target=np.array([axis,side,np.cross(axis,side)]).T
sources=[('1-FeIV-O-MDAP-forHAA-scan.out','MDAP','mdap',Counter({6:16,1:20,7:4})),
 ('1-FeII-Me2S-DPADP-fragment-opt-freq-stable.out','Me2S-DPADP','me2s-dpadp',Counter({6:13,1:21,7:3,16:1})),
 ('1-Fe-H2NO2-Nbinding-H2O-opt-freq-re-ts-freq-ircrev-opt.out','TBDAP (이름 확인 필요)','tbdap-candidate',Counter({6:22,1:32,7:4}))]
for filename,name,key,formula in sources:
    source=Path.home()/'Documents'/filename;rows=read_last(source);adj=graph(rows)
    metal=next(i for i,r in enumerate(rows) if r[1] in (25,26,27))
    # The ligand is the largest covalent component containing donor nitrogen.
    components=[];seen=set()
    for i in adj:
        if i in seen:continue
        ids={i};queue=[i];seen.add(i)
        while queue:
            for j in adj[queue.pop()]:
                if j not in seen:ids.add(j);seen.add(j);queue.append(j)
        if any(rows[j][1]==7 for j in ids):components.append(ids)
    indices=sorted(max(components,key=len));assert Counter(rows[i][1] for i in indices)==formula
    mapping={old:new for new,old in enumerate([metal]+indices)}
    coords=np.array([r[2:] for r in rows]);coords-=coords[metal].copy()
    donors=[i for i in indices if rows[i][1] in (7,16) and np.linalg.norm(coords[i])<2.8]
    assert len(donors)==4
    cycles=set()
    def visit(path):
        if len(path)==6:
            if path[0] in adj[path[-1]] and Counter(rows[i][1] for i in path)=={6:5,7:1}:cycles.add(tuple(sorted(path)))
            return
        for j in adj[path[-1]]:
            if j not in path and rows[j][1] in (6,7):visit(path+[j])
    for i in indices:
        if rows[i][1] in (6,7):visit([i])
    aromatic=set()
    for ring in cycles:
        for i in ring:
            for j in adj[i]:
                if j in ring:aromatic.add(tuple(sorted((i,j))))
    connections=[];orders=[];valence=Counter()
    for i in donors:connections += [0,mapping[i]];orders.append(1)
    for i in indices:
        for j in sorted(adj[i]):
            if j<=i or j not in mapping:continue
            order=1.5 if (i,j) in aromatic else 1
            connections += [mapping[i],mapping[j]];orders.append(order);valence[i]+=order;valence[j]+=order
    for i in indices:assert valence[i]=={1:1,6:4,7:3,16:2}[rows[i][1]],(name,i+1,rows[i][1],valence[i])
    opening=-coords[donors].mean(axis=0);opening/=np.linalg.norm(opening)
    lateral=coords[donors[0]]-opening*np.dot(coords[donors[0]],opening);lateral/=np.linalg.norm(lateral)
    original=np.array([opening,lateral,np.cross(opening,lateral)]).T
    aligned=coords[[metal]+indices]@(target@original.T).T
    best=None;score=float('inf')
    for step in range(72):
        angle=np.radians(step*5);c,s=np.cos(angle),np.sin(angle)
        cross=np.array([[0,-axis[2],axis[1]],[axis[2],0,-axis[0]],[-axis[1],axis[0],0]])
        rotation=c*np.eye(3)+(1-c)*np.outer(axis,axis)+s*cross
        candidate=aligned@rotation.T
        distances=np.linalg.norm(candidate[1:,None,:]-oxygen[None,:,:],axis=2)
        penalty=np.maximum(0,1.5-distances).sum()
        if penalty<score:score=penalty;best=candidate
    assert np.min(np.linalg.norm(best[1:,None,:]-oxygen[None,:,:],axis=2))>.45
    obj={'chemicalJson':1,'atoms':{'elements':{'number':[0]+[rows[i][1] for i in indices]},
        'coords':{'3d':best.flatten().tolist()},'formalCharges':[0]*(len(indices)+1)},
        'bonds':{'connections':{'index':connections},'order':orders},
        'properties':{'name':name,'category':'lab-ligand','totalCharge':0,
            'source':{'file':filename,'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
                'orientation':'last available Gaussian orientation; rigid alignment only',
                'originalMetal':metal+1,'originalDonors':[i+1 for i in donors],
                'identityConfirmed':key!='tbdap-candidate',
                'bondPerception':'distance, C5N aromatic cycles, exact explicit-H valence checks'},
            'originalAtomNumbers':[i+1 for i in [metal]+indices]}}
    (root/f'lab_fragments/ligands/{key}.cjson').write_text(json.dumps(obj,indent=2),encoding='utf-8')
    print(name,dict(formula),'aromatic rings',len(cycles),'donors',[i+1 for i in donors])
