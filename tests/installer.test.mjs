// Tests package identity, SHA-256 and the browser installer's no-file state.
import assert from 'node:assert/strict';
import { webcrypto } from 'node:crypto';
import { readFileSync } from 'node:fs';
import { verifyPackage } from '../site/verify.js';
import { readReleaseManifest, loadVerifiedRelease } from '../site/releases.js';
import { pack7, unpack7 } from '../site/lib/fm1ota.js';
globalThis.crypto ??= webcrypto;
const raw=new Uint8Array(1200);
const product='FM-1_9062500';
for(let i=0;i<20;i++)raw[i*48+47]=i<product.length?(product.charCodeAt(i)+i+1)&255:125;
const meta={product,bytes:raw.length,sha256:Buffer.from(await crypto.subtle.digest('SHA-256',raw)).toString('hex')};
assert.equal((await verifyPackage(raw,meta)).length,1180);
const changed=raw.slice();changed[100]^=1;
await assert.rejects(verifyPackage(changed,meta),/SHA-256/);
await assert.rejects(verifyPackage(raw.slice(1),meta),/size/);
await assert.rejects(verifyPackage(raw,{...meta,product:'FM-1_015'}),/metadata/);
await assert.rejects(verifyPackage(raw,{...meta,product:'FM-1_9062501'}),/identity/);
const bytes=Array.from({length:512},(_,i)=>i%256);
assert.deepEqual(unpack7(pack7(bytes)),bytes);
const manifest=JSON.parse(readFileSync(new URL('../site/firmware.json',import.meta.url),'utf8'));
assert.equal(manifest.schema,1);assert.deepEqual(manifest.builds,[]);
const html=readFileSync(new URL('../site/index.html',import.meta.url),'utf8');
assert.match(html,/id="install" disabled/);
assert.match(html,/id="ack" type="checkbox"/);
assert.match(html,/installer\.js\?v=3/);

// A long-lived tab must not verify today's download with yesterday's metadata.
const base='https://example.test/noct-fm/';
async function firmware(product,size) {
  const raw=new Uint8Array(size);
  for(let i=0;i<20;i++)raw[i*48+47]=i<product.length?(product.charCodeAt(i)+i+1)&255:125;
  const sha256=Buffer.from(await crypto.subtle.digest('SHA-256',raw)).toString('hex');
  return {raw,meta:{product,bytes:size,sha256,version:product,url:`./firmware/noct-usb-${sha256}.fwsc`}};
}
const oldBuild=await firmware('FM-1_90625000700',1200);
const newBuild=await firmware('FM-1_90625000701',1300);
const json=meta=>new Response(JSON.stringify({schema:1,builds:meta?[meta]:[]}));
let current=oldBuild;
const requests=[];
const fetchCurrent=async (url,options)=>{
  const u=new URL(url);requests.push({url:u,options});
  return u.pathname.endsWith('/firmware.json')?json(current.meta):new Response(current.raw);
};
const opened=await readReleaseManifest(base,fetchCurrent);
assert.equal(opened.meta.product,oldBuild.meta.product);
current=newBuild;
await assert.rejects(verifyPackage(newBuild.raw,opened.meta),/size/);
const loaded=await loadVerifiedRelease(base,fetchCurrent);
assert.equal(loaded.meta.product,newBuild.meta.product);
assert.equal(loaded.image.length,newBuild.raw.length-20);
assert.equal(opened.meta.product,oldBuild.meta.product);
assert.notEqual(requests[0].url.href,requests[1].url.href);
assert.ok(requests.every(r=>r.options.cache==='no-store'));
assert.equal(requests.at(-1).url.searchParams.get('sha256'),newBuild.meta.sha256);

// Deployment changes between requests: one refresh/retry, including old shared URLs.
for(const missingOldFile of [false,true]) {
  let manifests=0,packages=0;
  const fetchDuringDeploy=async url=>{
    if(new URL(url).pathname.endsWith('/firmware.json'))
      return json(++manifests===1?{...oldBuild.meta,url:'./firmware/noct-usb-alpha.fwsc'}:newBuild.meta);
    packages++;
    return missingOldFile&&packages===1?new Response('',{status:404}):new Response(newBuild.raw);
  };
  const release=await loadVerifiedRelease(base,fetchDuringDeploy);
  assert.equal(release.meta.sha256,newBuild.meta.sha256);
  assert.equal(manifests,2);assert.equal(packages,2);
}

// Corruption is never accepted, and an unchanged manifest does not cause a loop.
for(const sameSize of [false,true]) {
  let manifests=0,packages=0;
  const damaged=sameSize?newBuild.raw.slice():oldBuild.raw;
  if(sameSize)damaged[100]^=1;
  const badFetch=async url=>{
    if(new URL(url).pathname.endsWith('/firmware.json')) {manifests++;return json(newBuild.meta);}
    packages++;return new Response(damaged);
  };
  await assert.rejects(loadVerifiedRelease(base,badFetch),sameSize?/SHA-256/:/expected 1300 bytes, got 1200/);
  assert.equal(manifests,2);assert.equal(packages,1);
}
let attempts=0;
await assert.rejects(loadVerifiedRelease(base,async url=>{
  if(new URL(url).pathname.endsWith('/firmware.json'))
    return json(++attempts===1?oldBuild.meta:newBuild.meta);
  return new Response(new Uint8Array(960));
}),/size/);
assert.equal(attempts,2);
for(const url of ['https://other.test/firmware/evil.fwsc','../outside.fwsc'])
  await assert.rejects(readReleaseManifest(base,async()=>json({...newBuild.meta,url})),/address/);
await assert.rejects(loadVerifiedRelease(base,async()=>json(null)),/has been published yet/);

// Exercise the actual page module: open on the old release, then press Load
// after publishing the new release, without reloading the page or probing USB.
const elements=new Map();
globalThis.document={getElementById(id){
  if(!elements.has(id))elements.set(id,{textContent:'',checked:false,disabled:false,
    listeners:new Map(),addEventListener(name,callback){this.listeners.set(name,callback);}});
  return elements.get(id);
}};
globalThis.location=new URL(base);
globalThis.isSecureContext=true;
globalThis.navigator ??= {};
navigator.requestMIDIAccess=async()=>{throw new Error('USB must not be accessed by Load');};
globalThis.addEventListener=()=>{};
globalThis.fetch=fetchCurrent;
current=oldBuild;
await import('../site/installer.js?stale-page-regression');
assert.ok(elements.get('build').textContent.includes(oldBuild.meta.version));
current=newBuild;
await elements.get('load').listeners.get('click')();
assert.ok(elements.get('build').textContent.includes(newBuild.meta.version));
assert.ok(elements.get('package').textContent.includes(newBuild.meta.product));
assert.equal(elements.get('error').textContent,'');
assert.match(elements.get('status').textContent,/Package verified/);
console.log('Installer checks passed: page reload-free upgrade, package integrity, deployment races, cache isolation, bounded retries, URL validation and MIDI encoding.');
