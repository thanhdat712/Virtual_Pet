/* ===== Pet Snack Run - Practice mini game ===== */
(() => {
    'use strict';
    if (window.__petPracticeLoaded) return;      // chống nạp script 2 lần
    window.__petPracticeLoaded = true;

    /* ---------- Cấu hình ---------- */
    const T = 35;                                 // kích thước logic của một ô
    let COLS = 15, ROWS = 9;
    let SW = (COLS + 2) * T, SH = (ROWS + 2) * T;
    let CX = COLS >> 1, CY = ROWS >> 1;
    const SPEED = { easy: { start: 4, max: 9 }, hard: { start: 6, max: 13 } };   // bước/giây; +0.25 mỗi 5 con
    const COUNTDOWN_MS = 5000;
    const DIRS = { up: { x: 0, y: -1 }, down: { x: 0, y: 1 }, left: { x: -1, y: 0 }, right: { x: 1, y: 0 } };
    const OPP = { up: 'down', down: 'up', left: 'right', right: 'left' };
    const KEYMAP = { arrowup: 'up', arrowdown: 'down', arrowleft: 'left', arrowright: 'right', w: 'up', s: 'down', a: 'left', d: 'right' };
    const MEME_READY = ['meopixel2.gif', 'meopixel6.gif', 'meopixel1.gif', 'meopixel6.gif', 'meopixel3.gif'];
    const MEME_GAME_OVER = ['meopixel2.gif', 'meopixel9.gif', 'meopixel4.jpg', 'meopixel6.gif', 'meopixel3.gif'];
    const SPR = {
        cat: 'assets/study/meo.png',    // Ảnh mèo: dùng trên sân, nút đổi thú và màn hình chọn thú.
        dog: 'assets/study/cho.png',    // Ảnh chó: dùng trên sân và màn hình chọn thú.
        fish: 'assets/study/ca.png',    // Ảnh cá: thức ăn khi chơi với mèo.
        bone: 'assets/study/xuong.png'  // Ảnh xương: thức ăn khi chơi với chó.
    };
    const ART = Object.fromEntries(Object.entries(SPR).map(([name, src]) => {
        const image = new Image();
        image.src = src;
        return [name, image];
    }));
    const FOOD = {
        cat: { label: 'Fish', done: 'Fish eaten', spr: 'fish' },
        dog: { label: 'Bone', done: 'Bones eaten', spr: 'bone' }
    };

    /* ---------- Tiện ích ---------- */
    const $ = (s, r = document) => r.querySelector(s);
    const $$ = (s, r = document) => Array.from(r.querySelectorAll(s));
    const rand = n => Math.floor(Math.random() * n);
    const memeRequests = new WeakMap();
    const key = (x, y) => x + ',' + y;
    const shuffle = a => { for (let i = a.length - 1; i > 0; i--) { const j = rand(i + 1);[a[i], a[j]] = [a[j], a[i]]; } return a; };
    const rect = (g, col, x, y, w = 1, h = 1) => { g.fillStyle = col; g.fillRect(x, y, w, h); };
    const fmtTime = ms => {
        const s = Math.floor(ms / 1000);
        return String(Math.floor(s / 60)).padStart(2, '0') + ':' + String(s % 60).padStart(2, '0');
    };
    const makeCanvas = (w, h) => { const c = document.createElement('canvas'); c.width = w; c.height = h; return c; };

    /* ---------- Sprite & tile pixel ---------- */
    const makeTile = fn => { const c = makeCanvas(T, T); fn(c.getContext('2d')); return c; };
    const TILE = {
        grass: [0, 1].map(v => makeTile(g => {
            rect(g, v ? '#aae49c' : '#9fdc91', 0, 0, T, T);
            [[3, 4], [11, 2], [7, 10], [12, 12], [2, 13]].forEach(([x, y]) => {
                rect(g, '#86cc7a', x + v, y, 1, 2);
                rect(g, '#86cc7a', x + v + 1, y + 1);
            });
        })),
        stone: makeTile(g => {
            rect(g, '#9aa3b5', 0, 0, T, T);
            [[1, 1, 7, 6], [9, 1, 6, 6], [1, 9, 3, 6], [5, 9, 7, 6], [13, 9, 2, 6]].forEach(([x, y, w, h]) => {
                rect(g, '#d3d9e6', x, y, w, h);
                rect(g, '#eef1f8', x, y, w, 1);
                rect(g, '#b4bccd', x, y + h - 1, w, 1);
            });
        }),
        wall: makeTile(g => {
            rect(g, '#8a5a3c', 0, 0, T, T);
            rect(g, '#eaa978', 1, 1, 14, 14);
            rect(g, '#f6c79e', 1, 1, 14, 1);
            for (let i = 2; i < 14; i++) { rect(g, '#c98652', i, i); rect(g, '#c98652', 15 - i, i); }
            rect(g, '#8a5a3c', 1, 7, 14, 2);
            rect(g, '#8a5a3c', 7, 1, 2, 14);
        })
    };
    const PORTAL_COLS = [['#ffc2d6', '#ff7fa6', '#c2487a'], ['#bfe2ff', '#6fb4f2', '#3279b8'], ['#ffe9a3', '#f5b942', '#b97f12']];

    /* ---------- Trạng thái ---------- */
    const S = {
        species: null, mode: 'easy',
        state: 'idle',                           // idle | countdown | playing | paused | over
        snake: [], dir: 'right', queue: [], food: null,
        walls: new Set(), portH: new Map(), portV: new Map(), portalTiles: [],
        eaten: 0, interval: 250, acc: 0, time: 0, cd: 0, cdShown: 5,
        best: 0, pendingBest: 0, prevBest: 0, coinsGiven: false, newRecord: false,
        gameId: null, practiceReady: false, starting: false, leavingPractice: false
    };

    /* ---------- DOM (lấy 1 lần) ---------- */
    const board = $('#board'), bctx = board.getContext('2d', { alpha: false });
    board.width = SW; board.height = SH;
    bctx.imageSmoothingEnabled = false;
    const overlay = $('#overlay'), toastEl = $('#toast'), cdNum = $('#cd-num'), pauseBtn = $('#pause-btn');
    const bottomActions = $('#bottom-actions'), practiceStatus = $('#practice-status');
    const switchesEl = $('#switches'), swSp = $('#sw-species'), swMode = $('#sw-mode');
    const hud = { score: $('#st-score'), eaten: $('#st-eaten'), best: $('#st-best'), bestLabel: $('#lb-best'), time: $('#st-time'), label: $('#lb-eaten') };
    let lastTimeText = '';

    function setState(s) {
        S.state = s;
        document.body.dataset.state = s;
        pauseBtn.disabled = !(s === 'playing' || s === 'paused');
        pauseBtn.textContent = s === 'paused' ? 'Resume' : 'Pause';
    }
    function abandonCurrentGame(keepalive = false) {
        if (!S.gameId || S.state === 'over') return Promise.resolve();
        const gameId = S.gameId;
        S.gameId = null;
        return window.api.abandonPracticeGame(gameId, keepalive).catch(error => {
            console.error('Unfinished Practice game could not be abandoned:', error);
        });
    }
    function showScreen(name) {
        $$('.screen', overlay).forEach(el => { el.hidden = el.dataset.screen !== name; });
        overlay.hidden = !name;
        overlay.classList.toggle('is-count', name === 'count');
        bottomActions.hidden = true;
        $$('[data-bottom-actions]', bottomActions).forEach(el => { el.hidden = true; });
        if (name === 'ready') setMeme($('#meme-start'), MEME_READY);
        if (name === 'over') setMeme($('#meme-end'), MEME_GAME_OVER);
        const b = name && $('.screen:not([hidden]) button:not(.screen-close)', overlay);
        if (b) b.focus({ preventScroll: true });
    }
    function toast(msg) {
        toastEl.textContent = msg;
        toastEl.hidden = false;
        clearTimeout(toast.t);
        toast.t = setTimeout(() => { toastEl.hidden = true; }, 1800);
    }
    function showPracticeStatus(message, isError = false) {
        practiceStatus.textContent = message;
        practiceStatus.dataset.error = String(isError);
        practiceStatus.hidden = !message;
    }
    function createGameId() {
        if (window.crypto && typeof window.crypto.randomUUID === 'function')
            return window.crypto.randomUUID();
        const bytes = new Uint8Array(16);
        if (window.crypto && typeof window.crypto.getRandomValues === 'function')
            window.crypto.getRandomValues(bytes);
        else
            bytes.forEach((_, index) => { bytes[index] = Math.floor(Math.random() * 256); });
        const hex = Array.from(bytes, value => value.toString(16).padStart(2, '0')).join('');
        return `${hex.slice(0, 8)}-${hex.slice(8, 12)}-4${hex.slice(13, 16)}-${((parseInt(hex[16], 16) & 3) | 8).toString(16)}${hex.slice(17, 20)}-${hex.slice(20)}`;
    }
    function setGameOverActionsDisabled(disabled) {
        $$('button[data-action="again"], button[data-action="menu"]').forEach(button => {
            button.disabled = disabled;
        });
    }
    async function saveGameRewards() {
        if (S.state !== 'over' || !S.gameId || S.rewardSaving) return;
        S.rewardSaving = true;
        setGameOverActionsDisabled(true);
        $('#reward-retry').hidden = true;
        showPracticeStatus('Saving Practice rewards...');
        try {
            const saved = await window.api.finishPracticeGame(S.gameId, S.eaten);
            const result = window.PracticeAPI.lastResult;
            result.points = saved.awardedPoints;
            result.happinessPercent = saved.awardedPoints * 0.05;
            result.coins = saved.awardedCoins;
            S.best = saved.state.bestScore;
            S.pendingBest = S.best;
            updateHud();
            $('#rs-score').textContent = saved.awardedPoints;
            $('#rs-happy').textContent = '+' + result.happinessPercent.toFixed(2) + '%';
            $('#rs-coins').textContent = '+' + saved.awardedCoins;
            S.rewardSaving = false;
            setGameOverActionsDisabled(false);
            $('#reward-retry').hidden = true;
            showPracticeStatus(saved.duplicate ? 'These rewards were already saved.' : 'Practice rewards saved to your account.');
        } catch (error) {
            S.rewardSaving = false;
            console.error('Practice rewards could not be saved:', error);
            showPracticeStatus(`Could not save Practice rewards: ${error.message} Retry before starting another game.`, true);
            $('#reward-retry').hidden = false;
        }
    }
    async function initializePractice() {
        try {
            const state = await window.api.practiceState();
            S.best = state.bestScore;
            S.pendingBest = S.best;
            updateHud();
            await window.api.practiceTime('start');
            S.practiceReady = true;
            S.practiceHeartbeat = setInterval(() => {
                window.api.practiceTime('heartbeat').catch(error => {
                    console.error('Practice timer heartbeat failed:', error);
                    showPracticeStatus(`Could not update Practice time: ${error.message}`, true);
                });
            }, 20000);
            showPracticeStatus('');
        } catch (error) {
            console.error('Practice account state could not be loaded:', error);
            showPracticeStatus(`Practice rewards require an active login: ${error.message}`, true);
        }
    }
    function updateHud() {
        hud.score.textContent = Math.floor(S.eaten / 5) * 15;
        hud.eaten.textContent = S.eaten;
        hud.best.textContent = Math.max(S.best, S.pendingBest);
        hud.bestLabel.textContent = S.pendingBest > S.best ? 'Best*' : 'Best';
    }
    function updateTimeHud() {
        const t = fmtTime(S.time);
        if (t !== lastTimeText) { hud.time.textContent = t; lastTimeText = t; }
    }

    /* ---------- Meme (tên file đầy đủ trong assets/study) ---------- */
    function loadMeme(img, filename) {
        const box = img.parentElement;
        const url = new URL(`assets/study/${filename}`, document.baseURI).href;
        const request = {};
        memeRequests.set(img, request);
        box.hidden = true;
        img.hidden = true;
        img.onload = () => {
            if (memeRequests.get(img) !== request) return;
            img.hidden = false;
            box.hidden = false;
        };
        img.onerror = () => {
            if (memeRequests.get(img) !== request) return;
            img.hidden = true;
            box.hidden = true;
            console.error(`Practice meme failed to load: ${url}`);
        };
        img.src = url;
    }
    function setMeme(img, memes) { loadMeme(img, memes[rand(memes.length)]); }

    /* ---------- Tạo bản đồ ---------- */
    function genPortals() {
        const all = [];
        for (let i = 0; i < ROWS; i++) all.push({ h: true, i });
        for (let i = 0; i < COLS; i++) all.push({ h: false, i });
        S.portH = new Map(); S.portV = new Map(); S.portalTiles = [];
        shuffle(all).slice(0, 2 + rand(2)).forEach((p, k) => {      // 2-3 cặp cổng, mỗi cặp 1 màu
            if (p.h) { S.portH.set(p.i, k); S.portalTiles.push({ x: 0, y: p.i + 1, ci: k }, { x: COLS + 1, y: p.i + 1, ci: k }); }
            else { S.portV.set(p.i, k); S.portalTiles.push({ x: p.i + 1, y: 0, ci: k }, { x: p.i + 1, y: ROWS + 1, ci: k }); }
        });
    }
    function portalInner(x, y) {
        return ((x === 0 || x === COLS - 1) && S.portH.has(y)) || ((y === 0 || y === ROWS - 1) && S.portV.has(x));
    }
    function connected(walls) {
        const seen = new Set([key(CX, CY)]), st = [[CX, CY]];
        while (st.length) {
            const [x, y] = st.pop();
            for (const d of Object.values(DIRS)) {
                const nx = x + d.x, ny = y + d.y, k = key(nx, ny);
                if (nx < 0 || ny < 0 || nx >= COLS || ny >= ROWS || walls.has(k) || seen.has(k)) continue;
                seen.add(k); st.push([nx, ny]);
            }
        }
        return seen.size === COLS * ROWS - walls.size;
    }
    function genWalls() {                        // 3-5 vách, luôn chừa lối đi, không chắn hàng xuất phát
        for (let tries = 0; tries < 300; tries++) {
            const w = new Set(), bars = 3 + rand(3);
            let failed = false;
            for (let b = 0; b < bars && !failed; b++) {
                let placed = false;
                for (let k = 0; k < 40 && !placed; k++) {
                    const horiz = Math.random() < 0.5, x = rand(COLS), y = rand(ROWS), len = 1 + rand(2);
                    const cells = Array.from({ length: len }, (_, i) => [x + (horiz ? i : 0), y + (horiz ? 0 : i)]);
                    if (cells.every(([cx, cy]) => cx < COLS && cy < ROWS && cy !== CY && !portalInner(cx, cy) && !w.has(key(cx, cy)))) {
                        cells.forEach(([cx, cy]) => w.add(key(cx, cy)));
                        placed = true;
                    }
                }
                failed = !placed;
            }
            if (!failed && connected(w)) return w;
        }
        return new Set();
    }
    function spawnFood() {                       // chỉ ở ô trống; sân đầy thì trả về false
        const used = new Set(S.snake.map(s => key(s.x, s.y)));
        const free = [];
        for (let y = 0; y < ROWS; y++) for (let x = 0; x < COLS; x++) {
            const k = key(x, y);
            if (!used.has(k) && !S.walls.has(k)) free.push({ x, y });
        }
        S.food = free.length ? free[rand(free.length)] : null;
        return !!S.food;
    }

    /* ---------- Vẽ bàn chơi (nền tĩnh vẽ 1 lần mỗi ván) ---------- */
    const base = makeCanvas(SW, SH), bx = base.getContext('2d');
    function configureBoardGrid() {
        let cols, rows;
        if (innerHeight >= innerWidth) {
            cols = innerWidth < 600 ? (innerHeight <= 600 ? 9 : 8) : 10;
            rows = innerHeight <= 600 ? 10 : 13;
        } else if (innerWidth < 1100) {
            cols = 14; rows = 9;
        } else {
            cols = Math.max(15, Math.min(20, Math.round(innerWidth / 96)));
            rows = Math.max(8, Math.round(cols * .6));
        }
        if (cols === COLS && rows === ROWS) return false;
        COLS = cols; ROWS = rows;
        CX = COLS >> 1; CY = ROWS >> 1;
        SW = (COLS + 2) * T; SH = (ROWS + 2) * T;
        board.width = base.width = SW;
        board.height = base.height = SH;
        bctx.imageSmoothingEnabled = false;
        return true;
    }

    function buildBase() {
        for (let ty = 0; ty < ROWS + 2; ty++) for (let tx = 0; tx < COLS + 2; tx++) {
            const edge = tx === 0 || ty === 0 || tx === COLS + 1 || ty === ROWS + 1;
            bx.drawImage(edge ? TILE.stone : TILE.grass[(tx + ty) & 1], tx * T, ty * T);
        }
        S.walls.forEach(k => { const [x, y] = k.split(',').map(Number); bx.drawImage(TILE.wall, (x + 1) * T, (y + 1) * T); });
    }
    function drawPortal(g, x, y, t, ci) {
        const [lt, md, dk] = PORTAL_COLS[ci % 3];
        rect(g, dk, x, y, T, T);
        rect(g, lt, x + 1, y + 1, T - 2, T - 2);
        rect(g, md, x + 3, y + 3, T - 6, T - 6);
        rect(g, dk, x + 5, y + 5, T - 10, T - 10);
        const f = Math.floor(t / 150) % 4;
        [[3, 3], [11, 3], [11, 11], [3, 11]].forEach((p, i) => rect(g, (i + f) % 4 === 0 ? '#ffffff' : lt, x + p[0], y + p[1], 2, 2));
    }
    function drawBoard(t) {
        const g = bctx;
        g.drawImage(base, 0, 0);
        S.portalTiles.forEach(p => drawPortal(g, p.x * T, p.y * T, t, p.ci));
        if (!S.species) return;
        if (S.food && ART[FOOD[S.species].spr].complete && ART[FOOD[S.species].spr].naturalWidth) {
            const bob = Math.floor(t / 280) % 2;
            g.drawImage(ART[FOOD[S.species].spr], (S.food.x + 1) * T, (S.food.y + 1) * T - bob, T, T);
        }
        g.globalAlpha = S.state === 'over' ? 0.6 : 1;
        for (let i = S.snake.length - 1; i >= 0; i--) {
            const s = S.snake[i], x = (s.x + 1) * T, y = (s.y + 1) * T;
            if (ART[S.species].complete && ART[S.species].naturalWidth) g.drawImage(ART[S.species], x, y, T, T);
        }
        g.globalAlpha = 1;
    }

    /* ---------- Vòng đời game ---------- */
    function calcInterval() {
        const c = SPEED[S.mode];
        return 1000 / Math.min(c.max, c.start + 0.25 * Math.floor(S.eaten / 5));
    }
    function resetGame() {
        abandonCurrentGame();
        S.snake = [{ x: CX, y: CY }, { x: CX - 1, y: CY }, { x: CX - 2, y: CY }];
        S.dir = 'right'; S.queue = [];
        S.eaten = 0; S.interval = calcInterval(); S.acc = 0; S.time = 0;
        S.prevBest = S.best; S.coinsGiven = false; S.newRecord = false;
        S.gameId = null; S.pendingBest = S.best;
        genPortals();
        S.walls = S.mode === 'hard' ? genWalls() : new Set();
        buildBase();
        spawnFood();
        setState('idle');
        toastEl.hidden = true;
        updateHud(); updateTimeHud();
    }
    async function startGame() {                 // đếm ngược 5 giây rồi mới chạy
        if (S.state !== 'idle' || S.starting) return;
        if (!S.practiceReady) {
            toast('Practice is unavailable until your account state loads.');
            return;
        }
        const gameId = createGameId();
        S.starting = true;
        try {
            await window.api.startPracticeGame(gameId);
        } catch (error) {
            console.error('Practice game could not start:', error);
            showPracticeStatus(`Could not start Practice: ${error.message}`, true);
            return;
        } finally {
            S.starting = false;
        }
        S.gameId = gameId;
        S.prevBest = S.best;
        S.pendingBest = S.best;
        S.coinsGiven = false;
        S.newRecord = false;
        S.cd = COUNTDOWN_MS; S.cdShown = 5;
        cdNum.textContent = '5';
        setState('countdown');
        showScreen('count');
    }
    function pauseGame() {
        if (S.state !== 'playing') return;
        setState('paused');
        showScreen('pause');
    }
    function resumeGame() {
        if (S.state !== 'paused') return;
        setState('playing');
        showScreen(null);
    }
    function restartGame() { resetGame(); startGame(); }
    function gameOver(win) {
        if (S.state !== 'playing') return;
        setState('over');
        const points = Math.floor(S.eaten / 5) * 15;
        const happy = points * 0.05;                       // 10 điểm = 0.5% happiness
        const coins = S.coinsGiven ? 15 : 0;
        const result = { species: S.species, mode: S.mode, eaten: S.eaten, points, happinessPercent: happy, coins, seconds: Math.floor(S.time / 1000), newRecord: S.newRecord, win: !!win };
        window.PracticeAPI.lastResult = result;
        window.dispatchEvent(new CustomEvent('practice:gameover', { detail: result }));
        $('#over-title').textContent = win ? 'You filled the field!' : 'Game over';
        $('#rs-label').textContent = FOOD[S.species].done;
        $('#rs-eaten').textContent = S.eaten;
        $('#rs-score').textContent = points;
        $('#rs-happy').textContent = '+' + (Math.round(happy * 100) / 100) + '%';
        $('#rs-coins').textContent = '+' + coins;
        $('#rs-time').textContent = fmtTime(S.time);
        $('#rs-record').hidden = !S.newRecord;
        $('#reward-retry').hidden = true;
        setGameOverActionsDisabled(true);
        showScreen('over');
        saveGameRewards();
    }
    function step() {
        const head = S.snake[0];
        if (S.queue.length) S.dir = S.queue.shift();
        const d = DIRS[S.dir];
        let nx = head.x + d.x, ny = head.y + d.y;
        if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS) {       // chỉ qua được ở cổng xuyên tường
            if ((nx < 0 || nx >= COLS) && S.portH.has(head.y)) nx = (nx + COLS) % COLS;
            else if ((ny < 0 || ny >= ROWS) && S.portV.has(head.x)) ny = (ny + ROWS) % ROWS;
            else return gameOver(false);
        }
        if (S.walls.has(key(nx, ny))) return gameOver(false);
        const willEat = !!S.food && S.food.x === nx && S.food.y === ny;
        const body = willEat ? S.snake : S.snake.slice(0, -1);    // đuôi rời ô nếu không ăn
        if (body.some(s => s.x === nx && s.y === ny)) return gameOver(false);

        S.snake.unshift({ x: nx, y: ny });
        if (!willEat) { S.snake.pop(); return; }
        S.eaten++;
        S.interval = calcInterval();
        if (S.eaten > S.best) {
            S.pendingBest = S.eaten;
            S.newRecord = true;
            if (S.prevBest > 0 && !S.coinsGiven) { S.coinsGiven = true; toast('NEW RECORD - finish game to save!'); }
        }
        updateHud();
        if (!spawnFood()) gameOver(true);                         // kín sân = thắng, không lặp vô hạn
    }
    function enqueueDir(name) {                  // chống bấm nhanh: không quay 180 độ, tối đa 2 lệnh chờ
        if (S.state !== 'playing' || !DIRS[name]) return;
        const last = S.queue.length ? S.queue[S.queue.length - 1] : S.dir;
        if (name === last || name === OPP[last] || S.queue.length >= 2) return;
        S.queue.push(name);
    }

    /* ---------- Chọn thú / chế độ ---------- */
    function paintIcon(canvas, src) {
        const g = canvas.getContext('2d');
        g.imageSmoothingEnabled = false;
        g.clearRect(0, 0, canvas.width, canvas.height);
        g.drawImage(src, 0, 0, canvas.width, canvas.height);
    }
    function setSpecies(sp) {
        S.species = sp;
        document.body.dataset.species = sp;
        hud.label.textContent = FOOD[sp].label;
        swSp.dataset.on = String(sp === 'dog');
        $('#sw-species-label').innerHTML = sp === 'cat' ? '<b>Cat</b> <i>Dog</i>' : '<i>Cat</i> <b>Dog</b>';
        $('#sw-species-icon').src = SPR[sp];
        resetGame();
    }
    function setMode(m) {
        S.mode = m;
        swMode.dataset.on = String(m === 'hard');
        $('#sw-mode-label').innerHTML = m === 'hard' ? '<i>Easy</i> <b>Hard</b>' : '<b>Easy</b> <i>Hard</i>';
        paintIcon($('#sw-mode-icon'), m === 'hard' ? TILE.wall : TILE.grass[0]);
        resetGame();
    }

    /* ---------- Nền động pixel (lớp tĩnh vẽ 1 lần, chỉ vẽ lại phần chuyển động) ---------- */
    const bg = $('#bg'), bgx = bg.getContext('2d', { alpha: false });
    const layer = makeCanvas(1, 1), lx = layer.getContext('2d');
    const FLOWER_COLS = ['#ff9fbd', '#ffffff', '#ffd76a', '#c4a7ff', '#ffb38a'];
    let BW = 0, BH = 0, groundY = 0, flowers = [], petals = [], clouds = [], flies = [], birds = [];
    function initBg() {
        BW = Math.ceil(window.innerWidth / 4); BH = Math.ceil(window.innerHeight / 4);
        bg.width = layer.width = BW; bg.height = layer.height = BH;
        groundY = Math.floor(BH * 0.66);
        const bands = ['#bfe6ff', '#c9ebff', '#d4f0fb', '#dff3f6', '#e9f6f1', '#f3f8ec', '#fdf3ea', '#ffe9ee'];
        const bh = Math.ceil(groundY / bands.length);
        bands.forEach((c, i) => rect(lx, c, 0, i * bh, BW, bh + 1));
        rect(lx, '#fff4b8', BW - 28, 8, 10, 10); rect(lx, '#fff9d6', BW - 26, 6, 6, 14); rect(lx, '#fff9d6', BW - 30, 10, 14, 6);
        for (let x = 0; x < BW; x++) {
            const h1 = groundY - 8 + Math.floor(Math.sin(x / 17) * 3 + Math.sin(x / 7) * 1.5);
            rect(lx, '#c4ebb8', x, h1, 1, BH - h1);
            const h2 = groundY - 2 + Math.floor(Math.sin(x / 11 + 2) * 2);
            rect(lx, '#afe2a0', x, h2, 1, BH - h2);
        }
        rect(lx, '#9ad78b', 0, groundY + 4, BW, BH - groundY - 4);
        for (let x = 0; x < BW; x += 6) for (let y = groundY + 6; y < BH; y += 6) rect(lx, '#8acd7c', x + ((y / 6) % 2) * 3, y, 1, 1);

        flowers = Array.from({ length: Math.max(16, Math.floor(BW / 5)) }, () => ({
            x: rand(BW), y: groundY + 6 + rand(Math.max(4, BH - groundY - 10)), c: FLOWER_COLS[rand(5)], ph: Math.random() * 6
        })).sort((a, b) => a.y - b.y);
        petals = Array.from({ length: 18 }, () => ({ x: rand(BW), y: rand(BH), sp: 0.6 + Math.random(), ph: Math.random() * 6, c: Math.random() < 0.6 ? '#ffc6d9' : '#ffffff' }));
        clouds = Array.from({ length: 4 }, (_, i) => ({ x: rand(BW + 40), y: 6 + i * 9 + rand(6), sp: 0.5 + Math.random(), w: 12 + rand(10) }));
        flies = Array.from({ length: 4 }, () => ({ x: rand(BW), y: groundY - 4 + rand(30), ph: Math.random() * 6, sp: 0.6 + Math.random(), c: ['#ff9fbd', '#ffd76a', '#9fd6ff', '#c4a7ff'][rand(4)] }));
        birds = Array.from({ length: 3 }, (_, i) => ({ x: rand(BW + 20), y: 10 + i * 8 + rand(6), sp: 0.8 + Math.random() * 0.6 }));
    }
    function drawBg(t) {
        const g = bgx;
        g.drawImage(layer, 0, 0);
        clouds.forEach(c => {
            const x = Math.floor((c.x + t * 0.004 * c.sp) % (BW + 40)) - 20;
            rect(g, '#ffffff', x, c.y, c.w, 3); rect(g, '#ffffff', x + 3, c.y - 2, c.w - 7, 2); rect(g, '#f1f8ff', x, c.y + 3, c.w, 1);
        });
        flowers.forEach(f => {
            const sw = Math.round(Math.sin(t / 700 + f.ph));
            rect(g, '#6fbf6a', f.x, f.y, 1, 4);
            rect(g, f.c, f.x - 1 + sw, f.y - 2, 3, 3);
            rect(g, '#fff3b0', f.x + sw, f.y - 1);
        });
        flies.forEach(b => {
            const x = Math.floor((b.x + t * 0.01 * b.sp) % (BW + 10)) - 5;
            const y = Math.floor(b.y + Math.sin(t / 500 + b.ph) * 6);
            rect(g, '#4a4a63', x, y, 1, 3);
            if (Math.floor(t / 150 + b.ph) % 2) { rect(g, b.c, x - 2, y, 2, 2); rect(g, b.c, x + 1, y, 2, 2); }
            else { rect(g, b.c, x - 1, y, 1, 2); rect(g, b.c, x + 1, y, 1, 2); }
        });
        birds.forEach(b => {
            const x = BW + 10 - Math.floor((b.x + t * 0.02 * b.sp) % (BW + 20));
            const y = Math.floor(b.y + Math.sin(t / 400 + b.x) * 2);
            const wy = Math.floor(t / 220 + b.x) % 2 ? y - 1 : y + 1;
            rect(g, '#6b7a99', x, y); rect(g, '#6b7a99', x - 2, wy, 2, 1); rect(g, '#6b7a99', x + 1, wy, 2, 1);
        });
        petals.forEach(p => {
            const y = Math.floor((p.y + t * 0.012 * p.sp) % (BH + 6)) - 3;
            const x = Math.floor((p.x + t * 0.006 * p.sp + Math.sin(t / 600 + p.ph) * 5 + BW * 2) % BW);
            rect(g, p.c, x, y, 2, 1); rect(g, p.c, x + 1, y + 1);
        });
    }

    /* ---------- Co giãn bàn chơi theo kích thước lưới ---------- */
    function fit() {
        const wide = window.innerWidth >= 1100;
        let fittedScale;
        if (window.innerWidth <= 600) {
            const game = $('.game');
            const gameStyle = getComputedStyle(game);
            const boardStyle = getComputedStyle($('.board-wrap'));
            const gap = parseFloat(gameStyle.rowGap) || 0;
            const paddingX = parseFloat(gameStyle.paddingLeft) + parseFloat(gameStyle.paddingRight);
            const paddingY = parseFloat(gameStyle.paddingTop) + parseFloat(gameStyle.paddingBottom);
            const frameX = parseFloat(boardStyle.paddingLeft) + parseFloat(boardStyle.paddingRight);
            const frameY = parseFloat(boardStyle.paddingTop) + parseFloat(boardStyle.paddingBottom);
            const flowItems = [$('.top'), $('.stats'), bottomActions, $('.legend')]
                .filter(el => !el.hidden && getComputedStyle(el).display !== 'none');
            const flowHeight = flowItems.reduce((sum, el) => sum + el.getBoundingClientRect().height, 0);
            const availableW = Math.max(0, document.documentElement.clientWidth - paddingX - frameX);
            const controlsHeight = $('.mobile-controls').getBoundingClientRect().height;
            const bottomSpace = controlsHeight + 16 + parseFloat(getComputedStyle(game).paddingBottom);
            const availableH = Math.max(0, window.innerHeight - paddingY - flowHeight - gap * flowItems.length - frameY - bottomSpace);
            fittedScale = Math.min(availableW / SW, availableH / SH);
        } else {
            const availW = wide ? Math.min(window.innerWidth - 460, 1400) : window.innerWidth - 32;
            const scale = availW / SW;
            const heightLimit = Math.max(0.1, (window.innerHeight - (wide ? 250 : 320)) / SH);
            fittedScale = wide || (window.innerWidth >= 600 && window.innerHeight >= 600) ? Math.min(scale, heightLimit) : scale;
        }
        const pw = Math.round(SW * fittedScale);
        board.style.width = pw + 'px';
        board.style.height = Math.round(SH * fittedScale) + 'px';
        document.documentElement.style.setProperty('--bw', (pw + 16) + 'px');
    }

    /* ---------- Vòng lặp duy nhất ---------- */
    let rafId = 0, lastT = 0, lastBoard = 0, lastBg = 0;
    function scheduleFrame() {
        if (!document.hidden && !rafId) rafId = requestAnimationFrame(frame);
    }
    function frame(t) {
        rafId = 0;
        if (document.hidden) return;
        const dt = Math.min(100, t - (lastT || t));
        lastT = t;
        if (S.state === 'countdown') {
            S.cd -= dt;
            const n = Math.max(1, Math.ceil(S.cd / 1000));
            if (n !== S.cdShown) { S.cdShown = n; cdNum.textContent = n; }
            if (S.cd <= 0) { S.acc = 0; setState('playing'); showScreen(null); }
        } else if (S.state === 'playing') {
            S.time += dt;
            S.acc += dt;
            for (let guard = 0; S.state === 'playing' && S.acc >= S.interval && guard < 4; guard++) { S.acc -= S.interval; step(); }
            if (S.acc > S.interval * 4) S.acc = 0;
        }
        if (t - lastBoard >= 33) { lastBoard = t; drawBoard(t); updateTimeHud(); }   // ~30 fps là đủ cho game theo ô
        if (t - lastBg >= 66) { lastBg = t; drawBg(t); }                              // nền ~15 fps
        scheduleFrame();
    }

    /* ---------- Sự kiện (đăng ký đúng 1 lần) ---------- */
    function handleAction(a, el) {
        switch (a) {
            case 'close-panel': {
                const screen = $('.screen:not([hidden])', overlay);
                if (!screen || !['ready', 'over'].includes(screen.dataset.screen)) break;
                const actions = screen.dataset.screen;
                showScreen(null);
                bottomActions.hidden = false;
                $(`[data-bottom-actions="${actions}"]`, bottomActions).hidden = false;
                fit();
                $('[data-bottom-actions]:not([hidden]) button', bottomActions).focus({ preventScroll: true });
                break;
            }
            case 'species': setSpecies(el.dataset.species); showScreen('mode'); break;
            case 'mode':
                setMode(el.dataset.mode);
                switchesEl.hidden = false;
                fit();
                showScreen('ready');
                break;
            case 'start': startGame(); break;
            case 'pause': S.state === 'paused' ? resumeGame() : pauseGame(); break;
            case 'resume': resumeGame(); break;
            case 'restart': case 'again': restartGame(); break;
            case 'retry-reward': saveGameRewards(); break;
            case 'menu': switchesEl.hidden = true; resetGame(); showScreen('species'); break;
            case 'swap-species': setSpecies(S.species === 'cat' ? 'dog' : 'cat'); showScreen('ready'); break;
            case 'swap-mode': setMode(S.mode === 'easy' ? 'hard' : 'easy'); showScreen('ready'); break;
        }
    }
    document.addEventListener('click', e => {
        const el = e.target.closest('[data-action]');
        if (!el || el.disabled) return;
        handleAction(el.dataset.action, el);
        if (el.id === 'pause-btn' || el.classList.contains('switch')) el.blur();
    });
    document.addEventListener('keydown', e => {
        if (e.ctrlKey || e.metaKey || e.altKey) return;
        const k = e.key.toLowerCase(), dir = KEYMAP[k];
        if (dir) {
            if (S.state === 'playing') { e.preventDefault(); if (!e.repeat) enqueueDir(dir); }
        } else if (k === 'p' || k === 'escape') {
            if (S.state === 'playing') pauseGame(); else if (S.state === 'paused') resumeGame();
        }
    });
    const dpad = $('#dpad');
    let swipeStart = null;
    board.addEventListener('pointerdown', e => {
        if (S.state !== 'playing' || !e.isPrimary) return;
        e.preventDefault();
        swipeStart = { id: e.pointerId, x: e.clientX, y: e.clientY };
        board.setPointerCapture(e.pointerId);
    });
    board.addEventListener('pointerup', e => {
        if (!swipeStart || swipeStart.id !== e.pointerId) return;
        const dx = e.clientX - swipeStart.x;
        const dy = e.clientY - swipeStart.y;
        swipeStart = null;
        if (Math.max(Math.abs(dx), Math.abs(dy)) < 18) return;
        enqueueDir(Math.abs(dx) > Math.abs(dy)
            ? (dx > 0 ? 'right' : 'left')
            : (dy > 0 ? 'down' : 'up'));
    });
    board.addEventListener('pointercancel', () => { swipeStart = null; });
    dpad.addEventListener('pointerdown', e => {
        const b = e.target.closest('button[data-dir]');
        if (!b) return;
        e.preventDefault();
        enqueueDir(b.dataset.dir);
        b.classList.add('is-down');
    });
    ['pointerup', 'pointercancel', 'pointerleave'].forEach(ev =>
        dpad.addEventListener(ev, () => $$('.is-down', dpad).forEach(b => b.classList.remove('is-down'))));
    document.addEventListener('visibilitychange', () => {
        if (document.hidden) {
            pauseGame();
            cancelAnimationFrame(rafId);
            rafId = 0;
        } else {
            lastT = lastBoard = lastBg = 0;
            scheduleFrame();
        }
    });
    window.addEventListener('pagehide', () => {
        if (!S.practiceReady) return;
        clearInterval(S.practiceHeartbeat);
        window.api.practiceTime('pause', true).catch(error => {
            console.error('Practice timer could not be paused:', error);
        });
        abandonCurrentGame(true);
    });
    document.addEventListener('click', async event => {
        const link = event.target.closest('a[href="index.html"]');
        if (!link || !S.practiceReady || event.button !== 0 ||
            event.ctrlKey || event.metaKey || event.shiftKey || event.altKey || link.target === '_blank') return;
        event.preventDefault();
        if (S.leavingPractice) return;
        S.leavingPractice = true;
        clearInterval(S.practiceHeartbeat);
        try {
            await window.api.practiceTime('pause');
            await abandonCurrentGame();
        } catch (error) {
            console.error('Practice session could not be closed cleanly before returning Home:', error);
        }
        window.location.assign(link.href);
    });
    window.addEventListener('pageshow', event => {
        if (!event.persisted || !S.practiceReady) return;
        window.api.practiceTime('start').then(() => {
            S.practiceHeartbeat = setInterval(() => {
                window.api.practiceTime('heartbeat').catch(error => {
                    console.error('Practice timer heartbeat failed:', error);
                    showPracticeStatus(`Could not update Practice time: ${error.message}`, true);
                });
            }, 20000);
        }).catch(error => {
            console.error('Practice timer could not resume:', error);
            showPracticeStatus(`Could not resume Practice time: ${error.message}`, true);
        });
    });
    window.addEventListener('blur', pauseGame);
    let resizeTimer = 0;
    window.addEventListener('resize', () => {
        clearTimeout(resizeTimer);
        resizeTimer = setTimeout(() => {
            const activeScreen = $('.screen:not([hidden])', overlay)?.dataset.screen;
            const resizedGrid = configureBoardGrid();
            fit(); initBg();
            if (resizedGrid) {
                resetGame();
                if (S.species && (!activeScreen || ['ready', 'count', 'pause', 'over'].includes(activeScreen))) showScreen('ready');
                else if (activeScreen) showScreen(activeScreen);
            }
        }, 120);
    });

    /* ---------- Khởi tạo ---------- */
    window.PracticeAPI = {
        lastResult: null,
        getBest: () => S.best
    };
    $$('canvas[data-icon]').forEach(c => {
        const k = c.dataset.icon;
        paintIcon(c, k === 'grass' ? TILE.grass[0] : TILE.wall);
    });
    configureBoardGrid();
    resetGame();
    fit(); initBg();
    showScreen('species');
    scheduleFrame();
    initializePractice();
})();
