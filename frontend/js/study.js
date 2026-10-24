/* ===================== CẤU HÌNH ===================== */
const KEY = 'petStudy';
const LV = Object.keys(D);
const TI = ['easy', 'medium', 'hard'];
const TNAME = { easy: 'Dễ', medium: 'Vừa', hard: 'Khó' };
const PT = { easy: 15, medium: 18, hard: 20 };    // điểm mỗi câu đúng
const QUIZ_SIZE = 5;                              // mỗi lần làm rút ngẫu nhiên 5 câu từ nhóm
const PASS_RATE = 0.8;                            // 4/5 câu đúng là qua nhóm
const TIER_BONUS = 25;                            // thưởng khi qua nhóm Dễ hoặc Vừa (không có xu)
const LEVEL_BONUS = { pts: 50, coins: 20 };       // thưởng khi qua nhóm Khó = lên cấp (A1→A2, A2→B1...)

/* MEME: ảnh nằm trong assets/memes/. Bạn chỉ cần đổi tên file tải về cho khớp tên bên dưới
   (hoặc sửa tên/đuôi .gif/.jpg/.png trong danh sách). Ảnh nào chưa có thì ô đó tự ẩn. */
const MEMES_DIR = 'assets/study/';
const MEMES = {
    corner: ['chopixel9.gif', 'm.gif', 'c.gif', 'meopixel1.gif'],           // 4 góc màn hình lúc mở trang Study
    right: ['chopixel5.gif', 'chopixel3.jpg', 'meopixel3.gif', 'chopixel7.gif', 'meopixel2.gif', 'meopixel6.gif', 'chopixel6.gif'], // 5 meme khi trả lời ĐÚNG (mỗi lần rút 2)
    wrong: ['chopixel2.gif', 'meopixel9.gif', 'meopixel4.jpg', 'chopixel8.jpg', 'chopixel1.jpg', 'meopixel7.jpg']       // 5 meme khi trả lời SAI (mỗi lần rút 2)
};
const CAP = {
    right: ['Bá Khí! ', 'Hay Lắm, human! ', 'Bá đạo trên từng hạt gạo. ', 'Mèo Méo Meo Mèo Meo. ',
        'Cook tiếp đi đầu bếp 👨‍🍳🍳. ', 'Cổ điển, tôn trọng. ', '  Trên cái đất Thủ Đức này tui nể mỗi bạn. '],
    wrong: ['Hồi chiều...hồi chiều làm sai câu TA. ', 'Cạn Phước. ', 'Chắc kiếp trước tặng chùa máy sấy tóc. ', 'Mong Tôn Hoa Sen thấy và làm ngơ. ', 'Thông báo: Pet của bạn đã gửi đơn xin đổi chủ. ',
        'Pet 7 vọng về bạn. ', 'Tui nói này bạn đừng buồn nha. ', 'Pet nước mình vất vả. ', 'Lên núi khỉ còn đuổi xuống. ']
};

/* ===================== TIỆN ÍCH ===================== */
let S = JSON.parse(localStorage.getItem(KEY) || '{"done":{},"pts":0,"coins":0}');
let cur = null;      // bài đang làm: {l, t, qs, i, ok, gain, review, answered, build}
let selLv = null;    // cấp đang chọn trên dây leo
let treeScroll = null;

const $ = id => document.getElementById(id);
const save = () => localStorage.setItem(KEY, JSON.stringify(S));
const pick = a => a[Math.floor(Math.random() * a.length)];
const shuffle = a => a.map(v => [Math.random(), v]).sort((x, y) => x[0] - y[0]).map(x => x[1]);
const seeded = seed => () => (seed = (seed * 16807) % 2147483647) / 2147483647;

function show(id) {
    ['tree', 'quiz', 'result'].forEach(s => $(s).classList.toggle('hidden', s !== id));
    $('stickers').classList.toggle('hidden', id !== 'tree');
}

/* Điểm lưu riêng để phần Happiness đọc: 10 điểm study = 0.2% happiness (happiness += pts / 10 * 0.2) */
function addPts(n) {
    S.pts += n;
    localStorage.setItem('petStudyPoints', S.pts);
    save();
}

