let petLevel = 0;

function applyAccountState(state) {
	coins = state.user.coins;
	if (state.inventory) {
		inventory.splice(0, inventory.length, ...state.inventory.map((owned) => ({
			...shopItems.find((item) => item.id === owned.id), quantity: owned.quantity
		})));
	}
	if (state.pet) {
		document.querySelector('#pet-name').textContent = state.pet.name;
		document.querySelector('#pet-level').textContent = state.pet.level;
		petLevel = state.pet.level;
		const exp = state.pet.exp ?? state.pet.experience ?? 0;
		const expMax = state.pet.level * 100;
		document.querySelector('#exp-text').textContent = `EXP ${exp} / ${expMax}`;
		document.querySelector('#exp-fill').style.width = `${Math.min(100, exp / expMax * 100)}%`;
		['happiness', 'hunger', 'energy'].forEach((name) => {
			const stat = document.querySelector(`[data-stat="${name}"]`);
			stat.querySelector('.hud-stat-value').textContent = `${state.pet[name]} / 100`;
			stat.querySelector('.px-bar i').style.width = `${state.pet[name]}%`;
		});
	}
	if (state.decor) {
		equippedDecor.background = state.decor.background ?? null;
		equippedDecor.floor = state.decor.floor ?? null;
		equippedDecor.headwear = state.decor.headwear ?? null;
	}
	accountReady = true;
	updateCoinDisplays();
	renderInventory();
	renderShop();
	renderFood();
	applyLobbyDecor();
	renderDecorate();
}

async function loadAccount() {
	const catalog = await api.listShop();
	shopItems.splice(0, shopItems.length, ...catalog);
	applyAccountState(await api.me());
}

function reportFailure(failure) {
	return failure.message || 'Unable to update the demo.';
}

document.querySelector('#open-food').addEventListener('click', () => {
	if (!accountReady) return;
	const drawer = document.querySelector('#food-drawer');
	drawer.hidden = !drawer.hidden;
	document.querySelector('#open-food').setAttribute('aria-expanded', String(!drawer.hidden));
	renderFood();
});

let feeding = false;
let selectedFood = null;
const petStage = document.querySelector('.pet-stage');
const feedingStatus = document.querySelector('#feeding-status');
petStage.tabIndex = 0;
petStage.setAttribute('role', 'button');
petStage.setAttribute('aria-label', 'Feed selected food to UIA');

document.querySelector('#close-food').addEventListener('click', () => {
	document.querySelector('#food-drawer').hidden = true;
	document.querySelector('#open-food').setAttribute('aria-expanded', 'false');
});

function renderFood() {
	const list = document.querySelector('#food-list');
	list.replaceChildren();
	const foods = inventory.filter((item) => item.category === 'food' && item.quantity > 0);
	if (!foods.length) {
		const empty = document.createElement('p');
		empty.textContent = 'No food in inventory';
		list.append(empty);
	}
	foods.forEach((item) => {
		const button = document.createElement('button');
		button.type = 'button';
		button.className = 'food-choice';
		button.disabled = feeding;
		button.setAttribute('aria-label', `${item.name}, ${item.quantity} remaining`);
		button.setAttribute('aria-pressed', String(selectedFood === item.id));
		const image = document.createElement('img');
		image.src = item.image;
		image.alt = '';
		image.draggable = false;
		const label = document.createElement('span');
		label.textContent = item.name;
		const count = document.createElement('b');
		count.textContent = `\u00d7${item.quantity}`;
		button.append(image, label, count);
		button.addEventListener('click', () => {
			selectedFood = item.id;
			list.querySelectorAll('button').forEach((choice) => choice.setAttribute('aria-pressed', String(choice === button)));
		});
		button.addEventListener('pointerdown', (event) => {
			if (feeding || event.button !== 0) return;
			const ghost = image.cloneNode();
			ghost.className = 'food-drag-ghost';
			document.body.append(ghost);
			button.setPointerCapture(event.pointerId);
			const move = (pointer) => {
				ghost.style.left = `${pointer.clientX}px`;
				ghost.style.top = `${pointer.clientY}px`;
				const bounds = petStage.getBoundingClientRect();
				petStage.classList.toggle('is-drop-target', pointer.clientX >= bounds.left && pointer.clientX <= bounds.right && pointer.clientY >= bounds.top && pointer.clientY <= bounds.bottom);
			};
			move(event);
			const finish = (pointer) => {
				const dropped = petStage.classList.contains('is-drop-target');
				ghost.remove();
				petStage.classList.remove('is-drop-target');
				button.removeEventListener('pointermove', move);
				button.removeEventListener('pointerup', finish);
				button.removeEventListener('pointercancel', finish);
				if (pointer.type === 'pointerup' && dropped) feedFood(item);
			};
			button.addEventListener('pointermove', move);
			button.addEventListener('pointerup', finish);
			button.addEventListener('pointercancel', finish);
		});
		list.append(button);
	});
}

async function feedFood(item) {
	if (feeding || !accountReady || !item) return;
	feeding = true;
	feedingStatus.textContent = '';
	renderFood();
	try {
		const previousLevel = petLevel;
		const fed = await api.feedPet(item.id);
		applyAccountState(fed);
		petStage.classList.remove('is-eating');
		void petStage.offsetWidth;
		petStage.classList.add('is-eating');
		const bite = document.createElement('img');
		bite.src = item.image;
		bite.alt = '';
		bite.className = 'eating-food';
		petStage.append(bite);
		bite.addEventListener('animationend', () => bite.remove(), { once: true });
		feedingStatus.textContent = fed.pet.level > previousLevel
			? `${item.name} eaten! Level up! UIA is now level ${fed.pet.level}`
			: `${item.name} eaten!`;
	} catch (failure) {
		feedingStatus.textContent = reportFailure(failure);
	} finally {
		feeding = false;
		renderFood();
	}
}

petStage.addEventListener('animationend', () => petStage.classList.remove('is-eating'));
petStage.addEventListener('click', () => feedFood(inventory.find((item) => item.id === selectedFood)));
petStage.addEventListener('keydown', (event) => {
	if (event.key === 'Enter' || event.key === ' ') {
		event.preventDefault();
		feedFood(inventory.find((item) => item.id === selectedFood));
	}
});

document.querySelector('#sleep-pet').addEventListener('click', async (event) => {
	if (!accountReady) return;
	const button = event.currentTarget;
	button.disabled = true;
	try {
		applyAccountState(await api.sleepPet());
		feedingStatus.textContent = 'UIA took a nap and feels rested!';
	} catch (failure) {
		feedingStatus.textContent = reportFailure(failure);
	} finally {
		button.disabled = false;
	}
});

async function initializeAccount() {
	try { await loadAccount(); }
	catch (failure) { feedingStatus.textContent = reportFailure(failure); }
}

initializeAccount();