// SPDX-License-Identifier: MPL-2.0
import { verifyPackage } from './verify.js?v=2';

let manifestRequest = 0;

export async function readReleaseManifest(baseUrl, fetcher = globalThis.fetch) {
  const url = new URL('./firmware.json', baseUrl);
  // no-store bypasses the browser cache; a fresh URL also avoids an older
  // manifest at a CDN edge while a Pages deployment is being propagated.
  url.searchParams.set('request', `${Date.now()}-${++manifestRequest}`);
  const response = await fetcher(url.href, { cache: 'no-store' });
  if (!response.ok) throw new Error(`Манифест недоступен: HTTP ${response.status}`);
  const manifest = await response.json();
  if (manifest.schema !== 1 || !Array.isArray(manifest.builds))
    throw new Error('Неподдерживаемый манифест.');
  const meta = manifest.builds[0] || null;
  if (meta) {
    const packageUrl = new URL(meta.url, baseUrl);
    const directory = new URL('./firmware/', baseUrl);
    if (packageUrl.origin !== directory.origin || !packageUrl.pathname.startsWith(directory.pathname))
      throw new Error('Неверный адрес сборки.');
  }
  return { meta, message: manifest.message };
}

async function download(meta, baseUrl, fetcher) {
  const url = new URL(meta.url, baseUrl);
  // Also isolate older releases that still used a shared mutable file name.
  url.searchParams.set('sha256', meta.sha256);
  const response = await fetcher(url.href, { cache: 'no-store' });
  if (!response.ok) throw new Error(`Не удалось скачать сборку: HTTP ${response.status}`);
  const image = await verifyPackage(new Uint8Array(await response.arrayBuffer()), meta);
  return { meta, image };
}

export async function loadVerifiedRelease(baseUrl, fetcher = globalThis.fetch) {
  const { meta } = await readReleaseManifest(baseUrl, fetcher);
  if (!meta) throw new Error('Проверенная сборка пока не опубликована.');
  try {
    return await download(meta, baseUrl, fetcher);
  } catch (error) {
    // A release may be replaced between the manifest and package requests.
    // Retry once only when a different published release actually appeared.
    let latest;
    try { latest = (await readReleaseManifest(baseUrl, fetcher)).meta; }
    catch { throw error; }
    if (!latest || (latest.sha256 === meta.sha256 && latest.bytes === meta.bytes &&
                    latest.product === meta.product && latest.url === meta.url))
      throw error;
    return await download(latest, baseUrl, fetcher);
  }
}
