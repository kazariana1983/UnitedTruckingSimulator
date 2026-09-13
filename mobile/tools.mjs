import { cp, mkdir, readFile } from 'node:fs/promises';
import { createServer } from 'node:http';
import { fileURLToPath } from 'node:url';
const root = new URL('./', import.meta.url);
const files = ['index.html', 'style.css', 'app.mjs', 'core.mjs'];
if (process.argv[2] === 'build') {
  await mkdir(new URL('dist/', root), {recursive:true});
  for (const file of files) await cp(new URL(file, root), new URL('dist/'+file, root));
  console.log('Built self-contained web assets in mobile/dist');
} else {
  createServer(async (req,res) => {
    const path = new URL(req.url, 'http://localhost').pathname.slice(1) || 'index.html';
    if (!files.includes(path)) {res.writeHead(404); res.end(); return;}
    try {
      const data=await readFile(new URL(path,root));
      res.writeHead(200, {'Content-Type': path.endsWith('.html')?'text/html':path.endsWith('.css')?'text/css':'text/javascript', 'Cache-Control':'no-store'});res.end(data);
    } catch {res.writeHead(500);res.end('Read failed');}
  }).listen(4173,'0.0.0.0',()=>console.log('Prototype at http://localhost:4173 ('+fileURLToPath(root)+')'));
}