function unlocked(l, t) {
    const li = LV.indexOf(l), ti = TI.indexOf(t);
    if (ti > 0) return !!S.done[l + '-' + TI[ti - 1]];
    return li === 0 || !!S.done[LV[li - 1] + '-hard'];
}

function setMeme(el, file) { // chỉ hiện khi ảnh tải được
    el.style.display = 'none';
    const im = new Image();
    im.onload = () => { el.replaceChildren(im); el.style.display = 'block'; };
    im.src = MEMES_DIR + file;
}

/* ===================== DÂY LEO PIXEL ===================== */
const W = 3800; // chiều dài dây leo (px), khớp .path-in trong CSS
const NODE = [[520, 190], [1060, 100], [1620, 235], [2140, 120], [2700, 220], [3260, 105]].map(([x, y]) => ({ x, y })); // 6 nút A1..C2
const VP = (() => { // các điểm dây đi qua: lệch ngẫu nhiên nhưng cố định, luôn qua các nút
    const r = seeded(5), P = [[-140, 210]]; let px = -140;
    NODE.forEach(n => {
        const gap = n.x - px;
        P.push([px + gap * (.30 + r() * .1), 70 + r() * 200], [px + gap * (.66 + r() * .1), 70 + r() * 200], [n.x, n.y]);
        px = n.x;
    });
    P.push([px + 200, 120 + r() * 120], [px + 420, 80 + r() * 160], [W + 140, 190]);
    return P;
})();

function smooth(P) { // đường cong mượt (Catmull-Rom -> Bézier)
    let d = `M${P[0][0]} ${P[0][1]}`;
    for (let i = 0; i < P.length - 1; i++) {
        const p0 = P[i - 1] || P[i], p1 = P[i], p2 = P[i + 1], p3 = P[i + 2] || p2;
        d += ` C${p1[0] + (p2[0] - p0[0]) / 6} ${p1[1] + (p2[1] - p0[1]) / 6},${p2[0] - (p3[0] - p1[0]) / 6} ${p2[1] - (p3[1] - p1[1]) / 6},${p2[0]} ${p2[1]}`;
    }
    return d;
}

