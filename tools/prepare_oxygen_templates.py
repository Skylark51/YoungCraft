"""Reproducible editable oxygen starting models in the existing lab metal frame."""
import json, math
from pathlib import Path
import numpy as np

root = Path(__file__).resolve().parents[1]
folder = root / 'lab_fragments/adducts'
reference = json.loads((folder / 'peroxo-eta2.cjson').read_text(encoding='utf-8'))
coords = np.array(reference['atoms']['coords']['3d']).reshape(-1, 3)
axis = (coords[1] + coords[2]) / 2
axis /= np.linalg.norm(axis)
perp = coords[2] - coords[1]
perp -= axis * np.dot(perp, axis)
perp /= np.linalg.norm(perp)
for species, length, charge in [('peroxo', 1.45, -2), ('superoxo', 1.30, -1), ('hydroperoxo', 1.45, -1)]:
    for mode in ('end-on', 'side-on'):
        if species == 'peroxo' and mode == 'side-on':
            continue  # Preserve the supplied reference, including its exact O-O length.
        if mode == 'side-on':
            center = axis * math.sqrt(1.95**2 - (length/2)**2)
            o1, o2 = center-perp*length/2, center+perp*length/2
            connections, orders = [0,1,0,2,1,2], [1,1,1.5 if species == 'superoxo' else 1]
        else:
            o1 = axis * 1.95
            o2 = o1 + length * (0.5*axis + math.sqrt(0.75)*perp)
            connections, orders = [0,1,1,2], [1,1.5 if species == 'superoxo' else 1]
        positions, numbers = [np.zeros(3),o1,o2], [0,8,8]
        charges = [0,-1,-1] if species == 'peroxo' else [0,-1,0]
        if species == 'hydroperoxo':
            # Distal proton: O-O-H = 104 degrees, O-H = 0.98 A.
            oo = (o2-o1)/length
            normal = np.cross(axis,perp)
            positions.append(o2+0.98*(math.cos(math.radians(76))*oo+math.sin(math.radians(76))*normal))
            numbers.append(1); charges.append(0); connections += [2,3]; orders += [1]
        data = {'chemicalJson':1,
            'atoms':{'elements':{'number':numbers}, 'coords':{'3d':np.array(positions).flatten().tolist()}, 'formalCharges':charges},
            'bonds':{'connections':{'index':connections},'order':orders},
            'properties':{'name':f'{species} ({mode}, eta{2 if mode == "side-on" else 1})',
                'category':'lab-adduct','totalCharge':charge,
                'source':{'kind':'idealized editable starting model', 'metalOAngstrom':1.95,
                    'oxygenOxygenAngstrom':length,'notOptimized':True,
                    'reference':'https://doi.org/10.1073/pnas.0535926100',
                    'note':'Binding geometry/charge are input assumptions, not an electronic-state assignment. Superoxo O-O uses a schematic bond order of 1.5.'}}}
        (folder/f'{species}-{mode}.cjson').write_text(json.dumps(data,indent=2),encoding='utf-8')

# Embed the verified lab library; external templates can override a bundled name.
qrc = root/'resources/fragments.qrc'
text=qrc.read_text(encoding='utf-8')
for path in sorted((root/'lab_fragments').rglob('*.cjson')):
    relative=path.relative_to(root).as_posix()
    entry=f'<file alias="{relative}">../{relative}</file>'
    if entry not in text: text=text.replace('</qresource>',entry+'\n</qresource>')
qrc.write_text(text,encoding='utf-8')
