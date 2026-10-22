(() => {
	const storageKey = 'virtual-pet-demo-v1';
	const catalog = [
		{ id: 1, name: 'Bread', description: 'A filling snack at a friendly price.', category: 'food', categoryLabel: 'Food', filter: 'food', price: 5, hungerRestore: 30, happinessRestore: 0, image: 'assets/items/banh_mi.png', icon: '\ud83e\udd56' },
		{ id: 2, name: 'Apple', description: 'A crisp and refreshing fruit.', category: 'food', categoryLabel: 'Food', filter: 'food', price: 8, hungerRestore: 20, happinessRestore: 5, image: 'assets/items/tao_do.png', icon: '\ud83c\udf4e' },
		{ id: 3, name: 'Golden Apple', description: 'A rare apple with a generous boost.', category: 'food', categoryLabel: 'Food', filter: 'food', price: 25, hungerRestore: 40, happinessRestore: 15, image: 'assets/items/tao_vang.png', icon: '\ud83c\udf4f' },
		{ id: 4, name: 'Cream Cake', description: 'A soft, sweet treat that lifts the mood.', category: 'food', categoryLabel: 'Food', filter: 'food', price: 6, hungerRestore: 10, happinessRestore: 20, image: 'assets/items/banh_kem.png', icon: '\ud83c\udf70' },
		{ id: 5, name: 'Honey', description: 'Naturally sweet and nourishing.', category: 'food', categoryLabel: 'Food', filter: 'food', price: 3, hungerRestore: 5, happinessRestore: 25, image: 'assets/items/mat_ong.png', icon: '\ud83c\udf6f' },
		{ id: 6, name: 'Wool Hat', description: 'A warm knitted hat for your pet.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Headwear', filter: 'headwear', price: 15, image: 'assets/items/mu_len.png', icon: '\ud83e\udde2' },
		{ id: 7, name: 'Crown', description: 'A royal crown for your pet.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Headwear', filter: 'headwear', price: 50, image: 'assets/items/vuong_mien.png', icon: '\ud83d\udc51' },
		{ id: 8, name: 'Beach Escape', description: 'A sunny shore with clear blue water.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Background', filter: 'background', price: 20, image: 'assets/items/nen_bai_bien.png', icon: '\ud83c\udfd6\ufe0f' },
		{ id: 9, name: 'Starry Sky', description: 'A calm night sky filled with stars.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Background', filter: 'background', price: 30, image: 'assets/items/nen_bau_troi.png', icon: '\ud83c\udf0c' },
		{ id: 12, name: 'Forest Hideaway', description: 'A peaceful scene among green trees.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Background', filter: 'background', price: 25, image: 'assets/items/nen_khu_rung.png', icon: '\ud83c\udf33' },
		{ id: 13, name: 'City Lights', description: 'A lively city glowing after dark.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Background', filter: 'background', price: 35, image: 'assets/items/nen_thanh_pho.png', icon: '\ud83c\udf03' },
		{ id: 10, name: 'Wooden Floor', description: 'A warm wooden floor for a cozy room.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Floor', filter: 'floor', price: 18, image: 'assets/items/san_go.png', icon: '\ud83e\udeb5' },
		{ id: 11, name: 'Flower Rug', description: 'A soft rug with a cheerful floral pattern.', category: 'decoration', categoryLabel: 'Decoration \u00b7 Floor', filter: 'floor', price: 22, image: 'assets/items/tham_hoa.png', icon: '\ud83c\udf3c' }
	];
	const exercises = [
		{ id: 1, title: 'Hello World', description: 'Return exactly Hello World! (case sensitive).', answer: 'Hello World!' },
		{ id: 2, title: 'Addition', description: 'Compute 7 + 5. Return the integer result.', answer: '12' },
		{ id: 3, title: 'Parity', description: 'Is 14 even or odd? Return exactly even (lowercase).', answer: 'even' },
		{ id: 4, title: 'Maximum', description: 'Find the maximum of 3 9 2 7. Return the integer result.', answer: '9' },
		{ id: 5, title: 'Sorting', description: 'Sort 5 1 4 2 3 ascending. Return space-separated integers.', answer: '1 2 3 4 5' }
	];
	const copy = (value) => JSON.parse(JSON.stringify(value));
	const initialState = () => ({
		user: { id: 1, coins: 100 },
		pet: { name: 'UIA', level: 1, exp: 0, happiness: 85, hunger: 30, energy: 70 },
		inventory: [{ id: 1, quantity: 3 }, { id: 2, quantity: 2 }],
		progress: []
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
			&& Array.isArray(saved.progress) && saved.progress.every((entry) => entry
				&& exercises.some((exercise) => exercise.id === entry.exerciseId)
				&& [0, 100].includes(entry.bestScore) && Number.isSafeInteger(entry.attempts) && entry.attempts > 0
				&& entry.status === (entry.bestScore === 100 ? 'completed' : 'in_progress'))
			&& new Set(saved.progress.map((entry) => entry.exerciseId)).size === saved.progress.length;
	}
	let state = initialState();
	try {
		const saved = JSON.parse(localStorage.getItem(storageKey));
		if (validState(saved)) state = saved;
	} catch {
		state = initialState();
	}
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
		async listExercises() {
			return exercises.map(({ answer, ...exercise }) => ({ ...exercise, difficulty: 'easy', maxScore: 100 }));
		},
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
			next.pet.happiness = Math.min(100, next.pet.happiness + item.happinessRestore);
			return persist(next);
		},
		async submitExercise(exerciseId, answer) {
			const exercise = exercises.find((entry) => entry.id === exerciseId);
			if (!exercise) throw new Error('Exercise not found.');
			if (typeof answer !== 'string' || !answer.trim() || answer.length > 2048) throw new Error('Enter an answer of up to 2048 characters.');
			const score = answer.trim().replace(/\s+/g, ' ') === exercise.answer ? 100 : 0;
			const next = copy(state);
			let progress = next.progress.find((entry) => entry.exerciseId === exerciseId);
			if (!progress) {
				progress = { exerciseId, status: 'in_progress', bestScore: 0, attempts: 0, completedAt: '' };
				next.progress.push(progress);
			}
			progress.attempts += 1;
			progress.bestScore = Math.max(progress.bestScore, score);
			if (score === 100) {
				progress.status = 'completed';
				if (!progress.completedAt) progress.completedAt = new Date().toISOString();
			}
			persist(next);
			return { score, progress: copy(progress) };
		}
	};
})();