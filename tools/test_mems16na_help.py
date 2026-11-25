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
    for token in ('µs','TPS','DTC 0','DTC 1','DTC 2','DTC 3, 4, 5','RPM'):
        assert token in section,(lang,token)
    assert section.count('<tr>')==22,(lang,section.count('<tr>'))
    # The public help must not contain internal formulas, field mapping,
    # protocol bytes or the former detailed firmware examples.
    forbidden=r'(?i)(?:\b(?:AANMP002|EKDMF00[023]|MEMS16NA_SCHEMA|32768|8E1|XOR)\b|\b(?:7D|80)/[0-9A-F]{2}\b|\b31 CB\b|\b71 CB\b|\bB8 13 F7\b|\b14 00 00\b|\b54 00 00\b|×\s*\d|/\s*256\b)'
    assert not re.search(forbidden,html),(lang,'technical implementation in help')
print('MEMS16NA_HELP_OK: six offline guides, 16 measurement rows, four DTC rows, anchors, images and presentation confidentiality')
