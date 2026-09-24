const d=JSON.parse(require('fs').readFileSync('.impeccable/design.json','utf8'));
const c=d.components||[];
const bad=c.filter(x=>!x.name||!x.kind||!x.html||!x.css||!/ds-/.test(x.html)||!/ds-/.test(x.css));
const noFocus=c.filter(x=>!/focus-visible/.test(x.css));
const ok=c.length>=7 && bad.length===0 && noFocus.length<=3
  && d.narrative && d.narrative.northStar==='The Analysis Room'
  && d.extensions && Object.keys(d.extensions.colorMeta||{}).length>=14;
if(!ok){console.error('components incomplete:',JSON.stringify({count:c.length,bad:bad.map(x=>x.name||'(unnamed)'),noFocus:noFocus.map(x=>x.name),narrative:!!(d.narrative&&d.narrative.northStar),colors:Object.keys((d.extensions||{}).colorMeta||{}).length}));process.exit(1);}
console.log('components ok:',c.length);