function growVine() { // vẽ một lần: 1 ô pixel = 4px màn hình
    const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    svg.style.cssText = 'position:absolute;width:0;height:0;visibility:hidden';
    svg.innerHTML = `<path d="${smooth(VP)}"/><path d="${smooth(VP.map(([x, y], i) => [x + ((i % 3) - 1) * 8, y + (i % 2 ? 22 : -22)]))}"/>`;
    document.body.appendChild(svg);
    const [m, m2] = svg.querySelectorAll('path');
    const U = 4, cv = $('vc'), c = cv.getContext('2d'), r = seeded(11), L = m.getTotalLength();
    const dot = (x, y, w, col) => { c.fillStyle = col; c.fillRect(Math.round(x / U - w / 2), Math.round(y / U - w / 2), w, w); };
    const trace = (path, dx, dy, w, col) => { const n = path.getTotalLength(); for (let s = 0; s <= n; s += 2) { const p = path.getPointAtLength(s); dot(p.x + dx, p.y + dy, w, col); } };
    const LEAF = [['#d4f3dc', '#8fd1a3', '#5fae7c'], ['#e3f6c9', '#b5dc8f', '#86bb62'], ['#bfe9d3', '#7cc4a0', '#4f9d7e'], ['#d4f3dc', '#8fd1a3', '#5fae7c'], ['#fde0ec', '#f3a9c8', '#d17fa6']]; // lá chủ yếu xanh lá pastel, thỉnh thoảng lá hồng
    const leaf = (x, y, a, len, hw, pal) => {
        for (let t = 0; t <= len; t++) {
            const w = Math.round(Math.sin(Math.PI * t / len) * hw);
            for (let k = -w; k <= w; k++) { c.fillStyle = k < 0 ? pal[0] : k === 0 ? pal[1] : pal[2]; c.fillRect(Math.round(x + Math.cos(a) * t - Math.sin(a) * k), Math.round(y + Math.sin(a) * t + Math.cos(a) * k), 1, 1); }
        }
    };
    for (let k = 0; k < 16; k++) { // tua cuốn xoắn ốc
        const a = m.getPointAtLength(L * (.04 + k * .06) + r() * 40), dir = r() < .5 ? -1 : 1, th0 = -dir * Math.PI / 2, ax = a.x / U, ay = a.y / U;
        c.fillStyle = '#9fd8b0';
        for (let t = 0; t <= 1; t += .03) { const th = th0 + dir * t * 3.2 * Math.PI, R = 11 * (1 - .9 * t); c.fillRect(Math.round(ax + R * Math.cos(th)), Math.round(ay + dir * 11 + R * Math.sin(th)), 1, 1); }
    }
    trace(m, 0, 8, 5, 'rgba(30,70,50,.3)');
    trace(m, 0, 0, 5, '#3f7a5c'); trace(m, 0, 0, 3, '#7fc197'); trace(m, -4, -4, 1, '#d8f5df');
    trace(m2, 0, 0, 3, '#3f7a5c'); trace(m2, 0, 0, 1, '#b4e3bf');
    for (let s = 50; s < L - 50; s += 50 + r() * 55) { // lá mọc so le hai bên
        const a = m.getPointAtLength(s), b = m.getPointAtLength(s + 4), ang = Math.atan2(b.y - a.y, b.x - a.x), side = r() < .5 ? -1 : 1;
        leaf(a.x / U, a.y / U, ang + side * (35 + r() * 40) * Math.PI / 180, 8 + Math.round(r() * 7), 2 + (r() > .5 ? 1 : 0), LEAF[Math.floor(r() * 5)]);
    }
    let berries = '';
    for (let k = 0; k < 14; k++) { const a = m.getPointAtLength(r() * L); berries += `<i class="pb" style="left:${a.x + (r() - .5) * 30}px;top:${a.y + (r() - .5) * 40}px;animation-delay:${-r()}s"></i>`; }
    $('pathIn').insertAdjacentHTML('beforeend', berries);
    svg.remove();
}

function buildNodes() {
    $('pathIn').insertAdjacentHTML('beforeend', LV.map((l, n) =>
        `<button class="node" data-l="${l}" style="left:${NODE[n].x}px;top:${NODE[n].y}px" onclick="pickLv('${l}')"><b>${l}</b><em></em></button>`).join(''));
}

function enableDrag(pe) { // kéo chuột / lăn con lăn để lướt ngang (cảm ứng tự vuốt được)
    let down = false, x0 = 0, l0 = 0;
    pe.onpointerdown = e => { if (e.pointerType !== 'mouse' || e.target.closest('.node')) return; down = true; x0 = e.clientX; l0 = pe.scrollLeft; pe.classList.add('drag'); };
    pe.onpointermove = e => { if (down) pe.scrollLeft = l0 - (e.clientX - x0); };
    pe.onpointerup = pe.onpointerleave = () => { down = false; pe.classList.remove('drag'); };
    pe.onwheel = e => {
        const max = pe.scrollWidth - pe.clientWidth;
        if (Math.abs(e.deltaY) > Math.abs(e.deltaX) && ((e.deltaY > 0 && pe.scrollLeft < max) || (e.deltaY < 0 && pe.scrollLeft > 0))) { pe.scrollLeft += e.deltaY; e.preventDefault(); }
    };
    pe.addEventListener('scroll', () => { if (pe.offsetParent) treeScroll = pe.scrollLeft; }, { passive: true });
}

function pickLv(l) { selLv = l; renderTree(); }

