// 生成「电极脱落」全局提示弹窗专用子集字库（LVGL 9.x bin，运行时 lv_binfont_create 加载）。
// 背景：弹窗中文用同事子集字库 lv_font_Btn.bin(size15)，但它不含新文案里的
//       脱/落/检/查/贴 等字，渲染成豆腐块（字体没显示全）。本脚本只收录弹窗用到的
//       汉字，size=15 / bpp=4 与 lv_font_Btn.bin 同字号同清晰度，视觉一致。
//       输出 spiffs_data/png/font/lv_font_Elec.bin（随 SPIFFS 镜像打包，不占 app Flash）。
// 用法：node tools/gen_elec_font.js
const { execFileSync } = require('child_process');
const path = require('path');
const fs = require('fs');

const ROOT = path.resolve(__dirname, '..');
const FONT_CONV = 'C:/Users/lizhi/AppData/Local/npm-cache/_npx/b62fd1a864044392/node_modules/lv_font_conv/lv_font_conv.js';
const SRC_FONT = 'C:/Windows/Fonts/simhei.ttf';
const OUT_FILE = path.join(ROOT, 'spiffs_data', 'png', 'font', 'lv_font_Elec.bin');

/* 弹窗中文文案去重汉字：
 *   "电极脱落 / 请检查电极是否贴好" + 按钮 "确定"
 * （确定/检查 等虽可能已在 Btn 字库，但子集字库自成一体，全部收录最稳妥，不依赖 Btn 含字） */
const CN = '电极脱落请检查是否贴好确定';

/* 弹窗文案暂无中文标点；半角字母数字由 -r 0x20-0x7F 覆盖（英文走 montserrat_14，此处仅兜底） */
const SYM = '';

const symbols = CN + SYM;
console.log('symbols len =', symbols.length);

const args = [
  '--font', SRC_FONT,
  '--size', '15',          /* 与 lv_font_Btn.bin 同字号 */
  '--bpp', '4',
  '--format', 'bin',
  '-r', '0x20-0x7F',
  '--symbols', symbols,
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
