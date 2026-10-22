(() => {
	const storageKey = 'virtual-pet-demo-v1';
	const catalog = [
		{ id: 1, name: 'Bread', description: 'A filling snack at a friendly price.', category: 'food', categoryLabel: 'Food', filter: 'food', price: 5, hungerRestore: 30, happinessRestore: 0, image: 'assets/items/banh_mi.png', icon: '\ud83e\udd56' },
		{ id: 2, name: 'Apple', description: 'A crisp and refreshing fruit.', category: 'food', categoryLabel: 'Food', filter: 'food', price: 8, hungerRestore: 20, happinessRestore: 5, image: 'assets/items/tao_do.png', icon: '\ud83c\udf4e' },
		{ id: 3, name: 'Golden Apple', description: 'A rare apple with a generous boost.', category: 'food', categoryLabel: 'Food', filter: 'food', price: 25, hungerRestore: 40, happinessRestore: 15, image: 'assets/items/tao_vang.png', icon: '\ud83c\udf4f' },
		{ id: 4, name: 'Cream Cake', description: 'A soft, sweet treat that lifts the mood.', category: 'food', categoryLabel: 'Food', filter: 'food', price: 6, hungerRestore: 10, happinessRestore: 20, image: 'assets/items/banh_kem.png', icon: '\ud83c\udf70' },
		{ id: 5, name: 'Honey', description: 'Naturally sweet and nourishing.', category: 'food', categoryLabel: 'Food', filter: 'food', price: 3, hungerRestore: 5, happinessRestore: 25, image: 'assets/items/mat_ong.png', icon: '\ud83c\udf6f' },
		{ id: 6, name: 'Wool Hat', description: 'A warm knitted hat for your pet.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Headwear', filter: 'headwear', price: 15, image: 'assets/items/mu_len.png', petImage: 'assets/pets/meomeo_mulen_pixel.png', icon: '\ud83e\udde2' },
		{ id: 7, name: 'Crown', description: 'A royal crown for your pet.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Headwear', filter: 'headwear', price: 50, image: 'assets/items/vuong_mien.png', petImage: 'assets/pets/meomeo_vuongmien.png', icon: '\ud83d\udc51' },
		{ id: 8, name: 'Beach Escape', description: 'A sunny shore with clear blue water.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Background', filter: 'background', price: 20, image: 'assets/backgrounds/baibien.png', icon: '\ud83c\udfd6\ufe0f' },
		{ id: 9, name: 'Starry Sky', description: 'A calm night sky filled with stars.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Background', filter: 'background', price: 30, image: 'assets/backgrounds/bautroisao.png', icon: '\ud83c\udf0c' },
		{ id: 12, name: 'Forest Hideaway', description: 'A peaceful scene among green trees.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Background', filter: 'background', price: 25, image: 'assets/backgrounds/rungcay.png', icon: '\ud83c\udf33' },
		{ id: 13, name: 'City Lights', description: 'A lively city glowing after dark.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Background', filter: 'background', price: 35, image: 'assets/backgrounds/thanhpho.png', icon: '\ud83c\udf03' },
		{ id: 10, name: 'Wooden Floor', description: 'A warm wooden floor for a cozy room.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Floor', filter: 'floor', price: 18, image: 'assets/floor/sango.png', icon: '\ud83e\udeb5' },
		{ id: 11, name: 'Flower Rug', description: 'A soft rug with a cheerful floral pattern.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Floor', filter: 'floor', price: 22, image: 'assets/floor/santhamhoa.png', icon: '\ud83c\udf3c' }
	];
	const copy = (value) => JSON.parse(JSON.stringify(value));
	const initialState = () => ({
		user: { id: 1, coins: 100 },
		pet: { name: 'UIA', level: 1, exp: 0, happiness: 85, hunger: 30, energy: 70 },
		inventory: [{ id: 1, quantity: 3 }, { id: 2, quantity: 2 }],
		decor: { background: null, floor: null, headwear: null }
	});
	function validState(saved) {
		return saved && Number.isSafeInteger(saved.user?.coins) && saved.user.coins >= 0
			&& typeof saved.pet?.name === 'string' && saved.pet.name.trim().length > 0
			&& Number.isSafeInteger(saved.pet.level) && saved.pet.level > 0
			&& Number.isSafeInteger(saved.pet.exp) && saved.pet.exp >= 0
			&& ['happiness', 'hunger', 'energy'].every((stat) => Number.isFinite(saved.pet[stat]) && saved.pet[stat] >= 0 && saved.pet[stat] <= 100)
			&& Array.isArray(saved.inventory) && saved.inventory.every((owned) => {
				const item = catalog.find((entry) => entry.id === owned?.id);
				return item && Number.isSafeInteger(owned.quantity) && owned.quantity > 0 && (item.category === 'food' || owned.quantity === 1);
			}) && new Set(saved.inventory.map((owned) => owned.id)).size === saved.inventory.length
			&& (saved.decor === undefined || saved.decor === null || (typeof saved.decor === 'object'
				&& ['background', 'floor', 'headwear'].every((slot) => {
					const value = saved.decor[slot];
					return value === null || value === undefined
						|| (catalog.some((entry) => entry.id === value && entry.filter === slot)
							&& saved.inventory.some((owned) => owned.id === value));
				})));
	}
	let state = initialState();
	try {
		const saved = JSON.parse(localStorage.getItem(storageKey));
		if (validState(saved)) state = saved;
	} catch {
		state = initialState();
	}
	state.decor = {
		background: state.decor?.background ?? null,
		floor: state.decor?.floor ?? null,
		headwear: state.decor?.headwear ?? null
	};
	function persist(next) {
		try {
			localStorage.setItem(storageKey, JSON.stringify(next));
		} catch {
			console.warn('Demo storage is unavailable. Progress will last until this page closes.');
		}
		state = next;
		return copy(state);
	}
	persist(state);
	window.api = {
		async me() { return copy(state); },
		async listShop() { return copy(catalog); },
		async buyItem(itemId, quantity) {
			const item = catalog.find((entry) => entry.id === itemId);
			if (!item) throw new Error('Item not found.');
			if (!Number.isSafeInteger(quantity) || quantity < 1 || (item.category === 'decoration' && quantity !== 1)) throw new Error('Choose a valid quantity.');
			const owned = state.inventory.find((entry) => entry.id === itemId);
			if (item.category === 'decoration' && owned) throw new Error('You already own this decoration.');
			if (item.price * quantity > state.user.coins) throw new Error('Not enough coins for this quantity.');
			const next = copy(state);
			next.user.coins -= item.price * quantity;
			if (owned) next.inventory.find((entry) => entry.id === itemId).quantity += quantity;
			else next.inventory.push({ id: itemId, quantity });
			return persist(next);
		},
		async feedPet(itemId) {
			const item = catalog.find((entry) => entry.id === itemId && entry.category === 'food');
			if (!item || !state.inventory.some((entry) => entry.id === itemId)) throw new Error('No food remaining.');
			const next = copy(state);
			next.inventory.find((entry) => entry.id === itemId).quantity -= 1;
			next.inventory = next.inventory.filter((entry) => entry.quantity > 0);
			next.pet.hunger = Math.min(100, next.pet.hunger + item.hungerRestore);
			next.pet.happiness += item.happinessRestore;
			while (next.pet.happiness >= 100) {
				next.pet.level += 1;
				next.pet.happiness -= 100;
			}
			return persist(next);
		},
		async sleepPet() {
			if (state.pet.energy >= 100) throw new Error('UIA is not sleepy right now.');
			const next = copy(state);
			next.pet.energy = Math.min(100, next.pet.energy + 40);
			return persist(next);
		},
		async equipDecoration(slot, itemId) {
			if (!['background', 'floor', 'headwear'].includes(slot)) throw new Error('Unknown decoration slot.');
			const next = copy(state);
			if (!next.decor) next.decor = { background: null, floor: null, headwear: null };
			if (itemId === null || itemId === undefined) {
				next.decor[slot] = null;
				return persist(next);
			}
			const item = catalog.find((entry) => entry.id === itemId && entry.category === 'decoration' && entry.filter === slot);
			if (!item) throw new Error('That decoration does not fit here.');
			if (!state.inventory.some((entry) => entry.id === itemId)) throw new Error('You do not own this decoration yet.');
			next.decor[slot] = itemId;
			return persist(next);
		}
	};
})();