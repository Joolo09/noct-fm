// SPDX-License-Identifier: MPL-2.0
import { productOf, logicalImage } from './lib/fm1pkg.js';
export async function verifyPackage(raw, meta) {
  if (!meta || !/^FM-1_90625\d+$/.test(meta.product) || !/^[a-f0-9]{64}$/.test(meta.sha256))
    throw new Error('Неверные метаданные сборки NOCT.');
  if (raw.byteLength !== meta.bytes || raw.byteLength < 960 || raw.byteLength > 2 * 1024 * 1024)
    throw new Error('Размер файла не соответствует опубликованной сборке.');
  const hash = [...new Uint8Array(await crypto.subtle.digest('SHA-256', raw))]
    .map(b => b.toString(16).padStart(2, '0')).join('');
  if (hash !== meta.sha256) throw new Error('SHA-256 не совпадает. Файл повреждён или относится к другой сборке.');
  if (productOf(raw) !== meta.product) throw new Error('Файл содержит другой идентификатор прошивки.');
  return logicalImage(raw);
}
