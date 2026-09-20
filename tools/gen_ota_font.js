// 生成 OTA 升级页专用子集字库（LVGL 9.x bin 格式，运行时 lv_binfont_create 加载）。
// 背景：OTA 页用的同事子集字库 lv_font_Btn.bin 不含新增文案汉字（联/网/失/败/热/点/重/试…），
//      渲染成豆腐块。此脚本只收录 OTA 页用到的汉字 + 标点 + 符号 + ASCII，bpp=4 清晰，
//      输出到 spiffs_data/png/font/lv_font_OTA.bin（随 SPIFFS 镜像打包，不占 app Flash）。
// 用法：node tools/gen_ota_font.js
const { execFileSync } = require('child_process');
const path = require('path');
const fs = require('fs');

const ROOT = path.resolve(__dirname, '..');
const FONT_CONV = 'C:/Users/lizhi/AppData/Local/npm-cache/_npx/b62fd1a864044392/node_modules/lv_font_conv/lv_font_conv.js';
const SRC_FONT = 'C:/Windows/Fonts/simhei.ttf';
const OUT_FILE = path.join(ROOT, 'spiffs_data', 'png', 'font', 'lv_font_OTA.bin');

/* OTA 页全部静态 + 动态文案里出现的汉字（重复无害，lv_font_conv 自动去重） */
const CN =
  '账号密码联网中正在检查更新发现新版本是否升级开始已是最新请勿断电' +
  '完成请重启生效点击部分项目失败红条可重试确认热点后获取版本信息返回图片成功';

/* 中文标点 / 特殊符号（半角 () : % 及字母数字由 -r 0x20-0x7F 覆盖）
 * 注意：SimHei 不含 ✓ U+2713 / ✔ U+2714，"已是最新"借用 √ U+221A（视觉即对勾）；
 *       版本号用 → U+2192；省略号 … U+2026。三者 SimHei 均含。 */
const SYM = '，。？！…（）：→√';

const symbols = CN + SYM;
console.log('symbols len =', symbols.length);

const args = [
  '--font', SRC_FONT,
  '--size', '20',
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
