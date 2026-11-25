from pathlib import Path
from html.parser import HTMLParser
import re

class Guide(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids=[]
        self.links=[]
        self.images=[]
    def handle_starttag(self, tag, attrs):
        a=dict(attrs)
        if 'id' in a: self.ids.append(a['id'])
        if tag=='a' and a.get('href','').startswith('#'): self.links.append(a['href'][1:])
        if tag=='img': self.images.append(a.get('src',''))

for lang in ('fr','en','de','es','it','pt'):
    p=Path('help.html.in' if lang=='fr' else f'help_{lang}.html.in')
    html=p.read_text(encoding='utf-8')
    guide=Guide();guide.feed(html)
    assert len(guide.ids)==len(set(guide.ids)),lang
    assert guide.ids.count('chapter-mems16na')==1,lang
    assert guide.links.count('chapter-mems16na')==4,lang
    assert guide.images.count(':/icons/helpbubble.png')==1,lang
    assert len([x for x in guide.images if x.startswith(':/help/images/')])==13,lang
    assert all(x.startswith(':/help/images/') or x==':/icons/helpbubble.png' for x in guide.images),lang
    section=html.split('<h2 id="chapter-mems16na">',1)[1].split('<h2 id="chapter-9">',1)[0]
    for token in ('123','AANMP002','EKDMF002','EKDMF000','EKDMF003','80/04','80/06','80/0B','80/0C','80/09','80/0F','80/10','80/11','80/1B','7D/02','7D/0B','7D/0F','7D/13','7D/14–15','7D/17–1E','7D/1F','32768','µs','TPS','DTC 0','DTC 1','DTC 2','DTC 3, 4, 5','7D/10','MEMS16NA_SCHEMA=2','×5','160','64','58'):
        assert token in section,(lang,token)
    assert section.count('<tr>')==22,(lang,section.count('<tr>'))
    assert '32768' in html.split('<h2 id="chapter-4">',1)[1].split('<h2 id="chapter-5">',1)[0],lang
print('MEMS16NA_HELP_OK: six offline guides, 16 measurement rows, four DTC rows, anchors, images and scoped idle correction')