function renderTree() {
    show('tree');
    $('pts').textContent = '✦ ' + S.pts;
    $('coins').textContent = '◎ ' + S.coins;
    document.querySelectorAll('.node').forEach(n => {
        const l = n.dataset.l, open = unlocked(l, 'easy'), done = S.done[l + '-hard'];
        n.className = 'node ' + (done ? 'done' : open ? 'open' : 'locked') + (l === selLv ? ' sel' : '');
        n.lastChild.textContent = done ? 'CLEAR' : open ? 'START' : 'LOCK';
    });
    const pn = $('panel');
    pn.classList.toggle('hidden', !selLv);
    if (selLv) {
        const l = selLv, open = unlocked(l, 'easy');
        pn.innerHTML = `<h3>Cấp ${l}</h3><small>${S.done[l + '-hard'] ? 'Hoàn thành! Bạn có thể ôn lại.' : open ? 'Qua Dễ → Vừa → Khó để mở cấp tiếp theo' : 'Hoàn thành cấp trước để mở khóa'}</small><div class="tiers">`
            + TI.map(t => { const ok = unlocked(l, t); return `<button class="tier ${t} ${ok ? '' : 'locked'} ${S.done[l + '-' + t] ? 'done' : ''}" ${ok ? '' : 'disabled'} onclick="startQuiz('${l}','${t}')">${TNAME[t]}</button>`; }).join('') + '</div>';
    }
    const pe = $('path');
    if (treeScroll === null) { const ci = LV.findIndex(l => !S.done[l + '-hard']); treeScroll = NODE[ci < 0 ? 5 : ci].x - pe.clientWidth / 2; }
    pe.scrollLeft = treeScroll;
}

/* ===================== NỀN PIXEL ===================== */
function pixelScene() { // vẽ cảnh rừng lên canvas 320x180, CSS phóng to không làm mịn
    const cv = $('px'), c = cv.getContext('2d'), W = cv.width, H = cv.height, rnd = seeded(7);
    const R = (x, y, w, h, col) => { c.fillStyle = col; c.fillRect(Math.round(x), Math.round(y), w, h); };
    const disc = (cx, cy, rad, col) => { for (let dy = -rad; dy <= rad; dy++) { const w = Math.floor(Math.sqrt(rad * rad - dy * dy)); R(cx - w, cy + dy, 2 * w + 1, 1, col); } };
    ['#9fd3ee', '#aedcf0', '#bde4f1', '#cdecef', '#dcf1ea', '#e8f5e2', '#f1f8dc', '#f8f6d8'].forEach((col, i) => R(0, i * H / 8, W, H / 8 + 1, col));
    [[30, '#f6ecd0'], [22, '#f9efcf'], [14, '#fcf3d8'], [8, '#fffbea']].forEach(([rad, col]) => disc(250, 30, rad, col));
    for (let x = 0; x < W; x++) { const h = Math.round(112 + 10 * Math.sin(x * .045) + 6 * Math.sin(x * .13 + 1)); R(x, h, 1, H - h, '#b8dcc0'); }
    [['#b4dcc0', 118, 14, 26], ['#93cba6', 134, 22, 36], ['#6fb48c', 152, 32, 50], ['#4f9a77', 172, 44, 70]].forEach(([col, base, a, b], k) => {
        for (let x = -6; x < W + 6; x += 9 + rnd() * (8 + k * 3)) {
            const h = Math.round(a + rnd() * (b - a));
            for (let i = 0; i < h; i++) R(x - Math.floor(i * .32), base - h + i, 2 * Math.floor(i * .32) + 1, 1, col);
            R(x - 1, base, 2, H - base, col);
        }
        R(0, base - 6, W, 8, 'rgba(240,255,240,.12)');
    });
    R(0, H - 8, W, 8, '#3f8566');
    for (let bx = -40; bx < W; bx += 46) for (let y = 0; y < 110; y++) R(bx + Math.floor(y * .4), y, 12, 1, 'rgba(255,250,225,.1)'); // tia nắng pixel
}

