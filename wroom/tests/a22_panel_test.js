// Exercise the actual embedded page: command acknowledgement, rendered confirmation,
// reboot request and failure recovery. No browser or external dependencies required.
const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const page=fs.readFileSync('wroom/a22-expresso/panel.h','utf8');
const script=page.split('<script>')[1].split('</script>')[0];
const moods=['normal','atento','curioso','feliz','surpreso','pensando','piscadinha','serio','auto'];
const nodes=Object.fromEntries(['connection','metrics','state','return'].map(id=>[id,{textContent:'',dataset:{}}]));
const buttons=moods.map(mood=>({dataset:{mood},disabled:false}));buttons.push(nodes.return);
let requests=[],fail=false,sequence=0,expression='normal';
const sandbox={document:{hidden:false,getElementById:id=>nodes[id],querySelectorAll:()=>buttons,addEventListener:()=>{}},AbortController,performance,setTimeout,clearTimeout,setInterval:()=>1,clearInterval:()=>{},confirm:()=>true,fetch:async(path,options={})=>{
 requests.push({path,options});if(fail)throw Error('NETWORK_DOWN');
 let data={version:'0.0.33',expression,frames:99,free_heap:120000,applied_id:sequence};
 if(path==='/api/expression'){assert.equal(options.method,'POST');assert.equal(options.headers['X-Botizin-Key'],'__SESSION_KEY__');expression=new URLSearchParams(options.body).get('expression');data={command_id:++sequence};}
 if(path==='/api/return'){assert.equal(options.method,'POST');data={accepted:true};}
 return {ok:true,json:async()=>data};
}};
vm.createContext(sandbox);vm.runInContext(script,sandbox);
const tick=()=>new Promise(resolve=>setImmediate(resolve));
(async()=>{
 await tick();assert.match(nodes.connection.textContent,/WROOM conectada/);
 await buttons[3].onclick();await tick();assert.equal(expression,'feliz');assert.match(nodes.state.textContent,/aplicada/);assert(buttons.every(b=>!b.disabled));
 fail=true;await buttons[2].onclick();await tick();assert.match(nodes.state.textContent,/NETWORK_DOWN/);assert(buttons.every(b=>!b.disabled));
 fail=false;await nodes.return.onclick();assert.match(nodes.state.textContent,/Voltando ao menu/);assert(buttons.every(b=>b.disabled));
 assert(requests.some(r=>r.path==='/api/return'));
 console.log('A22_PANEL_OK: commands, applied acknowledgement, network failure and menu return');
})().catch(e=>{console.error(e);process.exitCode=1});
