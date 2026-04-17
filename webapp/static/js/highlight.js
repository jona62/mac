// Mac Studio — Syntax highlighting (from mac-lang/syntaxes/mac.tmLanguage.json)

const HL_KEYWORDS = new Set("val,var,fun,effect,style,class,enum,for,while,if,else,return,print,in,break,continue,and,or,gif,grid,loop,match".split(","));
const HL_CONSTANTS = new Set("true,false,nil,this,super,Top,Bottom,Center,PNG,JPG,GIF,crossfade,slideLeft,slideRight,slideUp,slideDown,wipe,deepfry".split(","));
const HL_CLASSES = new Set("Template,Meme,Gif,Frame,Size,Duration,Position,Format".split(","));

function highlightMac(code) {
  const TOKEN_RE = /(\/\/.*$)|("(?:[^"\\]|\\.)*")|(\b\d+(?:ms|s)\b)|(\b\d+(?:\.\d+)?\b)|(=>|->|\|>|>>|---)|(@\w+(?:\.\w+)*)|(\b[A-Za-z_]\w*\b)/gm;
  let result = "";
  let last = 0;

  for (const m of code.matchAll(TOKEN_RE)) {
    if (m.index > last) result += hlEsc(code.slice(last, m.index));
    last = m.index + m[0].length;

    if (m[1])      result += `<span class="hl-comment">${hlEsc(m[1])}</span>`;
    else if (m[2]) result += `<span class="hl-string">${hlEsc(m[2])}</span>`;
    else if (m[3]) result += `<span class="hl-number">${hlEsc(m[3])}</span>`;
    else if (m[4]) result += `<span class="hl-number">${hlEsc(m[4])}</span>`;
    else if (m[5]) result += `<span class="hl-operator">${hlEsc(m[5])}</span>`;
    else if (m[6]) result += `<span class="hl-template">${hlEsc(m[6])}</span>`;
    else if (m[7]) {
      const w = m[7];
      if (HL_KEYWORDS.has(w))           result += `<span class="hl-keyword">${w}</span>`;
      else if (HL_CONSTANTS.has(w))     result += `<span class="hl-constant">${w}</span>`;
      else if (HL_CLASSES.has(w))       result += `<span class="hl-class">${w}</span>`;
      else if (code[m.index + w.length] === "(") result += `<span class="hl-function">${w}</span>`;
      else result += w;
    }
    else result += hlEsc(m[0]);
  }

  if (last < code.length) result += hlEsc(code.slice(last));
  return result + "\n";
}

function hlEsc(s) {
  return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}