function grass() { // cỏ + hoa đung đưa theo gió dưới đáy màn hình (vẽ lại ~4,5 khung hình/giây cho nhẹ máy)
    const cv = $('gr'), g = cv.getContext('2d'), H = 30;
    const BL = ['#5fa47f', '#7fc197', '#b4e3bf'], FL = ['#f7bfd6', '#ffcfb8', '#d4c3f5', '#fff6e6'];
    let plants = [], t = 0;
    const build = () => {
        cv.width = Math.ceil(innerWidth / 4); cv.height = H;
        const r = seeded(3); plants = [];
        for (let x = 0; x < cv.width; x += 2 + Math.floor(r() * 4)) {
            const fl = r() < .2;
            plants.push({ x, fl, h: fl ? 12 + Math.floor(r() * 9) : 5 + Math.floor(r() * 9), ph: r() * 6, c: Math.floor(r() * 3), f: Math.floor(r() * 4) });
        }
    };
    const draw = () => {
        if (document.hidden) return;
        g.clearRect(0, 0, cv.width, H);
        g.fillStyle = '#4a8f6f'; g.fillRect(0, H - 3, cv.width, 3);
        plants.forEach(p => {
            const lean = i => Math.round(Math.sin(t * .8 + p.ph) * 1.6 * i / p.h);
            g.fillStyle = BL[p.fl ? 1 : p.c];
            for (let i = 0; i < p.h; i++) g.fillRect(p.x + lean(i), H - 3 - i, 2, 1);
            if (p.fl) {
                const x = p.x + lean(p.h), y = H - 3 - p.h;
                g.fillStyle = FL[p.f]; g.fillRect(x, y - 4, 2, 1); g.fillRect(x - 2, y - 3, 6, 2); g.fillRect(x, y - 1, 2, 1);
                g.fillStyle = p.f === 3 ? '#f7bfd6' : '#fff6e6'; g.fillRect(x, y - 3, 2, 2);
            }
        });
        t++;
    };
    build(); draw(); setInterval(draw, 220);
    let timer; addEventListener('resize', () => { clearTimeout(timer); timer = setTimeout(() => { build(); draw(); }, 150); });
}

function tiles() { // dải cỏ hoa mọc trên mép bảng + dây leo rủ xuống dưới bảng (ảnh pixel nhỏ, lặp ngang)
    const mk = (w, h, fn) => {
        const c = document.createElement('canvas'); c.width = w; c.height = h; const g = c.getContext('2d');
        fn((x, y, col, pw = 1, ph = 1) => { g.fillStyle = col; g.fillRect(x, y, pw, ph); });
        return `url(${c.toDataURL()})`;
    };
    const root = document.documentElement.style;
    root.setProperty('--tile-top', mk(96, 10, p => {
        for (let x = 0; x < 96; x += 2) { const h = 2 + (x * 7) % 4; p(x, 9 - h, x % 6 ? '#7fc197' : '#b4e3bf', 1, h + 1); }
        [[14, '#d4c3f5'], [50, '#f7bfd6'], [80, '#a9d4f5']].forEach(([x, col]) => { p(x, 4, '#7fc197', 1, 6); p(x, 1, col); p(x - 1, 2, col, 3, 1); p(x, 3, col); p(x, 2, '#fff6e6'); });
    }));
    root.setProperty('--tile-bot', mk(96, 12, p => {
        for (let x = 0; x < 96; x++) { const y = 1 + Math.round(Math.sin(x * .2)); p(x, y, '#7fc197'); p(x, y + 1, '#5fa47f'); }
        for (let x = 4; x < 96; x += 9) { const y = 2 + Math.round(Math.sin(x * .2)); p(x, y + 1, '#b4e3bf', 1, 2); p(x + 1, y + 2, '#b4e3bf'); p(x - 1, y + 2, '#7fc197'); }
        [[24, '#d4c3f5'], [66, '#f7bfd6']].forEach(([x, col]) => { p(x, 3, '#7fc197', 1, 3); p(x, 6, col); p(x - 1, 7, col, 3, 1); p(x, 8, col); p(x, 7, '#fff6e6'); });
        [45, 85].forEach(x => { p(x, 3, '#c4ecd6', 1, 3); p(x + 1, 6, '#c4ecd6', 2, 1); p(x + 2, 5, '#c4ecd6'); });
    }));
}

