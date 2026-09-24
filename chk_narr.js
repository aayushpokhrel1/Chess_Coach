const d=JSON.parse(require('fs').readFileSync('.impeccable/design.json','utf8'));
const n=d.narrative||{};
const ok = n.northStar==='The Analysis Room'
  && typeof n.overview==='string' && n.overview.length>600
  && Array.isArray(n.keyCharacteristics) && n.keyCharacteristics.length>=5
  && Array.isArray(n.rules) && n.rules.length>=7
  && Array.isArray(n.dos) && n.dos.length>=7
  && Array.isArray(n.donts) && n.donts.length>=8
  && Array.isArray(d.components);
if(!ok){console.error('narrative incomplete:',JSON.stringify({northStar:n.northStar,ov:(n.overview||'').length,kc:(n.keyCharacteristics||[]).length,rules:(n.rules||[]).length,dos:(n.dos||[]).length,donts:(n.donts||[]).length}));process.exit(1);}
console.log('narrative ok');
