"""Extract and validate the supplied Mn(III)/12-MTP/peroxo reference."""
from pathlib import Path
import argparse,re,math,json,hashlib
from collections import Counter
p=argparse.ArgumentParser();p.add_argument('source',type=Path);a=p.parse_args()
text=a.source.read_text(errors='replace');tables=[]
for match in re.finditer(r'(?:Standard|Input) orientation:',text):
    rows=[];separators=0
    for line in text[match.end():].splitlines():
        if '-----' in line:
            separators+=1
            if separators==3:break
        elif separators==2:
            f=line.split()
            if len(f)==6:rows.append((int(f[0]),int(f[1]),*map(float,f[3:])))
    if rows:tables.append(rows)
if not tables:raise ValueError('No orientation table')
rows=tables[-1]
assert len(rows)==62 and rows[0][1]==25
assert Counter(r[1] for r in rows[1:43])=={6:14,1:24,7:4}
assert [rows[i][1] for i in (2,5,9,17)]==[7]*4
assert [rows[i][1] for i in (43,44)]==[8,8]
assert Counter(r[1] for r in rows[45:])=={6:6,1:10,8:1}
assert 1.2<math.dist(rows[43][2:],rows[44][2:])<1.6
root=Path(__file__).resolve().parents[1]
source={'file':a.source.name,'sha256':hashlib.sha256(a.source.read_bytes()).hexdigest(),'referenceMetal':'Mn(III)',
'referenceCharge':1,'referenceMultiplicity':5,'excludedOriginalAtoms':list(range(46,63)),
 'templateFrame':'MnIII-12MTP-peroxo-reference-v1'}
radii={1:.31,6:.76,7:.71,8:.66};aromatic={(12,13),(13,14),(14,15),(15,16),(16,17),(12,17)}
def export(indices,donors,name,category,charge,target):
    original=[0]+indices;mapping={old:i for i,old in enumerate(original)}
    coords=[rows[i][j]-rows[0][j] for i in original for j in range(2,5)]
    pairs=[];orders=[]
    for donor in donors:pairs += [0,mapping[donor]];orders.append(1)
    for ix,i in enumerate(indices):
        for j in indices[ix+1:]:
            d=math.dist(rows[i][2:],rows[j][2:])
            if .4<d<1.20*(radii[rows[i][1]]+radii[rows[j][1]]):
                pairs += [mapping[i],mapping[j]];orders.append(1.5 if (min(i,j),max(i,j)) in aromatic else 1)
    obj={'chemicalJson':1,'atoms':{'elements':{'number':[0]+[rows[i][1] for i in indices]},
         'coords':{'3d':coords},'formalCharges':[0]+[(-1 if category=='lab-adduct' else 0) for i in indices]},
         'bonds':{'connections':{'index':pairs},'order':orders},
         'properties':{'name':name,'category':category,'totalCharge':charge,'source':source,
                       'originalAtomNumbers':[i+1 for i in original]}}
    target=root/target;target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(json.dumps(obj,ensure_ascii=False,indent=2),encoding='utf-8')
export(list(range(1,43)),[2,5,9,17],'12-MTP (Me3-TPADP)','lab-ligand',0,'lab_fragments/ligands/12-mtp.cjson')
export([43,44],[43,44],'peroxo (eta2, O2 2-)','lab-adduct',-2,'lab_fragments/adducts/peroxo-eta2.cjson')
folder=root/'.build/lab-complex-check';folder.mkdir(parents=True,exist_ok=True)
for metal in ('Fe','Mn'):
    coords=[(metal,0.,0.,0.)]+[({1:'H',6:'C',7:'N',8:'O'}[rows[i][1]],*[rows[i][j]-rows[0][j] for j in range(2,5)]) for i in range(1,45)]
    xyz='45\n'+metal+' + 12-MTP + eta2-peroxo; charge +1; cyclohexanone removed\n'
    xyz+=''.join('%s %.9f %.9f %.9f\n'%r for r in coords)
    d=folder/metal;d.mkdir(exist_ok=True);(d/'input.xyz').write_text(xyz)
print('Extracted C14H24N4 (42 atoms) + O2 (2 atoms); excluded C6H10O (17 atoms).')
print('Original donors N3/N6/N10/N18 and O44/O45; O-O %.4f A'%math.dist(rows[43][2:],rows[44][2:]))
