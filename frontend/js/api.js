// API client for the same-origin C++ server. Authentication uses its HttpOnly session cookie.
async function request(path, { method = 'GET', body } = {}) {
	const headers = {};
	if (body !== undefined) headers['Content-Type'] = 'application/json';

	const response = await fetch(path, {
		method,
		headers,
		credentials: 'same-origin',
		body: body === undefined ? undefined : JSON.stringify(body),
		signal: AbortSignal.timeout(15000)
	});
	const text = await response.text();
	let data = {};
	if (text) {
		try {
			data = JSON.parse(text);
		} catch {
			throw new Error(`The server returned an invalid response (HTTP ${response.status}).`);
		}
	}
	if (!response.ok) {
		const error = new Error(data.message || `Request failed (HTTP ${response.status}).`);
		error.status = response.status;
		error.code = data.code;
		throw error;
	}
	return data;
}

async function action(path, body, method = 'POST') {
	const result = await request(path, { method, body });
	if (!result.success || !result.state) {
		throw new Error(result.message || 'The server did not return an updated game state.');
	}
	return result.state;
}

const api = {
	async register(username, email, password) {
		return request('/api/auth/register', {
			method: 'POST',
			body: { username, email, password }
		});
	},

	async login(email, password) {
		return request('/api/auth/login', {
			method: 'POST',
			body: { email, password }
		});
	},

	session() {
		return request('/api/auth/session');
	},

	logout() {
		return request('/api/auth/logout', { method: 'POST' });
	},

	gameState() {
		return request('/api/game/state');
	},

	savePetProfile(name, species) {
		return action('/api/pet', { name, species }, 'PUT');
	},

	saveSoundEnabled(soundEnabled) {
		return action('/api/settings', { soundEnabled }, 'PUT');
	},

	listShop() {
		return request('/api/shop');
	},

	buyItem(itemId, quantity) {
		return action('/api/shop/purchases', { itemId, quantity });
	},

	feedPet(itemId) {
		return action('/api/pet/feed', { itemId });
	},

	playPet() {
		return action('/api/pet/play');
	},

	sleepPet() {
		return action('/api/pet/sleep');
	},

	bathPet() {
		return action('/api/pet/bath');
	},

	equipDecoration(slot, itemId) {
		return itemId === null
			? action('/api/pet/unequip', { slot })
			: action('/api/pet/decorate', { itemId });
	}
};

window.api = api;
