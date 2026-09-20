// 生成 LVGL 9.x 24px 全量 GB2312（6763 字）中文字库。
// 直接以 UTF-16 参数调用 lv_font_conv.js（不经 npx.cmd 的 ANSI 转换），
// 确保 --symbols 中文字符零损坏。
// 注：bpp=2 —— LVGL 单字库 bitmap_index 为 20 位位域（上限 ~1MB），
//     bpp=4 全量 6763 字位图 ~1.24MB 会溢出截断（编译 -Woverflow），故降为 2。
const { execFileSync } = require('child_process');
const path = require('path');
const fs = require('fs');

const ROOT = path.resolve(__dirname, '..');
const FONT_CONV = 'C:/Users/lizhi/AppData/Local/npm-cache/_npx/b62fd1a864044392/node_modules/lv_font_conv/lv_font_conv.js';
const SRC_FONT = 'C:/Windows/Fonts/simhei.ttf';
const SYMBOLS_FILE = path.join(ROOT, 'tools', 'gb2312_chars.txt');
const OUT_FILE = path.join(ROOT, 'main', 'ui', 'font', 'lv_font_simhei_24px.c');

const symbols = fs.readFileSync(SYMBOLS_FILE, 'utf8');
console.log('symbols len =', symbols.length);

const args = [
  '--font', SRC_FONT,
  '--size', '24',
  '--bpp', '2',
  '--format', 'lvgl',
  '-r', '0x20-0x7F',
  '--symbols', symbols,
  '--lv-include', 'lvgl.h',
  '--lv-font-name', 'lv_font_simhei_24px',
  '-o', OUT_FILE,
];
try {
  execFileSync(process.execPath, [FONT_CONV, ...args], { stdio: 'inherit' });
  const stat = fs.statSync(OUT_FILE);
  console.log('DONE:', OUT_FILE, (stat.size / 1024).toFixed(1), 'KB');
} catch (e) {
  console.error('FAILED:', e.message);
  process.exit(1);
}
