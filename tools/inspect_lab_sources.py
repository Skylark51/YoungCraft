from pathlib import Path
import re, math
from collections import Counter

def read_last(path):
    text=path.read_text(errors='replace'); last=None
    for match in re.finditer(r'(?:Standard|Input) orientation:',text):
        rows=[];separators=0
        for line in text[match.end():].splitlines():
            if '-----' in line:
                separators+=1
                if separators==3:break
            elif separators==2:
                f=line.split()
                if len(f)==6:rows.append((int(f[0]),int(f[1]),*map(float,f[3:])))
        if rows:last=rows
    if not last:raise ValueError('No orientation: '+str(path))
    return last

def graph(rows):
    radii={1:.31,6:.76,7:.71,8:.66,16:1.05}
    adj={i:set() for i,r in enumerate(rows) if r[1] in radii}
    for i in adj:
        for j in adj:
            if j<=i:continue
            if .4<math.dist(rows[i][2:],rows[j][2:])<1.2*(radii[rows[i][1]]+radii[rows[j][1]]):adj[i].add(j);adj[j].add(i)
    return adj

if __name__=='__main__':
    for path in Path.home().joinpath('Documents').glob('1-*.out'):
        rows=read_last(path);adj=graph(rows);seen=set();components=[]
        for i in adj:
            if i in seen:continue
            ids={i};queue=[i];seen.add(i)
            while queue:
                for j in adj[queue.pop()]:
                    if j not in seen:ids.add(j);seen.add(j);queue.append(j)
            components.append(sorted(ids))
        metal=[i for i,r in enumerate(rows) if r[1] in (25,26,27)][0]
        print(path.name, 'metal',metal+1, 'atoms',len(rows))
        for ids in components:
            print(' component',Counter(rows[i][1] for i in ids),'original atoms',[i+1 for i in ids])
            print(' donors',[(i+1, rows[i][1],round(math.dist(rows[i][2:],rows[metal][2:]),3)) for i in ids if rows[i][1] in (7,8,16) and math.dist(rows[i][2:],rows[metal][2:])<3])
