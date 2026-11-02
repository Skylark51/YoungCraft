import urllib.request,json,tarfile,io
from pathlib import Path
root=Path('xtb_runtime/sources');root.mkdir(exist_ok=True)
projects=[('dftd4/dftd4','v3.4.0'),('dftd3/simple-dftd3','v1.0.0'),('grimme-lab/multicharge','v0.2.0'),('toml-f/toml-f','v0.4.1'),('grimme-lab/mctc-lib',None),('grimme-lab/CPCM-X',None),('grimme-lab/numsa',None)]
manifest=[]
for repo,rev in projects:
 try:
  if rev is None:
   req=urllib.request.Request('https://api.github.com/repos/'+repo+'/commits?until=2024-07-23T17:16:00Z&per_page=1',headers={'User-Agent':'Youngcraft-runtime-packaging'})
   rev=json.load(urllib.request.urlopen(req,timeout=20))[0]['sha']
  url='https://api.github.com/repos/'+repo+'/tarball/'+rev
  req=urllib.request.Request(url,headers={'User-Agent':'Youngcraft-runtime-packaging'})
  data=urllib.request.urlopen(req,timeout=30).read();name=repo.split('/')[-1]+'-'+rev[:12]+'.tar.gz';(root/name).write_bytes(data)
  t=tarfile.open(fileobj=io.BytesIO(data));notices=[n for n in t.getnames() if n.rsplit('/',1)[-1].upper() in ('COPYING','COPYING.LESSER','LICENSE','LICENSE.TXT')]
  for i,n in enumerate(notices): (Path('xtb_runtime/licenses')/(repo.split('/')[-1]+'-'+str(i)+'-'+n.rsplit('/',1)[-1])).write_bytes(t.extractfile(n).read())
  manifest.append({'repo':repo,'revision':rev,'source':url,'archive':name,'selection':'pinned release' if len(rev)<12 else 'last upstream commit before Windows compilation date; exact upstream dependency build revision is not published'})
  print(name,flush=True)
 except Exception as e:print(repo,str(e),flush=True)
(root/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
