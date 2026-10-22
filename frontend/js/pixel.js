// Icon pixel-art vẽ bằng ma trận ký tự (12x12) và đồng hồ/thời tiết theo giờ máy.
const PIXEL_COLORS = {
	k: '#17394a', w: '#ffffff', y: '#ffd23f', Y: '#e8921a', o: '#ff8a3d', r: '#e8454e',
	p: '#ff8fb8', P: '#d9588a', g: '#5ccf6e', G: '#2f9a4a', b: '#4aa8ff', B: '#2a6fd1',
	c: '#6fe3f0', n: '#a8683a', N: '#6e3f1f', s: '#b9c4cc', S: '#6b7a86'
};

const PIXEL_ICONS = {
	sun: [
		'.....YY.....', '.....YY.....', '..Y......Y..', '...yyyyyy...', '...yyyyyy...', 'YY.yyyyyy.YY',
		'YY.yyyyyy.YY', '...yyyyyy...', '...YYYYYY...', '..Y......Y..', '.....YY.....', '.....YY.....'
	],
	moon: [
		'....yyyy....', '..yyyy......', '.yyyy.......', '.yyy........', 'yyyy........', 'yyyy........',
		'yyyy........', 'yyyy........', '.yyy.....yy.', '.yyyy..yyyy.', '..yyyyyyyy..', '....yyyy....'
	],
	heart: [
		'............', '..kkk..kkk..', '.kpppkkpppk.', 'kpwppppppppk', 'kpwpppppppPk', 'kppppppppPPk',
		'.kpppppppPk.', '..kpppppPk..', '...kpppPk...', '....kpPk....', '.....kk.....', '............'
	],
	bolt: [
		'.....kkkk...', '....kyyyk...', '...kyyyk....', '..kyyyk.....', '.kyyyyyyk...', '.kkkkyyyk...',
		'.....kyyk...', '....kyyk....', '....kyk.....', '...kyk......', '...kk.......', '............'
	],
	utensils: [
		'.o.o.o..ooo.', '.o.o.o.ooooo', '.o.o.o.ooooo', '.ooooo.ooooo', '..ooo...ooo.', '...o.....o..',
		'...o.....o..', '...o.....o..', '...o.....o..', '...o.....o..', '...o.....o..', '...Y.....Y..'
	],
	book: [
		'............', '..kkkkkkkk..', '..kbbbbbbkk.', '..kbwwwwbkw.', '..kbbbbbbkw.', '..kbwwwwbkw.',
		'..kbbbbbbkw.', '..kbbbbbbkw.', '..kbbbbbbkk.', '..kkkkkkkkk.', '............', '............'
	],
	code: [
		'............', '......BB....', '......BB....', '..B..BB..B..', '.B...BB...B.', 'B...BB.....B',
		'B...BB.....B', '.B..BB....B.', '..B.BB...B..', '...BB.......', '...BB.......', '............'
	],
	dumbbell: [
		'............', '............', '............', 'BB........BB', 'BBB......BBB', 'BBBssssssBBB',
		'BBBssssssBBB', 'BBB......BBB', 'BB........BB', '............', '............', '............'
	],
	star: [
		'.....yy.....', '.....yy.....', '....yyyy....', 'yyyyyyyyyyyy', '.yyyyyyyyyy.', '..yyyyyyyy..',
		'...yyyyyy...', '...yyyyyy...', '..yyy..yyy..', '.yyy....yyy.', '.yy......yy.', '............'
	],
	bowl: [
		'............', '............', '............', '...ooo.rrr..', '..ooogggrrr.', '.ooooggggrr.',
		'kkkkkkkkkkkk', 'kbwbbbbbbbbk', 'kbbbbbbbbbbk', '.kbbbbbbbbk.', '..kkkkkkkk..', '............'
	],
	bed: [
		'............', '........yyy.', '.........y..', 'kk......yyy.', 'kk..........', 'kkwww.......',
		'kkbbbbbbbbb.', 'kkbwbbbwbbb.', 'kkbbbbbbbbb.', 'kkkkkkkkkkkk', 'kk........kk', 'kk........kk'
	],
	backpack: [
		'....kkkk....', '...k....k...', '..kkkkkkkk..', '.kooooooook.', '.kooooooook.', '.koNNNNNNok.',
		'.koNyyyyNok.', '.koNNNNNNok.', '.kooooooook.', '.kooooooook.', '..kkkkkkkk..', '............'
	],
	bag: [
		'............', '....kkkk....', '...k....k...', '...k....k...', '.kkkkkkkkkk.', '.kyyyyyyyyk.',
		'.kyyyyyyyyk.', '.kyyyyyyyyk.', '.kyyyyyyyyk.', '.kYYYYYYYYk.', '.kkkkkkkkkk.', '............'
	],
	brush: [
		'..........nn', '.........nn.', '........nn..', '.......nn...', '......ss....', '.....ss.....',
		'...ppp......', '..ppppp.....', '..pppp......', '...PP.......', '............', '............'
	],
	coin: [
		'............', '....kkkk....', '..kkyyyykk..', '.kyyyyyyyyk.', 'kyyYYYYYYyyk', 'kyyYyyyyYyyk',
		'kyyYyyyyYyyk', 'kyyYYYYYYyyk', '.kyyyyyyyyk.', '..kkyyyykk..', '....kkkk....', '............'
	],
	paw: [
		'............', '............', '...pp..pp...', '...pp..pp...', 'pp........pp', 'pp..pppp..pp',
		'...pppppp...', '..pppppppp..', '..pppppppp..', '..ppp..ppp..', '..pp....pp..', '............'
	]
};

const pixelIconCache = {};

// Trả về data-URI SVG với shape-rendering=crispEdges để giữ nét vuông.
function pixelIconUrl(name) {
	if (pixelIconCache[name]) return pixelIconCache[name];
	const rows = PIXEL_ICONS[name];
	if (!rows) return '';
	let rects = '';
	rows.forEach((row, y) => {
		[...row].forEach((cell, x) => {
			if (PIXEL_COLORS[cell]) rects += `<rect x="${x}" y="${y}" width="1" height="1" fill="${PIXEL_COLORS[cell]}"/>`;
		});
	});
	const svg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${rows[0].length} ${rows.length}" shape-rendering="crispEdges">${rects}</svg>`;
	pixelIconCache[name] = `url("data:image/svg+xml,${encodeURIComponent(svg)}")`;
	return pixelIconCache[name];
}

function applyPixelIcons(root = document) {
	root.querySelectorAll('[data-px]').forEach((element) => {
		element.style.backgroundImage = pixelIconUrl(element.dataset.px);
	});
}

function updateClock() {
	const now = new Date();
	const hour = now.getHours();
	const clock = document.querySelector('#hud-clock');
	const label = document.querySelector('#hud-weather-label');
	const icon = document.querySelector('#hud-weather-icon');
	if (!clock) return;
	clock.textContent = `${String(hour).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`;
	const isDay = hour >= 6 && hour < 18;
	label.textContent = isDay ? 'Sunny' : (hour >= 18 && hour < 20 ? 'Evening' : 'Night');
	const iconName = isDay || (hour >= 18 && hour < 20) ? 'sun' : 'moon';
	if (icon.dataset.px !== iconName) {
		icon.dataset.px = iconName;
		icon.style.backgroundImage = pixelIconUrl(iconName);
	}
}

applyPixelIcons();
updateClock();
setInterval(updateClock, 1000);