function decor() { // hạt bay: ít phần tử để nhẹ máy
    const rnd = Math.random; let h = '';
    for (let i = 0; i < 10; i++) h += `<i class="petal c${i % 3}" style="left:${rnd() * 100}%;--t:${12 + rnd() * 10}s;--d:${-rnd() * 20}s"></i>`;
    for (let i = 0; i < 12; i++) h += `<i class="fly" style="left:${rnd() * 100}%;top:${20 + rnd() * 60}%;--t:${4 + rnd() * 5}s;--d:${-rnd() * 6}s"></i>`;
    for (let i = 0; i < 2; i++) h += `<span class="bfly" style="top:${22 + i * 30}%;--t:${18 + i * 6}s;--d:${-i * 8}s"></span>`;
    $('fx').innerHTML = h;
}

/* ===================== LÀM BÀI ===================== */
function startQuiz(l, t) {
    // review: nhóm đã hoàn thành trước đó -> chế độ ôn tập, không cộng điểm/xu
    cur = { l, t, qs: shuffle(D[l][t]).slice(0, QUIZ_SIZE), i: 0, ok: 0, gain: 0, review: !!S.done[l + '-' + t] };
    show('quiz');
    showQ();
}

function showQ() {
    const q = cur.qs[cur.i];
    cur.answered = false;
    cur.build = [];
    $('fbWrap').classList.add('hidden');
    $('next').classList.add('hidden');
    $('barFill').style.width = (cur.i / cur.qs.length * 100) + '%';
    $('qTitle').textContent = `${cur.l} · ${TNAME[cur.t]}${cur.review ? ' · Ôn tập' : ''} · ${cur.i + 1}/${cur.qs.length}`;
    let h = '';
    if (q.t === 'mcq' || q.t === 'read') {
        q.opts = shuffle(q.o.map((txt, k) => ({ txt, ok: k === 0 })));
        h += `<span class="tag">${q.t === 'read' ? 'Đọc hiểu' : 'Chọn đáp án'}</span>`;
        if (q.v) h += `<div class="vi">${q.v}</div>`;
        if (q.x) h += `<div class="passage">${q.x}</div>`;
        h += `<div class="q">${q.q}</div>` + q.opts.map((o, k) => `<button class="opt" onclick="answerMcq(${k})">${o.txt}</button>`).join('');
    } else if (q.t === 'fill') {
        h += `<span class="tag">Điền từ</span><div class="q">${q.q}</div>
      <input class="fill" id="fillIn" autocomplete="off" placeholder="Gõ đáp án..." onkeydown="if(event.key==='Enter')answerFill()">
      <button class="btn" onclick="answerFill()">Kiểm tra</button>`;
    } else {
        q.words = shuffle(q.a.split(' '));
        h += `<span class="tag">Sắp xếp câu</span><div class="q">Xếp các từ thành câu đúng:</div>
      <div class="chips answer-zone" id="zone"></div><div class="chips" id="pool"></div>
      <button class="btn" onclick="answerOrder()">Kiểm tra</button>`;
    }
    $('qBox').innerHTML = h;
    if (q.t === 'order') drawChips();
    if (q.t === 'fill') $('fillIn').focus();
}

function drawChips() {
    const q = cur.qs[cur.i], inUse = new Set(cur.build);
    $('zone').innerHTML = cur.build.map(k => `<button class="chip" onclick="chip(${k})">${q.words[k]}</button>`).join('');
    $('pool').innerHTML = q.words.map((w, k) => inUse.has(k) ? '' : `<button class="chip" onclick="chip(${k})">${w}</button>`).join('');
}
function chip(k) {
    if (cur.answered) return;
    const p = cur.build.indexOf(k);
    p >= 0 ? cur.build.splice(p, 1) : cur.build.push(k);
    drawChips();
}

