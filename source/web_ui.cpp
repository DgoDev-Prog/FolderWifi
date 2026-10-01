#include "runtime.hpp"
namespace fw {
const char* webPage(){return R"FOLDERWIFI(<!doctype html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>FolderWifi</title>
<style>
[data-theme=light]{
    color-scheme:light;
    --bg:#edf2f7;
    --card:#fff;
    --line:#bdcbdc;
    --text:#182c3f;
    --muted:#4b5e72;
    --accent:#007e71;
    --danger:#a90f32
}
[data-theme=light] button,[data-theme=light] input,[data-theme=light] select{
    background:#e4edf5
}
[data-theme=light] button.primary{
    background:var(--accent);
    color:white
}
[data-theme=light] th{
    background:#e1eaf3
}
[data-theme=light] tr.selected{
    background:#ccece9
}
[data-theme=light] .toast,[data-theme=light] #actions{
    background:#eef9f7
}
[hidden]{
    display:none!important
}
:root{
    color-scheme:dark;
    --bg:#101621;
    --card:#1a2332;
    --line:#334155;
    --text:#edf4ff;
    --muted:#aebcd0;
    --accent:#52dfcd;
    --danger:#ff9eaa
}
*{
    box-sizing:border-box
}
body{
    margin:0;
    background:var(--bg);
    color:var(--text);
    font:16px system-ui,sans-serif
}
button,input,select{
    font:inherit;
    color:inherit;
    background:#263348;
    border:1px solid var(--line);
    border-radius:8px;
    padding:10px;
    min-height:44px
}
button{
    cursor:pointer
}
button:hover,button:focus-visible{
    border-color:var(--accent)
}
button:disabled{
    opacity:.45;
    cursor:default
}
button.primary{
    background:var(--accent);
    color:#06241e
}
button.danger{
    color:var(--danger)
}
a{
    color:var(--accent)
}
header{
    position:sticky;
    top:0;
    z-index:3;
    background:var(--bg);
    padding:16px 24px;
    border-bottom:1px solid var(--line);
    display:flex;
    gap:12px;
    align-items:center;
    flex-wrap:wrap
}
h1{
    font-size:24px;
    margin:0;
    margin-right:auto
}
h2{
    font-size:20px
}
h3{
    font-size:17px
}
.muted,small{
    color:var(--muted)
}
main{
    max-width:1500px;
    margin:auto;
    padding:20px 24px 100px
}
.panel{
    background:var(--card);
    border:1px solid var(--line);
    border-radius:12px;
    padding:16px;
    margin-bottom:20px
}
.toolbar,.row{
    display:flex;
    gap:8px;
    flex-wrap:wrap;
    align-items:center
}
.toolbar{
    position:sticky;
    top:81px;
    background:var(--card);
    z-index:2;
    padding:10px 0
}
#crumbs{
    overflow-wrap:anywhere;
    display:flex;
    gap:6px;
    flex-wrap:wrap
}
#files{
    height:55vh;
    min-height:260px;
    overflow:auto;
    border:1px solid var(--line);
    border-radius:8px;
    margin-top:12px;
    overscroll-behavior:contain
}
table{
    border-collapse:collapse;
    width:100%;
    font-size:15px
}
th{
    position:sticky;
    top:0;
    background:#253248;
    text-align:left;
    padding:10px;
    z-index:1
}
td{
    padding:8px 10px;
    border-bottom:1px solid var(--line);
    overflow-wrap:anywhere
}
tr.selected{
    background:#28404b
}
td button.name{
    border:0;
    background:none;
    text-align:left;
    min-width:120px;
    width:100%;
    padding:4px
}
td:first-child{
    width:46px
}
input[type=checkbox]{
    width:21px;
    height:21px;
    min-height:0;
    accent-color:var(--accent)
}
.actionbar{
    padding-top:10px;
    display:flex;
    gap:8px;
    flex-wrap:wrap
}
details{
    margin:12px 0
}
summary{
    cursor:pointer;
    padding:10px;
    overflow-wrap:anywhere
}
.queueitem{
    padding:10px;
    border-bottom:1px solid var(--line);
    display:grid;
    grid-template-columns:1fr auto;
    gap:8px
}
.queueitem div{
    overflow-wrap:anywhere
}
progress{
    width:100%;
    accent-color:var(--accent);
    height:14px
}
dialog{
    max-width:650px;
    width:calc(100% - 24px);
    max-height:85dvh;
    overflow:auto;
    background:var(--card);
    color:var(--text);
    border:1px solid var(--line);
    border-radius:12px;
    padding:24px
}
dialog::backdrop{
    background:#000b
}
dialog input[type=text],dialog input[type=password]{
    width:100%;
    margin:10px 0
}
dialog .buttons{
    display:flex;
    gap:8px;
    justify-content:flex-end;
    flex-wrap:wrap;
    margin-top:22px
}
#toasts{
    position:fixed;
    right:16px;
    bottom:20px;
    z-index:12;
    max-width:420px;
    width:calc(100% - 32px)
}
.toast{
    padding:15px;
    margin-top:8px;
    border:1px solid var(--accent);
    background:#203647;
    border-radius:10px;
    overflow-wrap:anywhere
}
#logbody{
    max-height:65dvh;
    overflow:auto;
    white-space:pre-wrap;
    overflow-wrap:anywhere;
    font:14px ui-monospace,monospace
}
#context{
    position:fixed;
    z-index:10;
    background:var(--card);
    padding:8px;
    border:1px solid var(--accent);
    border-radius:9px;
    max-height:70vh;
    overflow:auto
}
#context button{
    display:block;
    width:100%;
    margin:4px 0
}
#search{
    flex:1;
    min-width:150px
}
#jobpanel{
    display:none
}
#queueempty{
    padding:12px
}
#connection{
    font-size:13px
}
#offline{
    display:none;
    background:#55321c;
    padding:12px;
    border-radius:8px;
    margin-bottom:12px
}
.touchhint{
    font-size:13px;
    margin-top:8px
}
#moremenu{
    display:flex;
    gap:6px;
    flex-wrap:wrap
}
@media(max-width:700px){
    #toasts{
        bottom:132px
    }
    header{
        padding:12px;
        gap:6px
    }
    h1{
        font-size:22px
    }
    header button{
        padding:8px
    }
    main{
        padding:12px 10px 110px
    }
    .panel{
        padding:10px
    }
    .toolbar{
        position:static
    }
    #files{
        height:auto;
        min-height:200px;
        max-height:none;
        overflow:visible;
        border:0
    }
    th{
        position:static
    }
    .desktopcol{
        display:none
    }
    td{
        padding:6px
    }
    td button.name{
        min-width:0
    }
    #actions{
        position:fixed;
        bottom:0;
        left:0;
        right:0;
        z-index:6;
        background:#172333;
        border-top:1px solid var(--line);
        padding:8px;
        overflow-x:auto;
        flex-wrap:nowrap;
        padding-bottom:max(8px,env(safe-area-inset-bottom))
    }
    #actions{
        display:grid;
        grid-template-columns:1fr auto
    }
    #quickactions{
        grid-column:1/3;
        grid-row:2;
        flex-wrap:nowrap;
        overflow:auto
    }
    #selection{
        grid-column:1;
        grid-row:1
    }
    #more{
        grid-column:2;
        grid-row:1
    }
    #actions button{
        white-space:nowrap
    }
    #toasts{
        bottom:86px
    }
    .queueitem{
        grid-template-columns:1fr
    }
    .toolbar button{
        flex:1
    }
    #moremenu{
        gap:5px
    }
    #moremenu button{
        font-size:14px;
        padding:7px
    }
    .sizecol{
        width:65px
    }
    dialog{
        padding:18px
    }
}
</style></head>
<body>
<header>
<h1>FolderWifi</h1>
<span id="connection" class="muted">Conectando…</span>
<button id="logs">Registro</button>
<button id="network">Conexión</button>
<button id="theme">Tema</button>
<button id="pair">Vincular</button>
</header>
<main>
<div id="offline" role="status">
</div>
<section class="panel" aria-label="Administrador de archivos">
<nav id="crumbs" aria-label="Ruta">
</nav>
<div class="toolbar">
<button id="up">↑ Subir nivel</button>
<button id="refresh">Actualizar</button>
<input id="search" type="search" placeholder="Buscar en esta carpeta" aria-label="Buscar">
<select id="sort" aria-label="Orden">
<option value="name">Nombre ↑</option>
<option value="-name">Nombre ↓</option>
<option value="size">Tamaño ↑</option>
<option value="-size">Tamaño ↓</option>
<option value="modified">Fecha ↑</option>
<option value="-modified">Fecha ↓</option>
</select>
</div>
<div class="row">
<button id="addfiles" class="primary">Añadir archivos</button>
<button id="addfolder">Añadir carpeta</button>
<button id="mkdir">Nueva carpeta</button>
<button id="searchtree">Buscar en subcarpetas</button>
<button id="trash">Papelera</button>
</div>
<div id="files" tabindex="0">
<table>
<thead>
<tr>
<th>
<input id="selectall" type="checkbox" aria-label="Seleccionar todos">
</th>
<th>Nombre</th>
<th class="sizecol">Tamaño</th>
<th class="desktopcol">Modificado</th>
</tr>
</thead>
<tbody id="filebody">
</tbody>
</table>
</div>
<div class="actionbar" id="actions">
<span id="selection">0 seleccionados</span>
<div id="quickactions" class="row">
<button data-action="download">Descargar</button>
<button data-action="copy">Copiar</button>
<button data-action="cut">Mover</button>
<button data-action="paste">Pegar</button>
</div>
<button id="more">Más acciones</button>
</div>
<div id="moremenu" hidden>
<button data-action="rename">Renombrar</button>
<button data-action="duplicate">Duplicar</button>
<button data-action="zip">Crear ZIP</button>
<button data-action="extract">Extraer ZIP</button>
<button data-action="properties">Propiedades</button>
<button data-action="trash">Enviar a papelera</button>
<button data-action="delete" class="danger">Eliminar definitivamente</button>
</div>
<div class="row">
<button id="previouspage">Anterior</button>
<span id="pageinfo">
</span>
<button id="nextpage">Siguiente</button>
</div>
<p id="space" class="muted">
</p>
<p class="touchhint muted">Toca una carpeta para abrirla. Selecciona elementos con sus casillas. En PC: Ctrl+A, Ctrl+C, Ctrl+X, Ctrl+V, F2 y Supr.</p>
</section>
<section class="panel" id="jobpanel" aria-live="polite">
<h2 id="jobtitle">Operación</h2>
<p id="jobstatus">
</p>
<progress id="jobprogress" max="100" value="0">
</progress>
<button id="canceljob">Cancelar operación</button>
</section>
<section class="panel" aria-label="Cola de subida">
<h2>Subidas</h2>
<p class="muted">Cada lote conserva las carpetas de destino elegidas. Los nuevos archivos se preparan aparte mientras el lote activo continúa.</p>
<div class="row">
<button id="submitdraft" class="primary">Confirmar lote y subir</button>
<button id="pauseuploads">Pausar</button>
<button id="retryuploads">Reintentar pendientes</button>
<button id="clearfinished">Limpiar completados</button>
<button id="transferqueue">Trasladar a otra IP</button>
<button id="exportqueue">Guardar pendientes</button>
<button id="importqueue">Recuperar pendientes</button>
</div>
<p id="persistence" class="muted">
</p>
<div id="draft">
</div>
<div id="batches">
</div>
<p id="queueempty">No hay archivos pendientes.</p>
</section>
</main>
<input id="fileinput" type="file" multiple hidden>
<input id="folderinput" type="file" multiple webkitdirectory directory hidden>
<input id="manifestinput" type="file" accept="application/json" hidden>
<div id="toasts" aria-live="polite">
</div>
<div id="context" hidden>
</div>
<script>
'use strict';
const $=id=>document.getElementById(id), sleep=ms=>new Promise(r=>setTimeout(r,ms));
let filePage=0;
let key=localStorage.getItem('folderwifi-key')||'',current=new URL(location.href).searchParams.get('path')||'sdmc:/',entries=[],selected=new Set(),clipboard=null,anchor=-1,pickerDestination=current,activeJob=null,jobBusy=false,uploadBusy=false,paused=false,draft=[],batches=[],db=null,persistenceWarned=false,connected=false,activeBatch=null,lastNotice=null,storedFiles=new Set(),canWrite=true,trackingMoved=false,queueDetails=new Map();
const fragment=new URLSearchParams(location.hash.slice(1));
if(fragment.get('key')){
    key=fragment.get('key');
    localStorage.setItem('folderwifi-key',key);
    history.replaceState(null,'',location.pathname+location.search);
}
function el(tag,text,cls){
    const n=document.createElement(tag);
    if(text!==undefined)n.textContent=text;
    if(cls)n.className=cls;
    return n;
}
function button(text,fn,cls){
    const b=el('button',text,cls);
    b.type='button';
    b.addEventListener('click',()=>run(fn));
    return b;
}
function toast(message,type='info'){
    while($('toasts').children.length>=3)$('toasts').firstChild.remove();
    const n=el('div',message,'toast');
    n.append(button('Cerrar',()=>n.remove()));
    $('toasts').append(n);
    if(type!=='error')setTimeout(()=>n.remove(),5000);
}
function setConnection(ok,message){
    connected=ok;
    $('connection').textContent=ok?'Switch conectada':'Esperando conexión';
    $('offline').style.display=ok?'none':'block';
    $('offline').textContent=message||'Conexión interrumpida. Las subidas quedan en espera y se retoman al recuperar la conexión.';
}
async function run(fn){
    try{
        await fn();
    }catch(e){
        toast(e.message||String(e),'error');
    }
}
function form(values){
    const p=new URLSearchParams();
    for(const [k,v] of Object.entries(values))for(const item of Array.isArray(v)?v:[v])p.append(k,String(item));
    return p;
}
async function api(url,values,options={
}){
    const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),options.timeout||20000);
    try{
        const r=await fetch(url,{
            method:values===undefined?'GET':'POST',headers:{
                'X-FolderWifi-Key':key,...(options.raw?{
                }
                :values===undefined?{
                }
                :{
                    'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'
                })
            },body:options.raw?values:values===undefined?undefined:form(values),signal:controller.signal,cache:'no-store'
        });
        let data;
        try{
            data=await r.json();
        }catch{
            throw new Error('La Switch devolvió una respuesta incompleta.');
        }
        if(!r.ok){
            const e=new Error(data.error||'La operación necesita una decisión.');
            e.code=r.status;
            e.data=data;
            throw e;
        }
        setConnection(true);
        return data;
    }catch(e){
        if(!e.code)setConnection(false);
        throw e;
    }
    finally{
        clearTimeout(timer);
    }
}
function modal(title,message,choices,input,check=false){
    return new Promise(resolve=>{
        const d=el('dialog'),heading=el('h2',title),text=el('p',message);d.append(heading,text);let field=null,c=null;if(input!==undefined){
            field=el('input');field.type='text';field.value=input;field.autocomplete='off';field.setAttribute('aria-label',title);d.append(field);
        }
        if(check){
            const label=el('label',' Aplicar esta decisión a los siguientes conflictos');c=el('input');c.type='checkbox';label.prepend(c);d.append(label);
        }
        const row=el('div',undefined,'buttons');let resolved=false;function finish(value){
            if(resolved)return;resolved=true;resolve({
                choice:value,value:field?field.value:'',all:c?c.checked:false
            });d.close();d.remove();
        }
        for(const [label,value] of choices)row.append(button(label,()=>finish(value),value==='cancel'?'':'primary'));d.append(row);d.addEventListener('cancel',e=>{
            e.preventDefault();finish('cancel');
        });if(field)field.addEventListener('keydown',e=>{
            if(e.key==='Enter'){
                e.preventDefault();finish(choices[0][1]);
            }
        });document.body.append(d);d.showModal();(field||row.querySelector('button')).focus();
    });
}
async function confirmAction(title,message){
    return (await modal(title,message,[['Continuar','yes'],['Cancelar','cancel']])).choice==='yes';
}
async function ask(title,message,initial=''){
    const r=await modal(title,message,[['Aceptar','yes'],['Cancelar','cancel']],initial);
    return r.choice==='yes'?r.value.trim():null;
}
async function decide(path){
    return modal('El destino ya existe',path+' — Elige qué hacer. Al reemplazar una carpeta se combinan sus contenidos y se resuelve cada coincidencia. No se borran los demás archivos.',[['Reemplazar / combinar','replace'],['Conservar ambos','keep'],['Omitir','skip'],['Cancelar','cancel']],undefined,true);
}
async function pair(){
    const code=await ask('Vincular con la Switch','Escribe el código que aparece en la pantalla de FolderWifi.');
    if(code===null)return;
    const r=await api('/api/pair',{
        code:code.toLowerCase()
    });
    key=r.key;
    localStorage.setItem('folderwifi-key',key);
    await refresh();
}
function parent(p){
    const cut=p.lastIndexOf('/');
    return cut<=5?'sdmc:/':p.slice(0,cut);
}
function join(p,n){
    return p+(p.endsWith('/')?'':'/')+n;
}
function size(n){
    if(n<1024)return n+' B';
    const u=['KiB','MiB','GiB','TiB'];
    let i=-1;
    do{
        n/=1024;
        ++i;
    }
    while(n>=1024&&i<3);
    return n.toFixed(1)+' '+u[i];
}
function visible(){
    const q=$('search').value.toLocaleLowerCase(),sort=$('sort').value,k=sort.replace('-',''),dir=sort.startsWith('-')?-1:1;
    return entries.filter(e=>e.name.toLocaleLowerCase().includes(q)).sort((a,b)=>a.directory!==b.directory?a.directory?-1:1:dir*(k==='name'?a.name.localeCompare(b.name,undefined,{
        numeric:true,sensitivity:'base'
    }):a[k]-b[k]));
}
function render(){
    const list=visible(),body=$('filebody');
    filePage=Math.min(filePage,Math.max(0,Math.ceil(list.length/150)-1));
    $('pageinfo').textContent='Página '+(filePage+1)+' / '+Math.max(1,Math.ceil(list.length/150));
    $('previouspage').disabled=filePage===0;
    $('nextpage').disabled=(filePage+1)*150>=list.length;
    body.replaceChildren();
    list.slice(filePage*150,(filePage+1)*150).forEach((e,localIndex)=>{
        const index=filePage*150+localIndex;const tr=el('tr');if(selected.has(e.path))tr.className='selected';const td=el('td'),cb=el('input');cb.type='checkbox';cb.checked=selected.has(e.path);cb.setAttribute('aria-label','Seleccionar '+e.name);cb.addEventListener('click',event=>{
            if(event.shiftKey&&anchor>=0){
                for(let i=Math.min(anchor,index);i<=Math.max(anchor,index);++i)cb.checked?selected.add(list[i].path):selected.delete(list[i].path);
            }else cb.checked?selected.add(e.path):selected.delete(e.path);anchor=index;render();
        });td.append(cb);tr.append(td);const nameCell=el('td'),b=button((e.directory?'📁 ':'📄 ')+e.name,()=>e.directory?navigate(e.path):download(e.path),'name');nameCell.append(b);tr.append(nameCell,el('td',e.directory?'—':size(e.size),'sizecol'),el('td',new Date(e.modified*1000).toLocaleString(),'desktopcol'));tr.addEventListener('contextmenu',ev=>{
            ev.preventDefault();if(!selected.has(e.path)){
                selected=new Set([e.path]);render();
            }
            contextMenu(ev.clientX,ev.clientY);
        });body.append(tr);
    });
    if(!list.length){
        const td=el('td',entries.length?'Sin coincidencias.':'Esta carpeta está vacía.');
        td.colSpan=4;
        body.append(el('tr'));
        body.lastChild.append(td);
    }
    $('selection').textContent=selected.size+' seleccionados';
    $('selectall').checked=list.length>0&&list.every(e=>selected.has(e.path));
    $('selectall').indeterminate=list.some(e=>selected.has(e.path))&&!$('selectall').checked;
    for(const b of document.querySelectorAll('[data-action]'))b.disabled=b.dataset.action==='paste'?!clipboard:!selected.size;
}
function crumbs(){
    const root=button('SDMC',()=>navigate('sdmc:/'));
    $('crumbs').replaceChildren(root);
    let path='sdmc:/';
    for(const part of current.slice(6).split('/').filter(Boolean)){
        path=join(path,part);
        const dest=path;
        $('crumbs').append(el('span','›'),button(part,()=>navigate(dest)));
    }
    $('up').disabled=current==='sdmc:/';
}
async function refresh(){
    const data=await api('/api/list?'+form({
        path:current
    }));
    current=data.path;
    entries=data.entries;
    selected=new Set([...selected].filter(p=>entries.some(e=>e.path===p)));
    $('space').textContent=entries.length+' elementos · '+size(data.free)+' disponibles en la SD';
    crumbs();
    render();
}
async function navigate(p,push=true){
    const previous=current;
    current=p;
    filePage=0;
    selected.clear();
    $('search').value='';
    try{
        await refresh();
        $('files').scrollTop=0;
        if(push)history.pushState(null,'','?'+form({
            path:current
        }));
    }catch(e){
        current=previous;
        throw e;
    }
}
async function download(p){
    const r=await api('/api/download/ticket',{
        path:p
    });
    const a=el('a');
    a.href=r.url;
    a.download=p.split('/').pop();
    document.body.append(a);
    a.click();
    a.remove();
}
function contextMenu(x,y){
    const menu=$('context');
    menu.replaceChildren();
    for(const [name,action] of [['Descargar','download'],['Copiar','copy'],['Mover','cut'],['Renombrar','rename'],['Duplicar','duplicate'],['Propiedades','properties'],['Enviar a papelera','trash']])menu.append(button(name,()=>{
        menu.hidden=true;return fileAction(action);
    }));
    menu.hidden=false;
    menu.style.left=Math.min(x,innerWidth-210)+'px';
    menu.style.top=Math.max(0,Math.min(y,innerHeight-menu.offsetHeight-10))+'px';
}
async function execute(action,sources,destination=current,policy='ask',extra={
}){
    if(jobBusy){
        toast('Hay otra operación en seguimiento. Espera a que termine.');
        return null;
    }
    jobBusy=true;
    let result=null;
    try{
        const decisions={
        };
        if(policy==='ask'&&['copy','move','rename','zip'].includes(action)){
            const plan=await api('/api/preflight',{
                action,source:sources,destination
            });
            for(const path of plan.conflicts||[]){
                const d=await decide(path);
                if(d.choice==='cancel')return null;
                if(d.all){
                    policy=d.choice;
                    break;
                }
                decisions['decision:'+path]=d.choice;
            }
            if(plan.truncated&&policy==='ask')toast('Los conflictos adicionales se resolverán al ejecutar.');
        }
        const requestId=id();
        localStorage.setItem('folderwifi-job',requestId);
        let r;
        for(;;){
            try{
                r=await api('/api/operation',{
                    id:requestId,action,source:sources,destination,policy,...decisions,...extra
                });
                break;
            }catch(e){
                if(e.code){
                    localStorage.removeItem('folderwifi-job');
                    throw e;
                }
                await sleep(1500);
            }
        }
        activeJob=r.id;
        result=await watchJob(r.id,action);
        return result;
    }
    finally{
        jobBusy=false;
    }
}
async function watchJob(id,label='Operación pendiente'){
    activeJob=id;
    $('canceljob').disabled=false;
    $('jobpanel').style.display='block';
    const titles={
        copy:'Copiar',move:'Mover',rename:'Renombrar',trash:'Enviar a papelera',delete:'Eliminar',zip:'Crear ZIP',extract:'Extraer ZIP',properties:'Propiedades',search:'Buscar',restore:'Restaurar',purge:'Vaciar elemento de papelera'
    };
    $('jobtitle').textContent=titles[label]||label;
    let prompting=false;
    for(;;){
        if(trackingMoved){
            activeJob=null;
            $('canceljob').disabled=true;
            $('jobstatus').textContent='Seguimiento trasladado a la nueva dirección.';
            return null;
        }
        let j;
        try{
            j=await api('/api/job?'+form({
                id
            }));
        }catch(e){
            if(e.code===404){
                localStorage.removeItem('folderwifi-job');
                activeJob=null;
                $('canceljob').disabled=true;
                $('jobstatus').textContent='La operación ya no está disponible. Comprueba los archivos antes de repetirla.';
                return null;
            }
            await sleep(1500);
            continue;
        }
        $('jobstatus').textContent=statusText(j.status)+' · '+size(j.bytes)+' / '+size(j.total)+' · '+j.files+' archivos · '+(j.skipped||0)+' omitidos · '+(j.replaced||0)+' reemplazados';
        $('jobprogress').value=j.status==='done'?100:j.total?Math.min(100,j.bytes/j.total*100):0;
        if(j.status==='conflict'&&!prompting){
            prompting=true;
            const d=await decide(j.conflict);
            await api('/api/job/decision',{
                id,choice:d.choice,all:d.all?1:0
            });
            prompting=false;
        }
        if(['done','error','cancelled'].includes(j.status)){
            localStorage.removeItem('folderwifi-job');
            activeJob=null;
            $('canceljob').disabled=true;
            $('jobstatus').textContent=j.status==='done'?(titles[label]||label)+' completado.':j.error||'Cancelado. Se conservan los elementos ya completados.';
            if(j.status!=='done')toast($('jobstatus').textContent);
            await refresh();
            if(label==='search'&&j.results){
                const d=el('dialog');
                d.append(el('h2','Resultados de búsqueda'));
                for(const e of j.results)d.append(button(e.path,async()=>{
                    d.close();d.remove();await navigate(e.directory?e.path:parent(e.path));if(!e.directory){
                        selected=new Set([e.path]);render();
                    }
                }));
                if(!j.results.length)d.append(el('p','Sin coincidencias.'));
                d.append(button('Cerrar',()=>{
                    d.close();d.remove();
                }));
                document.body.append(d);
                d.showModal();
            }
            if(label==='properties')await modal('Propiedades',j.files+' archivos · '+(j.dirs||0)+' carpetas · '+size(j.total),[['Cerrar','cancel']]);
            return j;
        }
        await sleep(500);
    }
}
async function fileAction(action){
    const sources=[...selected];
    if(action==='paste'){
        if(!clipboard)return;
        const clip=clipboard;
        const result=await execute(clip.action,clip.sources,current);
        if(result&&clip.action==='move'){
            clip.sources=result.remaining||clip.sources;
            if(!clip.sources.length)clipboard=null;
        }
        render();
        return;
    }
    if(!sources.length){
        toast('Selecciona al menos un elemento.');
        return;
    }
    if(action==='copy'||action==='cut'){
        clipboard={
            action:action==='copy'?'copy':'move',sources
        };
        toast(sources.length+' elementos preparados. Abre su carpeta de destino y pulsa Pegar.');
        render();
        return;
    }
    if(action==='rename'){
        if(sources.length!==1){
            toast('Para renombrar, selecciona un solo elemento.');
            return;
        }
        const name=await ask('Renombrar','Escribe el nuevo nombre.',sources[0].split('/').pop());
        if(name!==null)await execute('rename',sources,join(parent(sources[0]),name));
        return;
    }
    if(action==='download'){
        if(sources.length===1&&!entries.find(e=>e.path===sources[0])?.directory)await download(sources[0]);
        else{
            const name=await ask('Descargar selección como ZIP','Se creará un ZIP en la carpeta actual.','FolderWifi_Selection.zip');
            if(name===null)return;
            const dest=join(current,name),result=await execute('zip',sources,dest);
            if(result?.status==='done')await download(dest);
        }
        return;
    }
    if(action==='zip'){
        const name=await ask('Crear ZIP','Nombre del archivo ZIP.','FolderWifi_Selection.zip');
        if(name!==null)await execute('zip',sources,join(current,name));
        return;
    }
    if(action==='extract'){
        if(sources.some(p=>!p.toLowerCase().endsWith('.zip'))){
            toast('Selecciona solamente archivos ZIP.');
            return;
        }
        const dest=await ask('Extraer ZIP','Ruta de destino en la SD.',current);
        if(dest!==null)await execute('extract',sources,dest);
        return;
    }
    if(action==='duplicate'){
        await execute('copy',sources,current,'keep');
        return;
    }
    if(action==='trash'||action==='delete'){
        if(await confirmAction(action==='trash'?'Enviar a papelera':'Eliminar definitivamente',sources.length+' elementos seleccionados. '+(action==='trash'?'Podrás restaurarlos desde Papelera.':'Esta eliminación no se puede deshacer.')))await execute(action,sources);
        return;
    }
    await execute(action,sources);
}
async function trashView(){
    const items=await api('/api/trash'),d=el('dialog');
    d.append(el('h2','Papelera'),el('p','Los elementos ocupan espacio en la SD hasta eliminarlos definitivamente.'));
    if(!items.length)d.append(el('p','Papelera vacía.'));
    for(const item of items){
        const row=el('div',undefined,'queueitem');
        row.append(el('span',item.path),button('Restaurar',async()=>{
            d.close();d.remove();await execute('restore',[item.id],current,'keep');
        }),button('Eliminar definitivamente',async()=>{
            if(await confirmAction('Eliminar definitivamente',item.path)){
                d.close();d.remove();await execute('purge',[item.id]);
            }
        },'danger'));
        d.append(row);
    }
    d.append(button('Cerrar',()=>{
        d.close();d.remove();
    }));
    d.addEventListener('close',()=>d.remove(),{
        once:true
    });
    document.body.append(d);
    d.showModal();
}
const crcTable=Array.from({
    length:256
},(_,n)=>{
    for(let k=0;k<8;k++)n=n&1?0xedb88320^(n>>>1):n>>>1;return n>>>0;
});
function crc32(bytes,previous=0){
    let c=(previous^0xffffffff)>>>0;
    for(const b of bytes)c=crcTable[(c^b)&255]^(c>>>8);
    return (c^0xffffffff)>>>0;
}
async function checksumFile(file){
    let crc=0;
    for(let offset=0;offset<file.size;offset+=2*1024*1024){
        if(paused)throw Object.assign(new Error('En pausa'),{
            paused:true
        });
        if(activeBatch?.cancel)throw Object.assign(new Error('Cancelado'),{
            cancelled:true
        });
        crc=crc32(new Uint8Array(await file.slice(offset,offset+2*1024*1024).arrayBuffer()),crc);
        await sleep(0);
    }
    return crc;
}
function id(){
    const bytes=new Uint8Array(16);
    crypto.getRandomValues(bytes);
    return Array.from(bytes,b=>b.toString(16).padStart(2,'0')).join('');
}
async function fingerprint(file){
    const head=new Uint8Array(await file.slice(0,65536).arrayBuffer()),tail=new Uint8Array(await file.slice(Math.max(0,file.size-65536)).arrayBuffer());
    return file.size+'-'+crc32(head)+'-'+crc32(tail);
}
async function initDb(){
    if(!window.indexedDB){
        $('persistence').textContent='Este navegador no guarda archivos pendientes entre sesiones.';
        return;
    }
    try{
        db=await new Promise((resolve,reject)=>{
            const q=indexedDB.open('FolderWifi',2);q.onupgradeneeded=()=>{
                if(!q.result.objectStoreNames.contains('state'))q.result.createObjectStore('state');if(!q.result.objectStoreNames.contains('files'))q.result.createObjectStore('files');
            };q.onsuccess=()=>resolve(q.result);q.onerror=()=>reject(q.error);
        });
        function read(store,key){
            return new Promise((resolve,reject)=>{
                const q=db.transaction(store).objectStore(store).get(key);q.onsuccess=()=>resolve(q.result);q.onerror=()=>reject(q.error);
            });
        }
        const state=await read('state','queue');
        const keys=await new Promise((resolve,reject)=>{
            const q=db.transaction('files').objectStore('files').getAllKeys();q.onsuccess=()=>resolve(q.result);q.onerror=()=>reject(q.error);
        });
        storedFiles=new Set(keys);
        if(state){
            draft=state.draft||[];
            batches=state.batches||[];
            paused=state.paused||false;
            for(const item of [...draft,...batches.flatMap(b=>b.items)])if(item.kind!=='directory'&&!['done','skipped','cancelled'].includes(item.status)){
                item.file=item.file||await read('files',item.id)||null;
                if(!item.file)item.status='needs-file';
            }
            for(const batch of batches)if(batch.state==='active')batch.state='waiting';
        }
        $('persistence').textContent='Los archivos pendientes se conservan en este navegador mientras haya espacio y no borres sus datos.';
        await saveQueue();
        renderQueue();
    }catch(e){
        db=null;
        toast('No se pudo recuperar la cola guardada: '+e.message,'error');
    }
}
let saving=Promise.resolve();
function saveQueue(){
    const files=[...draft,...batches.flatMap(b=>b.items)].filter(item=>item.file).map(item=>({
        id:item.id,file:item.file
    }));
    const snapshot={
        draft:draft.map(({
            file,...i
        })=>({
            ...i,file:null
        })),batches:batches.map(b=>({
            ...b,groups:b.groups?Object.fromEntries(Object.entries(b.groups).map(([k,g])=>[k,{
                ...g
            }])):undefined,items:b.items.map(({
                file,...i
            })=>({
                ...i,file:null
            }))
        })),paused
    };
    saving=saving.catch(()=>{
    }).then(async()=>{
        if(!db)return;try{
            const wanted=new Set(files.map(item=>item.id));await new Promise((resolve,reject)=>{
                const tx=db.transaction(['state','files'],'readwrite'),store=tx.objectStore('files');for(const item of files)if(!storedFiles.has(item.id))store.put(item.file,item.id);for(const existing of storedFiles)if(!wanted.has(existing))store.delete(existing);tx.objectStore('state').put(snapshot,'queue');tx.oncomplete=resolve;tx.onerror=()=>reject(tx.error);tx.onabort=()=>reject(tx.error);
            });storedFiles=wanted;
        }catch(e){
            $('persistence').textContent='No se pudo guardar toda la cola. Mantén esta pestaña abierta o guarda los pendientes y vuelve a seleccionar sus archivos.';if(!persistenceWarned){
                toast('El almacenamiento del navegador está lleno o bloqueado. La cola actual sigue en esta pestaña.','error');persistenceWarned=true;
            }
        }
    });
    return saving;
}
async function addFiles(files,destination,folder=false){
    for(const file of files){
        const relative=file.webkitRelativePath||file.name,fp=await fingerprint(file);
        let restored=false;
        for(const item of [...draft,...batches.flatMap(b=>b.items)])if(item.kind!=='directory'&&!['done','skipped','cancelled'].includes(item.status)&&!item.file&&(item.originalRelative||item.relative)===relative&&item.fingerprint===fp){
            item.file=file;
            if(item.status==='needs-file')item.status='waiting';
            restored=true;
        }
        if(restored)continue;
        draft.push({
            id:id(),file,relative,destination,folder:folder&&relative.includes('/'),group:folder&&relative.includes('/')?relative.split('/')[0]:'',size:file.size,fingerprint:fp,status:'waiting',offset:0
        });
    }
    await saveQueue();
    renderQueue();
    pump();
}
function statusText(status){
    return ({
        queued:'En espera',waiting:'En espera',active:'Subiendo',uploading:'Subiendo',checking:'Comprobando integridad',done:'Completado',skipped:'Omitido',error:'Requiere atención',cancelled:'Cancelado',cancelling:'Cancelando',running:'En curso',conflict:'Decisión pendiente','needs-file':'Vuelve a seleccionar el archivo'
    })[status]||status;
}
function queueRow(item,remove){
    const row=el('div',undefined,'queueitem'),info=el('div');
    info.append(el('strong',(item.folder?'Carpeta '+item.group+' · ':'')+item.relative),el('div','Destino: '+join(item.destination,item.relative),'muted'),el('small',statusText(item.status)+' · '+size(item.offset||0)+' / '+size(item.size)));
    const p=el('progress');
    p.max=item.size||1;
    p.value=['done','skipped'].includes(item.status)?p.max:item.offset||0;
    info.append(p);
    row.append(info);
    if(remove)row.append(button('Quitar',remove));
    return row;
}
function renderQueue(){
    const d=$('draft'),b=$('batches');
    d.replaceChildren();
    b.replaceChildren();
    if(draft.length){
        const details=el('details');
        details.open=queueDetails.get('draft')!==false;
        details.addEventListener('toggle',()=>{
            if(details.isConnected)queueDetails.set('draft',details.open);
        });
        details.append(el('summary','Preparando nuevo lote · '+draft.length+' archivos'));
        draft.forEach(item=>details.append(queueRow(item,async()=>{
            draft=draft.filter(i=>i.id!==item.id);await saveQueue();renderQueue();
        })));
        d.append(details);
    }
    for(const batch of batches){
        const details=el('details');
        details.open=queueDetails.has(batch.id)?queueDetails.get(batch.id):batch.state==='active'||batch.state==='error';
        details.addEventListener('toggle',()=>{
            if(details.isConnected)queueDetails.set(batch.id,details.open);
        });
        details.append(el('summary','Lote '+batch.label+' · '+statusText(batch.state)+' · '+batch.items.filter(i=>['done','skipped'].includes(i.status)).length+'/'+batch.items.length+' archivos'));
        if(!['done','cancelled'].includes(batch.state))details.append(button('Cancelar lote',async()=>{
            if(await confirmAction('Cancelar lote','Se conservarán los archivos ya completados. Los temporales pendientes se eliminarán cuando haya conexión.')){
                batch.cancel=true;batch.state='cancelling';await saveQueue();renderQueue();pump();
            }
        }));
        batch.items.forEach(item=>details.append(queueRow(item)));
        b.append(details);
    }
    $('submitdraft').disabled=!draft.length;
    $('pauseuploads').textContent=paused?'Continuar':'Pausar';
    $('queueempty').hidden=draft.length>0||batches.length>0;
}
async function retry(fn){
    let delay=500;
    for(;;){
        if(activeBatch?.cancel)throw Object.assign(new Error('Cancelado'),{
            cancelled:true
        });
        if(paused||!uploadBusy)throw Object.assign(new Error('En pausa'),{
            paused:true
        });
        try{
            return await fn();
        }catch(e){
            if(e.code===403){
                canWrite=false;
                e.paused=true;
                throw e;
            }
            if(e.code&&![423,503,429].includes(e.code))throw e;
            await sleep(delay);
            delay=Math.min(5000,delay*2);
        }
    }
}
async function prepareGroups(batch){
    batch.groups=batch.groups||{
    };
    for(const item of batch.items){
        if(!item.folder||!item.group)continue;
        const groupKey=item.destination+'|'+item.group;
        if(batch.groups[groupKey])continue;
        const groupId=id();
        batch.groups[groupKey]={
            id:groupId,original:item.group,state:'pending'
        };
        await saveQueue();
    }
    for(const [groupKey,group]of Object.entries(batch.groups)){
        if(group.state==='done')continue;
        const items=batch.items.filter(i=>i.destination+'|'+i.group===groupKey);
        if(!items.length)continue;
        let policy='ask',remote;
        for(;;){
            try{
                remote=await retry(()=>api('/api/upload/directory',{
                    id:group.id,destination:items[0].destination,relative:group.original,policy
                }));
                break;
            }catch(e){
                if(e.code!==409||!e.data.conflict)throw e;
                const d=batch.allDecision?{
                    choice:batch.allDecision
                }
                :await decide(e.data.conflict);
                if(d.choice==='cancel'){
                    batch.cancel=true;
                    return;
                }
                policy=d.choice;
                if(d.all)batch.allDecision=policy;
            }
        }
        if(remote.outcome==='skipped'){
            for(const item of items){
                item.status='skipped';
                item.file=null;
            }
        }else{
            const newGroup=remote.destination.split('/').pop();
            if(newGroup!==group.original)for(const item of items){
                item.originalRelative=item.originalRelative||item.relative;
                item.relative=newGroup+item.relative.slice(group.original.length);
            }
        }
        group.state='done';
        await saveQueue();
        renderQueue();
    }
}
async function transfer(item,batch){
    if(item.kind==='directory'){
        await retry(()=>api('/api/upload/directory',{
            id:item.id,destination:item.destination,relative:item.relative
        }));
        item.status='done';
        await saveQueue();
        renderQueue();
        return;
    }
    if(!item.file){
        item.status='needs-file';
        throw new Error('Vuelve a seleccionar '+item.relative+' para recuperar el acceso a su contenido.');
    }
    let policy='ask',u;
    for(;;){
        try{
            u=await retry(()=>api('/api/upload/begin',{
                id:item.id,destination:item.destination,relative:item.relative,size:item.size,fingerprint:item.fingerprint,policy
            }));
            break;
        }catch(e){
            if(e.code!==409||!e.data.conflict)throw e;
            const choice=batch.allDecision?{
                choice:batch.allDecision
            }
            :await decide(e.data.conflict);
            if(choice.choice==='cancel'){
                batch.cancel=true;
                return;
            }
            policy=choice.choice;
            if(choice.all)batch.allDecision=policy;
        }
    }
    item.status='uploading';
    item.offset=u.offset;
    await saveQueue();
    renderQueue();
    while(u.phase!=='done'&&!batch.cancel){
        if(paused)throw Object.assign(new Error('En pausa'),{
            paused:true
        });
        // Consultar antes de cada bloque permite reconciliar una respuesta perdida.
        u=await retry(()=>api('/api/upload/status?'+form({
            id:item.id
        })));
        item.offset=u.offset;
        renderQueue();
        if(u.phase==='done')break;
        if(u.offset>item.size)throw new Error('La transferencia remota tiene un tamaño inesperado.');
        if(u.offset<item.size){
            const bytes=new Uint8Array(await item.file.slice(u.offset,u.offset+1024*1024).arrayBuffer());
            try{
                u=await retry(()=>api('/api/upload/chunk?'+form({
                    id:item.id,offset:u.offset,crc:crc32(bytes)
                }),bytes,{
                    raw:true
                }));
                item.offset=u.offset;
                await saveQueue();
                renderQueue();
            }catch(e){
                if(e.code===409&&e.data.offset!==undefined){
                    item.offset=e.data.offset;
                    continue;
                }
                throw e;
            }
        } else{
            item.status='checking';
            renderQueue();
            if(item.checksum===undefined)item.checksum=await checksumFile(item.file);
            let finishPolicy='ask';
            for(;;){
                try{
                    u=await retry(()=>api('/api/upload/finish',{
                        id:item.id,policy:finishPolicy,crc:item.checksum
                    },{
                        timeout:120000
                    }));
                    break;
                }catch(e){
                    if(e.code!==409||!e.data.conflict)throw e;
                    const choice=batch.allDecision?{
                        choice:batch.allDecision
                    }
                    :await decide(e.data.conflict);
                    if(choice.choice==='cancel'){
                        batch.cancel=true;
                        return;
                    }
                    finishPolicy=choice.choice;
                    if(choice.all)batch.allDecision=finishPolicy;
                }
            }
        }
    }
    if(!batch.cancel){
        if(u.outcome!=='skipped'&&u.crc!==undefined){
            if(item.checksum===undefined)item.checksum=await checksumFile(item.file);
            if(item.checksum!==u.crc)throw new Error('El archivo original cambió. Crea una nueva subida con su contenido actual.');
        }
        item.status=u.outcome==='skipped'?'skipped':'done';
        item.offset=item.size;
        item.actualDestination=u.destination;
        item.file=null;
        await saveQueue();
        renderQueue();
    }
}
async function pump(){
    if(uploadBusy||paused||!key||(!canWrite&&!batches.some(b=>b.cancel&&b.state!=='cancelled')))return;
    uploadBusy=true;
    try{
        for(;;){
            const batch=batches.find(b=>b.cancel&&b.state!=='cancelled'||['waiting','active','error'].includes(b.state));
            if(!batch||(!canWrite&&!batch.cancel))break;
            activeBatch=batch;
            if(batch.cancel){
                let clean=true;
                for(const item of batch.items.filter(i=>!['done','skipped'].includes(i.status))){
                    try{
                        const remote=await api('/api/upload/status?'+form({
                            id:item.id
                        }));
                        if(remote.phase==='done'){
                            item.status=remote.outcome==='skipped'?'skipped':'done';
                            item.offset=item.size;
                            item.file=null;
                            continue;
                        }
                        await api('/api/upload/abort',{
                            id:item.id
                        });
                    }catch(e){
                        if(e.code!==404){
                            clean=false;
                            break;
                        }
                    }
                    item.status='cancelled';
                    item.file=null;
                }
                if(!clean){
                    batch.state='cancelling';
                    break;
                }
                batch.state='cancelled';
                await saveQueue();
                renderQueue();
                continue;
            }
            batch.state='active';
            renderQueue();
            let failed=false;
            try{
                await prepareGroups(batch);
            }catch(e){
                batch.state=e.paused?'waiting':'error';
                if(!e.paused&&!e.cancelled)toast(e.message);
                failed=true;
            }
            if(batch.cancel)continue;
            if(failed)break;
            for(const item of batch.items){
                if(['done','skipped'].includes(item.status))continue;
                if(batch.cancel)break;
                try{
                    await transfer(item,batch);
                }catch(e){
                    item.status=e.paused?'waiting':item.file||item.kind==='directory'?'error':'needs-file';
                    batch.state=e.paused?'waiting':'error';
                    if(!e.paused)toast(e.message);
                    failed=true;
                    break;
                }
            }
            if(batch.cancel)continue;
            if(failed)break;
            batch.state='done';
            batch.allDecision=null;
            await saveQueue();
            renderQueue();
            try{
                await refresh();
            }catch{
            }
            toast('Lote '+batch.label+' completado.');
        }
    }
    finally{
        uploadBusy=false;
        activeBatch=null;
        await saveQueue();
        renderQueue();
    }
}
async function submit(){
    if(!draft.length)return;
    const batch={
        id:id(),label:new Date().toLocaleTimeString(),items:draft,state:'waiting'
    };
    draft=[];
    batches.push(batch);
    await saveQueue();
    renderQueue();
    pump();
}
async function droppedEntries(items,destination){
    const collected=[],empty=[];
    let seen=0;
    async function walk(entry,prefix=''){
        if(++seen>100000)throw new Error('Demasiados elementos en la carpeta.');
        const relative=prefix+entry.name;
        if(entry.isFile){
            const file=await new Promise((resolve,reject)=>entry.file(resolve,reject));
            Object.defineProperty(file,'webkitRelativePath',{
                value:relative
            });
            collected.push(file);
        }else if(entry.isDirectory){
            const reader=entry.createReader(),children=[];
            for(;;){
                const batch=await new Promise((resolve,reject)=>reader.readEntries(resolve,reject));
                if(!batch.length)break;
                children.push(...batch);
            }
            if(!children.length)empty.push(relative);
            for(const child of children)await walk(child,relative+'/');
        }
    }
    for(const entry of items)await walk(entry);
    await addFiles(collected,destination,true);
    for(const relative of empty)draft.push({
        id:id(),kind:'directory',relative,destination,folder:true,group:relative.split('/')[0],size:0,fingerprint:'directory',status:'waiting',offset:0
    });
    await saveQueue();
    renderQueue();
    if(empty.length)toast(empty.length+' carpetas vacías añadidas.');
}
function localOrigin(url){
    if(url.protocol!=='http:'||url.port!=='8080'&&url.hostname!=='127.0.0.1')return false;
    const numbers=url.hostname.split('.').map(Number);
    if(numbers.length!==4||numbers.some(n=>!Number.isInteger(n)||n<0||n>255))return false;
    return numbers[0]===10||numbers[0]===192&&numbers[1]===168||numbers[0]===172&&numbers[1]>=16&&numbers[1]<=31||numbers[0]===169&&numbers[1]===254||numbers[0]===127;
}
async function transferQueue(){
    const answer=await ask('Trasladar cola a la nueva IP','Introduce la dirección HTTP que muestra tu Switch. La nueva pestaña recibirá los pendientes y sus archivos locales; el borrador seguirá sin confirmar.');
    if(answer===null)return;
    const target=new URL(answer.startsWith('http://')?answer:'http://'+answer);
    if(!target.port)target.port='8080';
    if(!localOrigin(target)||target.username||target.password||target.origin===location.origin)throw new Error('Escribe una dirección local nueva de la Switch, con su puerto 8080.');
    const nonce=id();
    target.pathname='/';
    target.search='';
    target.hash=form({
        bridge:nonce,from:location.origin
    }).toString();
    paused=true;
    const persisted=saveQueue();
    renderQueue();
    const receiver=window.open(target.href,'FolderWifi-transfer');
    if(!receiver)throw new Error('No se pudo abrir la nueva pestaña. Permite su apertura o usa Guardar pendientes.');
    let sent=false;
    const listener=async event=>{
        if(event.source!==receiver||event.origin!==target.origin||event.data?.folderwifiBridge!==nonce)return;
        if(event.data.type==='ready'&&!sent){
            sent=true;
            const pending=batches.filter(b=>!['done','cancelled'].includes(b.state)).map(b=>({
                ...b,state:b.cancel?'cancelling':'waiting'
            }));
            receiver.postMessage({
                folderwifiBridge:nonce,type:'queue',key,state:{
                    draft,batches:pending,paused:false,job:activeJob||localStorage.getItem('folderwifi-job')
                }
            },target.origin);
        }
        if(event.data.type==='received'){
            trackingMoved=true;
            window.removeEventListener('message',listener);
            toast('La cola llegó a la nueva dirección. Esta pestaña queda en pausa.');
        }
    };
    window.addEventListener('message',listener);
    await persisted;
    setTimeout(()=>{
        window.removeEventListener('message',listener);if(!sent)toast('La nueva pestaña no respondió. Puedes guardar los pendientes para recuperarlos.','error');
    },30000);
}
async function receiveBridge(){
    const nonce=fragment.get('bridge'),from=fragment.get('from');
    if(!nonce||!window.opener)return false;
    let origin;
    try{
        origin=new URL(from);
        if(!localOrigin(origin)||origin.origin!==from)throw new Error();
    }catch{
        return false;
    }
    return new Promise(resolve=>{
        let done=false;const listener=async event=>{
            if(done||event.source!==window.opener||event.origin!==from||event.data?.folderwifiBridge!==nonce||event.data.type!=='queue')return;done=true;window.removeEventListener('message',listener);try{
                const state=event.data.state;if(!Array.isArray(state?.draft)||!Array.isArray(state?.batches)||typeof event.data.key!=='string'||event.data.key.length!==64)throw new Error('Datos de traslado inválidos.');key=event.data.key;localStorage.setItem('folderwifi-key',key);await api('/api/status');const known=new Set([...draft,...batches.flatMap(b=>b.items)].map(i=>i.id));for(const item of state.draft)if(!known.has(item.id)){
                    draft.push(item);known.add(item.id);
                }
                for(const batch of state.batches){
                    const items=batch.items.filter(i=>!known.has(i.id));if(items.length){
                        batches.push({
                            ...batch,items,state:batch.cancel?'cancelling':'waiting'
                        });for(const item of items)known.add(item.id);
                    }
                }
                paused=false;if(typeof state.job==='string'&&/^[a-f0-9]{32}$/.test(state.job))localStorage.setItem('folderwifi-job',state.job);await saveQueue();renderQueue();window.opener.postMessage({
                    folderwifiBridge:nonce,type:'received'
                },from);history.replaceState(null,'',location.pathname);toast('Pendientes recuperados desde la otra dirección.');resolve(true);
            }catch(e){
                toast('No se pudo trasladar la cola: '+e.message,'error');resolve(false);
            }
        };window.addEventListener('message',listener);window.opener.postMessage({
            folderwifiBridge:nonce,type:'ready'
        },from);setTimeout(()=>{
            if(!done){
                done=true;window.removeEventListener('message',listener);resolve(false);
            }
        },30000);
    });
}
async function exportQueue(){
    const items=[...draft,...batches.filter(b=>b.state!=='done'&&b.state!=='cancelled').flatMap(b=>b.items.filter(i=>!['done','skipped'].includes(i.status)))].map(({
        file,...item
    })=>item);
    const blob=new Blob([JSON.stringify({
        format:'FolderWifi-queue-v1',items
    },null,2)],{
        type:'application/json'
    }),url=URL.createObjectURL(blob),a=el('a');
    a.href=url;
    a.download='FolderWifi-pendientes.json';
    a.click();
    setTimeout(()=>URL.revokeObjectURL(url),10000);
    toast('El archivo guarda destinos y progreso. Para recuperar el contenido deberás seleccionar nuevamente los archivos originales.');
}
async function importQueue(file){
    const data=JSON.parse(await file.text());
    if(data.format!=='FolderWifi-queue-v1'||!Array.isArray(data.items)||data.items.length>10000)throw new Error('Archivo de pendientes inválido.');
    for(const i of data.items){
        if(typeof i.id!=='string'||!i.id.match(/^[a-f0-9]{32}$/)||typeof i.relative!=='string'||typeof i.destination!=='string'||!i.destination.startsWith('sdmc:/')||!Number.isSafeInteger(i.size)||i.size<0)throw new Error('Datos de cola inválidos.');
        if([...draft,...batches.flatMap(b=>b.items)].some(item=>item.id===i.id))continue;
        draft.push({
            ...i,file:null,status:'needs-file'
        });
    }
    await saveQueue();
    renderQueue();
    toast('Pendientes recuperados. Selecciona otra vez los archivos o carpetas originales para asociar su contenido.');
}
async function logsView(){
    const d=el('dialog');
    d.append(el('h2','Registro de actividad'));
    const body=el('div');
    body.id='logbody';
    d.append(body,button('Cerrar',()=>d.close()));
    let open=true;
    async function update(){
        try{
            const r=await api('/api/status');
            body.textContent=r.logs.join('\n');
            body.scrollTop=body.scrollHeight;
        }catch{
        }
        if(open)setTimeout(update,1500);
    }
    d.addEventListener('close',()=>{
        open=false;d.remove();
    });
    document.body.append(d);
    d.showModal();
    update();
}
async function networkView(){
    const n=await api('/api/status');
    await modal('Conexión con la Switch',n.message+' · IP: '+n.ip+'\nLa consola permite crear una red local en [ZL]. En [Y] encontrarás tres códigos QR: acceso HTTP, conexión Wi-Fi y credenciales como texto. No se necesita Internet. La contraseña se conserva hasta que decidas cambiarla.',[['Cerrar','cancel']]);
}
document.documentElement.dataset.theme=localStorage.getItem('folderwifi-theme')||'dark';
$('theme').onclick=()=>{
    const next=document.documentElement.dataset.theme==='dark'?'light':'dark';
    document.documentElement.dataset.theme=next;
    localStorage.setItem('folderwifi-theme',next);
};
$('pair').onclick=()=>run(pair);
$('logs').onclick=()=>run(logsView);
$('network').onclick=()=>run(networkView);
$('up').onclick=()=>run(()=>navigate(parent(current)));
$('refresh').onclick=()=>run(refresh);
$('search').oninput=()=>{
    filePage=0;
    anchor=-1;
    render();
};
$('sort').onchange=()=>{
    filePage=0;
    anchor=-1;
    render();
};
$('previouspage').onclick=()=>{
    filePage=Math.max(0,filePage-1);
    render();
    $('files').scrollTop=0;
};
$('nextpage').onclick=()=>{
    filePage++;
    render();
    $('files').scrollTop=0;
};
$('selectall').onchange=()=>{
    for(const e of visible())$('selectall').checked?selected.add(e.path):selected.delete(e.path);
    render();
};
$('more').onclick=()=>{
    const d=el('dialog');
    d.append(el('h2','Acciones de archivos'));
    for(const [name,action]of [['Renombrar','rename'],['Duplicar','duplicate'],['Crear ZIP','zip'],['Extraer ZIP','extract'],['Propiedades','properties'],['Enviar a papelera','trash'],['Eliminar definitivamente','delete']]){
        const b=button(name,()=>{
            d.close();d.remove();return fileAction(action);
        });
        b.disabled=!selected.size;
        d.append(b,el('br'));
    }
    d.append(button('Cerrar',()=>{
        d.close();d.remove();
    }));
    document.body.append(d);
    d.showModal();
};
for(const b of document.querySelectorAll('[data-action]'))b.onclick=()=>run(()=>fileAction(b.dataset.action));
$('mkdir').onclick=()=>run(async()=>{
    const name=await ask('Nueva carpeta','Nombre de la carpeta.');if(name!==null){
        await api('/api/mkdir',{
            path:join(current,name)
        });await refresh();
    }
});
$('trash').onclick=()=>run(trashView);
$('searchtree').onclick=()=>run(async()=>{
    const term=await ask('Buscar en subcarpetas','Busca nombres de archivos y carpetas.',$('search').value);if(term)await execute('search',[current],current,'ask',{
        term
    });
});
$('addfiles').onclick=()=>{
    pickerDestination=current;
    $('fileinput').value='';
    $('fileinput').click();
};
$('addfolder').onclick=()=>{
    if(!('webkitdirectory' in $('folderinput'))){
        toast('Este navegador no permite elegir carpetas. Puedes añadir sus archivos o un ZIP.');
        return;
    }
    pickerDestination=current;
    toast('El selector conserva las carpetas de los archivos. Para incluir carpetas vacías, arrástralas al listado desde un PC o utiliza un ZIP.');
    $('folderinput').value='';
    $('folderinput').click();
};
$('files').addEventListener('dragover',e=>{
    e.preventDefault();
});
$('files').addEventListener('drop',e=>{
    e.preventDefault();const destination=current,items=Array.from(e.dataTransfer.items||[]).map(item=>item.webkitGetAsEntry?.()).filter(Boolean),files=[...e.dataTransfer.files];run(()=>items.length?droppedEntries(items,destination):addFiles(files,destination));
});
$('fileinput').onchange=()=>run(()=>addFiles([...$('fileinput').files],pickerDestination));
$('folderinput').onchange=()=>run(()=>addFiles([...$('folderinput').files],pickerDestination,true));
$('submitdraft').onclick=()=>run(submit);
$('pauseuploads').onclick=()=>run(async()=>{
    paused=!paused;await saveQueue();renderQueue();if(!paused)pump();
});
$('retryuploads').onclick=()=>run(async()=>{
    paused=false;for(const batch of batches)if(batch.state==='error'){
        batch.state='waiting';batch.allDecision=null;
    }
    await saveQueue();renderQueue();pump();
});
$('clearfinished').onclick=()=>run(async()=>{
    const finished=batches.filter(b=>['done','cancelled'].includes(b.state));const ids=finished.flatMap(b=>[...b.items.map(i=>i.id),...Object.values(b.groups||{
    }).map(g=>g.id)]);for(let i=0;i<ids.length;i+=256)await api('/api/upload/forget',{
        id:ids.slice(i,i+256)
    });batches=batches.filter(b=>!['done','cancelled'].includes(b.state));await saveQueue();renderQueue();
});
$('transferqueue').onclick=()=>run(transferQueue);
$('exportqueue').onclick=()=>run(exportQueue);
$('importqueue').onclick=()=>{
    $('manifestinput').value='';
    $('manifestinput').click();
};
$('manifestinput').onchange=()=>run(()=>importQueue($('manifestinput').files[0]));
$('canceljob').onclick=()=>run(async()=>{
    if(activeJob)await api('/api/job/cancel',{
        id:activeJob
    });
});
document.addEventListener('click',e=>{
    if(!$('context').contains(e.target))$('context').hidden=true;
});
document.addEventListener('keydown',e=>{
    if(document.querySelector('dialog[open]')||['INPUT','TEXTAREA','SELECT'].includes(e.target.tagName))return;const mod=e.ctrlKey||e.metaKey,k=e.key.toLowerCase();let action=null;if(mod&&k==='a'){
        e.preventDefault();selected=new Set(visible().map(i=>i.path));render();return;
    }
    if(mod&&k==='c')action='copy';if(mod&&k==='x')action='cut';if(mod&&k==='v')action='paste';if(e.key==='F2')action='rename';if(e.key==='Delete')action=e.shiftKey?'delete':'trash';if(action){
        e.preventDefault();run(()=>fileAction(action));
    }
});
window.addEventListener('popstate',()=>run(()=>navigate(new URL(location.href).searchParams.get('path')||'sdmc:/',false)));
window.addEventListener('online',()=>{
    if(!paused)pump();
});
document.addEventListener('visibilitychange',()=>{
    if(!document.hidden&&!paused)pump();
});
async function heartbeat(){
    if(key){
        try{
            const n=await api('/api/status');
            canWrite=n.writable!==false;
            $('connection').textContent=(n.local?'Red local':'Switch')+' · '+n.ip+(canWrite?'':' · Solo lectura');
            if(!paused&&batches.some(b=>b.state==='waiting'||b.state==='cancelling'))pump();
            const notices=await api('/api/notifications?'+form({
                since:lastNotice||0
            }));
            if(Array.isArray(notices)){
                for(const notice of notices){
                    if(lastNotice!==null&&['error','warning'].includes(notice.type))toast(notice.message);
                }
                if(notices.length)lastNotice=notices.at(-1).id;
                else if(lastNotice===null)lastNotice=0;
            }
        }catch(e){
            if(e.code===401)$('connection').textContent='Vinculación necesaria';
        }
    }
    setTimeout(heartbeat,3000);
}
async function start(){
    await initDb();
    await receiveBridge();
    if(!key)await pair();
    else{
        try{
            await refresh();
        }catch(e){
            if(e.code===401)await pair();
            else toast(e.message);
        }
    }
    const job=localStorage.getItem('folderwifi-job');
    if(job){
        jobBusy=true;
        watchJob(job).finally(()=>jobBusy=false);
    }
    heartbeat();
    pump();
}
run(start);
</script></body></html>)FOLDERWIFI";}
}