function answerMcq(k) {
    if (cur.answered) return;
    const q = cur.qs[cur.i], btns = document.querySelectorAll('.opt');
    btns.forEach((b, i) => { b.disabled = true; if (q.opts[i].ok) b.classList.add('ok'); });
    if (!q.opts[k].ok) btns[k].classList.add('bad');
    finish(q.opts[k].ok, q.opts.find(o => o.ok).txt);
}
function answerFill() {
    if (cur.answered) return;
    const q = cur.qs[cur.i], v = $('fillIn').value.trim().toLowerCase().replace(/\s+/g, ' ');
    if (v) finish(q.a.some(x => x.toLowerCase() === v), q.a[0]);
}
function answerOrder() {
    if (cur.answered) return;
    const q = cur.qs[cur.i];
    finish(cur.build.map(k => q.words[k]).join(' ').toLowerCase() === q.a.toLowerCase(), q.a);
}

function finish(correct, answer) {
    cur.answered = true;
    if (correct) { cur.ok++; if (!cur.review) { cur.gain += PT[cur.t]; $('pts').textContent = '✦ ' + (S.pts + cur.gain); } } // điểm tạm tính, chỉ lưu khi qua màn
    $('fb').className = 'feedback ' + (correct ? 'good' : 'wrong');
    $('fbWrap').classList.remove('hidden');
    $('fbVerdict').textContent = correct ? '✓ ĐÚNG!' : '✕ SAI RỒI';
    $('fbText').textContent = correct ? `${pick(CAP.right)}${cur.review ? '' : ' +' + PT[cur.t] + ' điểm tạm tính'}` : `${pick(CAP.wrong)} Đáp án đúng: ${answer}`;
    $('fbExp').textContent = cur.qs[cur.i].e;
    const pair = shuffle(MEMES[correct ? 'right' : 'wrong']); // 2 meme ngẫu nhiên khác nhau, hai bên bảng
    setMeme($('fbL'), pair[0]); setMeme($('fbR'), pair[1]);
    $('next').classList.remove('hidden');
    $('next').textContent = cur.i + 1 < cur.qs.length ? 'Tiếp tục →' : 'Xem kết quả';
}

function endQuiz() {
    const n = cur.qs.length, need = Math.ceil(n * PASS_RATE), pass = cur.ok >= need, li = LV.indexOf(cur.l);
    let msg = `Đúng ${cur.ok}/${n} câu.`;
    if (cur.review) msg += ' Chế độ ôn tập nên không cộng điểm và xu.';
    else if (!pass) msg += ` Cần đúng ít nhất ${need} câu để qua. ${cur.gain} điểm tạm tính đã được hoàn lại, thử lại nhé!`;
    else {
        S.done[cur.l + '-' + cur.t] = 1;
        const hard = cur.t === 'hard', bonus = hard ? LEVEL_BONUS.pts : TIER_BONUS;
        if (hard) S.coins += LEVEL_BONUS.coins;
        addPts(cur.gain + bonus); // lưu điểm + xu
        msg += ` Nhận ${cur.gain} điểm làm bài + ${bonus} điểm thưởng` + (hard
            ? ` + ${LEVEL_BONUS.coins} xu. ${li < LV.length - 1 ? `Qua cấp ${cur.l}, đã mở khóa ${LV[li + 1]}!` : 'Bạn đã chinh phục toàn bộ dây leo!'}`
            : '. Đã mở khóa nhóm tiếp theo!');
    }
    $('pts').textContent = '✦ ' + S.pts;
    $('coins').textContent = '◎ ' + S.coins;
    $('resTitle').textContent = pass ? 'Tuyệt vời!' : 'Cố lên nào!';
    $('resText').textContent = msg;
    setMeme($('resMeme'), pick(MEMES[pass ? 'right' : 'wrong']));
    show('result');
}

/* ===================== KHỞI ĐỘNG ===================== */
$('next').onclick = () => { cur.i++; cur.i < cur.qs.length ? showQ() : endQuiz(); };
$('quit').onclick = renderTree;
$('toTree').onclick = renderTree;
$('retry').onclick = () => startQuiz(cur.l, cur.t);
pixelScene(); tiles(); grass(); decor();
growVine(); buildNodes(); enableDrag($('path'));
$('stickers').querySelectorAll('.stk').forEach((el, i) => setMeme(el, MEMES.corner[i]));
renderTree();
